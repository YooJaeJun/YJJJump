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

// 공격 손맛 튜닝 값
USTRUCT(BlueprintType)
struct FYJJCombatTuning
{
	GENERATED_BODY()

	// 이 속도로 부딪히면 기본 공격력 1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float BaseAttackSpeed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float MinAttackPower = 0.2f;

	// 발사할 때의 판정에 따른 공격력 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float PerfectAttackMultiplier = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float GoodAttackMultiplier = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float MissAttackMultiplier = 0.8f;

	// 연속 퍼펙트 1회당 추가 공격력 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float ChainAttackBonus = 0.1f;

	// 처치하면 관통하며 속도를 이만큼 유지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float KillSpeedRetain = 0.85f;

	// 처치 못 하면 이 속도로 튕겨 나온다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float BlockedBounceSpeed = 1100.f;

	// 같은 몹을 연속으로 때릴 수 없는 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float HitCooldown = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float JumperRadius = 40.f;

	// 처치 점수에 곱해지는 연속 퍼펙트 보너스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score")
	float ChainScoreBonus = 0.2f;
};

// 히트 판정에 필요한 몹 정보 (액터와 분리해 서버에서도 같은 계산)
struct FYJJMobTarget
{
	int32 Id = INDEX_NONE;
	FVector2f Position = FVector2f::ZeroVector;	// (X, Z)
	float Radius = 0.f;
	float Health = 0.f;
	int32 ScoreValue = 0;
};

struct FYJJHitResult
{
	bool bHit = false;
	bool bKilled = false;
	float Damage = 0.f;
	int32 ScoreGained = 0;
};
