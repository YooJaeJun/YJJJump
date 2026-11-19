#pragma once

#include "CoreMinimal.h"
#include "Combat/YJJCombatTypes.h"
#include "Misc/AutomationTest.h"
#include "Rope/YJJRopeTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

// 자동화 테스트 공용 도우미와 시나리오 값. 튜닝 값은 실제 게임과 같은 Tuning.csv (FYJJData)
namespace YJJTests
{
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;

	// 테스트 로프: 높이 0, 폭 (-HalfWidth ~ HalfWidth)
	inline constexpr float RopeHalfWidth = 500.f;

	// 떨어뜨리는 높이 (cm)
	inline constexpr float DropHeight = 600.f;
	inline constexpr float LowDropHeight = 300.f;

	// 최저점 기준 이른/늦은 입력 시간 (초)
	inline constexpr float EarlyPressSeconds = 0.03f;
	inline constexpr float LatePressSeconds = 0.10f;

	// 시뮬레이션을 돌리는 최대 스텝 (무한 루프 방지)
	inline constexpr int32 MaxSteps = 2000;
	inline constexpr int32 SettleSeconds = 30;
	inline constexpr int32 FlySeconds = 60;
	inline constexpr int32 DashMaxSteps = 120;

	// 테스트 몬스터
	inline constexpr int32 MobId = 7;
	inline constexpr float MobRadius = 50.f;
	inline constexpr int32 MobScore = 50;

	enum class EPressTiming { None, Early, OnBottom, Late };

	TArray<FYJJRopeSegment> MakeSingleRope();
	void StepOnce(FYJJJumperState& State, const FYJJRopeTuning& Tuning, TConstArrayView<FYJJRopeSegment> Ropes);

	// 로프 위 Height에서 떨어뜨린 상태
	FYJJJumperState MakeDropped(float Height);

	// 최저점에 도달하는 스텝 수 (시뮬레이션이 결정적이라 미리 계산 가능)
	int32 StepsUntilBottom(FYJJJumperState State, const FYJJRopeTuning& Tuning, TConstArrayView<FYJJRopeSegment> Ropes);

	// 떨어져서 한 번 튕길 때까지 돌리고 상태를 돌려준다
	FYJJJumperState RunBounce(EPressTiming Timing, const FYJJRopeTuning& Tuning);

	FYJJMobTarget MakeMob(const FVector2f& Position, float Health, int32 XP = 1);
	FYJJJumperState MakeFlying(const FVector2f& Velocity);

	// 돌진을 끝까지 진행
	FYJJHitResult RunDash(FYJJJumperState& State, const FYJJCombatTuning& Tuning, const FYJJMobTarget& Mob);

	// 로프에서 LaunchSpeed로 쏘아 올려 다시 로프에 닿을 때까지: (최고 높이, 비행 시간)
	TPair<float, float> FlyFromRope(float LaunchSpeed, const FYJJRopeTuning& Tuning);
}

#endif
