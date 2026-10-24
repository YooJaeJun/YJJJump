// 로프 반동/전투 계산 자동화 테스트.
// 실행: 에디터 Session Frontend > Automation 에서 "YJJJump" 필터,
// 또는 Tools/RunTests.ps1

#include "Combat/YJJCombat.h"
#include "Rope/YJJRopeSimulation.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace YJJTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;

	TArray<FYJJRopeSegment> MakeSingleRope()
	{
		FYJJRopeSegment Rope;
		Rope.Id = 0;
		Rope.MinX = -500.f;
		Rope.MaxX = 500.f;
		Rope.Height = 0.f;
		return { Rope };
	}

	void StepOnce(FYJJJumperState& State, const FYJJRopeTuning& Tuning, TConstArrayView<FYJJRopeSegment> Ropes)
	{
		FYJJSimEvents Events;
		YJJRopeSim::Step(State, Tuning, Ropes, 0.f, Events);
	}

	// 로프 위 Height에서 떨어뜨린 상태
	FYJJJumperState MakeDropped(float Height)
	{
		FYJJJumperState State;
		State.Mode = EYJJJumperMode::Airborne;
		State.Position = FVector2f(0.f, Height);
		return State;
	}

	// 최저점에 도달하는 스텝 수 (시뮬레이션이 결정적이라 미리 계산 가능)
	int32 StepsUntilBottom(FYJJJumperState State, const FYJJRopeTuning& Tuning, TConstArrayView<FYJJRopeSegment> Ropes)
	{
		for (int32 Step = 1; Step < 2000; ++Step)
		{
			StepOnce(State, Tuning, Ropes);
			if (State.Mode == EYJJJumperMode::OnRope && State.bReachedBottom)
			{
				return Step;
			}
		}
		return INDEX_NONE;
	}

	enum class EPressTiming { None, Early, OnBottom, Late };

	// 떨어져서 한 번 튕길 때까지 돌리고 상태를 돌려준다
	FYJJJumperState RunBounce(EPressTiming Timing, const FYJJRopeTuning& Tuning)
	{
		const TArray<FYJJRopeSegment> Ropes = MakeSingleRope();
		FYJJJumperState State = MakeDropped(600.f);
		const int32 BottomStep = StepsUntilBottom(State, Tuning, Ropes);
		const int32 EarlySteps = FMath::RoundToInt(0.03f / YJJRopeSim::FixedStep);
		const int32 LateSteps = FMath::RoundToInt(0.10f / YJJRopeSim::FixedStep);

		int32 PressStep = INDEX_NONE;
		switch (Timing)
		{
		case EPressTiming::Early: PressStep = BottomStep - EarlySteps; break;
		case EPressTiming::OnBottom: PressStep = BottomStep; break;
		case EPressTiming::Late: PressStep = BottomStep + LateSteps; break;
		default: break;
		}

		bool bLeftRope = false;
		for (int32 Step = 1; Step < 2000; ++Step)
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
			// 발사 후 늦은 입력 판정 창까지 지나면 끝
			if (bLeftRope && State.Mode == EYJJJumperMode::Airborne && Step > FMath::Max(PressStep, BottomStep) + LateSteps + 2)
			{
				break;
			}
		}
		return State;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJRopeDragLaunchTest, "YJJJump.Rope.DragLaunchClimbs", YJJTests::Flags)
bool FYJJRopeDragLaunchTest::RunTest(const FString& Parameters)
{
	const FYJJRopeTuning Tuning;
	const TArray<FYJJRopeSegment> Ropes = YJJTests::MakeSingleRope();

	FYJJJumperState State;
	YJJRopeSim::PlaceOnRope(State, Tuning, Ropes[0], 0.f);
	TestTrue(TEXT("Can drag while resting"), YJJRopeSim::BeginDrag(State));

	YJJRopeSim::UpdateDrag(State, Tuning, FVector2f(-100.f, 1000.f));
	TestEqual(TEXT("Drag is clamped to max deflection"), State.Deflection, Tuning.MaxDeflection);
	YJJRopeSim::ReleaseDrag(State, Tuning);

	float Apex = State.Position.Y;
	bool bLaunched = false;
	float LaunchVelocityX = 0.f;
	for (int32 Step = 0; Step < 1200; ++Step)
	{
		YJJTests::StepOnce(State, Tuning, Ropes);
		if (!bLaunched && State.Mode == EYJJJumperMode::Airborne)
		{
			bLaunched = true;
			LaunchVelocityX = State.Velocity.X;
		}
		Apex = FMath::Max(Apex, State.Position.Y);
		if (bLaunched && State.Velocity.Y < 0.f)
		{
			break;
		}
	}

	TestTrue(TEXT("Launched into the air"), bLaunched);
	TestTrue(FString::Printf(TEXT("Full drag climbs above 5m (apex %.0f)"), Apex), Apex > 500.f);
	// 발사 순간 기준 (오래 날면 벽에 튕겨 방향이 바뀔 수 있다)
	TestTrue(TEXT("Pulled left, flies right"), LaunchVelocityX > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJRopeSettlesTest, "YJJJump.Rope.NoInputSettles", YJJTests::Flags)
bool FYJJRopeSettlesTest::RunTest(const FString& Parameters)
{
	const FYJJRopeTuning Tuning;
	const TArray<FYJJRopeSegment> Ropes = YJJTests::MakeSingleRope();
	FYJJJumperState State = YJJTests::MakeDropped(300.f);

	for (int32 Step = 0; Step < 120 * 30 && State.Mode != EYJJJumperMode::Resting; ++Step)
	{
		YJJTests::StepOnce(State, Tuning, Ropes);
	}

	TestTrue(TEXT("Bounces die out and the jumper rests"), State.Mode == EYJJJumperMode::Resting);
	TestEqual(TEXT("Rests on the rope it fell on"), State.RopeId, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJRopeTimingTest, "YJJJump.Rope.TimingGrades", YJJTests::Flags)
bool FYJJRopeTimingTest::RunTest(const FString& Parameters)
{
	const FYJJRopeTuning Tuning;
	using YJJTests::EPressTiming;

	const FYJJJumperState None = YJJTests::RunBounce(EPressTiming::None, Tuning);
	const FYJJJumperState Early = YJJTests::RunBounce(EPressTiming::Early, Tuning);
	const FYJJJumperState OnBottom = YJJTests::RunBounce(EPressTiming::OnBottom, Tuning);
	const FYJJJumperState Late = YJJTests::RunBounce(EPressTiming::Late, Tuning);

	TestTrue(TEXT("No press -> no grade"), None.LaunchGrade == EYJJBounceGrade::None);
	TestTrue(TEXT("Press slightly early -> Perfect"), Early.LaunchGrade == EYJJBounceGrade::Perfect);
	TestTrue(TEXT("Press on the bottom -> Perfect"), OnBottom.LaunchGrade == EYJJBounceGrade::Perfect);
	TestTrue(TEXT("Press 0.1s late -> Good"), Late.LaunchGrade == EYJJBounceGrade::Good);

	TestTrue(FString::Printf(TEXT("Perfect launches higher than none (%.0f > %.0f)"), OnBottom.LastLaunchSpeed, None.LastLaunchSpeed),
		OnBottom.LastLaunchSpeed > None.LastLaunchSpeed);
	TestTrue(FString::Printf(TEXT("Good launches higher than none (%.0f > %.0f)"), Late.LastLaunchSpeed, None.LastLaunchSpeed),
		Late.LastLaunchSpeed > None.LastLaunchSpeed);
	TestTrue(FString::Printf(TEXT("Perfect launches higher than good (%.0f > %.0f)"), OnBottom.LastLaunchSpeed, Late.LastLaunchSpeed),
		OnBottom.LastLaunchSpeed > Late.LastLaunchSpeed);
	TestEqual(TEXT("Perfect starts a chain"), OnBottom.PerfectChain, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJRopeDeterminismTest, "YJJJump.Rope.Deterministic", YJJTests::Flags)
bool FYJJRopeDeterminismTest::RunTest(const FString& Parameters)
{
	const FYJJRopeTuning Tuning;
	const FYJJJumperState A = YJJTests::RunBounce(YJJTests::EPressTiming::Early, Tuning);
	const FYJJJumperState B = YJJTests::RunBounce(YJJTests::EPressTiming::Early, Tuning);

	// 서버/클라 예측이 같은 결과를 내려면 비트 단위로 같아야 한다
	TestTrue(TEXT("Same inputs give identical position"), A.Position == B.Position);
	TestTrue(TEXT("Same inputs give identical velocity"), A.Velocity == B.Velocity);
	TestEqual(TEXT("Same inputs give identical time"), A.SimTime, B.SimTime);
	return true;
}

namespace YJJTests
{
	FYJJMobTarget MakeMob(const FVector2f& Position, float Health, int32 XP = 1)
	{
		FYJJMobTarget Mob;
		Mob.Id = 7;
		Mob.Position = Position;
		Mob.Radius = 50.f;
		Mob.Health = Health;
		Mob.ScoreValue = 50;
		Mob.XPValue = XP;
		return Mob;
	}

	FYJJJumperState MakeFlying(const FVector2f& Velocity)
	{
		FYJJJumperState State;
		State.Mode = EYJJJumperMode::Airborne;
		State.Velocity = Velocity;
		return State;
	}

	// 돌진을 끝까지 진행
	FYJJHitResult RunDash(FYJJJumperState& State, const FYJJCombatTuning& Tuning, const FYJJMobTarget& Mob)
	{
		for (int32 Step = 0; Step < 120 && !YJJCombat::StepDash(State, YJJRopeSim::FixedStep); ++Step)
		{
		}
		return YJJCombat::FinishDash(State, Tuning, &Mob);
	}

	// 로프에서 LaunchSpeed로 쏘아 올려 다시 로프에 닿을 때까지: (최고 높이, 비행 시간)
	TPair<float, float> FlyFromRope(float LaunchSpeed, const FYJJRopeTuning& Tuning)
	{
		const TArray<FYJJRopeSegment> Ropes = MakeSingleRope();
		FYJJJumperState State;
		State.Mode = EYJJJumperMode::Airborne;
		State.Velocity = FVector2f(0.f, LaunchSpeed);
		State.SpeedScale = YJJRopeSim::ComputeSpeedScale(Tuning, LaunchSpeed);

		float Apex = 0.f;
		int32 Steps = 0;
		for (; Steps < 120 * 60 && State.Mode == EYJJJumperMode::Airborne; ++Steps)
		{
			StepOnce(State, Tuning, Ropes);
			Apex = FMath::Max(Apex, State.Position.Y);
		}
		return TPair<float, float>(Apex, Steps * YJJRopeSim::FixedStep);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJSpeedScaleTest, "YJJJump.Rope.FasterMeansHigherNotLonger", YJJTests::Flags)
bool FYJJSpeedScaleTest::RunTest(const FString& Parameters)
{
	const FYJJRopeTuning Tuning;
	const TPair<float, float> Slow = YJJTests::FlyFromRope(Tuning.SpeedReference, Tuning);
	const TPair<float, float> Fast = YJJTests::FlyFromRope(Tuning.SpeedReference * 4.f, Tuning);

	// 4배 빠르면 4배 높이 가지만 비행 시간은 같다 -> 점점 미친 속도감
	TestTrue(FString::Printf(TEXT("4x speed goes ~4x higher (%.0f vs %.0f)"), Fast.Key, Slow.Key), FMath::IsNearlyEqual(Fast.Key / Slow.Key, 4.f, 0.2f));
	TestTrue(FString::Printf(TEXT("Flight time stays the same (%.2fs vs %.2fs)"), Fast.Value, Slow.Value), FMath::IsNearlyEqual(Fast.Value, Slow.Value, 0.05f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJKillComboTest, "YJJJump.Rope.KillsBoostNextLaunch", YJJTests::Flags)
bool FYJJKillComboTest::RunTest(const FString& Parameters)
{
	const FYJJRopeTuning Tuning;
	const TArray<FYJJRopeSegment> Ropes = YJJTests::MakeSingleRope();

	auto LaunchSpeedWithKills = [&](int32 Kills)
	{
		FYJJJumperState State = YJJTests::MakeDropped(600.f);
		State.KillCombo = Kills;
		for (int32 Step = 0; Step < 2000; ++Step)
		{
			FYJJSimEvents Events;
			YJJRopeSim::Step(State, Tuning, Ropes, 0.f, Events);
			if (Events.bLaunched)
			{
				return State.LastLaunchSpeed;
			}
		}
		return 0.f;
	};

	const float NoKills = LaunchSpeedWithKills(0);
	const float FiveKills = LaunchSpeedWithKills(5);
	TestTrue(FString::Printf(TEXT("Kills in the last flight boost the launch (%.0f > %.0f)"), FiveKills, NoKills), FiveKills > NoKills * 1.07f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJDashKillTest, "YJJJump.Combat.TapDashKills", YJJTests::Flags)
bool FYJJDashKillTest::RunTest(const FString& Parameters)
{
	const FYJJCombatTuning Tuning;
	FYJJJumperState State = YJJTests::MakeFlying(FVector2f(300.f, 2000.f));
	State.LaunchGrade = EYJJBounceGrade::Perfect;
	const FYJJMobTarget Mob = YJJTests::MakeMob(FVector2f(150.f, 250.f), 1.f, 5);

	TestTrue(TEXT("Tap lands on the mob"), YJJCombat::IsTapOnMob(Tuning, FVector2f(170.f, 230.f), Mob));
	TestTrue(TEXT("Dash starts"), YJJCombat::BeginDash(State, Tuning, Mob));
	TestFalse(TEXT("Cannot start another dash mid-dash"), YJJCombat::CanDashTo(State, Tuning, Mob));

	const FYJJHitResult Result = YJJTests::RunDash(State, Tuning, Mob);
	TestTrue(TEXT("Dash hits"), Result.bHit);
	TestTrue(TEXT("Lv1 perfect dash kills a 1 HP sparrow"), Result.bKilled);
	TestTrue(TEXT("Ends at the mob"), State.Position.Equals(Mob.Position, 0.1f));
	TestTrue(TEXT("Keeps flying on the original trajectory"), State.Velocity == FVector2f(300.f, 2000.f));
	TestEqual(TEXT("Kill combo counted"), State.KillCombo, 1);
	TestEqual(TEXT("5 XP levels up a Lv1 (needs 4)"), State.Level, 2);
	TestEqual(TEXT("Leftover XP"), State.XP, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJDashRulesTest, "YJJJump.Combat.DashRules", YJJTests::Flags)
bool FYJJDashRulesTest::RunTest(const FString& Parameters)
{
	const FYJJCombatTuning Tuning;

	FYJJJumperState Flying = YJJTests::MakeFlying(FVector2f(0.f, 1000.f));
	TestFalse(TEXT("Out of range"), YJJCombat::CanDashTo(Flying, Tuning, YJJTests::MakeMob(FVector2f(0.f, 3000.f), 1.f)));

	FYJJJumperState OnRope;
	OnRope.Mode = EYJJJumperMode::OnRope;
	TestFalse(TEXT("No dashing from the rope"), YJJCombat::CanDashTo(OnRope, Tuning, YJJTests::MakeMob(FVector2f(0.f, 100.f), 1.f)));

	// 튼튼한 몹은 한 방에 안 죽고 비행은 그대로 이어진다
	const FYJJMobTarget Tough = YJJTests::MakeMob(FVector2f(0.f, 200.f), 50.f);
	TestTrue(TEXT("Dash starts"), YJJCombat::BeginDash(Flying, Tuning, Tough));
	const FYJJHitResult Result = YJJTests::RunDash(Flying, Tuning, Tough);
	TestTrue(TEXT("Hit"), Result.bHit);
	TestFalse(TEXT("Tough mob survives"), Result.bKilled);
	TestTrue(TEXT("Velocity restored"), Flying.Velocity == FVector2f(0.f, 1000.f));
	TestEqual(TEXT("No kill combo"), Flying.KillCombo, 0);

	// 튜토리얼 Lv 999Gp는 뭐든 한 방, 경험치는 안 쌓인다
	FYJJJumperState Tutorial = YJJTests::MakeFlying(FVector2f(0.f, 1000.f));
	Tutorial.bTutorial = true;
	TestTrue(TEXT("Tutorial range is huge"), YJJCombat::GetAttackRange(Tutorial, Tuning) >= Tuning.MaxAttackRange);
	TestTrue(TEXT("Dash starts"), YJJCombat::BeginDash(Tutorial, Tuning, Tough));
	const FYJJHitResult TutorialResult = YJJTests::RunDash(Tutorial, Tuning, Tough);
	TestTrue(TEXT("Lv 999Gp one-shots"), TutorialResult.bKilled);
	TestEqual(TEXT("No XP in tutorial"), Tutorial.XP, 0);
	TestEqual(TEXT("Level unchanged in tutorial"), Tutorial.Level, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJCombatPowerTest, "YJJJump.Combat.PowerFollowsGradeLevelSpeed", YJJTests::Flags)
bool FYJJCombatPowerTest::RunTest(const FString& Parameters)
{
	const FYJJCombatTuning Tuning;
	FYJJJumperState State;

	State.LaunchGrade = EYJJBounceGrade::Miss;
	const float Miss = YJJCombat::ComputeAttackPower(State, Tuning);
	State.LaunchGrade = EYJJBounceGrade::None;
	const float None = YJJCombat::ComputeAttackPower(State, Tuning);
	State.LaunchGrade = EYJJBounceGrade::Good;
	const float Good = YJJCombat::ComputeAttackPower(State, Tuning);
	State.LaunchGrade = EYJJBounceGrade::Perfect;
	const float Perfect = YJJCombat::ComputeAttackPower(State, Tuning);
	State.PerfectChain = 3;
	const float Chained = YJJCombat::ComputeAttackPower(State, Tuning);
	State.Level = 10;
	const float Leveled = YJJCombat::ComputeAttackPower(State, Tuning);
	State.SpeedScale = 4.f;
	const float Fast = YJJCombat::ComputeAttackPower(State, Tuning);

	TestTrue(TEXT("Miss < None < Good < Perfect"), Miss < None && None < Good && Good < Perfect);
	TestTrue(TEXT("Perfect chain adds power"), Chained > Perfect);
	TestTrue(TEXT("Levels add power"), Leveled > Chained);
	TestTrue(TEXT("Speed adds power"), Fast > Leveled);
	TestTrue(TEXT("Range grows with level"), YJJCombat::GetAttackRange(State, Tuning) > Tuning.BaseAttackRange);
	return true;
}

#endif
