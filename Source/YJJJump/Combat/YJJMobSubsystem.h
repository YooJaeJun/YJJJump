#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/YJJCombatTypes.h"
#include "YJJMobSubsystem.generated.h"

class AYJJMob;

// 월드에 살아 있는 몹 목록
UCLASS()
class YJJJUMP_API UYJJMobSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	int32 RegisterMob(AYJJMob* Mob);
	void UnregisterMob(AYJJMob* Mob);

	void GatherTargets(TArray<FYJJMobTarget>& OutTargets) const;
	AYJJMob* FindMob(int32 MobId) const;
	int32 Num() const { return Mobs.Num(); }

private:
	UPROPERTY()
	TArray<TObjectPtr<AYJJMob>> Mobs;

	int32 NextMobId = 0;
};
