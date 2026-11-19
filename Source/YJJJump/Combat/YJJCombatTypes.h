#pragma once

#include "CoreMinimal.h"
#include "YJJCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EYJJMobMovement : uint8
{
	Static,		// 제자리에서 살짝 둥실
	Patrol,		// 좌우 왕복
	Hover,		// 위아래 왕복
};

// 공격 손맛 튜닝 값. 공중에서 몬스터를 탭하면 뿔 돌진으로 들이받는다
USTRUCT(BlueprintType)
struct FYJJCombatTuning
{
	GENERATED_BODY()

	// 탭으로 돌진할 수 있는 거리 (cm). 레벨마다 늘어난다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float BaseAttackRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float AttackRangePerLevel = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float MaxAttackRange = 0.f;

	// 몬스터 반지름 밖으로 이만큼 빗나가게 탭해도 인정 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float TapTolerance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float DashDuration = 0.f;

	// 돌진이 끝난 뒤 다음 돌진까지 쉬는 시간 (연타로 정신없지 않게)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float DashCooldown = 0.f;

	// 공격력 = (1 + 레벨당 증가 * (Lv-1)) * 판정 배율 * 연속 퍼펙트 * 속도 배율^지수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float PowerPerLevel = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float PerfectAttackMultiplier = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float GoodAttackMultiplier = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float MissAttackMultiplier = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float ChainAttackBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float SpeedPowerExponent = 0.f;

	// 튜토리얼의 Lv 999Gp 장수풍뎅이는 뭐든 한 방
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float TutorialPower = 0.f;

	// 튜토리얼 동안 사거리 계산에 쓰는 레벨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	int32 TutorialLevel = 0;

	// 다음 레벨까지 경험치 = XPBase * Lv^XPExponent
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	float XPBase = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	float XPExponent = 0.f;

	// 처치 점수에 곱해지는 연속 퍼펙트 보너스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score")
	float ChainScoreBonus = 0.f;
};

// 판정에 필요한 몬스터 정보 (액터와 분리해 서버에서도 같은 계산)
struct FYJJMobTarget
{
	int32 Id = INDEX_NONE;
	FVector2f Position = FVector2f::ZeroVector;	// (X, Z)
	float Radius = 0.f;
	float Health = 0.f;
	int32 ScoreValue = 0;
	int32 XPValue = 0;
};

struct FYJJHitResult
{
	bool bHit = false;
	bool bKilled = false;
	float Damage = 0.f;
	int32 ScoreGained = 0;
	int32 XPGained = 0;
	int32 LevelsGained = 0;
};
