#include "Visual/YJJSpriteFx.h"
#include "Visual/YJJSpriteUtils.h"
#include "Engine/World.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

AYJJSpriteFx::AYJJSpriteFx()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = Sprite;
}

AYJJSpriteFx* AYJJSpriteFx::Spawn(UWorld* World, const FVector& Location, const FYJJSpriteFxParams& InParams)
{
	if (!World || InParams.Frames.IsEmpty())
	{
		return nullptr;
	}

	const FTransform Transform(Location);
	AYJJSpriteFx* Fx = World->SpawnActorDeferred<AYJJSpriteFx>(AYJJSpriteFx::StaticClass(), Transform);
	if (Fx)
	{
		Fx->Params = InParams;
		Fx->FinishSpawning(Transform);
	}
	return Fx;
}

void AYJJSpriteFx::BeginPlay()
{
	Super::BeginPlay();

	YJJSpriteUtils::SetSpriteWithWidth(Sprite, Params.Frames[0], Params.Width);
	BaseScale = static_cast<float>(Sprite->GetRelativeScale3D().X);
	Sprite->SetSpriteColor(Params.Color);
	CurrentFrame = 0;
	SetLifeSpan(Params.Lifetime + 0.1f);
}

void AYJJSpriteFx::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	const float Alpha = FMath::Clamp(Age / FMath::Max(Params.Lifetime, KINDA_SMALL_NUMBER), 0.f, 1.f);

	// 프레임 넘기기
	const int32 FrameCount = Params.Frames.Num();
	int32 Frame = FMath::FloorToInt(Age / FMath::Max(Params.FrameTime, 0.01f));
	Frame = Params.bLoopFrames ? Frame % FrameCount : FMath::Min(Frame, FrameCount - 1);
	if (Frame != CurrentFrame && Params.Frames.IsValidIndex(Frame))
	{
		CurrentFrame = Frame;
		Sprite->SetSprite(Params.Frames[Frame]);
	}

	// 이동/중력/회전/크기
	Params.Velocity.Z -= Params.Gravity * DeltaSeconds;
	AddActorWorldOffset(Params.Velocity * DeltaSeconds);
	Rotation += Params.SpinDegreesPerSecond * DeltaSeconds;
	Sprite->SetRelativeRotation(FRotator(Rotation, 0.f, 0.f));
	Sprite->SetRelativeScale3D(FVector(BaseScale * FMath::Lerp(Params.StartScale, Params.EndScale, Alpha)));

	if (Age >= Params.Lifetime)
	{
		Destroy();
	}
}
