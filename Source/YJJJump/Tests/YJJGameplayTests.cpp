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
	for (int32 Step = 0; Step < 1200; ++Step)
	{
		YJJTests::StepOnce(State, Tuning, Ropes);
		bLaunched |= State.Mode == EYJJJumperMode::Airborne;
		Apex = FMath::Max(Apex, State.Position.Y);
		if (bLaunched && State.Velocity.Y < 0.f)
		{
			break;
		}
	}

	TestTrue(TEXT("Launched into the air"), bLaunched);
	TestTrue(FString::Printf(TEXT("Full drag climbs above 5m (apex %.0f)"), Apex), Apex > 500.f);
	TestTrue(TEXT("Pulled left, flies right"), State.Velocity.X > 0.f);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJCombatKillTest, "YJJJump.Combat.FastHitKills", YJJTests::Flags)
bool FYJJCombatKillTest::RunTest(const FString& Parameters)
{
	const FYJJCombatTuning Tuning;

	FYJJJumperState State;
	State.Mode = EYJJJumperMode::Airborne;
	State.Velocity = FVector2f(0.f, 3000.f);
	State.LaunchGrade = EYJJBounceGrade::Perfect;

	FYJJMobTarget Mob;
	Mob.Id = 7;
	Mob.Position = FVector2f(0.f, 30.f);
	Mob.Radius = 50.f;
	Mob.Health = 2.f;
	Mob.ScoreValue = 100;

	const FYJJHitResult Result = YJJCombat::ResolveHit(State, Tuning, Mob);
	TestTrue(TEXT("Overlap hits"), Result.bHit);
	TestTrue(FString::Printf(TEXT("Perfect fast hit kills (damage %.2f)"), Result.Damage), Result.bKilled);
	TestTrue(TEXT("Keeps flying up through the mob"), State.Velocity.Y > 2000.f);
	TestEqual(TEXT("Score added"), State.Score, 100);
	TestEqual(TEXT("Kill counted"), State.Kills, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJCombatBlockedTest, "YJJJump.Combat.SlowHitBounces", YJJTests::Flags)
bool FYJJCombatBlockedTest::RunTest(const FString& Parameters)
{
	const FYJJCombatTuning Tuning;

	FYJJJumperState State;
	State.Mode = EYJJJumperMode::Airborne;
	State.Velocity = FVector2f(0.f, 600.f);

	FYJJMobTarget Mob;
	Mob.Position = FVector2f(0.f, 40.f);
	Mob.Radius = 50.f;
	Mob.Health = 10.f;

	const FYJJHitResult Result = YJJCombat::ResolveHit(State, Tuning, Mob);
	TestTrue(TEXT("Overlap hits"), Result.bHit);
	TestFalse(TEXT("Slow hit does not kill a tough mob"), Result.bKilled);
	TestTrue(TEXT("Bounced back down"), State.Velocity.Y < 0.f);
	TestTrue(TEXT("Pushed out of the mob"), FVector2f::Distance(State.Position, Mob.Position) > Mob.Radius + Tuning.JumperRadius);

	FYJJJumperState OnRope;
	OnRope.Mode = EYJJJumperMode::OnRope;
	TestFalse(TEXT("No hits while on the rope"), YJJCombat::ResolveHit(OnRope, Tuning, Mob).bHit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJCombatPowerTest, "YJJJump.Combat.PowerFollowsGrade", YJJTests::Flags)
bool FYJJCombatPowerTest::RunTest(const FString& Parameters)
{
	const FYJJCombatTuning Tuning;

	FYJJJumperState State;
	State.Velocity = FVector2f(0.f, 2000.f);

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

	TestTrue(TEXT("Miss < None < Good < Perfect"), Miss < None && None < Good && Good < Perfect);
	TestTrue(TEXT("Perfect chain adds power"), Chained > Perfect);
	return true;
}

#endif
