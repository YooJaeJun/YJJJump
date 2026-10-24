#include "Visual/YJJSpriteUtils.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

namespace YJJSpriteUtils
{
	FVector2f GetSpriteSize(const UPaperSprite* Sprite)
	{
		if (!Sprite)
		{
			return FVector2f::ZeroVector;
		}

		// 스프라이트는 XZ 평면에 그려진다
		const FBoxSphereBounds Bounds = Sprite->GetRenderBounds();
		return FVector2f(static_cast<float>(Bounds.BoxExtent.X * 2.0), static_cast<float>(Bounds.BoxExtent.Z * 2.0));
	}

	void SetSpriteWithWidth(UPaperSpriteComponent* Component, UPaperSprite* Sprite, float Width)
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
}
