#pragma once

#include "CoreMinimal.h"

class UPaperSprite;
class UPaperSpriteComponent;

namespace YJJSpriteUtils
{
	// 스프라이트 원본 크기 (X=너비, Y=높이, cm). 스프라이트가 없으면 (0,0)
	YJJJUMP_API FVector2f GetSpriteSize(const UPaperSprite* Sprite);

	// 컴포넌트에 스프라이트를 넣고 지정한 너비가 되도록 균일 스케일
	YJJJUMP_API void SetSpriteWithWidth(UPaperSpriteComponent* Component, UPaperSprite* Sprite, float Width);
}
