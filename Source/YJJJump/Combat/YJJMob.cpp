#include "Combat/YJJMob.h"
#include "Combat/YJJMobData.h"
#include "Combat/YJJMobSubsystem.h"
#include "Visual/YJJHitEffect.h"
#include "Visual/YJJSpriteUtils.h"
#include "Engine/World.h"
#include "PaperSpriteComponent.h"

AYJJMob::AYJJMob()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(Root);
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AYJJMob::BeginPlay()
{
	Super::BeginPlay();

	Home = GetActorLocation();
	Health = (Data ? Data->MaxHealth : 1.f) * StatMultiplier;

	if (Data)
	{
		YJJSpriteUtils::SetSpriteWithWidth(Sprite, Data->Sprite, Data->SpriteWidth);
		BaseScale = static_cast<float>(Sprite->GetRelativeScale3D().X);
	}

	if (UYJJMobSubsystem* Mobs = GetWorld()->GetSubsystem<UYJJMobSubsystem>())
	{
		MobId = Mobs->RegisterMob(this);
	}
}

void AYJJMob::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UYJJMobSubsystem* Mobs = GetWorld()->GetSubsystem<UYJJMobSubsystem>())
	{
		Mobs->UnregisterMob(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AYJJMob::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	HitPunch = FMath::Max(HitPunch - DeltaSeconds * 4.f, 0.f);

	FVector Offset = FVector::ZeroVector;
	if (Data)
	{
		const float Phase = UE_TWO_PI * Age / Data->MovePeriod + MovePhase;
		switch (Data->Movement)
		{
		case EYJJMobMovement::Patrol:
			Offset.X = FMath::Sin(Phase) * FMath::Min(Data->MoveRange, MaxMoveRange);
			// 진행 방향을 바라본다
			Facing = FMath::Cos(Phase) >= 0.f ? 1.f : -1.f;
			break;
		case EYJJMobMovement::Hover:
			Offset.Z = FMath::Sin(Phase) * Data->MoveRange;
			break;
		default:
			Offset.Z = FMath::Sin(Phase) * 12.f;
			break;
		}
	}
	SetActorLocation(Home + Offset);
	UpdateVisualScale();
}

void AYJJMob::UpdateVisualScale()
{
	// 맞으면 잠깐 커졌다 돌아오고 빨갛게 번쩍
	const float Punch = 1.f + HitPunch * 0.3f;
	Sprite->SetRelativeScale3D(FVector(BaseScale * Punch * Facing, BaseScale, BaseScale * Punch));
	Sprite->SetSpriteColor(FLinearColor::LerpUsingHSV(FLinearColor::White, FLinearColor(1.f, 0.3f, 0.3f), HitPunch));
}

FYJJMobTarget AYJJMob::MakeTarget() const
{
	const FVector3f Location(GetActorLocation());

	FYJJMobTarget Target;
	Target.Id = MobId;
	Target.Position = FVector2f(Location.X, Location.Z);
	Target.Radius = Data ? Data->Radius : 50.f;
	Target.Health = Health;
	Target.ScoreValue = Data ? FMath::RoundToInt(Data->ScoreValue * StatMultiplier) : 0;
	Target.XPValue = Data ? FMath::Max(FMath::RoundToInt(Data->XPValue * StatMultiplier), 1) : 1;
	return Target;
}

bool AYJJMob::ApplyHit(float Damage)
{
	Health -= Damage;
	HitPunch = 1.f;

	if (Health > 0.f)
	{
		return false;
	}

	const float EffectWidth = (Data ? Data->SpriteWidth : 120.f) * 1.2f;
	AYJJHitEffect::Spawn(GetWorld(), GetActorLocation() + FVector(0.f, 5.f, 0.f), EffectWidth, FLinearColor(1.f, 0.9f, 0.3f));
	Destroy();
	return true;
}
