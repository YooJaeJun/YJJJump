#include "Rope/YJJRopeSimulation.h"

namespace YJJRopeSim
{
	namespace
	{
		EYJJBounceGrade Classify(const FYJJRopeTuning& Tuning, double TimeFromBottom)
		{
			const double Abs = FMath::Abs(TimeFromBottom);
			if (Abs <= Tuning.PerfectWindow)
			{
				return EYJJBounceGrade::Perfect;
			}
			if (Abs <= Tuning.GoodWindow)
			{
				return EYJJBounceGrade::Good;
			}
			return EYJJBounceGrade::Miss;
		}

		// 판정 결과를 기록하고 발사 배율을 돌려준다
		float ApplyGrade(FYJJJumperState& State, const FYJJRopeTuning& Tuning, EYJJBounceGrade Grade, FYJJSimEvents& OutEvents)
		{
			State.bGraded = true;
			State.LastGrade = Grade;
			OutEvents.bGraded = true;
			OutEvents.Grade = Grade;

			switch (Grade)
			{
			case EYJJBounceGrade::Perfect:
				State.PerfectChain++;
				// 연속 퍼펙트 보너스 (최대 +0.25)
				return Tuning.PerfectMultiplier + FMath::Min(State.PerfectChain - 1, 5) * 0.05f;
			case EYJJBounceGrade::Good:
				return Tuning.GoodMultiplier;
			default:
				State.PerfectChain = 0;
				return Tuning.MissMultiplier;
			}
		}

		void Launch(FYJJJumperState& State, const FYJJRopeTuning& Tuning, FYJJSimEvents& OutEvents)
		{
			const float UpSpeed = FMath::Min(-State.DeflectionSpeed * State.PendingLaunchMultiplier, Tuning.MaxLaunchSpeed);

			State.Mode = EYJJJumperMode::Airborne;
			State.Velocity.Y = UpSpeed;
			if (State.bHasPendingHorizontal)
			{
				State.Velocity.X = State.PendingHorizontalSpeed;
			}
			State.Position.Y = State.RopeHeight;
			State.Deflection = 0.f;
			State.DeflectionSpeed = 0.f;
			State.PendingLaunchMultiplier = 1.f;
			State.PendingHorizontalSpeed = 0.f;
			State.bHasPendingHorizontal = false;
			State.LastLaunchSpeed = UpSpeed;

			OutEvents.bLaunched = true;
		}

		void StepAirborne(FYJJJumperState& State, const FYJJRopeTuning& Tuning, TConstArrayView<FYJJRopeSegment> Ropes, float Dt, FYJJSimEvents& OutEvents)
		{
			const float PrevZ = State.Position.Y;

			State.Velocity.Y -= Tuning.Gravity * Dt;
			State.Position += State.Velocity * Dt;

			if (State.Velocity.Y >= 0.f)
			{
				return;
			}

			// 위에서 아래로 로프를 통과하면 착지. 여러 개면 가장 높은 로프
			const FYJJRopeSegment* Hit = nullptr;
			for (const FYJJRopeSegment& Rope : Ropes)
			{
				if (PrevZ >= Rope.Height && State.Position.Y < Rope.Height
					&& State.Position.X >= Rope.MinX && State.Position.X <= Rope.MaxX
					&& (!Hit || Rope.Height > Hit->Height))
				{
					Hit = &Rope;
				}
			}

			if (!Hit)
			{
				return;
			}

			State.Mode = EYJJJumperMode::OnRope;
			State.RopeId = Hit->Id;
			State.LastRopeId = Hit->Id;
			State.RopeHeight = Hit->Height;
			State.Deflection = FMath::Min(Hit->Height - State.Position.Y, Tuning.MaxDeflection);
			State.DeflectionSpeed = -State.Velocity.Y;
			State.Velocity.Y = 0.f;
			State.bReachedBottom = false;
			State.bGraded = false;
			State.BottomTime = -1.0;
			State.PendingLaunchMultiplier = 1.f;

			OutEvents.bLanded = true;
		}

		void StepOnRope(FYJJJumperState& State, const FYJJRopeTuning& Tuning, TConstArrayView<FYJJRopeSegment> Ropes, float Dt, FYJJSimEvents& OutEvents)
		{
			const FYJJRopeSegment* Rope = FindRope(Ropes, State.RopeId);
			if (!Rope)
			{
				State.Mode = EYJJJumperMode::Airborne;
				return;
			}

			// 처짐은 아래(+). 중력은 처짐을 늘리고, 스프링/감쇠는 되돌린다
			const float PrevSpeed = State.DeflectionSpeed;
			const float Accel = Tuning.Gravity - Tuning.RopeStiffness * State.Deflection - Tuning.RopeDamping * State.DeflectionSpeed;
			State.DeflectionSpeed += Accel * Dt;
			State.Deflection += State.DeflectionSpeed * Dt;

			if (State.Deflection > Tuning.MaxDeflection)
			{
				State.Deflection = Tuning.MaxDeflection;
				State.DeflectionSpeed = FMath::Min(State.DeflectionSpeed, 0.f);
			}

			// 최저점: 처짐 속도가 + 에서 0 이하로 바뀌는 순간
			if (!State.bReachedBottom && PrevSpeed > 0.f && State.DeflectionSpeed <= 0.f)
			{
				State.bReachedBottom = true;
				State.BottomTime = State.SimTime;

				// 최저점 전에 누른 입력 판정
				if (!State.bGraded && State.LastPressTime >= 0.0)
				{
					State.PendingLaunchMultiplier = ApplyGrade(State, Tuning, Classify(Tuning, State.SimTime - State.LastPressTime), OutEvents);
					State.LastPressTime = -1.0;
				}
			}

			State.Velocity.X *= FMath::Exp(-Tuning.RopeHorizontalFriction * Dt);
			State.Position.X = FMath::Clamp(State.Position.X + State.Velocity.X * Dt, Rope->MinX, Rope->MaxX);
			State.Position.Y = State.RopeHeight - State.Deflection;

			// 로프가 원래 높이를 넘어 위로 올라가면 발사
			if (State.Deflection <= 0.f && State.DeflectionSpeed < 0.f)
			{
				Launch(State, Tuning, OutEvents);
				return;
			}

			// 에너지가 거의 다 빠지면 정지 -> 다시 드래그 가능
			const float RestD = RestDeflection(Tuning);
			if (State.bReachedBottom
				&& FMath::Abs(State.DeflectionSpeed) < Tuning.RestSpeedThreshold
				&& FMath::Abs(State.Deflection - RestD) < Tuning.RestDeflectionTolerance)
			{
				State.Mode = EYJJJumperMode::Resting;
				State.Deflection = RestD;
				State.DeflectionSpeed = 0.f;
				State.Velocity = FVector2f::ZeroVector;
				State.Position.Y = State.RopeHeight - RestD;
				State.PendingLaunchMultiplier = 1.f;
				State.bHasPendingHorizontal = false;
				State.LastPressTime = -1.0;
				OutEvents.bRested = true;
			}
		}

		void ApplyWalls(FYJJJumperState& State, const FYJJRopeTuning& Tuning, float CenterX)
		{
			const float MinX = CenterX - Tuning.WallHalfWidth;
			const float MaxX = CenterX + Tuning.WallHalfWidth;
			if (State.Position.X < MinX)
			{
				State.Position.X = MinX;
				State.Velocity.X = FMath::Abs(State.Velocity.X) * Tuning.WallRestitution;
			}
			else if (State.Position.X > MaxX)
			{
				State.Position.X = MaxX;
				State.Velocity.X = -FMath::Abs(State.Velocity.X) * Tuning.WallRestitution;
			}
		}
	}

	float RestDeflection(const FYJJRopeTuning& Tuning)
	{
		return Tuning.RopeStiffness > 0.f ? Tuning.Gravity / Tuning.RopeStiffness : 0.f;
	}

	void Step(FYJJJumperState& State, const FYJJRopeTuning& Tuning, TConstArrayView<FYJJRopeSegment> Ropes, float CenterX, FYJJSimEvents& OutEvents)
	{
		State.SimTime += FixedStep;

		switch (State.Mode)
		{
		case EYJJJumperMode::Airborne:
			StepAirborne(State, Tuning, Ropes, FixedStep, OutEvents);
			break;
		case EYJJJumperMode::OnRope:
			StepOnRope(State, Tuning, Ropes, FixedStep, OutEvents);
			break;
		case EYJJJumperMode::Resting:
		case EYJJJumperMode::Dragging:
			// 입력으로만 변한다
			break;
		}

		ApplyWalls(State, Tuning, CenterX);
		State.MaxHeightReached = FMath::Max(State.MaxHeightReached, State.Position.Y);
	}

	void PressBounce(FYJJJumperState& State, const FYJJRopeTuning& Tuning, float AimX, FYJJSimEvents& OutEvents)
	{
		const float AimSpeed = FMath::Clamp(AimX, -1.f, 1.f) * Tuning.MaxHorizontalSpeed;

		if (State.Mode == EYJJJumperMode::OnRope)
		{
			State.PendingHorizontalSpeed = AimSpeed;
			State.bHasPendingHorizontal = true;

			if (!State.bReachedBottom)
			{
				// 이른 입력 - 최저점에서 판정
				State.LastPressTime = State.SimTime;
			}
			else if (!State.bGraded)
			{
				// 최저점 이후 늦은 입력
				State.PendingLaunchMultiplier = ApplyGrade(State, Tuning, Classify(Tuning, State.SimTime - State.BottomTime), OutEvents);
			}
			return;
		}

		if (State.Mode == EYJJJumperMode::Airborne)
		{
			// 로프를 막 떠난 직후의 늦은 입력은 속도에 바로 반영
			if (!State.bGraded && State.BottomTime >= 0.0 && State.Velocity.Y > 0.f
				&& State.SimTime - State.BottomTime <= Tuning.GoodWindow)
			{
				const float Multiplier = ApplyGrade(State, Tuning, Classify(Tuning, State.SimTime - State.BottomTime), OutEvents);
				State.Velocity.Y = FMath::Min(State.Velocity.Y * Multiplier, Tuning.MaxLaunchSpeed);
				State.Velocity.X = AimSpeed;
				State.LastLaunchSpeed = State.Velocity.Y;
				return;
			}

			// 다음 착지를 위한 이른 입력
			State.LastPressTime = State.SimTime;
			State.PendingHorizontalSpeed = AimSpeed;
			State.bHasPendingHorizontal = true;
		}
	}

	bool BeginDrag(FYJJJumperState& State)
	{
		if (State.Mode != EYJJJumperMode::Resting)
		{
			return false;
		}
		State.Mode = EYJJJumperMode::Dragging;
		return true;
	}

	void UpdateDrag(FYJJJumperState& State, const FYJJRopeTuning& Tuning, const FVector2f& DragPixels)
	{
		if (State.Mode != EYJJJumperMode::Dragging)
		{
			return;
		}

		const float RestD = RestDeflection(Tuning);
		State.Deflection = FMath::Clamp(RestD + DragPixels.Y * Tuning.DragPixelsToDeflection, RestD, Tuning.MaxDeflection);
		State.DeflectionSpeed = 0.f;
		State.Position.Y = State.RopeHeight - State.Deflection;
		// 새총처럼 당긴 반대 방향으로 날아간다
		State.PendingHorizontalSpeed = FMath::Clamp(-DragPixels.X * Tuning.DragPixelsToHorizontalSpeed, -Tuning.MaxHorizontalSpeed, Tuning.MaxHorizontalSpeed);
		State.bHasPendingHorizontal = true;
	}

	void ReleaseDrag(FYJJJumperState& State, const FYJJRopeTuning& Tuning)
	{
		if (State.Mode != EYJJJumperMode::Dragging)
		{
			return;
		}

		// 거의 안 당겼으면 취소
		if (State.Deflection < RestDeflection(Tuning) + 10.f)
		{
			State.Mode = EYJJJumperMode::Resting;
			State.Deflection = RestDeflection(Tuning);
			State.Position.Y = State.RopeHeight - State.Deflection;
			State.bHasPendingHorizontal = false;
			return;
		}

		// 최저점에서 놓은 것과 같다. 드래그 발사는 타이밍 판정 없음
		State.Mode = EYJJJumperMode::OnRope;
		State.DeflectionSpeed = 0.f;
		State.bReachedBottom = true;
		State.bGraded = true;
		State.BottomTime = -1.0;
		State.LastPressTime = -1.0;
		State.PendingLaunchMultiplier = 1.f;
	}

	void PlaceOnRope(FYJJJumperState& State, const FYJJRopeTuning& Tuning, const FYJJRopeSegment& Rope, float X)
	{
		State.Mode = EYJJJumperMode::Resting;
		State.RopeId = Rope.Id;
		State.LastRopeId = Rope.Id;
		State.RopeHeight = Rope.Height;
		State.Deflection = RestDeflection(Tuning);
		State.DeflectionSpeed = 0.f;
		State.Velocity = FVector2f::ZeroVector;
		State.Position = FVector2f(FMath::Clamp(X, Rope.MinX, Rope.MaxX), Rope.Height - State.Deflection);
		State.PendingLaunchMultiplier = 1.f;
		State.bHasPendingHorizontal = false;
		State.LastPressTime = -1.0;
		State.BottomTime = -1.0;
	}

	const FYJJRopeSegment* FindRope(TConstArrayView<FYJJRopeSegment> Ropes, int32 RopeId)
	{
		return Ropes.FindByPredicate([RopeId](const FYJJRopeSegment& Rope) { return Rope.Id == RopeId; });
	}
}
