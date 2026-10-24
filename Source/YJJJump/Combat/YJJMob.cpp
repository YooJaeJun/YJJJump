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

	if (bDying)
	{
		TickDeath(DeltaSeconds);
		return;
	}

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

		// 날갯짓: 두 프레임을 번갈아
		if (Data->FlapSprite)
		{
			const bool bWingsUp = FMath::FloorToInt((Age + MovePhase) / FMath::Max(Data->FlapInterval, 0.02f)) % 2 == 1;
			Sprite->SetSprite(bWingsUp ? Data->FlapSprite.Get() : Data->Sprite.Get());
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
	if (bDying)
	{
		return false;
	}

	Health -= Damage;
	HitPunch = 1.f;

	if (Health > 0.f)
	{
		return false;
	}

	BeginDeath();
	return true;
}

void AYJJMob::BeginDeath()
{
	// 더 이상 탭/스폰 대상이 아니다. 연출이 끝나면 사라진다
	bDying = true;
	DeathAge = 0.f;
	if (UYJJMobSubsystem* Mobs = GetWorld()->GetSubsystem<UYJJMobSubsystem>())
	{
		Mobs->UnregisterMob(this);
	}

	const float EffectWidth = (Data ? Data->SpriteWidth : 120.f) * 1.1f;
	AYJJHitEffect::Spawn(GetWorld(), GetActorLocation() + FVector(0.f, 30.f, 0.f), EffectWidth, FLinearColor(1.f, 0.9f, 0.3f));

	// 맞은 반대쪽으로 빙글빙글 날아간다
	DeathSpin = FMath::RandBool() ? 1.f : -1.f;
	if (Data && Data->Sprite)
	{
		Sprite->SetSprite(Data->Sprite);
	}
}

void AYJJMob::TickDeath(float DeltaSeconds)
{
	DeathAge += DeltaSeconds;
	constexpr float FreezeTime = 0.14f;	// 히트스톱 동안은 제자리에서 번쩍번쩍 떨기
	constexpr float FlyTime = 0.45f;

	if (DeathAge < FreezeTime)
	{
		const float Shake = 12.f;
		Sprite->SetRelativeLocation(FVector(FMath::FRandRange(-Shake, Shake), 0.f, FMath::FRandRange(-Shake, Shake)));
		Sprite->SetRelativeScale3D(FVector(BaseScale * 1.3f * Facing, BaseScale, BaseScale * 1.3f));
		const bool bFlash = FMath::FloorToInt(DeathAge / 0.035f) % 2 == 0;
		Sprite->SetSpriteColor(bFlash ? FLinearColor(1.f, 0.25f, 0.25f) : FLinearColor(1.f, 1.f, 0.5f));
		return;
	}

	const float T = FMath::Clamp((DeathAge - FreezeTime) / FlyTime, 0.f, 1.f);
	Sprite->SetRelativeLocation(FVector(DeathSpin * 260.f * T, 0.f, 220.f * T - 500.f * T * T));
	Sprite->SetRelativeRotation(FRotator(DeathSpin * 720.f * T, 0.f, 0.f));
	const float Shrink = FMath::Lerp(1.2f, 0.2f, T);
	Sprite->SetRelativeScale3D(FVector(BaseScale * Shrink * Facing, BaseScale, BaseScale * Shrink));
	Sprite->SetSpriteColor(FLinearColor(0.6f, 0.6f, 0.6f));

	if (T >= 1.f)
	{
		Destroy();
	}
}
