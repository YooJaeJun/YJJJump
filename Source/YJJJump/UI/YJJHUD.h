#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "YJJHUD.generated.h"

class AYJJJumper;
class UFont;

// 프로토타입 HUD. 에셋 없이 Canvas로 높이/점수/공격력/판정 글자/발사 궤적을 그린다
UCLASS()
class YJJJUMP_API AYJJHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawStats(const AYJJJumper& Jumper, float UIScale);
	void DrawAttackGauge(const AYJJJumper& Jumper, float UIScale);
	void DrawLaunchPreview(const AYJJJumper& Jumper);
	void DrawPopups(const AYJJJumper& Jumper, float UIScale);
	void DrawHint(const AYJJJumper& Jumper, float UIScale);

	void DrawOutlinedText(const FString& Text, const FLinearColor& Color, float X, float Y, float Scale, bool bCenter);
	UFont* GetFont() const;
};
