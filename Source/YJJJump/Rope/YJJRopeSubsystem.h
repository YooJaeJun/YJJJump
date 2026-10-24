#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Rope/YJJRopeTypes.h"
#include "YJJRopeSubsystem.generated.h"

class AYJJRope;

// 월드에 있는 로프들을 모아 시뮬레이션용 세그먼트 목록으로 제공
UCLASS()
class YJJJUMP_API UYJJRopeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	int32 RegisterRope(AYJJRope* Rope);
	void UnregisterRope(AYJJRope* Rope);

	TConstArrayView<FYJJRopeSegment> GetSegments() const { return Segments; }
	AYJJRope* FindRopeActor(int32 RopeId) const;
	bool HasRopes() const { return Segments.Num() > 0; }
	float GetLowestHeight() const;

	// 해당 X에서 Z 아래에 있는 가장 높은 로프
	const FYJJRopeSegment* FindRopeBelow(float X, float Z) const;

private:
	UPROPERTY()
	TArray<TObjectPtr<AYJJRope>> Ropes;

	TArray<FYJJRopeSegment> Segments;
	int32 NextRopeId = 0;
};
