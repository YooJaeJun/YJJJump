#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Rope/YJRopeTypes.h"
#include "YJRopeSubsystem.generated.h"

class AYJRope;

// 월드에 있는 로프들을 모아 시뮬레이션용 세그먼트 목록으로 제공
UCLASS()
class YJJJUMP_API UYJRopeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	int32 RegisterRope(AYJRope* Rope);
	void UnregisterRope(AYJRope* Rope);

	TConstArrayView<FYJRopeSegment> GetSegments() const { return Segments; }
	AYJRope* FindRopeActor(int32 RopeId) const;
	bool HasRopes() const { return Segments.Num() > 0; }
	float GetLowestHeight() const;

	// 해당 X에서 Z 아래에 있는 가장 높은 로프
	const FYJRopeSegment* FindRopeBelow(float X, float Z) const;

private:
	UPROPERTY()
	TArray<TObjectPtr<AYJRope>> Ropes;

	TArray<FYJRopeSegment> Segments;
	int32 NextRopeId = 0;
};
