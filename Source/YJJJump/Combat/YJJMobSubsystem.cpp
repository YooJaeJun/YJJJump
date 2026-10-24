#include "Combat/YJJMobSubsystem.h"
#include "Combat/YJJMob.h"

int32 UYJJMobSubsystem::RegisterMob(AYJJMob* Mob)
{
	check(Mob);
	Mobs.Add(Mob);
	return NextMobId++;
}

void UYJJMobSubsystem::UnregisterMob(AYJJMob* Mob)
{
	Mobs.Remove(Mob);
}

void UYJJMobSubsystem::GatherTargets(TArray<FYJJMobTarget>& OutTargets) const
{
	OutTargets.Reset(Mobs.Num());
	for (const AYJJMob* Mob : Mobs)
	{
		if (IsValid(Mob))
		{
			OutTargets.Add(Mob->MakeTarget());
		}
	}
}

AYJJMob* UYJJMobSubsystem::FindMob(int32 MobId) const
{
	for (AYJJMob* Mob : Mobs)
	{
		if (IsValid(Mob) && Mob->GetMobId() == MobId)
		{
			return Mob;
		}
	}
	return nullptr;
}
