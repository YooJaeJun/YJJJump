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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UPaperSprite> Sprite;

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

// 로프 사이 빈칸에 몹을 배치하는 규칙. 나중에 바이옴 데이터로 확장
UCLASS(BlueprintType)
class YJJJUMP_API UYJJSpawnTableData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TArray<FYJJMobSpawnEntry> Entries;

	// 로프 사이 빈칸 하나에 몹이 생길 확률
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0", ClampMax = "1"))
	float MobChancePerGap = 0.6f;

	// 첫 몇 칸은 몹 없이 연습
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0"))
	int32 SafeGapCount = 2;

	// 높이에 맞는 항목 중 가중치로 하나 고른다
	const FYJJMobSpawnEntry* PickEntry(FRandomStream& Random, float HeightAboveStart) const;
};
