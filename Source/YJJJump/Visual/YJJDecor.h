#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJDecor.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;

// 배경 장식 스프라이트 (구름 등). 충돌/틱 없음. 휙휙 지나가며 속도감을 준다
UCLASS()
class YJJJUMP_API AYJJDecor : public AActor
{
	GENERATED_BODY()

public:
	AYJJDecor();

	virtual void BeginPlay() override;

	// 스폰 전에 설정 (SpawnActorDeferred)
	UPROPERTY(EditAnywhere, Category = "Decor")
	TObjectPtr<UPaperSprite> Sprite;

	UPROPERTY(EditAnywhere, Category = "Decor")
	float Width = 0.f;

	UPROPERTY(EditAnywhere, Category = "Decor")
	bool bFlip = false;

	UPROPERTY(EditAnywhere, Category = "Decor")
	FLinearColor Tint = FLinearColor::White;

private:
	UPROPERTY(VisibleAnywhere, Category = "Decor")
	TObjectPtr<UPaperSpriteComponent> SpriteComponent;
};
