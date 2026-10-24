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

	float GetAttackRange(const FYJJJumperState& State, const FYJJCombatTuning& Tuning)
	{
		const int32 Level = State.bTutorial ? 999 : State.Level;
		return FMath::Min(Tuning.BaseAttackRange + Tuning.AttackRangePerLevel * (Level - 1), Tuning.MaxAttackRange);
	}

	float ComputeAttackPower(const FYJJJumperState& State, const FYJJCombatTuning& Tuning)
	{
		if (State.bTutorial)
		{
			return Tuning.TutorialPower;
		}

		const float LevelFactor = 1.f + Tuning.PowerPerLevel * (State.Level - 1);
		const float ChainFactor = 1.f + State.PerfectChain * Tuning.ChainAttackBonus;
		const float SpeedFactor = FMath::Pow(FMath::Max(State.SpeedScale, 1.f), Tuning.SpeedPowerExponent);
		return LevelFactor * GradeMultiplier(Tuning, State.LaunchGrade) * ChainFactor * SpeedFactor;
	}

	bool IsTapOnMob(const FYJJCombatTuning& Tuning, const FVector2f& TapPosition, const FYJJMobTarget& Mob)
	{
		const float Reach = Mob.Radius + Tuning.TapTolerance;
		return Mob.Health > 0.f && FVector2f::DistSquared(TapPosition, Mob.Position) <= Reach * Reach;
	}

	bool CanDashTo(const FYJJJumperState& State, const FYJJCombatTuning& Tuning, const FYJJMobTarget& Mob)
	{
		if (State.Mode != EYJJJumperMode::Airborne || State.IsDashing() || Mob.Health <= 0.f)
		{
			return false;
		}
		const float Range = GetAttackRange(State, Tuning) + Mob.Radius;
		return FVector2f::DistSquared(State.Position, Mob.Position) <= Range * Range;
	}

	bool BeginDash(FYJJJumperState& State, const FYJJCombatTuning& Tuning, const FYJJMobTarget& Mob)
	{
		if (!CanDashTo(State, Tuning, Mob))
		{
			return false;
		}

		State.DashMobId = Mob.Id;
		State.DashFrom = State.Position;
		State.DashTo = Mob.Position;
		State.DashSavedVelocity = State.Velocity;
		State.DashDuration = Tuning.DashDuration;
		State.DashTimeLeft = Tuning.DashDuration;
		return true;
	}

	bool StepDash(FYJJJumperState& State, float DeltaSeconds)
	{
		if (!State.IsDashing())
		{
			return false;
		}

		// 돌진 중엔 중력 없이 직선으로 날아간다
		State.DashTimeLeft = FMath::Max(State.DashTimeLeft - DeltaSeconds, 0.f);
		const float Alpha = 1.f - State.DashTimeLeft / FMath::Max(State.DashDuration, KINDA_SMALL_NUMBER);
		State.Position = FMath::Lerp(State.DashFrom, State.DashTo, Alpha);
		State.MaxHeightReached = FMath::Max(State.MaxHeightReached, State.Position.Y);
		return State.DashTimeLeft <= 0.f;
	}

	FYJJHitResult FinishDash(FYJJJumperState& State, const FYJJCombatTuning& Tuning, const FYJJMobTarget* Mob)
	{
		FYJJHitResult Result;

		// 처치하든 못 하든 원래 궤도(속도)로 계속 날아간다
		State.Velocity = State.DashSavedVelocity;
		State.DashMobId = INDEX_NONE;
		State.DashTimeLeft = 0.f;

		if (!Mob || Mob->Health <= 0.f)
		{
			return Result;
		}

		Result.bHit = true;
		Result.Damage = ComputeAttackPower(State, Tuning);
		Result.bKilled = Result.Damage >= Mob->Health;
		if (Result.bKilled)
		{
			Result.ScoreGained = FMath::RoundToInt(Mob->ScoreValue * (1.f + State.PerfectChain * Tuning.ChainScoreBonus));
			Result.XPGained = State.bTutorial ? 0 : Mob->XPValue;
			State.Score += Result.ScoreGained;
			State.Kills++;
			State.KillCombo++;
			Result.LevelsGained = GainXP(State, Tuning, Result.XPGained);
		}
		return Result;
	}

	int32 XPToNextLevel(const FYJJCombatTuning& Tuning, int32 Level)
	{
		return FMath::Max(FMath::RoundToInt(Tuning.XPBase * FMath::Pow(static_cast<float>(FMath::Max(Level, 1)), Tuning.XPExponent)), 1);
	}

	int32 GainXP(FYJJJumperState& State, const FYJJCombatTuning& Tuning, int32 XP)
	{
		if (State.bTutorial || XP <= 0)
		{
			return 0;
		}

		int32 Gained = 0;
		State.XP += XP;
		for (int32 Need = XPToNextLevel(Tuning, State.Level); State.XP >= Need; Need = XPToNextLevel(Tuning, State.Level))
		{
			State.XP -= Need;
			State.Level++;
			Gained++;
		}
		return Gained;
	}
}
