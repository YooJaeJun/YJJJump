#include "Tests/YJJTestHelpers.h"
#include "Combat/YJJCombat.h"
#include "Rope/YJJRopeSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS

TArray<FYJJRopeSegment> YJJTests::MakeSingleRope()
{
	FYJJRopeSegment Rope;
	Rope.Id = 0;
	Rope.MinX = -RopeHalfWidth;
	Rope.MaxX = RopeHalfWidth;
	Rope.Height = 0.f;
	return { Rope };
}

void YJJTests::StepOnce(FYJJJumperState& State, const FYJJRopeTuning& Tuning, TConstArrayView<FYJJRopeSegment> Ropes)
{
	FYJJSimEvents Events;
	YJJRopeSim::Step(State, Tuning, Ropes, 0.f, Events);
}

FYJJJumperState YJJTests::MakeDropped(float Height)
{
	FYJJJumperState State;
	State.Mode = EYJJJumperMode::Airborne;
	State.Position = FVector2f(0.f, Height);
	return State;
}

int32 YJJTests::StepsUntilBottom(FYJJJumperState State, const FYJJRopeTuning& Tuning, TConstArrayView<FYJJRopeSegment> Ropes)
{
	for (int32 Step = 1; Step < MaxSteps; ++Step)
	{
		StepOnce(State, Tuning, Ropes);
		if (State.Mode == EYJJJumperMode::OnRope && State.bReachedBottom)
		{
			return Step;
		}
	}
	return INDEX_NONE;
}

FYJJJumperState YJJTests::RunBounce(EPressTiming Timing, const FYJJRopeTuning& Tuning)
{
	const TArray<FYJJRopeSegment> Ropes = MakeSingleRope();
	FYJJJumperState State = MakeDropped(DropHeight);
	const int32 BottomStep = StepsUntilBottom(State, Tuning, Ropes);
	const int32 EarlySteps = FMath::RoundToInt(EarlyPressSeconds / YJJRopeSim::FixedStep);
	const int32 LateSteps = FMath::RoundToInt(LatePressSeconds / YJJRopeSim::FixedStep);

	int32 PressStep = INDEX_NONE;
	switch (Timing)
	{
	case EPressTiming::Early: PressStep = BottomStep - EarlySteps; break;
	case EPressTiming::OnBottom: PressStep = BottomStep; break;
	case EPressTiming::Late: PressStep = BottomStep + LateSteps; break;
	default: break;
	}

	bool bLeftRope = false;
	for (int32 Step = 1; Step < MaxSteps; ++Step)
	{
		StepOnce(State, Tuning, Ropes);
		if (Step == PressStep)
		{
			FYJJSimEvents Events;
			YJJRopeSim::PressBounce(State, Tuning, 0.f, Events);
		}
		if (State.Mode == EYJJJumperMode::OnRope)
		{
			bLeftRope = true;
		}
		// 발사 후 늦은 입력 판정 창까지 지나면 끝 (2스텝 여유)
		if (bLeftRope && State.Mode == EYJJJumperMode::Airborne && Step > FMath::Max(PressStep, BottomStep) + LateSteps + 2)
		{
			break;
		}
	}
	return State;
}

FYJJMobTarget YJJTests::MakeMob(const FVector2f& Position, float Health, int32 XP)
{
	FYJJMobTarget Mob;
	Mob.Id = MobId;
	Mob.Position = Position;
	Mob.Radius = MobRadius;
	Mob.Health = Health;
	Mob.ScoreValue = MobScore;
	Mob.XPValue = XP;
	return Mob;
}

FYJJJumperState YJJTests::MakeFlying(const FVector2f& Velocity)
{
	FYJJJumperState State;
	State.Mode = EYJJJumperMode::Airborne;
	State.Velocity = Velocity;
	return State;
}

FYJJHitResult YJJTests::RunDash(FYJJJumperState& State, const FYJJCombatTuning& Tuning, const FYJJMobTarget& Mob)
{
	for (int32 Step = 0; Step < DashMaxSteps && !YJJCombat::StepDash(State, YJJRopeSim::FixedStep); ++Step)
	{
	}
	return YJJCombat::FinishDash(State, Tuning, &Mob);
}

TPair<float, float> YJJTests::FlyFromRope(float LaunchSpeed, const FYJJRopeTuning& Tuning)
{
	const TArray<FYJJRopeSegment> Ropes = MakeSingleRope();
	FYJJJumperState State;
	State.Mode = EYJJJumperMode::Airborne;
	State.Velocity = FVector2f(0.f, LaunchSpeed);
	State.SpeedScale = YJJRopeSim::ComputeSpeedScale(Tuning, LaunchSpeed);

	const int32 MaxFlySteps = FMath::RoundToInt(FlySeconds / YJJRopeSim::FixedStep);
	float Apex = 0.f;
	int32 Steps = 0;
	for (; Steps < MaxFlySteps && State.Mode == EYJJJumperMode::Airborne; ++Steps)
	{
		StepOnce(State, Tuning, Ropes);
		Apex = FMath::Max(Apex, State.Position.Y);
	}
	return TPair<float, float>(Apex, Steps * YJJRopeSim::FixedStep);
}

#endif
