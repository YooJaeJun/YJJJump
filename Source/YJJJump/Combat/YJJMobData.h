#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/YJJCombatTypes.h"
#include "YJJMobData.generated.h"

class UPaperSprite;

// 몹 한 종류의 정의
UCLASS(BlueprintType)
class YJJJUMP_API UYJJMobData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob", meta = (ClampMin = "0.1"))
	float MaxHealth = 1.f;

	// 히트 판정 반지름 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob", meta = (ClampMin = "1"))
	float Radius = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	int32 ScoreValue = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	int32 XPValue = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UPaperSprite> Sprite;

	// 날갯짓 두 번째 프레임 (Sprite와 번갈아 보여준다)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UPaperSprite> FlapSprite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.02"))
	float FlapInterval = 0.15f;

	// 화면에 보이는 너비 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "1"))
	float SpriteWidth = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	EYJJMobMovement Movement = EYJJMobMovement::Static;

	// 왕복 거리의 절반 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveRange = 200.f;

	// 왕복 1회 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.1"))
	float MovePeriod = 3.f;
};

USTRUCT(BlueprintType)
struct FYJJMobSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TObjectPtr<UYJJMobData> Mob;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0"))
	float Weight = 1.f;

	// 시작 지점에서 이 높이(cm) 이상부터 등장
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	float MinHeight = 0.f;
};

// 하늘을 높이 칸(Cell)으로 나눠 몬스터를 채우는 규칙. 나중에 바이옴 데이터로 확장
UCLASS(BlueprintType)
class YJJJUMP_API UYJJSpawnTableData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TArray<FYJJMobSpawnEntry> Entries;

	// 높이 칸 하나의 크기 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "50"))
	float CellHeight = 450.f;

	// 칸 하나에 몬스터가 생길 확률, 그리고 두 번째 몬스터가 생길 확률
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0", ClampMax = "1"))
	float MobChancePerCell = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0", ClampMax = "1"))
	float SecondMobChance = 0.25f;

	// 로프 바로 위는 비워 둔다 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0"))
	float MinSpawnHeight = 500.f;

	// 1m 올라갈 때마다 몬스터 체력/보상 증가율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0"))
	float ScalePerMeter = 0.02f;

	// 높이에 맞는 항목 중 가중치로 하나 고른다
	const FYJJMobSpawnEntry* PickEntry(FRandomStream& Random, float HeightAboveStart) const;
};
