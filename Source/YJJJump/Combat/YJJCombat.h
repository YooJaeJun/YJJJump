#pragma once

#include "CoreMinimal.h"
#include "Combat/YJJCombatTypes.h"
#include "Rope/YJJRopeTypes.h"

// 장수풍뎅이 vs 몬스터 전투 계산. YJJRopeSim과 마찬가지로 UObject 없는 순수 함수
namespace YJJCombat
{
	YJJJUMP_API float GetAttackRange(const FYJJJumperState& State, const FYJJCombatTuning& Tuning);

	// 레벨, 발사 판정, 연속 퍼펙트, 속도로 정해지는 공격력
	YJJJUMP_API float ComputeAttackPower(const FYJJJumperState& State, const FYJJCombatTuning& Tuning);

	// 탭한 지점(월드 X,Z)이 이 몬스터를 가리키는지
	YJJJUMP_API bool IsTapOnMob(const FYJJCombatTuning& Tuning, const FVector2f& TapPosition, const FYJJMobTarget& Mob);

	// 공중에서 사거리 안의 몬스터에게만 돌진 가능
	YJJJUMP_API bool CanDashTo(const FYJJJumperState& State, const FYJJCombatTuning& Tuning, const FYJJMobTarget& Mob);
	YJJJUMP_API bool BeginDash(FYJJJumperState& State, const FYJJCombatTuning& Tuning, const FYJJMobTarget& Mob);

	// 돌진 진행. 몬스터에 도착한 스텝에서 true -> FinishDash 호출
	YJJJUMP_API bool StepDash(FYJJJumperState& State, float DeltaSeconds);

	// 도착: 피해/처치/경험치를 처리하고 원래 속도로 비행을 이어간다.
	// Mob이 null이면 (그 사이 사라짐) 비행만 이어간다. 몬스터 체력 감소는 호출한 쪽이 Result.Damage로 적용
	YJJJUMP_API FYJJHitResult FinishDash(FYJJJumperState& State, const FYJJCombatTuning& Tuning, const FYJJMobTarget* Mob);

	YJJJUMP_API int32 XPToNextLevel(const FYJJCombatTuning& Tuning, int32 Level);

	// 경험치를 더하고 오른 레벨 수를 돌려준다
	YJJJUMP_API int32 GainXP(FYJJJumperState& State, const FYJJCombatTuning& Tuning, int32 XP);

	// 발사 판정에 따른 공격 배율
	YJJJUMP_API float GradeMultiplier(const FYJJCombatTuning& Tuning, EYJJBounceGrade Grade);
}
