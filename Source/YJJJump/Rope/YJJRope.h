#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rope/YJJRopeTypes.h"
#include "YJJRope.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;

// 수평으로 걸린 로프. 액터 위치가 로프 중앙, Width만큼 좌우로 뻗는다.
// 점퍼가 올라타면 접촉점을 꼭짓점으로 V자로 처진다.
UCLASS()
class YJJJUMP_API AYJJRope : public AActor
{
	GENERATED_BODY()

public:
	AYJJRope();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FYJJRopeSegment MakeSegment() const;
	int32 GetRopeId() const { return RopeId; }

	// 접촉점 X(월드)와 처짐(+아래)
	void SetContact(float ContactX, float Deflection);
	void ClearContact();

	// 게임모드가 스폰 전에 설정 (벽에서 벽까지). 굵기·말뚝은 Tuning.csv Stage.*
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope")
	float Width = 0.f;

	UPROPERTY(EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UPaperSprite> StrandSprite;

	UPROPERTY(EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UPaperSprite> PegSprite;

private:
	void ApplySprites();
	void UpdateVisual(float ContactX, float Deflection);
	void PlaceStrand(UPaperSpriteComponent* Strand, const FVector& From, const FVector& To) const;

	UPROPERTY(VisibleAnywhere, Category = "Rope")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Rope")
	TObjectPtr<UPaperSpriteComponent> LeftStrand;

	UPROPERTY(VisibleAnywhere, Category = "Rope")
	TObjectPtr<UPaperSpriteComponent> RightStrand;

	UPROPERTY(VisibleAnywhere, Category = "Rope")
	TObjectPtr<UPaperSpriteComponent> LeftPeg;

	UPROPERTY(VisibleAnywhere, Category = "Rope")
	TObjectPtr<UPaperSpriteComponent> RightPeg;

	FVector2f StrandSpriteSize = FVector2f::ZeroVector;
	int32 RopeId = INDEX_NONE;
};
