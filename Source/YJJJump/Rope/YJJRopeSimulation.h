#pragma once

#include "CoreMinimal.h"
#include "Rope/YJJRopeTypes.h"

// 로프 반동 시뮬레이션. UObject에 의존하지 않는 순수 함수라서
// 서버(권한)와 클라이언트(예측)가 같은 코드로 같은 결과를 낸다.
// 고정 스텝(FixedStep)으로만 호출할 것.
namespace YJJRopeSim
{
	constexpr float FixedStep = 1.f / 120.f;

	// 로프 위에서 가만히 있을 때의 처짐 (중력과 스프링이 평형)
	YJJJUMP_API float RestDeflection(const FYJJRopeTuning& Tuning);

	YJJJUMP_API void Step(FYJJJumperState& State, const FYJJRopeTuning& Tuning, TConstArrayView<FYJJRopeSegment> Ropes, float CenterX, FYJJSimEvents& OutEvents);

	// 튕기기 입력. AimX(-1~1)는 캐릭터 기준 좌우 방향
	YJJJUMP_API void PressBounce(FYJJJumperState& State, const FYJJRopeTuning& Tuning, float AimX, FYJJSimEvents& OutEvents);

	// 드래그 (Resting 상태에서만 시작 가능). DragPixels: 누른 지점 기준 이동량, Y+는 아래로 당김
	YJJJUMP_API bool BeginDrag(FYJJJumperState& State);
	YJJJUMP_API void UpdateDrag(FYJJJumperState& State, const FYJJRopeTuning& Tuning, const FVector2f& DragPixels);
	YJJJUMP_API void ReleaseDrag(FYJJJumperState& State, const FYJJRopeTuning& Tuning);

	// 지정한 로프 위에 정지 상태로 올려놓기 (시작/낙하 복귀용)
	YJJJUMP_API void PlaceOnRope(FYJJJumperState& State, const FYJJRopeTuning& Tuning, const FYJJRopeSegment& Rope, float X);

	YJJJUMP_API const FYJJRopeSegment* FindRope(TConstArrayView<FYJJRopeSegment> Ropes, int32 RopeId);
}
