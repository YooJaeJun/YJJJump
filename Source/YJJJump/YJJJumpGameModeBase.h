// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "YJJJumpGameModeBase.generated.h"

class UPaperSprite;
class UYJJSpawnTableData;

/**
 * 바닥에 로프 하나를 걸고, 하늘 스포너가 장수풍뎅이 주변 하늘을 몬스터로 채운다.
 */
UCLASS()
class YJJJUMP_API AYJJJumpGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AYJJJumpGameModeBase();

	virtual void StartPlay() override;

	// 레벨에 로프가 없으면 시작 지점 아래에 로프와 하늘 스포너를 만든다
	UPROPERTY(EditAnywhere, Category = "Stage")
	bool bSpawnStage = true;

	// 같은 시드면 같은 하늘 (랭킹 공정성)
	UPROPERTY(EditAnywhere, Category = "Stage")
	int32 StageSeed = 1234;

	// 로프 폭의 절반이자 좌우 벽 위치 (점퍼 Tuning.WallHalfWidth와 맞춘다)
	UPROPERTY(EditAnywhere, Category = "Stage")
	float StageHalfWidth = 900.f;

	UPROPERTY(EditAnywhere, Category = "Stage")
	TSoftObjectPtr<UYJJSpawnTableData> SpawnTable;

	UPROPERTY(EditAnywhere, Category = "Stage")
	TSoftObjectPtr<UPaperSprite> DecorSprite;

private:
	void SpawnStage();
};
