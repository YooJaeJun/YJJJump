#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJSkySpawner.generated.h"

class AYJJDecor;
class AYJJMob;
class UPaperSprite;
class UYJJSpawnTableData;

// 하늘을 높이 칸으로 나눠, 장수풍뎅이 근처 칸에만 몬스터/장식 구름을 깔고 멀어지면 지운다.
// 칸 내용은 (시드, 칸 번호, 세대)로 정해져서 같은 시드면 같은 하늘이 나온다.
// 칸이 화면 밖으로 나갔다가 다시 들어오면 세대가 바뀌어 몬스터가 새로 채워진다.
UCLASS()
class YJJJUMP_API AYJJSkySpawner : public AActor
{
	GENERATED_BODY()

public:
	AYJJSkySpawner();

	virtual void Tick(float DeltaSeconds) override;

	// 스폰 전에 설정
	UPROPERTY(EditAnywhere, Category = "Sky")
	TObjectPtr<UYJJSpawnTableData> SpawnTable;

	UPROPERTY(EditAnywhere, Category = "Sky")
	TObjectPtr<UPaperSprite> DecorSprite;

	UPROPERTY(EditAnywhere, Category = "Sky")
	int32 Seed = 1234;

	// 로프 높이 (칸 0의 바닥)
	UPROPERTY(EditAnywhere, Category = "Sky")
	float BaseHeight = 0.f;

	UPROPERTY(EditAnywhere, Category = "Sky")
	float HalfWidth = 900.f;

	// 장수풍뎅이 위아래로 이만큼은 항상 채워 둔다 (cm). 빠를수록 진행 방향으로 더 멀리
	UPROPERTY(EditAnywhere, Category = "Sky")
	float SpawnBelow = 2500.f;

	UPROPERTY(EditAnywhere, Category = "Sky")
	float SpawnAbove = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Sky")
	float LookAheadTime = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Sky")
	float DespawnMargin = 2500.f;

	// 한 프레임에 새로 채우는 칸 수 제한 (초고속일 때 렉 방지)
	UPROPERTY(EditAnywhere, Category = "Sky")
	int32 MaxCellsPerTick = 8;

	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0", ClampMax = "1"))
	float DecorChancePerCell = 0.45f;

	// 몬스터가 다 잡힌 칸은 이 시간(초)이 지나고 장수풍뎅이가 이만큼(cm) 떨어져 있으면 다시 채운다
	UPROPERTY(EditAnywhere, Category = "Sky")
	float RespawnDelay = 4.f;

	UPROPERTY(EditAnywhere, Category = "Sky")
	float RespawnMinDistance = 1200.f;

private:
	struct FCell
	{
		TArray<TWeakObjectPtr<AYJJMob>> Mobs;
		TArray<TWeakObjectPtr<AYJJDecor>> Decor;
		double EmptySince = -1.0;
	};

	void ActivateCell(int32 CellIndex, bool bRespawnMobsOnly = false);
	TArray<int32> ToRefill;
	void DeactivateCell(int32 CellIndex);
	float GetCellHeight() const;

	TMap<int32, FCell> ActiveCells;
	TMap<int32, int32> CellGenerations;
};
