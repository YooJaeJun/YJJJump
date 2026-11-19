#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJHitEffect.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;

// 타격/처치 순간 잠깐 커지며 사라지는 별 모양 이펙트
UCLASS()
class YJJJUMP_API AYJJHitEffect : public AActor
{
	GENERATED_BODY()

public:
	AYJJHitEffect();

	static AYJJHitEffect* Spawn(UWorld* World, const FVector& Location, float Width, const FLinearColor& Color);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Effect")
	TSoftObjectPtr<UPaperSprite> EffectSprite;

	// 시간·크기 변화는 Tuning.csv Fx.Impact*
	UPROPERTY(EditAnywhere, Category = "Effect")
	float Width = 0.f;

	UPROPERTY(EditAnywhere, Category = "Effect")
	FLinearColor Color = FLinearColor::White;

private:
	UPROPERTY(VisibleAnywhere, Category = "Effect")
	TObjectPtr<UPaperSpriteComponent> Sprite;

	float Age = 0.f;
	float BaseScale = 1.f;
};
