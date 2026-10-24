#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "YJJHUD.generated.h"

class AYJJJumper;
class UFont;
class UTexture2D;

// 프로토타입 HUD. 에셋 없이 Canvas로 높이/속도/레벨/공격력/판정 글자/사거리/속도선/튜토리얼을 그린다
UCLASS()
class YJJJUMP_API AYJJHUD : public AHUD
{
	GENERATED_BODY()

public:
	AYJJHUD();

	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSoftObjectPtr<UTexture2D> BaldEagleTexture;

private:
	void DrawSpeedLines(const AYJJJumper& Jumper);
	void DrawAttackRange(const AYJJJumper& Jumper);
	void DrawStats(const AYJJJumper& Jumper, float UIScale);
	void DrawLevel(const AYJJJumper& Jumper, float UIScale);
	void DrawAttackGauge(const AYJJJumper& Jumper, float UIScale);
	void DrawLaunchPreview(const AYJJJumper& Jumper);
	void DrawPopups(const AYJJJumper& Jumper, float UIScale);
	void DrawTutorial(const AYJJJumper& Jumper, float UIScale);
	void DrawLevelStolenBanner(const AYJJJumper& Jumper, float UIScale);

	void DrawOutlinedText(const FString& Text, const FLinearColor& Color, float X, float Y, float Scale, bool bCenter);
	UFont* GetFont() const;
	static FLinearColor LevelColor();

	float SpeedLineSeed = 0.f;
};
