#include "Combat/YJJMob.h"
#include "Combat/YJJMobData.h"
#include "Combat/YJJMobSubsystem.h"
#include "Visual/YJJHitEffect.h"
#include "Visual/YJJSpriteFx.h"
#include "Visual/YJJSpriteUtils.h"
#include "YJJAssetPaths.h"
#include "PaperSprite.h"
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

namespace
{
	constexpr float DeathFreezeTime = 0.14f;	// 히트스톱 동안 제자리에서 번쩍이며 떨기
	constexpr float DeathFlyTime = 0.55f;		// 빙글빙글 날아가기
}

void AYJJMob::BeginDeath()
{
	// 더 이상 탭/스폰 대상이 아니다. 연출이 끝나면 사라진다
	bDying = true;
	DeathAge = 0.f;
	bSpawnedStars = false;
	if (UYJJMobSubsystem* Mobs = GetWorld()->GetSubsystem<UYJJMobSubsystem>())
	{
		Mobs->UnregisterMob(this);
	}

	// 기절 프레임 (X 눈, 혹)
	if (Data)
	{
		Sprite->SetSprite(Data->HitSprite ? Data->HitSprite.Get() : Data->Sprite.Get());
	}

	const FVector Center = GetActorLocation();
	const float Width = Data ? Data->SpriteWidth : 120.f;
	AYJJHitEffect::Spawn(GetWorld(), Center + FVector(0.f, 30.f, 0.f), Width * 1.1f, FLinearColor(1.f, 0.9f, 0.3f));

	// 깃털이 사방으로 빠진다
	if (UPaperSprite* Feather = YJJSpriteUtils::LoadSprite(YJJAssetPaths::FeatherSprite))
	{
		const int32 Count = Data ? Data->DeathFeathers : 6;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Angle = UE_TWO_PI * (Index + FMath::FRand() * 0.5f) / FMath::Max(Count, 1);
			FYJJSpriteFxParams Params;
			Params.Frames = { Feather };
			Params.Lifetime = FMath::FRandRange(0.6f, 0.9f);
			Params.Width = Width * 0.42f;
			Params.Velocity = FVector(FMath::Cos(Angle), 0.f, FMath::Sin(Angle) + 0.4f) * FMath::FRandRange(350.f, 600.f);
			Params.Gravity = 900.f;
			Params.SpinDegreesPerSecond = FMath::FRandRange(-720.f, 720.f);
			Params.EndScale = 0.6f;
			AYJJSpriteFx::Spawn(GetWorld(), Center + FVector(0.f, 25.f, 0.f), Params);
		}
	}

	// 맞은 반대쪽으로 빙글빙글 날아간다
	DeathSpin = FMath::RandBool() ? 1.f : -1.f;
}

void AYJJMob::TickDeath(float DeltaSeconds)
{
	DeathAge += DeltaSeconds;

	if (DeathAge < DeathFreezeTime)
	{
		const float Shake = 14.f;
		Sprite->SetRelativeLocation(FVector(FMath::FRandRange(-Shake, Shake), 0.f, FMath::FRandRange(-Shake, Shake)));
		Sprite->SetRelativeScale3D(FVector(BaseScale * 1.7f * Facing, BaseScale, BaseScale * 1.55f));
		const bool bFlash = FMath::FloorToInt(DeathAge / 0.035f) % 2 == 0;
		Sprite->SetSpriteColor(bFlash ? FLinearColor(1.f, 0.3f, 0.3f) : FLinearColor(1.f, 1.f, 0.6f));
		return;
	}

	const float Width = Data ? Data->SpriteWidth : 120.f;
	const float T = FMath::Clamp((DeathAge - DeathFreezeTime) / DeathFlyTime, 0.f, 1.f);
	const FVector Offset(DeathSpin * 300.f * T, 0.f, 260.f * T - 520.f * T * T);
	Sprite->SetRelativeLocation(Offset);
	Sprite->SetRelativeRotation(FRotator(DeathSpin * 540.f * T, 0.f, 0.f));
	const float Shrink = FMath::Lerp(1.55f, 0.9f, T);
	Sprite->SetRelativeScale3D(FVector(BaseScale * Shrink * Facing, BaseScale, BaseScale * Shrink));
	Sprite->SetSpriteColor(FLinearColor::White);

	// 날아가기 시작할 때 머리 위로 기절 별
	if (!bSpawnedStars)
	{
		bSpawnedStars = true;
		if (UPaperSprite* Star = YJJSpriteUtils::LoadSprite(YJJAssetPaths::DizzyStarSprite))
		{
			for (int32 Index = 0; Index < 3; ++Index)
			{
				FYJJSpriteFxParams Params;
				Params.Frames = { Star };
				Params.Lifetime = DeathFlyTime;
				Params.Width = Width * 0.32f;
				Params.Velocity = FVector(DeathSpin * 300.f / DeathFlyTime * 0.6f + (Index - 1) * 120.f, 0.f, 180.f);
				Params.SpinDegreesPerSecond = 540.f;
				Params.EndScale = 0.5f;
				AYJJSpriteFx::Spawn(GetWorld(), GetActorLocation() + FVector((Index - 1) * Width * 0.2f, 35.f, Width * 0.45f), Params);
			}
		}
	}

	if (T >= 1.f)
	{
		// 마무리 "뿅" 연기
		FYJJSpriteFxParams Poof;
		Poof.Frames = YJJSpriteUtils::LoadPoofFrames();
		Poof.FrameTime = 0.12f;
		Poof.Lifetime = 0.28f;
		Poof.Width = Width * 1.5f;
		Poof.StartScale = 0.7f;
		Poof.EndScale = 1.2f;
		AYJJSpriteFx::Spawn(GetWorld(), GetActorLocation() + Offset + FVector(0.f, 30.f, 0.f), Poof);
		Destroy();
	}
}
