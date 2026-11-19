#include "Combat/YJJMob.h"
#include "Combat/YJJMobSubsystem.h"
#include "Data/YJJData.h"
#include "Visual/YJJHitEffect.h"
#include "Visual/YJJSpriteFx.h"
#include "Visual/YJJSpriteUtils.h"
#include "YJJAssetPaths.h"
#include "YJJJump.h"
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

	if (const FYJJMobRow* Row = FYJJData::Get().FindMob(MobType))
	{
		Def = *Row;
	}
	else
	{
		UE_LOG(LogYJJJump, Error, TEXT("[Mob] Mobs.csv 에 %s 가 없다"), *MobType.ToString());
	}

	Home = GetActorLocation();
	Health = Def.MaxHealth * StatMultiplier;

	IdleSprite = Def.Sprite.LoadSynchronous();
	FlapSprite = Def.FlapSprite.LoadSynchronous();
	HitSprite = Def.HitSprite.LoadSynchronous();
	YJJSpriteUtils::SetSpriteWithWidth(Sprite, IdleSprite, Def.SpriteWidth);
	BaseScale = static_cast<float>(Sprite->GetRelativeScale3D().X);

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

	const FYJJMobFxTuning& Fx = FYJJData::Get().MobFx;
	HitPunch = FMath::Max(HitPunch - DeltaSeconds * Fx.HitPunchDecay, 0.f);

	FVector Offset = FVector::ZeroVector;
	const float Phase = UE_TWO_PI * Age / FMath::Max(Def.MovePeriod, KINDA_SMALL_NUMBER) + MovePhase;
	switch (Def.Movement)
	{
	case EYJJMobMovement::Patrol:
		Offset.X = FMath::Sin(Phase) * FMath::Min(Def.MoveRange, MaxMoveRange);
		// 진행 방향을 바라본다
		Facing = FMath::Cos(Phase) >= 0.f ? 1.f : -1.f;
		break;
	case EYJJMobMovement::Hover:
		Offset.Z = FMath::Sin(Phase) * Def.MoveRange;
		break;
	default:
		Offset.Z = FMath::Sin(Phase) * Fx.IdleBob;
		break;
	}

	// 날갯짓: 두 프레임을 번갈아
	if (FlapSprite)
	{
		const bool bWingsUp = FMath::FloorToInt((Age + MovePhase) / FMath::Max(Def.FlapInterval, Fx.MinFlapInterval)) % 2 == 1;
		Sprite->SetSprite(bWingsUp ? FlapSprite.Get() : IdleSprite.Get());
	}
	SetActorLocation(Home + Offset);
	UpdateVisualScale();
}

void AYJJMob::UpdateVisualScale()
{
	// 맞으면 잠깐 커졌다 돌아오고 빨갛게 번쩍
	const FYJJData& Data = FYJJData::Get();
	const float Punch = 1.f + HitPunch * Data.MobFx.HitPunchScale;
	Sprite->SetRelativeScale3D(FVector(BaseScale * Punch * Facing, BaseScale, BaseScale * Punch));
	Sprite->SetSpriteColor(FLinearColor::LerpUsingHSV(FLinearColor::White, Data.Colors.MobHitFlash, HitPunch));
}

FYJJMobTarget AYJJMob::MakeTarget() const
{
	const FVector3f Location(GetActorLocation());

	FYJJMobTarget Target;
	Target.Id = MobId;
	Target.Position = FVector2f(Location.X, Location.Z);
	Target.Radius = Def.Radius;
	Target.Health = Health;
	Target.ScoreValue = FMath::RoundToInt(Def.ScoreValue * StatMultiplier);
	Target.XPValue = FMath::Max(FMath::RoundToInt(Def.XPValue * StatMultiplier), 1);
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
	bSpawnedStars = false;
	if (UYJJMobSubsystem* Mobs = GetWorld()->GetSubsystem<UYJJMobSubsystem>())
	{
		Mobs->UnregisterMob(this);
	}

	// 기절 프레임 (X 눈, 혹)
	Sprite->SetSprite(HitSprite ? HitSprite.Get() : IdleSprite.Get());

	const FYJJData& Data = FYJJData::Get();
	const FYJJMobFxTuning& Fx = Data.MobFx;
	const FVector Center = GetActorLocation();
	const float Width = Def.SpriteWidth;
	AYJJHitEffect::Spawn(GetWorld(), Center + FVector(0.f, Fx.ImpactDepth, 0.f), Width * Fx.ImpactWidthRatio, Data.Colors.MobDeathImpact);

	// 깃털이 사방으로 빠진다
	if (UPaperSprite* Feather = YJJSpriteUtils::LoadSprite(YJJAssetPaths::FeatherSprite))
	{
		const int32 Count = Def.DeathFeathers;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Angle = UE_TWO_PI * (Index + FMath::FRand() * Fx.FeatherAngleJitter) / FMath::Max(Count, 1);
			FYJJSpriteFxParams Params;
			Params.Frames = { Feather };
			Params.Lifetime = FMath::FRandRange(Fx.FeatherLifeMin, Fx.FeatherLifeMax);
			Params.Width = Width * Fx.FeatherWidthRatio;
			Params.Velocity = FVector(FMath::Cos(Angle), 0.f, FMath::Sin(Angle) + Fx.FeatherUpBias) * FMath::FRandRange(Fx.FeatherSpeedMin, Fx.FeatherSpeedMax);
			Params.Gravity = Fx.FeatherGravity;
			Params.SpinDegreesPerSecond = FMath::FRandRange(-Fx.FeatherSpinMax, Fx.FeatherSpinMax);
			Params.EndScale = Fx.FeatherEndScale;
			AYJJSpriteFx::Spawn(GetWorld(), Center + FVector(0.f, Fx.FeatherDepth, 0.f), Params);
		}
	}

	// 맞은 반대쪽으로 빙글빙글 날아간다
	DeathSpin = FMath::RandBool() ? 1.f : -1.f;
}

void AYJJMob::TickDeath(float DeltaSeconds)
{
	DeathAge += DeltaSeconds;

	const FYJJData& Data = FYJJData::Get();
	const FYJJMobFxTuning& Fx = Data.MobFx;
	if (DeathAge < Fx.DeathFreezeTime)
	{
		const float Shake = Fx.DeathShake;
		Sprite->SetRelativeLocation(FVector(FMath::FRandRange(-Shake, Shake), 0.f, FMath::FRandRange(-Shake, Shake)));
		Sprite->SetRelativeScale3D(FVector(BaseScale * Fx.DeathFreezeScaleX * Facing, BaseScale, BaseScale * Fx.DeathFreezeScaleZ));
		const bool bFlash = FMath::FloorToInt(DeathAge / Fx.DeathFlashInterval) % 2 == 0;
		Sprite->SetSpriteColor(bFlash ? Data.Colors.MobHitFlash : Data.Colors.MobDeathFlash);
		return;
	}

	const float Width = Def.SpriteWidth;
	const float T = FMath::Clamp((DeathAge - Fx.DeathFreezeTime) / Fx.DeathFlyTime, 0.f, 1.f);
	const FVector Offset(DeathSpin * Fx.DeathFlyDistance * T, 0.f, Fx.DeathFlyRise * T - Fx.DeathFlyDrop * T * T);
	Sprite->SetRelativeLocation(Offset);
	Sprite->SetRelativeRotation(FRotator(DeathSpin * Fx.DeathSpinDegrees * T, 0.f, 0.f));
	const float Shrink = FMath::Lerp(Fx.DeathShrinkStart, Fx.DeathShrinkEnd, T);
	Sprite->SetRelativeScale3D(FVector(BaseScale * Shrink * Facing, BaseScale, BaseScale * Shrink));
	Sprite->SetSpriteColor(FLinearColor::White);

	// 날아가기 시작할 때 머리 위로 기절 별
	if (!bSpawnedStars)
	{
		bSpawnedStars = true;
		if (UPaperSprite* Star = YJJSpriteUtils::LoadSprite(YJJAssetPaths::DizzyStarSprite))
		{
			// 가운데 별을 기준으로 좌우에 나란히
			const float Middle = (Fx.StarCount - 1) * 0.5f;
			for (int32 Index = 0; Index < Fx.StarCount; ++Index)
			{
				const float Side = Index - Middle;
				FYJJSpriteFxParams Params;
				Params.Frames = { Star };
				Params.Lifetime = Fx.DeathFlyTime;
				Params.Width = Width * Fx.StarWidthRatio;
				Params.Velocity = FVector(DeathSpin * Fx.DeathFlyDistance / Fx.DeathFlyTime * Fx.StarFollowRatio + Side * Fx.StarSpread, 0.f, Fx.StarRise);
				Params.SpinDegreesPerSecond = Fx.StarSpin;
				Params.EndScale = Fx.StarEndScale;
				AYJJSpriteFx::Spawn(GetWorld(), GetActorLocation() + FVector(Side * Width * Fx.StarSpacingRatio, Fx.StarDepth, Width * Fx.StarHeightRatio), Params);
			}
		}
	}

	if (T >= 1.f)
	{
		// 마무리 "뿅" 연기
		FYJJSpriteFxParams Poof;
		Poof.Frames = YJJSpriteUtils::LoadPoofFrames();
		Poof.FrameTime = Fx.PoofFrameTime;
		Poof.Lifetime = Fx.PoofLifetime;
		Poof.Width = Width * Fx.PoofWidthRatio;
		Poof.StartScale = Fx.PoofStartScale;
		Poof.EndScale = Fx.PoofEndScale;
		AYJJSpriteFx::Spawn(GetWorld(), GetActorLocation() + Offset + FVector(0.f, Fx.PoofDepth, 0.f), Poof);
		Destroy();
	}
}
