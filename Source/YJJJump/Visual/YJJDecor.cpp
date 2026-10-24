#include "Visual/YJJDecor.h"
#include "Visual/YJJSpriteUtils.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

AYJJDecor::AYJJDecor()
{
	PrimaryActorTick.bCanEverTick = false;

	SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	SpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = SpriteComponent;
}

void AYJJDecor::BeginPlay()
{
	Super::BeginPlay();

	YJJSpriteUtils::SetSpriteWithWidth(SpriteComponent, Sprite, Width);
	if (bFlip)
	{
		FVector Scale = SpriteComponent->GetRelativeScale3D();
		Scale.X = -Scale.X;
		SpriteComponent->SetRelativeScale3D(Scale);
	}
	SpriteComponent->SetSpriteColor(Tint);
}
