#include "Combat/YJJMobData.h"

const FYJJMobSpawnEntry* UYJJSpawnTableData::PickEntry(FRandomStream& Random, float HeightAboveStart) const
{
	float TotalWeight = 0.f;
	for (const FYJJMobSpawnEntry& Entry : Entries)
	{
		if (Entry.Mob && HeightAboveStart >= Entry.MinHeight)
		{
			TotalWeight += Entry.Weight;
		}
	}
	if (TotalWeight <= 0.f)
	{
		return nullptr;
	}

	float Roll = Random.FRandRange(0.f, TotalWeight);
	for (const FYJJMobSpawnEntry& Entry : Entries)
	{
		if (Entry.Mob && HeightAboveStart >= Entry.MinHeight)
		{
			Roll -= Entry.Weight;
			if (Roll <= 0.f)
			{
				return &Entry;
			}
		}
	}
	return nullptr;
}
