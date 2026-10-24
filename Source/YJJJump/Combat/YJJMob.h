#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/YJJCombatTypes.h"
#include "YJJMob.generated.h"

class UPaperSpriteComponent;
class UYJJMobData;

// 로프 사이에 떠 있는 몹. 점퍼가 충분히 빠르게 부딪히면 처치된다
UCLASS()
class YJJJUMP_API AYJJMob : public AActor
{
	GENERATED_BODY()

public:
	AYJJMob();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	FYJJMobTarget MakeTarget() const;

	// 피해를 주고 처치되면 true. 처치 시 이펙트를 남기고 사라진다
	bool ApplyHit(float Damage);

	int32 GetMobId() const { return MobId; }
	float GetHealth() const { return Health; }
	const UYJJMobData* GetData() const { return Data; }

	// 스폰 전에 설정 (SpawnActorDeferred)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	TObjectPtr<UYJJMobData> Data;

	// 이동 위상 (같은 종류가 똑같이 움직이지 않도록)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	float MovePhase = 0.f;

private:
	void UpdateVisualScale();

	UPROPERTY(VisibleAnywhere, Category = "Mob")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Mob")
	TObjectPtr<UPaperSpriteComponent> Sprite;

	float Health = 1.f;
	FVector Home = FVector::ZeroVector;
	float Age = 0.f;
	float HitPunch = 0.f;
	float BaseScale = 1.f;
	float Facing = 1.f;
	int32 MobId = INDEX_NONE;
};
