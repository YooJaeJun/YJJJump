#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rope/YJRopeTypes.h"
#include "YJRope.generated.h"

class UStaticMeshComponent;

// 수평으로 걸린 로프. 액터 위치가 로프 중앙, Width만큼 좌우로 뻗는다.
// 점퍼가 올라타면 접촉점을 꼭짓점으로 V자로 처진다.
UCLASS()
class YJJJUMP_API AYJRope : public AActor
{
	GENERATED_BODY()

public:
	AYJRope();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FYJRopeSegment MakeSegment() const;
	int32 GetRopeId() const { return RopeId; }

	// 접촉점 X(월드)와 처짐(+아래)
	void SetContact(float ContactX, float Deflection);
	void ClearContact();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope", meta = (ClampMin = "50"))
	float Width = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope", meta = (ClampMin = "1"))
	float Thickness = 6.f;

private:
	void UpdateVisual(float ContactX, float Deflection);
	void PlaceStrand(UStaticMeshComponent* Strand, const FVector& From, const FVector& To) const;

	UPROPERTY(VisibleAnywhere, Category = "Rope")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Rope")
	TObjectPtr<UStaticMeshComponent> LeftStrand;

	UPROPERTY(VisibleAnywhere, Category = "Rope")
	TObjectPtr<UStaticMeshComponent> RightStrand;

	int32 RopeId = INDEX_NONE;
};
