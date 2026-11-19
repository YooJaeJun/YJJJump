#include "Visual/YJJSpriteUtils.h"
#include "YJJAssetPaths.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

FVector2f YJJSpriteUtils::GetSpriteSize(const UPaperSprite* Sprite)
{
	if (!Sprite)
	{
		return FVector2f::ZeroVector;
	}

	// 스프라이트는 XZ 평면에 그려진다
	const FBoxSphereBounds Bounds = Sprite->GetRenderBounds();
	return FVector2f(static_cast<float>(Bounds.BoxExtent.X * 2.0), static_cast<float>(Bounds.BoxExtent.Z * 2.0));
}

void YJJSpriteUtils::SetSpriteWithWidth(UPaperSpriteComponent* Component, UPaperSprite* Sprite, float Width)
{
	if (!Component)
	{
		return;
	}

	Component->SetSprite(Sprite);
	const FVector2f Size = GetSpriteSize(Sprite);
	if (Size.X > KINDA_SMALL_NUMBER)
	{
		Component->SetRelativeScale3D(FVector(Width / Size.X));
	}
}

UPaperSprite* YJJSpriteUtils::LoadSprite(const TCHAR* Path)
{
	return LoadObject<UPaperSprite>(nullptr, Path);
}

TArray<TObjectPtr<UPaperSprite>> YJJSpriteUtils::LoadPoofFrames()
{
	TArray<TObjectPtr<UPaperSprite>> Frames;
	for (const TCHAR* Path : { YJJAssetPaths::Poof0Sprite, YJJAssetPaths::Poof1Sprite })
	{
		if (UPaperSprite* Frame = LoadSprite(Path))
		{
			Frames.Add(Frame);
		}
	}
	return Frames;
}
