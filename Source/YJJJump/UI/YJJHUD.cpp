#include "UI/YJJHUD.h"
#include "Player/YJJJumper.h"
#include "YJJAssetPaths.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "Misc/Paths.h"
#include "Fonts/CompositeFont.h"

namespace
{
	const FLinearColor Ink(0.08f, 0.06f, 0.09f);
	const FLinearColor Yellow(1.f, 0.82f, 0.25f);
	const FLinearColor Cream(1.f, 0.97f, 0.9f);
	const FLinearColor Cyan(0.35f, 0.95f, 1.f);
	const FLinearColor Orange(1.f, 0.5f, 0.15f);
	const FLinearColor Red(1.f, 0.25f, 0.2f);

	TSoftObjectPtr<UTexture2D> TexturePath(const TCHAR* Path)
	{
		return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Path));
	}
}

AYJJHUD::AYJJHUD()
{
	PanelTexture = TexturePath(YJJAssetPaths::UIPanelTexture);
	CoinTexture = TexturePath(YJJAssetPaths::UICoinTexture);
	SkullTexture = TexturePath(YJJAssetPaths::UISkullTexture);
	ArrowTexture = TexturePath(YJJAssetPaths::UIArrowTexture);
	FlameTexture = TexturePath(YJJAssetPaths::UIFlameTexture);
	BaldEagleTexture = TexturePath(YJJAssetPaths::BaldEagleTexture);
}

void AYJJHUD::DrawHUD()
{
	Super::DrawHUD();

	const AYJJJumper* Jumper = Cast<AYJJJumper>(GetOwningPawn());
	if (!Jumper || !Canvas)
	{
		return;
	}

	// 세로 1280 / 가로 720 기준
	UIScale = FMath::Max(FMath::Min(Canvas->ClipY / 1280.f, Canvas->ClipX / 720.f), 0.45f);

	DrawSpeedLines(*Jumper);
	DrawAttackRange(*Jumper);
	DrawLaunchPreview(*Jumper);
	DrawPopups(*Jumper);
	DrawLevelBadge(*Jumper);
	DrawHeight(*Jumper);
	DrawScoreChips(*Jumper);
	DrawAttackChip(*Jumper);
	DrawSpeedometer(*Jumper);
	DrawHint(*Jumper);
	DrawLevelStolenBanner(*Jumper);
}

TSharedPtr<const FCompositeFont> AYJJHUD::GetGameFont(const UFont* FallbackFont)
{
	if (GameFont.IsValid())
	{
		return GameFont;
	}

	// 캐주얼 게임에서 가장 흔한 배민 주아체(OFL). 주아체에 없는 글자는 Roboto로
	// 커맨드릿에서는 TTF 임포트가 안 돼서 에셋 대신 파일을 직접 읽는다 (패키징 시 DefaultGame.ini에서 스테이징)
	const FString FontPath = FPaths::ProjectContentDir() / GameFontFile;
	if (!FPaths::FileExists(FontPath))
	{
		return nullptr;
	}

	TSharedRef<FCompositeFont> Composite = MakeShared<FCompositeFont>();
	Composite->DefaultTypeface.Fonts.Add(FTypefaceEntry(TEXT("Regular"), FontPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
	if (const FCompositeFont* Fallback = FallbackFont ? FallbackFont->GetCompositeFont() : nullptr)
	{
		Composite->FallbackTypeface.Typeface = Fallback->DefaultTypeface;
	}
	GameFont = Composite;
	return GameFont;
}

void AYJJHUD::DrawLabel(const FString& Text, float X, float Y, float Size, const FLinearColor& Color, EAlign Align, float Outline, bool bVerticalCenter)
{
	// FCanvasTextItem은 UFont가 있어야 그려진다 (유효성/캐시 종류 판단용). 실제 글꼴은 주아체 컴포지트
	static const UFont* BaseFont = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	if (!BaseFont)
	{
		return;
	}
	const int32 FontSize = FMath::Max(FMath::RoundToInt(S(Size)), 6);
	const TSharedPtr<const FCompositeFont> Composite = GetGameFont(BaseFont);
	FSlateFontInfo Font = Composite.IsValid() ? FSlateFontInfo(Composite, FontSize) : FSlateFontInfo(BaseFont, FontSize, TEXT("Bold"));
	if (Outline > 0.f)
	{
		Font.OutlineSettings.OutlineSize = FMath::Max(FMath::RoundToInt(S(Outline)), 1);
		Font.OutlineSettings.OutlineColor = FLinearColor(Ink.R, Ink.G, Ink.B, Color.A);
	}

	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, Color);
	Item.Font = BaseFont;
	Item.bCentreX = Align == EAlign::Center;
	Item.bCentreY = bVerticalCenter;
	Canvas->DrawItem(Item);
}

void AYJJHUD::DrawPanel(float X, float Y, float W, float H, const FLinearColor& Color)
{
	UTexture2D* Texture = PanelTexture.LoadSynchronous();
	if (!Texture)
	{
		DrawRect(Color, X, Y, W, H);
		return;
	}

	// 24px 텍스처의 모서리 8px은 그대로, 가운데는 늘린다
	const float SourceCorner = 8.f / 24.f;
	const float Corner = FMath::Min(S(18.f), FMath::Min(W, H) * 0.5f);
	const float Xs[3] = { X, X + Corner, X + W - Corner };
	const float Ws[3] = { Corner, W - Corner * 2.f, Corner };
	const float Ys[3] = { Y, Y + Corner, Y + H - Corner };
	const float Hs[3] = { Corner, H - Corner * 2.f, Corner };
	const float Us[3] = { 0.f, SourceCorner, 1.f - SourceCorner };
	const float UWs[3] = { SourceCorner, 1.f - SourceCorner * 2.f, SourceCorner };

	for (int32 Row = 0; Row < 3; ++Row)
	{
		for (int32 Column = 0; Column < 3; ++Column)
		{
			DrawTexture(Texture, Xs[Column], Ys[Row], Ws[Column], Hs[Row], Us[Column], Us[Row], UWs[Column], UWs[Row], Color);
		}
	}
}

void AYJJHUD::DrawIcon(const TSoftObjectPtr<UTexture2D>& Icon, float X, float Y, float Size)
{
	if (UTexture2D* Texture = Icon.LoadSynchronous())
	{
		DrawTexture(Texture, X, Y, Size, Size, 0.f, 0.f, 1.f, 1.f);
	}
}

void AYJJHUD::DrawLevelBadge(const AYJJJumper& Jumper)
{
	const FYJJJumperState& State = Jumper.GetSimState();
	const float X = S(20.f);
	const float Y = S(20.f);
	const float W = S(190.f);
	const float H = S(70.f);

	DrawPanel(X, Y, W, H, Yellow);
	const FString LevelText = State.bTutorial ? FString(TEXT("Lv 999Gp")) : FString::Printf(TEXT("Lv %d"), State.Level);
	DrawLabel(LevelText, X + W * 0.5f, Y + H * 0.5f, 34.f, Ink, EAlign::Center, 0.f);

	// 경험치 바: 검은 테두리 패널 안에 하늘색 채움
	const float BarY = Y + H + S(8.f);
	const float BarH = S(26.f);
	DrawPanel(X, BarY, W, BarH, FLinearColor(0.25f, 0.22f, 0.3f));
	const float Fill = State.bTutorial ? 1.f : FMath::Clamp(static_cast<float>(State.XP) / FMath::Max(Jumper.GetXPToNextLevel(), 1), 0.f, 1.f);
	const float Inset = S(6.f);
	if (Fill > 0.f)
	{
		DrawRect(Cyan, X + Inset, BarY + Inset, (W - Inset * 2.f) * Fill, BarH - Inset * 2.f);
	}
	DrawLabel(TEXT("EXP"), X + W * 0.5f, BarY + BarH * 0.5f, 14.f, FLinearColor::White, EAlign::Center, 2.f);
}

void AYJJHUD::DrawHeight(const AYJJJumper& Jumper)
{
	const float CenterX = Canvas->ClipX * 0.5f;
	DrawIcon(ArrowTexture, CenterX - S(165.f), S(30.f), S(36.f));
	DrawLabel(FString::Printf(TEXT("%.0fm"), FMath::Max(Jumper.GetHeightMeters(), 0.f)), CenterX + S(10.f), S(48.f), 50.f, FLinearColor::White, EAlign::Center, 4.f);
	DrawLabel(FString::Printf(TEXT("최고 %.0fm"), Jumper.GetBestHeightMeters()), CenterX + S(10.f), S(98.f), 20.f, Cream, EAlign::Center, 3.f);

	const FYJJJumperState& State = Jumper.GetSimState();
	if (State.PerfectChain > 1)
	{
		// 연속 퍼펙트는 숫자가 커질수록 빨갛고 크게
		const float Heat = FMath::Clamp((State.PerfectChain - 1) / 9.f, 0.f, 1.f);
		const FLinearColor Color = FLinearColor::LerpUsingHSV(Yellow, Red, Heat);
		DrawLabel(FString::Printf(TEXT("퍼펙트 x%d"), State.PerfectChain), CenterX, S(140.f), 26.f + Heat * 10.f, Color, EAlign::Center, 3.f);
	}
}

void AYJJHUD::DrawScoreChips(const AYJJJumper& Jumper)
{
	const FYJJJumperState& State = Jumper.GetSimState();
	const float W = S(190.f);
	const float H = S(50.f);
	const float X = Canvas->ClipX - W - S(20.f);
	const float Icon = S(32.f);

	float Y = S(20.f);
	DrawPanel(X, Y, W, H, Cream);
	DrawIcon(CoinTexture, X + S(12.f), Y + (H - Icon) * 0.5f, Icon);
	DrawLabel(FString::Printf(TEXT("%d"), State.Score), X + S(54.f), Y + H * 0.5f, 24.f, Ink, EAlign::Left, 0.f);

	Y += H + S(8.f);
	DrawPanel(X, Y, W, H, Cream);
	DrawIcon(SkullTexture, X + S(12.f), Y + (H - Icon) * 0.5f, Icon);
	DrawLabel(FString::Printf(TEXT("%d"), State.Kills), X + S(54.f), Y + H * 0.5f, 24.f, Ink, EAlign::Left, 0.f);
}

void AYJJHUD::DrawAttackChip(const AYJJJumper& Jumper)
{
	const FYJJJumperState& State = Jumper.GetSimState();
	const bool bAirborne = State.Mode == EYJJJumperMode::Airborne;
	const float Power = State.bTutorial ? 0.f : Jumper.GetAttackPower();

	const float W = S(250.f);
	const float H = S(56.f);
	const float X = S(20.f);
	const float Y = Canvas->ClipY - H - S(24.f);

	// 공중에서 공격 가능할 때만 주황, 로프 위에선 회색
	DrawPanel(X, Y, W, H, bAirborne ? Orange : FLinearColor(0.6f, 0.6f, 0.6f));
	DrawIcon(FlameTexture, X + S(10.f), Y + (H - S(36.f)) * 0.5f, S(36.f));
	FString Text;
	if (State.bTutorial)
	{
		Text = TEXT("공격력 ∞");
	}
	else
	{
		Text = Power >= 100.f ? FString::Printf(TEXT("공격력 %.0f"), Power) : FString::Printf(TEXT("공격력 %.1f"), Power);
	}
	DrawLabel(Text, X + S(54.f), Y + H * 0.5f, 24.f, FLinearColor::White, EAlign::Left, 3.f);
}

void AYJJHUD::DrawSpeedometer(const AYJJJumper& Jumper)
{
	const float Speed = Jumper.GetSpeedKmh();
	const float Heat = FMath::Clamp(Speed / 550.f, 0.f, 1.f);
	const FLinearColor Color = FLinearColor::LerpUsingHSV(FLinearColor::White, Red, Heat);

	const float W = S(240.f);
	const float H = S(80.f);
	const float X = Canvas->ClipX - W - S(20.f);
	const float Y = Canvas->ClipY - H - S(24.f);

	DrawPanel(X, Y, W, H, FLinearColor(0.15f, 0.12f, 0.2f, 0.85f));
	// 빠를수록 불꽃 아이콘이 늘어난다
	const int32 Flames = FMath::Clamp(FMath::FloorToInt(Speed / 180.f), 0, 3);
	for (int32 Index = 0; Index < Flames; ++Index)
	{
		DrawIcon(FlameTexture, X + W - S(34.f) * (Index + 1) - S(6.f), Y - S(30.f), S(30.f));
	}
	DrawLabel(FString::Printf(TEXT("%.0f"), Speed), X + W * 0.5f, Y + H * 0.42f, 40.f + Heat * 8.f, Color, EAlign::Center, 3.f);
	DrawLabel(TEXT("km/h"), X + W * 0.5f, Y + H * 0.8f, 16.f, Cream, EAlign::Center, 2.f);
}

void AYJJHUD::DrawHint(const AYJJJumper& Jumper)
{
	const FYJJJumperState& State = Jumper.GetSimState();
	FString Hint;
	switch (Jumper.GetTutorialStep())
	{
	case EYJJTutorialStep::Drag:
		Hint = TEXT("줄을 아래로 쭉 끌었다 놔라!");
		break;
	case EYJJTutorialStep::TapMob:
		Hint = FString::Printf(TEXT("대머리를 탭해서 들이받아라! (%d/3)"), FMath::Min(State.Kills, 3));
		break;
	case EYJJTutorialStep::Perfect:
		Hint = TEXT("줄에 닿는 순간 탭! 퍼펙트 내봐");
		break;
	case EYJJTutorialStep::LevelStolen:
		Hint = TEXT("완벽하다. 이제 착지만 하면...");
		break;
	default:
		if (State.Mode == EYJJJumperMode::Resting || State.Mode == EYJJJumperMode::Dragging)
		{
			Hint = TEXT("줄을 아래로 끌었다 놔!");
		}
		break;
	}
	if (Hint.IsEmpty())
	{
		return;
	}

	// 말풍선 패널
	const float W = FMath::Min(Canvas->ClipX - S(40.f), S(520.f));
	const float H = S(64.f);
	const float X = (Canvas->ClipX - W) * 0.5f;
	const float Y = Canvas->ClipY * 0.68f;
	DrawPanel(X, Y, W, H, FLinearColor::White);
	DrawLabel(Hint, Canvas->ClipX * 0.5f, Y + H * 0.5f, 24.f, Ink, EAlign::Center, 0.f);
}

void AYJJHUD::DrawSpeedLines(const AYJJJumper& Jumper)
{
	// 빠를수록 화면에 세로 속도선이 지나간다 (과하지 않게)
	const float Speed = Jumper.GetSpeedKmh();
	const float Intensity = FMath::Clamp((Speed - 150.f) / 400.f, 0.f, 1.f);
	if (Intensity <= 0.f || Jumper.GetSimState().Mode != EYJJJumperMode::Airborne || Jumper.IsAttacking())
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
	const int32 Segments = 48;
	for (int32 Index = 0; Index < Segments; Index += 2)
	{
		const float A0 = UE_TWO_PI * Index / Segments;
		const float A1 = UE_TWO_PI * (Index + 1) / Segments;
		const FVector P0 = Project(Center + FVector(FMath::Cos(A0) * Range, 0.f, FMath::Sin(A0) * Range), false);
		const FVector P1 = Project(Center + FVector(FMath::Cos(A1) * Range, 0.f, FMath::Sin(A1) * Range), false);
		DrawLine(P0.X, P0.Y, P1.X, P1.Y, FLinearColor(1.f, 0.95f, 0.5f, 0.35f), S(3.f));
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
		const float Size = S(FMath::Lerp(14.f, 6.f, static_cast<float>(Index) / DotCount));
		DrawRect(Ink, Screen.X - Size * 0.5f - 2.f, Screen.Y - Size * 0.5f - 2.f, Size + 4.f, Size + 4.f);
		DrawRect(FLinearColor::White, Screen.X - Size * 0.5f, Screen.Y - Size * 0.5f, Size, Size);
	}
}

void AYJJHUD::DrawPopups(const AYJJJumper& Jumper)
{
	for (const FYJJPopup& Popup : Jumper.GetPopups())
	{
		const FVector Screen = Project(Popup.WorldLocation, false);
		const float Alpha = 1.f - FMath::Clamp((Popup.Age - Popup.Lifetime * 0.6f) / (Popup.Lifetime * 0.4f), 0.f, 1.f);
		// 처음에 확 커졌다 줄어드는 펀치
		const float Punch = 1.f + 0.5f * FMath::Max(1.f - Popup.Age * 8.f, 0.f);

		FLinearColor Color = Popup.Color;
		Color.A = Alpha;
		DrawLabel(Popup.Text, Screen.X, Screen.Y, 24.f * Popup.Scale * Punch, Color, EAlign::Center, 3.f);
	}
}

void AYJJHUD::DrawLevelStolenBanner(const AYJJJumper& Jumper)
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
		const float Size = S(300.f);
		const float EagleX = FMath::Lerp(-Size, Canvas->ClipX + Size, FMath::Clamp(Elapsed / 3.5f, 0.f, 1.f)) - Size * 0.5f;
		const float EagleY = Canvas->ClipY * 0.22f + FMath::Sin(Elapsed * 12.f) * S(20.f);
		DrawTexture(Eagle, EagleX, EagleY, Size, Size * 108.f / 84.f, 0.f, 0.f, 1.f, 1.f);
	}

	const float CenterX = Canvas->ClipX * 0.5f;
	const float Y = Canvas->ClipY * 0.58f;
	DrawLabel(TEXT("대머리 독수리가"), CenterX, Y, 32.f, FLinearColor::White, EAlign::Center);
	DrawLabel(TEXT("레벨을 몽땅 훔쳐갔다!!"), CenterX, Y + S(50.f), 32.f, FLinearColor::White, EAlign::Center);
	DrawLabel(TEXT("Lv 999Gp  →  Lv 1"), CenterX, Y + S(120.f), 44.f, Red, EAlign::Center);
	DrawLabel(TEXT("다시 키워라 ㅋ"), CenterX, Y + S(180.f), 24.f, Yellow, EAlign::Center);
}
