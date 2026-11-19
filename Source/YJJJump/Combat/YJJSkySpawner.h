#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJSkySpawner.generated.h"

class AYJJDecor;
class AYJJMob;
class UPaperSprite;

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

	// 스폰 전에 설정 (게임모드). 나머지 수치는 Tuning.csv Sky.*
	UPROPERTY(EditAnywhere, Category = "Sky")
	TObjectPtr<UPaperSprite> DecorSprite;

	UPROPERTY(EditAnywhere, Category = "Sky")
	int32 Seed = 0;

	// 로프 높이 (칸 0의 바닥)
	UPROPERTY(EditAnywhere, Category = "Sky")
	float BaseHeight = 0.f;

	UPROPERTY(EditAnywhere, Category = "Sky")
	float HalfWidth = 0.f;

	// Spawns.csv Biome
	UPROPERTY(EditAnywhere, Category = "Sky")
	FName Biome;

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

	TMap<int32, FCell> ActiveCells;
	TMap<int32, int32> CellGenerations;
};
