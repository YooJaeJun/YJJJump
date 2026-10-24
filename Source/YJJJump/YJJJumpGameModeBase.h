// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "YJJJumpGameModeBase.generated.h"

/**
 *
 */
UCLASS()
class YJJJUMP_API AYJJJumpGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AYJJJumpGameModeBase();

	virtual void StartPlay() override;

	// 레벨에 로프가 하나도 없으면 테스트용 탑을 자동 생성
	UPROPERTY(EditAnywhere, Category = "Prototype")
	bool bSpawnTestTower = true;

	UPROPERTY(EditAnywhere, Category = "Prototype")
	int32 TestTowerSeed = 1234;

	UPROPERTY(EditAnywhere, Category = "Prototype", meta = (ClampMin = "1"))
	int32 TestRopeCount = 40;

	UPROPERTY(EditAnywhere, Category = "Prototype")
	FVector2f TestRopeGapRange = FVector2f(320.f, 520.f);

	UPROPERTY(EditAnywhere, Category = "Prototype")
	FVector2f TestRopeWidthRange = FVector2f(300.f, 520.f);

	// 탑 중앙 기준 로프가 놓일 수 있는 좌우 반폭 (점퍼 벽과 맞춘다)
	UPROPERTY(EditAnywhere, Category = "Prototype")
	float TestTowerHalfWidth = 900.f;

	// 로프 사이 빈칸에 몹을 배치하는 규칙
	UPROPERTY(EditAnywhere, Category = "Prototype")
	TSoftObjectPtr<class UYJJSpawnTableData> SpawnTable;

private:
	void SpawnTestTower();
	void SpawnMobInGap(FRandomStream& Random, const UYJJSpawnTableData& Table, const FVector& Start, float GapBottom, float GapTop);
};
