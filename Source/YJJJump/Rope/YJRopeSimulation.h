#pragma once

#include "CoreMinimal.h"
#include "Rope/YJRopeTypes.h"

// 로프 반동 시뮬레이션. UObject에 의존하지 않는 순수 함수라서
// 서버(권한)와 클라이언트(예측)가 같은 코드로 같은 결과를 낸다.
// 고정 스텝(FixedStep)으로만 호출할 것.
namespace YJRopeSim
{
	constexpr float FixedStep = 1.f / 120.f;

	// 로프 위에서 가만히 있을 때의 처짐 (중력과 스프링이 평형)
	YJJJUMP_API float RestDeflection(const FYJRopeTuning& Tuning);

	YJJJUMP_API void Step(FYJJumperState& State, const FYJRopeTuning& Tuning, TConstArrayView<FYJRopeSegment> Ropes, float CenterX, FYJSimEvents& OutEvents);

	// 튕기기 입력. AimX(-1~1)는 캐릭터 기준 좌우 방향
	YJJJUMP_API void PressBounce(FYJJumperState& State, const FYJRopeTuning& Tuning, float AimX, FYJSimEvents& OutEvents);

	// 드래그 (Resting 상태에서만 시작 가능). DragPixels: 누른 지점 기준 이동량, Y+는 아래로 당김
	YJJJUMP_API bool BeginDrag(FYJJumperState& State);
	YJJJUMP_API void UpdateDrag(FYJJumperState& State, const FYJRopeTuning& Tuning, const FVector2f& DragPixels);
	YJJJUMP_API void ReleaseDrag(FYJJumperState& State, const FYJRopeTuning& Tuning);

	// 지정한 로프 위에 정지 상태로 올려놓기 (시작/낙하 복귀용)
	YJJJUMP_API void PlaceOnRope(FYJJumperState& State, const FYJRopeTuning& Tuning, const FYJRopeSegment& Rope, float X);

	YJJJUMP_API const FYJRopeSegment* FindRope(TConstArrayView<FYJRopeSegment> Ropes, int32 RopeId);
}
