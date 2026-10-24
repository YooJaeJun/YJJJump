#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "YJJHUD.generated.h"

class AYJJJumper;
class UTexture2D;
class UFont;
struct FCompositeFont;

// 게임 HUD. 9-슬라이스 패널 + 픽셀 아이콘 + 굵은 외곽선 글자로 그린다 (Canvas, 위젯 에셋 없음)
UCLASS()
class YJJJUMP_API AYJJHUD : public AHUD
{
	GENERATED_BODY()

public:
	AYJJHUD();

	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSoftObjectPtr<UTexture2D> PanelTexture;

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSoftObjectPtr<UTexture2D> CoinTexture;

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSoftObjectPtr<UTexture2D> SkullTexture;

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSoftObjectPtr<UTexture2D> ArrowTexture;

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSoftObjectPtr<UTexture2D> FlameTexture;

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSoftObjectPtr<UTexture2D> BaldEagleTexture;

	// HUD 글꼴 TTF (Content 기준 상대 경로). 배민 주아체, SIL OFL 1.1 (같은 폴더 OFL.txt)
	UPROPERTY(EditAnywhere, Category = "HUD")
	FString GameFontFile = TEXT("YJJJump/Fonts/Jua-Regular.ttf");

private:
	TSharedPtr<const FCompositeFont> GetGameFont(const UFont* FallbackFont);
	TSharedPtr<const FCompositeFont> GameFont;

	enum class EAlign : uint8 { Left, Center };

	void DrawSpeedLines(const AYJJJumper& Jumper);
	void DrawAttackRange(const AYJJJumper& Jumper);
	void DrawLaunchPreview(const AYJJJumper& Jumper);
	void DrawPopups(const AYJJJumper& Jumper);
	void DrawLevelBadge(const AYJJJumper& Jumper);
	void DrawHeight(const AYJJJumper& Jumper);
	void DrawScoreChips(const AYJJJumper& Jumper);
	void DrawAttackChip(const AYJJJumper& Jumper);
	void DrawSpeedometer(const AYJJJumper& Jumper);
	void DrawHint(const AYJJJumper& Jumper);
	void DrawLevelStolenBanner(const AYJJJumper& Jumper);

	// 굵은 글꼴 + 외곽선. Size는 1080p 기준 포인트
	void DrawLabel(const FString& Text, float X, float Y, float Size, const FLinearColor& Color, EAlign Align = EAlign::Left, float Outline = 3.f, bool bVerticalCenter = true);
	// 둥근 패널(9-슬라이스)을 Color로 틴트해서 그린다
	void DrawPanel(float X, float Y, float W, float H, const FLinearColor& Color);
	void DrawIcon(const TSoftObjectPtr<UTexture2D>& Icon, float X, float Y, float Size);

	float S(float Value) const { return Value * UIScale; }

	float UIScale = 1.f;
	float SpeedLineSeed = 0.f;
};
