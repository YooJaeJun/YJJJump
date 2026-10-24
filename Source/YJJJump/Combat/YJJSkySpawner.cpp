#include "Combat/YJJSkySpawner.h"
#include "Combat/YJJMob.h"
#include "Combat/YJJMobData.h"
#include "Player/YJJJumper.h"
#include "Visual/YJJDecor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	// 화면 깊이(Y): 장식은 몬스터/로프보다 뒤
	constexpr float DecorDepth = -400.f;
}

AYJJSkySpawner::AYJJSkySpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

float AYJJSkySpawner::GetCellHeight() const
{
	return SpawnTable ? SpawnTable->CellHeight : 450.f;
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

	const FYJJJumperState& State = Jumper->GetSimState();
	const float Z = State.Position.Y;
	const float Up = FMath::Max(State.Velocity.Y, 0.f) * LookAheadTime;
	const float Down = FMath::Max(-State.Velocity.Y, 0.f) * LookAheadTime;
	const float CellHeight = GetCellHeight();

	auto CellAt = [this, CellHeight](float Height) { return FMath::FloorToInt((Height - BaseHeight) / CellHeight); };

	// 멀어진 칸 정리
	const int32 KeepMin = CellAt(Z - SpawnBelow - Down - DespawnMargin);
	const int32 KeepMax = CellAt(Z + SpawnAbove + Up + DespawnMargin);
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
		else if (Now - Cell.EmptySince >= RespawnDelay && FMath::Abs(CellMid - Z) > RespawnMinDistance)
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
	const int32 FillMin = FMath::Max(CellAt(Z - SpawnBelow - Down), 0);
	const int32 FillMax = CellAt(Z + SpawnAbove + Up);
	TArray<int32> Missing;
	for (int32 CellIndex = FillMin; CellIndex <= FillMax; ++CellIndex)
	{
		if (!ActiveCells.Contains(CellIndex))
		{
			Missing.Add(CellIndex);
		}
	}
	Missing.Sort([Here](int32 A, int32 B) { return FMath::Abs(A - Here) < FMath::Abs(B - Here); });
	for (int32 Index = 0; Index < Missing.Num() && Index < MaxCellsPerTick; ++Index)
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

	const float CellHeight = GetCellHeight();
	const float Bottom = BaseHeight + CellIndex * CellHeight;
	const FVector Origin = GetActorLocation();

	// 장식 구름
	if (!bRespawnMobsOnly && DecorSprite && Random.FRand() < DecorChancePerCell)
	{
		const FTransform Transform(FVector(Origin.X + Random.FRandRange(-HalfWidth * 1.6f, HalfWidth * 1.6f), Origin.Y + DecorDepth - Random.FRandRange(0.f, 400.f), Bottom + Random.FRandRange(0.f, CellHeight)));
		if (AYJJDecor* Decor = GetWorld()->SpawnActorDeferred<AYJJDecor>(AYJJDecor::StaticClass(), Transform))
		{
			Decor->Sprite = DecorSprite;
			Decor->Width = Random.FRandRange(260.f, 520.f);
			Decor->bFlip = Random.FRand() < 0.5f;
			Decor->Tint = FLinearColor(1.f, 1.f, 1.f);
			Decor->FinishSpawning(Transform);
			Cell.Decor.Add(Decor);
		}
	}

	if (!SpawnTable || Bottom - BaseHeight < SpawnTable->MinSpawnHeight)
	{
		return;
	}

	// 높이 올라갈수록 강하고 보상 큰 몬스터
	const float HeightAboveBase = Bottom + CellHeight * 0.5f - BaseHeight;
	const float StatMultiplier = 1.f + SpawnTable->ScalePerMeter * HeightAboveBase / 100.f;
	int32 Count = Random.FRand() < SpawnTable->MobChancePerCell ? 1 : 0;
	if (Count > 0 && Random.FRand() < SpawnTable->SecondMobChance)
	{
		Count++;
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FYJJMobSpawnEntry* Entry = SpawnTable->PickEntry(Random, HeightAboveBase);
		if (!Entry)
		{
			break;
		}

		const UYJJMobData* Data = Entry->Mob;
		const float Travel = Data->Movement == EYJJMobMovement::Patrol ? FMath::Min(Data->MoveRange, HalfWidth * 0.5f) : 0.f;
		const float MaxOffset = FMath::Max(HalfWidth - Data->Radius - Travel, 0.f);
		const FTransform Transform(FVector(Origin.X + Random.FRandRange(-MaxOffset, MaxOffset), Origin.Y + Random.FRandRange(-40.f, 0.f), Bottom + Random.FRandRange(CellHeight * 0.2f, CellHeight * 0.8f)));

		if (AYJJMob* Mob = GetWorld()->SpawnActorDeferred<AYJJMob>(AYJJMob::StaticClass(), Transform))
		{
			Mob->Data = Entry->Mob;
			Mob->MovePhase = Random.FRandRange(0.f, UE_TWO_PI);
			Mob->StatMultiplier = StatMultiplier;
			Mob->MaxMoveRange = Travel;
			Mob->FinishSpawning(Transform);
			Cell.Mobs.Add(Mob);
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
