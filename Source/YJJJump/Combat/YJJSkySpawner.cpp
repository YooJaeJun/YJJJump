#include "Combat/YJJSkySpawner.h"
#include "Combat/YJJMob.h"
#include "Data/YJJData.h"
#include "Player/YJJJumper.h"
#include "Visual/YJJDecor.h"
#include "YJJUnits.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AYJJSkySpawner::AYJJSkySpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AYJJSkySpawner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const AYJJJumper* Jumper = PC ? Cast<AYJJJumper>(PC->GetPawn()) : nullptr;
	if (!Jumper)
	{
		return;
	}

	const FYJJSkyTuning& Sky = FYJJData::Get().Sky;
	const FYJJJumperState& State = Jumper->GetSimState();
	const float Z = State.Position.Y;
	const float Up = FMath::Max(State.Velocity.Y, 0.f) * Sky.LookAheadTime;
	const float Down = FMath::Max(-State.Velocity.Y, 0.f) * Sky.LookAheadTime;
	const float CellHeight = Sky.CellHeight;

	auto CellAt = [this, CellHeight](float Height) { return FMath::FloorToInt((Height - BaseHeight) / CellHeight); };

	// 멀어진 칸 정리
	const int32 KeepMin = CellAt(Z - Sky.SpawnBelow - Down - Sky.DespawnMargin);
	const int32 KeepMax = CellAt(Z + Sky.SpawnAbove + Up + Sky.DespawnMargin);
	TArray<int32> ToRemove;
	for (const TPair<int32, FCell>& Pair : ActiveCells)
	{
		if (Pair.Key < KeepMin || Pair.Key > KeepMax)
		{
			ToRemove.Add(Pair.Key);
		}
	}
	for (int32 CellIndex : ToRemove)
	{
		DeactivateCell(CellIndex);
	}

	// 다 잡혀서 한동안 비어 있던 칸은, 장수풍뎅이가 근처에 없을 때 다시 채운다
	const double Now = GetWorld()->GetTimeSeconds();
	for (TPair<int32, FCell>& Pair : ActiveCells)
	{
		FCell& Cell = Pair.Value;
		Cell.Mobs.RemoveAll([](const TWeakObjectPtr<AYJJMob>& Mob) { return !Mob.IsValid(); });
		if (!Cell.Mobs.IsEmpty())
		{
			continue;
		}

		const float CellMid = BaseHeight + (Pair.Key + 0.5f) * CellHeight;
		if (Cell.EmptySince < 0.0)
		{
			Cell.EmptySince = Now;
		}
		else if (Now - Cell.EmptySince >= Sky.RespawnDelay && FMath::Abs(CellMid - Z) > Sky.RespawnMinDistance)
		{
			ToRefill.Add(Pair.Key);
		}
	}
	for (int32 CellIndex : ToRefill)
	{
		ActivateCell(CellIndex, true);
	}
	ToRefill.Reset();

	// 가까운 칸부터 채운다
	const int32 Here = CellAt(Z);
	const int32 FillMin = FMath::Max(CellAt(Z - Sky.SpawnBelow - Down), 0);
	const int32 FillMax = CellAt(Z + Sky.SpawnAbove + Up);
	TArray<int32> Missing;
	for (int32 CellIndex = FillMin; CellIndex <= FillMax; ++CellIndex)
	{
		if (!ActiveCells.Contains(CellIndex))
		{
			Missing.Add(CellIndex);
		}
	}
	Missing.Sort([Here](int32 A, int32 B) { return FMath::Abs(A - Here) < FMath::Abs(B - Here); });
	for (int32 Index = 0; Index < Missing.Num() && Index < Sky.MaxCellsPerTick; ++Index)
	{
		ActivateCell(Missing[Index]);
	}
}

void AYJJSkySpawner::ActivateCell(int32 CellIndex, bool bRespawnMobsOnly)
{
	FCell& Cell = bRespawnMobsOnly ? ActiveCells.FindChecked(CellIndex) : ActiveCells.Add(CellIndex);
	Cell.EmptySince = -1.0;
	const int32 Generation = CellGenerations.FindOrAdd(CellIndex)++;
	FRandomStream Random(static_cast<int32>(HashCombineFast(HashCombineFast(GetTypeHash(Seed), GetTypeHash(CellIndex)), GetTypeHash(Generation))));

	const FYJJData& Data = FYJJData::Get();
	const FYJJSkyTuning& Sky = Data.Sky;
	const float CellHeight = Sky.CellHeight;
	const float Bottom = BaseHeight + CellIndex * CellHeight;
	const FVector Origin = GetActorLocation();

	// 장식 구름
	if (!bRespawnMobsOnly && DecorSprite && Random.FRand() < Sky.DecorChancePerCell)
	{
		const float Spread = HalfWidth * Sky.DecorSpreadRatio;
		const FTransform Transform(FVector(Origin.X + Random.FRandRange(-Spread, Spread), Origin.Y + Sky.DecorDepth - Random.FRandRange(0.f, Sky.DecorDepthJitter), Bottom + Random.FRandRange(0.f, CellHeight)));
		if (AYJJDecor* Decor = GetWorld()->SpawnActorDeferred<AYJJDecor>(AYJJDecor::StaticClass(), Transform))
		{
			Decor->Sprite = DecorSprite;
			Decor->Width = Random.FRandRange(Sky.DecorWidthMin, Sky.DecorWidthMax);
			Decor->bFlip = Random.FRand() < Sky.DecorFlipChance;
			Decor->Tint = Data.Colors.DecorTint;
			Decor->FinishSpawning(Transform);
			Cell.Decor.Add(Decor);
		}
	}

	if (Bottom - BaseHeight < Sky.MinSpawnHeight)
	{
		return;
	}

	// 높이 올라갈수록 강하고 보상 큰 몬스터
	const float HeightAboveBase = Bottom + CellHeight * 0.5f - BaseHeight;
	const float StatMultiplier = 1.f + Sky.ScalePerMeter * HeightAboveBase / YJJUnits::CmPerMeter;
	int32 Count = Random.FRand() < Sky.MobChancePerCell ? 1 : 0;
	if (Count > 0 && Random.FRand() < Sky.SecondMobChance)
	{
		Count++;
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FYJJSpawnRow* Entry = Data.PickSpawn(Biome, Random, HeightAboveBase);
		const FYJJMobRow* Mob = Entry ? Data.FindMob(Entry->Mob) : nullptr;
		if (!Mob)
		{
			break;
		}

		const float Travel = Mob->Movement == EYJJMobMovement::Patrol ? FMath::Min(Mob->MoveRange, HalfWidth * Sky.MobPatrolMaxRatio) : 0.f;
		const float MaxOffset = FMath::Max(HalfWidth - Mob->Radius - Travel, 0.f);
		const FTransform Transform(FVector(Origin.X + Random.FRandRange(-MaxOffset, MaxOffset), Origin.Y + Random.FRandRange(-Sky.MobDepthJitter, 0.f),
			Bottom + Random.FRandRange(CellHeight * Sky.MobHeightMin, CellHeight * Sky.MobHeightMax)));

		if (AYJJMob* Spawned = GetWorld()->SpawnActorDeferred<AYJJMob>(AYJJMob::StaticClass(), Transform))
		{
			Spawned->MobType = Entry->Mob;
			Spawned->MovePhase = Random.FRandRange(0.f, UE_TWO_PI);
			Spawned->StatMultiplier = StatMultiplier;
			Spawned->MaxMoveRange = Travel;
			Spawned->FinishSpawning(Transform);
			Cell.Mobs.Add(Spawned);
		}
	}
}

void AYJJSkySpawner::DeactivateCell(int32 CellIndex)
{
	FCell Cell;
	if (!ActiveCells.RemoveAndCopyValue(CellIndex, Cell))
	{
		return;
	}

	for (const TWeakObjectPtr<AYJJMob>& Mob : Cell.Mobs)
	{
		if (Mob.IsValid())
		{
			Mob->Destroy();
		}
	}
	for (const TWeakObjectPtr<AYJJDecor>& Decor : Cell.Decor)
	{
		if (Decor.IsValid())
		{
			Decor->Destroy();
		}
	}
}
