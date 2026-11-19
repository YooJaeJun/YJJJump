#include "UI/YJJHUD.h"
#include "Data/YJJData.h"
#include "Data/YJJLoc.h"
#include "Player/YJJJumper.h"
#include "YJJAssetPaths.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "Misc/Paths.h"
#include "Fonts/CompositeFont.h"

TSoftObjectPtr<UTexture2D> AYJJHUD::TexturePath(const TCHAR* Path)
{
	return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Path));
}

AYJJHUD::AYJJHUD()
{
	PanelTexture = TexturePath(YJJAssetPaths::UIPanelTexture);
	CoinTexture = TexturePath(YJJAssetPaths::UICoinTexture);
	SkullTexture = TexturePath(YJJAssetPaths::UISkullTexture);
	ArrowTexture = TexturePath(YJJAssetPaths::UIArrowTexture);
	FlameTexture = TexturePath(YJJAssetPaths::UIFlameTexture);
	BaldEagleTexture = TexturePath(YJJAssetPaths::BaldEagleTexture);
	GameFontFile = YJJAssetPaths::GameFontFile;
}

void AYJJHUD::DrawHUD()
{
	Super::DrawHUD();

	const AYJJJumper* Jumper = Cast<AYJJJumper>(GetOwningPawn());
	if (!Jumper || !Canvas)
	{
		return;
	}

	// 기준 해상도(세로 화면) 대비 배율
	const FYJJHudTuning& Hud = FYJJData::Get().Hud;
	UIScale = FMath::Max(FMath::Min(Canvas->ClipY / Hud.ReferenceHeight, Canvas->ClipX / Hud.ReferenceWidth), Hud.MinScale);

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

void AYJJHUD::DrawLabel(const FString& Text, float X, float Y, float Size, const FLinearColor& Color, EAlign Align, float Outline)
{
	if (!BaseFont)
	{
		BaseFont = LoadObject<UFont>(nullptr, YJJAssetPaths::FallbackFont);
		if (!BaseFont)
		{
			return;
		}
	}

	const FYJJData& Data = FYJJData::Get();
	const int32 FontSize = FMath::Max(FMath::RoundToInt(S(Size)), Data.Hud.MinFontSize);
	const TSharedPtr<const FCompositeFont> Composite = GetGameFont(BaseFont);
	FSlateFontInfo Font = Composite.IsValid() ? FSlateFontInfo(Composite, FontSize) : FSlateFontInfo(BaseFont, FontSize, YJJAssetPaths::FallbackFontTypeface);
	if (Outline > 0.f)
	{
		const FLinearColor& Ink = Data.Colors.Ink;
		Font.OutlineSettings.OutlineSize = FMath::Max(FMath::RoundToInt(S(Outline)), 1);
		Font.OutlineSettings.OutlineColor = FLinearColor(Ink.R, Ink.G, Ink.B, Color.A);
	}

	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, Color);
	Item.Font = BaseFont;
	Item.bCentreX = Align == EAlign::Center;
	Item.bCentreY = true;
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

	// 패널 텍스처의 모서리 픽셀은 그대로, 가운데는 늘린다
	const FYJJHudTuning& Hud = FYJJData::Get().Hud;
	const float SourceCorner = Hud.PanelCornerPixels / Hud.PanelTexturePixels;
	const float Corner = FMath::Min(S(Hud.PanelCorner), FMath::Min(W, H) * 0.5f);
	const float Xs[3] = { X, X + Corner, X + W - Corner };
	const float Ws[3] = { Corner, W - Corner * 2.f, Corner };
	const float Ys[3] = { Y, Y + Corner, Y + H - Corner };
	const float Hs[3] = { Corner, H - Corner * 2.f, Corner };
	const float Us[3] = { 0.f, SourceCorner, 1.f - SourceCorner };
	const float UWs[3] = { SourceCorner, 1.f - SourceCorner * 2.f, SourceCorner };

	for (int32 Row = 0; Row < UE_ARRAY_COUNT(Ys); ++Row)
	{
		for (int32 Column = 0; Column < UE_ARRAY_COUNT(Xs); ++Column)
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
	const FYJJData& Data = FYJJData::Get();
	const FYJJPalette& Colors = Data.Colors;
	const FYJJHudLayoutRow& Badge = Data.Layout(TEXT("LevelBadge"));
	const FYJJJumperState& State = Jumper.GetSimState();
	const float X = S(Badge.X);
	const float Y = S(Badge.Y);
	const float W = S(Badge.W);
	const float H = S(Badge.H);

	DrawPanel(X, Y, W, H, Colors.Yellow);
	const FString LevelText = State.bTutorial ? FYJJLoc::Get(TEXT("Hud.LevelTutorial")) : FYJJLoc::Format(TEXT("Hud.Level"), { State.Level });
	DrawLabel(LevelText, X + W * 0.5f, Y + H * 0.5f, Badge.TextSize, Colors.Ink, EAlign::Center, Badge.Outline);

	// 경험치 바: 검은 테두리 패널 안에 하늘색 채움
	const FYJJHudLayoutRow& Bar = Data.Layout(TEXT("ExpBar"));
	const float BarY = Y + H + S(Bar.Gap);
	const float BarH = S(Bar.H);
	DrawPanel(X, BarY, W, BarH, Colors.ExpBarBack);
	const float Fill = State.bTutorial ? 1.f : FMath::Clamp(static_cast<float>(State.XP) / FMath::Max(Jumper.GetXPToNextLevel(), 1), 0.f, 1.f);
	const float Inset = S(Bar.Inset);
	if (Fill > 0.f)
	{
		DrawRect(Colors.Cyan, X + Inset, BarY + Inset, (W - Inset * 2.f) * Fill, BarH - Inset * 2.f);
	}
	DrawLabel(FYJJLoc::Get(TEXT("Hud.Exp")), X + W * 0.5f, BarY + BarH * 0.5f, Bar.TextSize, Colors.HudText, EAlign::Center, Bar.Outline);
}

void AYJJHUD::DrawHeight(const AYJJJumper& Jumper)
{
	const FYJJData& Data = FYJJData::Get();
	const FYJJPalette& Colors = Data.Colors;
	const FYJJHudLayoutRow& Height = Data.Layout(TEXT("Height"));
	const FYJJHudLayoutRow& Best = Data.Layout(TEXT("BestHeight"));
	const float CenterX = Canvas->ClipX * 0.5f;

	DrawIcon(ArrowTexture, CenterX + S(Height.IconX), S(Height.IconY), S(Height.IconSize));
	DrawLabel(FYJJLoc::Format(TEXT("Hud.Height"), { FString::Printf(TEXT("%.0f"), FMath::Max(Jumper.GetHeightMeters(), 0.f)) }),
		CenterX + S(Height.TextX), S(Height.TextY), Height.TextSize, Colors.HudText, EAlign::Center, Height.Outline);
	DrawLabel(FYJJLoc::Format(TEXT("Hud.Best"), { FString::Printf(TEXT("%.0f"), Jumper.GetBestHeightMeters()) }),
		CenterX + S(Best.TextX), S(Best.TextY), Best.TextSize, Colors.Cream, EAlign::Center, Best.Outline);

	const FYJJJumperState& State = Jumper.GetSimState();
	if (State.PerfectChain > 1)
	{
		// 연속 퍼펙트는 숫자가 커질수록 빨갛고 크게
		const FYJJHudLayoutRow& Chain = Data.Layout(TEXT("PerfectChain"));
		const float Heat = FMath::Clamp((State.PerfectChain - 1) / Data.Hud.ChainHeatMax, 0.f, 1.f);
		const FLinearColor Color = FLinearColor::LerpUsingHSV(Colors.Yellow, Colors.Red, Heat);
		DrawLabel(FYJJLoc::Format(TEXT("Hud.PerfectChain"), { State.PerfectChain }), CenterX + S(Chain.TextX), S(Chain.TextY),
			Chain.TextSize + Heat * Data.Hud.ChainGrowSize, Color, EAlign::Center, Chain.Outline);
	}
}

void AYJJHUD::DrawScoreChips(const AYJJJumper& Jumper)
{
	const FYJJData& Data = FYJJData::Get();
	const FYJJPalette& Colors = Data.Colors;
	const FYJJHudLayoutRow& Chip = Data.Layout(TEXT("ScoreChip"));
	const FYJJJumperState& State = Jumper.GetSimState();
	const float W = S(Chip.W);
	const float H = S(Chip.H);
	const float X = Canvas->ClipX - W - S(Chip.X);
	const float Icon = S(Chip.IconSize);

	float Y = S(Chip.Y);
	DrawPanel(X, Y, W, H, Colors.Cream);
	DrawIcon(CoinTexture, X + S(Chip.IconX), Y + (H - Icon) * 0.5f, Icon);
	DrawLabel(FString::FromInt(State.Score), X + S(Chip.TextX), Y + H * 0.5f, Chip.TextSize, Colors.Ink, EAlign::Left, Chip.Outline);

	Y += H + S(Chip.Gap);
	DrawPanel(X, Y, W, H, Colors.Cream);
	DrawIcon(SkullTexture, X + S(Chip.IconX), Y + (H - Icon) * 0.5f, Icon);
	DrawLabel(FString::FromInt(State.Kills), X + S(Chip.TextX), Y + H * 0.5f, Chip.TextSize, Colors.Ink, EAlign::Left, Chip.Outline);
}

void AYJJHUD::DrawAttackChip(const AYJJJumper& Jumper)
{
	const FYJJData& Data = FYJJData::Get();
	const FYJJPalette& Colors = Data.Colors;
	const FYJJHudLayoutRow& Chip = Data.Layout(TEXT("AttackChip"));
	const FYJJJumperState& State = Jumper.GetSimState();
	const bool bAirborne = State.Mode == EYJJJumperMode::Airborne;

	const float W = S(Chip.W);
	const float H = S(Chip.H);
	const float X = S(Chip.X);
	const float Y = Canvas->ClipY - H - S(Chip.Y);
	const float Icon = S(Chip.IconSize);

	// 공중에서 공격 가능할 때만 주황, 로프 위에선 회색
	DrawPanel(X, Y, W, H, bAirborne ? Colors.Orange : Colors.AttackIdle);
	DrawIcon(FlameTexture, X + S(Chip.IconX), Y + (H - Icon) * 0.5f, Icon);
	FString Text;
	if (State.bTutorial)
	{
		Text = FYJJLoc::Get(TEXT("Hud.PowerInfinite"));
	}
	else
	{
		const float Power = Jumper.GetAttackPower();
		const FString Number = Power >= Data.Hud.PowerIntegerFrom ? FString::Printf(TEXT("%.0f"), Power) : FString::Printf(TEXT("%.1f"), Power);
		Text = FYJJLoc::Format(TEXT("Hud.Power"), { Number });
	}
	DrawLabel(Text, X + S(Chip.TextX), Y + H * 0.5f, Chip.TextSize, Colors.HudText, EAlign::Left, Chip.Outline);
}

void AYJJHUD::DrawSpeedometer(const AYJJJumper& Jumper)
{
	const FYJJData& Data = FYJJData::Get();
	const FYJJPalette& Colors = Data.Colors;
	const FYJJHudTuning& Hud = Data.Hud;
	const FYJJHudLayoutRow& Meter = Data.Layout(TEXT("Speedometer"));
	const FYJJHudLayoutRow& Unit = Data.Layout(TEXT("SpeedUnit"));

	const float Speed = Jumper.GetSpeedKmh();
	const float Heat = FMath::Clamp(Speed / Hud.SpeedHeatMax, 0.f, 1.f);
	const FLinearColor Color = FLinearColor::LerpUsingHSV(Colors.HudText, Colors.Red, Heat);

	const float W = S(Meter.W);
	const float H = S(Meter.H);
	const float X = Canvas->ClipX - W - S(Meter.X);
	const float Y = Canvas->ClipY - H - S(Meter.Y);

	DrawPanel(X, Y, W, H, Colors.SpeedPanel);
	// 빠를수록 불꽃 아이콘이 늘어난다
	const int32 Flames = FMath::Clamp(FMath::FloorToInt(Speed / Hud.SpeedFlameStep), 0, Hud.SpeedFlameMax);
	for (int32 Index = 0; Index < Flames; ++Index)
	{
		DrawIcon(FlameTexture, X + W - S(Meter.Gap) * (Index + 1) - S(Meter.IconX), Y + S(Meter.IconY), S(Meter.IconSize));
	}
	DrawLabel(FString::Printf(TEXT("%.0f"), Speed), X + W * 0.5f, Y + S(Meter.TextY), Meter.TextSize + Heat * Hud.SpeedGrowSize, Color, EAlign::Center, Meter.Outline);
	DrawLabel(FYJJLoc::Get(TEXT("Hud.SpeedUnit")), X + W * 0.5f, Y + S(Unit.TextY), Unit.TextSize, Colors.Cream, EAlign::Center, Unit.Outline);
}

void AYJJHUD::DrawHint(const AYJJJumper& Jumper)
{
	const FYJJData& Data = FYJJData::Get();
	const FYJJJumperState& State = Jumper.GetSimState();
	FString Hint;
	switch (Jumper.GetTutorialStep())
	{
	case EYJJTutorialStep::Drag:
		Hint = FYJJLoc::Get(TEXT("Hint.Drag"));
		break;
	case EYJJTutorialStep::TapMob:
	{
		const int32 Needed = Data.Player.TutorialKillsNeeded;
		Hint = FYJJLoc::Format(TEXT("Hint.TapMob"), { FMath::Min(State.Kills, Needed), Needed });
		break;
	}
	case EYJJTutorialStep::Perfect:
		Hint = FYJJLoc::Get(TEXT("Hint.Perfect"));
		break;
	case EYJJTutorialStep::LevelStolen:
		Hint = FYJJLoc::Get(TEXT("Hint.LevelStolen"));
		break;
	default:
		if (State.Mode == EYJJJumperMode::Resting || State.Mode == EYJJJumperMode::Dragging)
		{
			Hint = FYJJLoc::Get(TEXT("Hint.Rest"));
		}
		break;
	}
	if (Hint.IsEmpty())
	{
		return;
	}

	// 말풍선 패널
	const FYJJHudLayoutRow& Bubble = Data.Layout(TEXT("Hint"));
	const float W = FMath::Min(Canvas->ClipX - S(Bubble.X) * 2.f, S(Bubble.W));
	const float H = S(Bubble.H);
	const float X = (Canvas->ClipX - W) * 0.5f;
	const float Y = Canvas->ClipY * Bubble.ScreenY;
	DrawPanel(X, Y, W, H, Data.Colors.HintPanel);
	DrawLabel(Hint, Canvas->ClipX * 0.5f, Y + H * 0.5f, Bubble.TextSize, Data.Colors.Ink, EAlign::Center, Bubble.Outline);
}

void AYJJHUD::DrawSpeedLines(const AYJJJumper& Jumper)
{
	// 빠를수록 화면에 세로 속도선이 지나간다 (과하지 않게)
	const FYJJData& Data = FYJJData::Get();
	const FYJJHudTuning& Hud = Data.Hud;
	const float Speed = Jumper.GetSpeedKmh();
	const float Intensity = FMath::Clamp((Speed - Hud.SpeedLineFrom) / Hud.SpeedLineRange, 0.f, 1.f);
	if (Intensity <= 0.f || Jumper.GetSimState().Mode != EYJJJumperMode::Airborne || Jumper.IsAttacking())
	{
		return;
	}

	SpeedLineSeed += GetWorld()->GetDeltaSeconds() * Hud.SpeedLineSeedRate;
	FRandomStream Random(FMath::FloorToInt(SpeedLineSeed));
	const int32 Count = FMath::RoundToInt(FMath::Lerp(Hud.SpeedLineCountMin, Hud.SpeedLineCountMax, Intensity));
	FLinearColor Color = Data.Colors.SpeedLine;
	Color.A = Hud.SpeedLineAlphaBase + Hud.SpeedLineAlphaGain * Intensity;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float X = Random.FRandRange(0.f, Canvas->ClipX);
		const float Length = Canvas->ClipY * Random.FRandRange(Hud.SpeedLineLengthMin, Hud.SpeedLineLengthMax) * (Hud.SpeedLineLengthBase + Intensity);
		const float Y = Random.FRandRange(-Length, Canvas->ClipY);
		DrawLine(X, Y, X, Y + Length, Color, Random.FRandRange(Hud.SpeedLineWidthMin, Hud.SpeedLineWidthMax));
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

	const FYJJData& Data = FYJJData::Get();
	const FVector Center = Jumper.GetActorLocation();
	const float Range = Jumper.GetAttackRange();
	const int32 Segments = Data.Hud.RangeSegments;
	for (int32 Index = 0; Index < Segments; Index += 2)
	{
		const float A0 = UE_TWO_PI * Index / Segments;
		const float A1 = UE_TWO_PI * (Index + 1) / Segments;
		const FVector P0 = Project(Center + FVector(FMath::Cos(A0) * Range, 0.f, FMath::Sin(A0) * Range), false);
		const FVector P1 = Project(Center + FVector(FMath::Cos(A1) * Range, 0.f, FMath::Sin(A1) * Range), false);
		DrawLine(P0.X, P0.Y, P1.X, P1.Y, Data.Colors.RangeCircle, S(Data.Hud.RangeLineWidth));
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
	const FYJJData& Data = FYJJData::Get();
	const FYJJHudTuning& Hud = Data.Hud;
	const FVector Start = Jumper.GetActorLocation();
	const float Gravity = Jumper.Tuning.Gravity;
	const int32 DotCount = Hud.PreviewDots;
	const float Border = Hud.PreviewDotBorder;
	const float TotalTime = FMath::Clamp(Velocity.Y / FMath::Max(Gravity, 1.f) * Hud.PreviewTimeScale, Hud.PreviewTimeMin, Hud.PreviewTimeMax);
	for (int32 Index = 1; Index <= DotCount; ++Index)
	{
		const float T = TotalTime * Index / DotCount;
		const FVector Point(Start.X + Velocity.X * T, Start.Y, Start.Z + Velocity.Y * T - 0.5f * Gravity * T * T);
		const FVector Screen = Project(Point, false);
		const float Size = S(FMath::Lerp(Hud.PreviewDotStart, Hud.PreviewDotEnd, static_cast<float>(Index) / DotCount));
		DrawRect(Data.Colors.Ink, Screen.X - Size * 0.5f - Border, Screen.Y - Size * 0.5f - Border, Size + Border * 2.f, Size + Border * 2.f);
		DrawRect(Data.Colors.PreviewDot, Screen.X - Size * 0.5f, Screen.Y - Size * 0.5f, Size, Size);
	}
}

void AYJJHUD::DrawPopups(const AYJJJumper& Jumper)
{
	const FYJJHudTuning& Hud = FYJJData::Get().Hud;
	for (const FYJJPopup& Popup : Jumper.GetPopups())
	{
		const FVector Screen = Project(Popup.WorldLocation, false);
		const float FadeStart = Popup.Lifetime * Hud.PopupFadeFrom;
		const float Alpha = 1.f - FMath::Clamp((Popup.Age - FadeStart) / FMath::Max(Popup.Lifetime - FadeStart, KINDA_SMALL_NUMBER), 0.f, 1.f);
		// 처음에 확 커졌다 줄어드는 펀치
		const float Punch = 1.f + Hud.PopupPunch * FMath::Max(1.f - Popup.Age * Hud.PopupPunchSpeed, 0.f);

		FLinearColor Color = Popup.Color;
		Color.A = Alpha;
		DrawLabel(Popup.Text, Screen.X, Screen.Y, Hud.PopupTextSize * Popup.Scale * Punch, Color, EAlign::Center, Hud.PopupOutline);
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
	const FYJJData& Data = FYJJData::Get();
	const FYJJHudTuning& Hud = Data.Hud;
	const FYJJPalette& Colors = Data.Colors;
	const float Elapsed = Data.Player.LevelStolenBannerTime - Time;
	DrawRect(Colors.BannerDim, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);

	if (UTexture2D* Eagle = BaldEagleTexture.LoadSynchronous())
	{
		const FYJJHudLayoutRow& EagleLayout = Data.Layout(TEXT("BannerEagle"));
		const float Size = S(EagleLayout.W);
		const float EagleX = FMath::Lerp(-Size, Canvas->ClipX + Size, FMath::Clamp(Elapsed / Hud.BannerEagleTime, 0.f, 1.f)) - Size * 0.5f;
		const float EagleY = Canvas->ClipY * EagleLayout.ScreenY + FMath::Sin(Elapsed * Hud.BannerEagleBobSpeed) * S(Hud.BannerEagleBobSize);
		DrawTexture(Eagle, EagleX, EagleY, Size, Size * Hud.BannerEagleAspect, 0.f, 0.f, 1.f, 1.f);
	}

	// 줄마다 (글자 키, 색). 배치는 HudLayout.csv BannerLine1~4
	const TPair<const TCHAR*, const FLinearColor*> Lines[] = {
		{ TEXT("Banner.Line1"), &Colors.HudText },
		{ TEXT("Banner.Line2"), &Colors.HudText },
		{ TEXT("Banner.Line3"), &Colors.Red },
		{ TEXT("Banner.Line4"), &Colors.Yellow },
	};
	const TCHAR* LayoutNames[] = { TEXT("BannerLine1"), TEXT("BannerLine2"), TEXT("BannerLine3"), TEXT("BannerLine4") };
	const float CenterX = Canvas->ClipX * 0.5f;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Lines); ++Index)
	{
		const FYJJHudLayoutRow& Line = Data.Layout(LayoutNames[Index]);
		DrawLabel(FYJJLoc::Get(Lines[Index].Key), CenterX, Canvas->ClipY * Line.ScreenY + S(Line.TextY), Line.TextSize, *Lines[Index].Value, EAlign::Center, Line.Outline);
	}
}
