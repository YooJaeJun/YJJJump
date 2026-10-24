#include "Combat/YJJCombat.h"

namespace YJJCombat
{
	namespace
	{
		float GradeMultiplier(const FYJJCombatTuning& Tuning, EYJJBounceGrade Grade)
		{
			switch (Grade)
			{
			case EYJJBounceGrade::Perfect:
				return Tuning.PerfectAttackMultiplier;
			case EYJJBounceGrade::Good:
				return Tuning.GoodAttackMultiplier;
			case EYJJBounceGrade::Miss:
				return Tuning.MissAttackMultiplier;
			default:
				return 1.f;
			}
		}
	}

	float ComputeAttackPower(const FYJJJumperState& State, const FYJJCombatTuning& Tuning)
	{
		const float SpeedFactor = State.Velocity.Size() / FMath::Max(Tuning.BaseAttackSpeed, 1.f);
		const float ChainFactor = 1.f + State.PerfectChain * Tuning.ChainAttackBonus;
		return FMath::Max(SpeedFactor * GradeMultiplier(Tuning, State.LaunchGrade) * ChainFactor, Tuning.MinAttackPower);
	}

	FYJJHitResult ResolveHit(FYJJJumperState& State, const FYJJCombatTuning& Tuning, const FYJJMobTarget& Mob)
	{
		FYJJHitResult Result;
		if (State.Mode != EYJJJumperMode::Airborne || Mob.Health <= 0.f)
		{
			return Result;
		}

		const FVector2f Delta = State.Position - Mob.Position;
		const float Reach = Mob.Radius + Tuning.JumperRadius;
		if (Delta.SizeSquared() > Reach * Reach)
		{
			return Result;
		}

		Result.bHit = true;
		Result.Damage = ComputeAttackPower(State, Tuning);
		Result.bKilled = Result.Damage >= Mob.Health;

		if (Result.bKilled)
		{
			// 관통: 진행 방향 유지, 속도만 조금 손해
			State.Velocity *= Tuning.KillSpeedRetain;
			Result.ScoreGained = FMath::RoundToInt(Mob.ScoreValue * (1.f + State.PerfectChain * Tuning.ChainScoreBonus));
			State.Score += Result.ScoreGained;
			State.Kills++;
		}
		else
		{
			// 막힘: 몹 중심 반대 방향으로 튕겨 나오고 겹치지 않게 밀어낸다
			const FVector2f Normal = Delta.IsNearlyZero() ? FVector2f(0.f, -1.f) : Delta.GetSafeNormal();
			State.Velocity = Normal * Tuning.BlockedBounceSpeed;
			State.Position = Mob.Position + Normal * (Reach + 1.f);
		}
		return Result;
	}
}
