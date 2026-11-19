#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJSpriteFx.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;

// 범용 스프라이트 이펙트: 프레임 애니메이션 + 속도/중력/회전 + 크기 변화. 끝나면 사라진다.
// 처치 연기, 빠진 깃털, 기절 별 등에 쓴다
USTRUCT()
struct FYJJSpriteFxParams
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UPaperSprite>> Frames;

	// 값은 부르는 쪽이 Tuning.csv에서 채운다 (크기 배율 1 = 그대로)
	float FrameTime = 0.f;
	bool bLoopFrames = false;
	float Lifetime = 0.f;
	float Width = 0.f;
	FVector Velocity = FVector::ZeroVector;
	float Gravity = 0.f;
	float SpinDegreesPerSecond = 0.f;
	float StartScale = 1.f;
	float EndScale = 1.f;
	FLinearColor Color = FLinearColor::White;
};

UCLASS()
class YJJJUMP_API AYJJSpriteFx : public AActor
{
	GENERATED_BODY()

public:
	AYJJSpriteFx();

	static AYJJSpriteFx* Spawn(UWorld* World, const FVector& Location, const FYJJSpriteFxParams& Params);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY()
	FYJJSpriteFxParams Params;

private:
	UPROPERTY(VisibleAnywhere, Category = "Fx")
	TObjectPtr<UPaperSpriteComponent> Sprite;

	float Age = 0.f;
	float BaseScale = 1.f;
	float Rotation = 0.f;
	int32 CurrentFrame = INDEX_NONE;
};
