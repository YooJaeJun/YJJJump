#include "UI/YJJHUD.h"
#include "Player/YJJJumper.h"
#include "YJJAssetPaths.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

AYJJHUD::AYJJHUD()
{
	BaldEagleTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(YJJAssetPaths::BaldEagleTexture));
}

void AYJJHUD::DrawHUD()
{
	Super::DrawHUD();

	const AYJJJumper* Jumper = Cast<AYJJJumper>(GetOwningPawn());
	if (!Jumper || !Canvas)
	{
		return;
	}

	// 1080p 기준 크기
	const float UIScale = FMath::Max(FMath::Min(Canvas->ClipY / 1080.f, Canvas->ClipX / 600.f), 0.5f);

	DrawSpeedLines(*Jumper);
	DrawAttackRange(*Jumper);
	DrawLaunchPreview(*Jumper);
	DrawPopups(*Jumper, UIScale);
	DrawStats(*Jumper, UIScale);
	DrawLevel(*Jumper, UIScale);
	DrawAttackGauge(*Jumper, UIScale);
	DrawTutorial(*Jumper, UIScale);
	DrawLevelStolenBanner(*Jumper, UIScale);
}

UFont* AYJJHUD::GetFont() const
{
	// 기본 폰트(Roboto 컴포지트)에 한글 폴백이 들어 있다
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
	const float Offset = FMath::Max(Scale * 1.5f, 1.f);
	for (const FVector2D& Dir : { FVector2D(-1, 0), FVector2D(1, 0), FVector2D(0, -1), FVector2D(0, 1) })
	{
		DrawText(Text, Outline, X + Dir.X * Offset, Y + Dir.Y * Offset, Font, Scale);
	}
	DrawText(Text, Color, X, Y, Font, Scale);
}

void AYJJHUD::DrawSpeedLines(const AYJJJumper& Jumper)
{
	// 빠를수록 화면에 세로 속도선이 많고 길게 지나간다
	const float Speed = Jumper.GetSpeedKmh();
	const float Intensity = FMath::Clamp((Speed - 400.f) / 2600.f, 0.f, 1.f);
	if (Intensity <= 0.f || Jumper.GetSimState().Mode != EYJJJumperMode::Airborne)
	{
		return;
	}

	SpeedLineSeed += GetWorld()->GetDeltaSeconds() * 37.f;
	FRandomStream Random(FMath::FloorToInt(SpeedLineSeed));
	const int32 Count = FMath::RoundToInt(FMath::Lerp(3.f, 14.f, Intensity));
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float X = Random.FRandRange(0.f, Canvas->ClipX);
		const float Length = Canvas->ClipY * Random.FRandRange(0.1f, 0.35f) * (0.5f + Intensity);
		const float Y = Random.FRandRange(-Length, Canvas->ClipY);
		DrawLine(X, Y, X, Y + Length, FLinearColor(1.f, 1.f, 1.f, 0.12f + 0.18f * Intensity), Random.FRandRange(1.f, 2.f));
	}
}

void AYJJHUD::DrawAttackRange(const AYJJJumper& Jumper)
{
	// 공중에서만, 돌진 가능한 거리를 점선 원으로
	const FYJJJumperState& State = Jumper.GetSimState();
	if (State.Mode != EYJJJumperMode::Airborne || State.IsDashing())
	{
		return;
	}

	const FVector Center = Jumper.GetActorLocation();
	const float Range = Jumper.GetAttackRange();
	const int32 Segments = 40;
	for (int32 Index = 0; Index < Segments; Index += 2)
	{
		const float A0 = UE_TWO_PI * Index / Segments;
		const float A1 = UE_TWO_PI * (Index + 1) / Segments;
		const FVector P0 = Project(Center + FVector(FMath::Cos(A0) * Range, 0.f, FMath::Sin(A0) * Range), false);
		const FVector P1 = Project(Center + FVector(FMath::Cos(A1) * Range, 0.f, FMath::Sin(A1) * Range), false);
		DrawLine(P0.X, P0.Y, P1.X, P1.Y, FLinearColor(1.f, 0.9f, 0.3f, 0.5f), 2.f);
	}
}

void AYJJHUD::DrawStats(const AYJJJumper& Jumper, float UIScale)
{
	const FYJJJumperState& State = Jumper.GetSimState();
	const float X = 24.f * UIScale;
	float Y = 20.f * UIScale;
	const float Line = 38.f * UIScale;
	const float TextScale = 1.25f * UIScale;

	DrawOutlinedText(FString::Printf(TEXT("높이  %.1f m"), Jumper.GetHeightMeters()), FLinearColor::White, X, Y, TextScale, false);
	Y += Line;
	DrawOutlinedText(FString::Printf(TEXT("최고  %.1f m"), Jumper.GetBestHeightMeters()), FLinearColor(0.8f, 0.9f, 1.f), X, Y, TextScale, false);
	Y += Line;
	DrawOutlinedText(FString::Printf(TEXT("점수  %d"), State.Score), FLinearColor(1.f, 0.85f, 0.2f), X, Y, TextScale, false);
	Y += Line;
	DrawOutlinedText(FString::Printf(TEXT("처치  %d"), State.Kills), FLinearColor(1.f, 0.6f, 0.4f), X, Y, TextScale, false);

	// 속도계 (오른쪽 위). 빠를수록 빨갛고 크게
	const float Speed = Jumper.GetSpeedKmh();
	const float Hot = FMath::Clamp(Speed / 2000.f, 0.f, 1.f);
	const FLinearColor SpeedColor = FLinearColor::LerpUsingHSV(FLinearColor::White, FLinearColor(1.f, 0.2f, 0.1f), Hot);
	const FString SpeedText = FString::Printf(TEXT("%.0f km/h"), Speed);
	UFont* Font = GetFont();
	float Width = 0.f;
	float Height = 0.f;
	const float SpeedScale = (1.4f + Hot * 0.8f) * UIScale;
	GetTextSize(SpeedText, Width, Height, Font, SpeedScale);
	DrawOutlinedText(SpeedText, SpeedColor, Canvas->ClipX - Width - 24.f * UIScale, 20.f * UIScale, SpeedScale, false);
}

void AYJJHUD::DrawLevel(const AYJJJumper& Jumper, float UIScale)
{
	const FYJJJumperState& State = Jumper.GetSimState();
	const float X = 24.f * UIScale;
	const float Y = 20.f * UIScale + 38.f * UIScale * 4.f + 10.f * UIScale;

	if (State.bTutorial)
	{
		DrawOutlinedText(TEXT("Lv 999Gp"), FLinearColor(1.f, 0.4f, 1.f), X, Y, 1.5f * UIScale, false);
		DrawOutlinedText(TEXT("(구골플렉스. 우주 최강)"), FLinearColor(0.9f, 0.7f, 1.f), X, Y + 40.f * UIScale, 0.9f * UIScale, false);
		return;
	}

	DrawOutlinedText(FString::Printf(TEXT("Lv %d"), State.Level), LevelColor(), X, Y, 1.5f * UIScale, false);
	const float BarWidth = 180.f * UIScale;
	const float BarHeight = 10.f * UIScale;
	const float BarY = Y + 44.f * UIScale;
	const float Fill = FMath::Clamp(static_cast<float>(State.XP) / FMath::Max(Jumper.GetXPToNextLevel(), 1), 0.f, 1.f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X - 2.f, BarY - 2.f, BarWidth + 4.f, BarHeight + 4.f);
	DrawRect(LevelColor(), X, BarY, BarWidth * Fill, BarHeight);
}

FLinearColor AYJJHUD::LevelColor()
{
	return FLinearColor(0.4f, 1.f, 1.f);
}

void AYJJHUD::DrawAttackGauge(const AYJJJumper& Jumper, float UIScale)
{
	const FYJJJumperState& State = Jumper.GetSimState();
	const bool bAirborne = State.Mode == EYJJJumperMode::Airborne;
	const float Power = State.bTutorial ? 0.f : (bAirborne ? Jumper.GetAttackPower() : 0.f);

	const float Width = FMath::Min(Canvas->ClipX * 0.7f, 520.f * UIScale);
	const float Height = 20.f * UIScale;
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = Canvas->ClipY - 70.f * UIScale;

	// 공격력 10 이상이면 가득 참
	const float Fill = State.bTutorial ? 1.f : FMath::Clamp(Power / 10.f, 0.f, 1.f);
	const FLinearColor FillColor = State.bTutorial ? FLinearColor(1.f, 0.4f, 1.f) : FLinearColor::LerpUsingHSV(FLinearColor(0.3f, 0.8f, 1.f), FLinearColor(1.f, 0.3f, 0.2f), Fill);

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X - 3.f, Y - 3.f, Width + 6.f, Height + 6.f);
	DrawRect(FillColor, X, Y, Width * Fill, Height);
	const FString Label = State.bTutorial ? FString(TEXT("공격력 ∞")) : FString::Printf(TEXT("공격력 %.1f"), Power);
	DrawOutlinedText(Label, FLinearColor::White, Canvas->ClipX * 0.5f, Y - 24.f * UIScale, 1.1f * UIScale, true);

	if (State.PerfectChain > 1)
	{
		DrawOutlinedText(FString::Printf(TEXT("퍼펙트 연속 x%d"), State.PerfectChain), FLinearColor(1.f, 0.85f, 0.1f),
			Canvas->ClipX * 0.5f, Y - 58.f * UIScale, 1.2f * UIScale, true);
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

void AYJJHUD::DrawTutorial(const AYJJJumper& Jumper, float UIScale)
{
	const FYJJJumperState& State = Jumper.GetSimState();
	FString Hint;
	switch (Jumper.GetTutorialStep())
	{
	case EYJJTutorialStep::Drag:
		Hint = TEXT("줄을 아래로 쭉 끌었다 놔라!");
		break;
	case EYJJTutorialStep::TapMob:
		Hint = FString::Printf(TEXT("날아가면서 대머리를 탭해서 들이받아라! (%d/3)"), FMath::Min(State.Kills, 3));
		break;
	case EYJJTutorialStep::Perfect:
		Hint = TEXT("줄에 닿는 순간 탭! 퍼펙트 내봐");
		break;
	case EYJJTutorialStep::LevelStolen:
		Hint = TEXT("완벽하다. 이제 착지만 하면...");
		break;
	default:
		if (State.Mode == EYJJJumperMode::Resting)
		{
			Hint = TEXT("줄을 끌었다 놔서 다시 출발");
		}
		break;
	}

	if (!Hint.IsEmpty())
	{
		DrawOutlinedText(Hint, FLinearColor(1.f, 1.f, 0.6f, 0.95f), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.2f, 1.15f * UIScale, true);
	}
}

void AYJJHUD::DrawLevelStolenBanner(const AYJJJumper& Jumper, float UIScale)
{
	const float Time = Jumper.GetLevelStolenBannerTime();
	if (Time <= 0.f)
	{
		return;
	}

	// 화면을 어둡게 덮고 대머리 독수리가 레벨을 들고 튄다
	const float Elapsed = 4.5f - Time;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);

	if (UTexture2D* Eagle = BaldEagleTexture.LoadSynchronous())
	{
		const float Size = 260.f * UIScale;
		const float EagleX = FMath::Lerp(-Size, Canvas->ClipX + Size, FMath::Clamp(Elapsed / 3.5f, 0.f, 1.f)) - Size * 0.5f;
		const float EagleY = Canvas->ClipY * 0.28f + FMath::Sin(Elapsed * 12.f) * 20.f * UIScale;
		DrawTexture(Eagle, EagleX, EagleY, Size, Size, 0.f, 0.f, 1.f, 1.f);
		DrawOutlinedText(TEXT("Lv 999Gp"), FLinearColor(1.f, 0.4f, 1.f), EagleX + Size * 0.5f, EagleY - 20.f * UIScale, 1.2f * UIScale, true);
	}

	const float CenterX = Canvas->ClipX * 0.5f;
	DrawOutlinedText(TEXT("대머리 독수리가"), FLinearColor::White, CenterX, Canvas->ClipY * 0.55f, 1.4f * UIScale, true);
	DrawOutlinedText(TEXT("레벨을 몽땅 훔쳐갔다!!"), FLinearColor::White, CenterX, Canvas->ClipY * 0.55f + 46.f * UIScale, 1.4f * UIScale, true);
	DrawOutlinedText(TEXT("Lv 999Gp  →  Lv 1"), FLinearColor(1.f, 0.35f, 0.3f), CenterX, Canvas->ClipY * 0.55f + 110.f * UIScale, 1.9f * UIScale, true);
	DrawOutlinedText(TEXT("다시 키워라 ㅋ"), FLinearColor(1.f, 1.f, 0.6f), CenterX, Canvas->ClipY * 0.55f + 170.f * UIScale, 1.1f * UIScale, true);
}
