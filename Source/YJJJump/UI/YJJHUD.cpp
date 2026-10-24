#include "UI/YJJHUD.h"
#include "Player/YJJJumper.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

void AYJJHUD::DrawHUD()
{
	Super::DrawHUD();

	const AYJJJumper* Jumper = Cast<AYJJJumper>(GetOwningPawn());
	if (!Jumper || !Canvas)
	{
		return;
	}

	// 1080p 기준 크기
	const float UIScale = FMath::Max(Canvas->ClipY / 1080.f, 0.5f);

	DrawLaunchPreview(*Jumper);
	DrawPopups(*Jumper, UIScale);
	DrawStats(*Jumper, UIScale);
	DrawAttackGauge(*Jumper, UIScale);
	DrawHint(*Jumper, UIScale);
}

UFont* AYJJHUD::GetFont() const
{
	return GEngine ? GEngine->GetLargeFont() : nullptr;
}

void AYJJHUD::DrawOutlinedText(const FString& Text, const FLinearColor& Color, float X, float Y, float Scale, bool bCenter)
{
	UFont* Font = GetFont();
	if (!Font)
	{
		return;
	}

	if (bCenter)
	{
		float Width = 0.f;
		float Height = 0.f;
		GetTextSize(Text, Width, Height, Font, Scale);
		X -= Width * 0.5f;
		Y -= Height * 0.5f;
	}

	// 배경과 상관없이 읽히도록 검은 테두리
	const FLinearColor Outline(0.f, 0.f, 0.f, Color.A);
	const float Offset = FMath::Max(2.f * Scale * 0.5f, 1.f);
	for (const FVector2D& Dir : { FVector2D(-1, 0), FVector2D(1, 0), FVector2D(0, -1), FVector2D(0, 1) })
	{
		DrawText(Text, Outline, X + Dir.X * Offset, Y + Dir.Y * Offset, Font, Scale);
	}
	DrawText(Text, Color, X, Y, Font, Scale);
}

void AYJJHUD::DrawStats(const AYJJJumper& Jumper, float UIScale)
{
	const FYJJJumperState& State = Jumper.GetSimState();
	const float X = 30.f * UIScale;
	float Y = 24.f * UIScale;
	const float Line = 40.f * UIScale;
	const float TextScale = 1.3f * UIScale;

	DrawOutlinedText(FString::Printf(TEXT("HEIGHT  %.1f m"), Jumper.GetHeightMeters()), FLinearColor::White, X, Y, TextScale, false);
	Y += Line;
	DrawOutlinedText(FString::Printf(TEXT("BEST    %.1f m"), Jumper.GetBestHeightMeters()), FLinearColor(0.8f, 0.9f, 1.f), X, Y, TextScale, false);
	Y += Line;
	DrawOutlinedText(FString::Printf(TEXT("SCORE   %d"), State.Score), FLinearColor(1.f, 0.85f, 0.2f), X, Y, TextScale, false);
	Y += Line;
	DrawOutlinedText(FString::Printf(TEXT("KILLS   %d"), State.Kills), FLinearColor(1.f, 0.6f, 0.4f), X, Y, TextScale, false);
}

void AYJJHUD::DrawAttackGauge(const AYJJJumper& Jumper, float UIScale)
{
	const FYJJJumperState& State = Jumper.GetSimState();
	const bool bAirborne = State.Mode == EYJJJumperMode::Airborne;
	const float Power = bAirborne ? Jumper.GetAttackPower() : 0.f;

	const float Width = FMath::Min(Canvas->ClipX * 0.6f, 520.f * UIScale);
	const float Height = 22.f * UIScale;
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = Canvas->ClipY - 80.f * UIScale;

	// 공격력 4 이상이면 가득 참
	const float Fill = FMath::Clamp(Power / 4.f, 0.f, 1.f);
	const FLinearColor FillColor = FLinearColor::LerpUsingHSV(FLinearColor(0.3f, 0.8f, 1.f), FLinearColor(1.f, 0.3f, 0.2f), Fill);

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X - 3.f, Y - 3.f, Width + 6.f, Height + 6.f);
	DrawRect(FillColor, X, Y, Width * Fill, Height);
	DrawOutlinedText(FString::Printf(TEXT("ATK %.1f"), Power), FLinearColor::White, Canvas->ClipX * 0.5f, Y - 26.f * UIScale, 1.2f * UIScale, true);

	if (State.PerfectChain > 1)
	{
		DrawOutlinedText(FString::Printf(TEXT("PERFECT CHAIN x%d"), State.PerfectChain), FLinearColor(1.f, 0.85f, 0.1f),
			Canvas->ClipX * 0.5f, Y - 60.f * UIScale, 1.2f * UIScale, true);
	}
}

void AYJJHUD::DrawLaunchPreview(const AYJJJumper& Jumper)
{
	FVector2f Velocity;
	if (!Jumper.GetLaunchPreview(Velocity))
	{
		return;
	}

	// 포물선을 점으로 찍는다
	const FVector Start = Jumper.GetActorLocation();
	const float Gravity = Jumper.Tuning.Gravity;
	const int32 DotCount = 18;
	const float TotalTime = FMath::Clamp(Velocity.Y / FMath::Max(Gravity, 1.f) * 1.4f, 0.3f, 2.f);
	for (int32 Index = 1; Index <= DotCount; ++Index)
	{
		const float T = TotalTime * Index / DotCount;
		const FVector Point(Start.X + Velocity.X * T, Start.Y, Start.Z + Velocity.Y * T - 0.5f * Gravity * T * T);
		const FVector Screen = Project(Point, false);
		const float Size = FMath::Lerp(10.f, 4.f, static_cast<float>(Index) / DotCount);
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.9f), Screen.X - Size * 0.5f, Screen.Y - Size * 0.5f, Size, Size);
	}
}

void AYJJHUD::DrawPopups(const AYJJJumper& Jumper, float UIScale)
{
	for (const FYJJPopup& Popup : Jumper.GetPopups())
	{
		const FVector Screen = Project(Popup.WorldLocation, false);
		const float Alpha = 1.f - FMath::Clamp((Popup.Age - Popup.Lifetime * 0.6f) / (Popup.Lifetime * 0.4f), 0.f, 1.f);
		// 처음에 살짝 커졌다 줄어드는 펀치
		const float Punch = 1.f + 0.4f * FMath::Max(1.f - Popup.Age * 8.f, 0.f);

		FLinearColor Color = Popup.Color;
		Color.A = Alpha;
		DrawOutlinedText(Popup.Text, Color, Screen.X, Screen.Y, Popup.Scale * Punch * UIScale * 1.2f, true);
	}
}

void AYJJHUD::DrawHint(const AYJJJumper& Jumper, float UIScale)
{
	const FYJJJumperState& State = Jumper.GetSimState();
	FString Hint;
	if (State.Mode == EYJJJumperMode::Resting)
	{
		Hint = TEXT("DRAG DOWN & RELEASE TO JUMP");
	}
	else if (State.Mode == EYJJJumperMode::Airborne && State.Velocity.Y < 0.f && State.LastGrade == EYJJBounceGrade::None)
	{
		// 아직 한 번도 타이밍 입력을 안 해본 경우만
		Hint = TEXT("TAP WHEN YOU HIT THE ROPE!");
	}

	if (!Hint.IsEmpty())
	{
		DrawOutlinedText(Hint, FLinearColor(1.f, 1.f, 1.f, 0.85f), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.22f, 1.2f * UIScale, true);
	}
}
