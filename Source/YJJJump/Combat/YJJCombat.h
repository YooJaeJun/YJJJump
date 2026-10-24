#pragma once

#include "CoreMinimal.h"
#include "Combat/YJJCombatTypes.h"
#include "Rope/YJJRopeTypes.h"

// 점퍼 vs 몹 전투 계산. YJJRopeSim과 마찬가지로 UObject 없는 순수 함수
namespace YJJCombat
{
	// 현재 속도와 발사 판정으로 정해지는 공격력
	YJJJUMP_API float ComputeAttackPower(const FYJJJumperState& State, const FYJJCombatTuning& Tuning);

	// 공중에서 몹과 겹치면 피해를 주고 점퍼 상태(속도/점수)를 갱신한다.
	// 몹 체력 감소는 호출한 쪽이 Result.Damage로 적용
	YJJJUMP_API FYJJHitResult ResolveHit(FYJJJumperState& State, const FYJJCombatTuning& Tuning, const FYJJMobTarget& Mob);
}
