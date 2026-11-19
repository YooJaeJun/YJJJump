#include "Visual/YJJHitEffect.h"
#include "Data/YJJData.h"
#include "Visual/YJJSpriteUtils.h"
#include "YJJAssetPaths.h"
#include "Engine/World.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

AYJJHitEffect::AYJJHitEffect()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = Sprite;

	EffectSprite = TSoftObjectPtr<UPaperSprite>(FSoftObjectPath(YJJAssetPaths::ImpactSprite));
}

AYJJHitEffect* AYJJHitEffect::Spawn(UWorld* World, const FVector& Location, float InWidth, const FLinearColor& InColor)
{
	if (!World)
	{
		return nullptr;
	}

	const FTransform Transform(Location);
	AYJJHitEffect* Effect = World->SpawnActorDeferred<AYJJHitEffect>(AYJJHitEffect::StaticClass(), Transform);
	if (Effect)
	{
		Effect->Width = InWidth;
		Effect->Color = InColor;
		Effect->FinishSpawning(Transform);
	}
	return Effect;
}

void AYJJHitEffect::BeginPlay()
{
	Super::BeginPlay();

	YJJSpriteUtils::SetSpriteWithWidth(Sprite, EffectSprite.LoadSynchronous(), Width);
	BaseScale = static_cast<float>(Sprite->GetRelativeScale3D().X);
	Sprite->SetSpriteColor(Color);
	SetLifeSpan(FYJJData::Get().Fx.ImpactLifetime);
}

void AYJJHitEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	const FYJJFxTuning& Fx = FYJJData::Get().Fx;
	const float Alpha = FMath::Clamp(Age / FMath::Max(Fx.ImpactLifetime, KINDA_SMALL_NUMBER), 0.f, 1.f);

	// 빠르게 커졌다가 끝에서 오그라들며 사라진다
	const float Grow = FMath::InterpEaseOut(Fx.ImpactGrowStart, Fx.ImpactGrowEnd, Alpha, Fx.ImpactGrowExponent);
	const float Shrink = Alpha > Fx.ImpactShrinkFrom ? FMath::Lerp(1.f, 0.f, (Alpha - Fx.ImpactShrinkFrom) / (1.f - Fx.ImpactShrinkFrom)) : 1.f;
	Sprite->SetRelativeScale3D(FVector(BaseScale * Grow * Shrink));
	Sprite->SetRelativeRotation(FRotator(Alpha * Fx.ImpactSpinDegrees, 0.f, 0.f));
}
