// Copyright Epic Games, Inc. All Rights Reserved.


#include "YJJJumpGameModeBase.h"
#include "Combat/YJJMob.h"
#include "Combat/YJJMobData.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Player/YJJJumper.h"
#include "Player/YJJPlayerController.h"
#include "Rope/YJJRope.h"
#include "UI/YJJHUD.h"
#include "YJJAssetPaths.h"

AYJJJumpGameModeBase::AYJJJumpGameModeBase()
{
	DefaultPawnClass = AYJJJumper::StaticClass();
	PlayerControllerClass = AYJJPlayerController::StaticClass();
	HUDClass = AYJJHUD::StaticClass();

	SpawnTable = TSoftObjectPtr<UYJJSpawnTableData>(FSoftObjectPath(YJJAssetPaths::SkySpawnTable));
}

void AYJJJumpGameModeBase::StartPlay()
{
	if (bSpawnTestTower && !TActorIterator<AYJJRope>(GetWorld()))
	{
		SpawnTestTower();
	}

	Super::StartPlay();
}

void AYJJJumpGameModeBase::SpawnTestTower()
{
	UWorld* World = GetWorld();

	FVector Start = FVector::ZeroVector;
	if (TActorIterator<APlayerStart> It(World); It)
	{
		Start = It->GetActorLocation();
	}

	const UYJJSpawnTableData* Table = SpawnTable.LoadSynchronous();

	// 같은 시드면 항상 같은 탑 (나중에 랭킹 공정성용)
	FRandomStream Random(TestTowerSeed);

	// 첫 로프는 시작 지점 바로 아래, 중앙에 넓게
	float Height = static_cast<float>(Start.Z) - 150.f;
	for (int32 Index = 0; Index < TestRopeCount; ++Index)
	{
		const float Width = Index == 0 ? TestRopeWidthRange.Y : Random.FRandRange(TestRopeWidthRange.X, TestRopeWidthRange.Y);
		const float MaxOffset = FMath::Max(TestTowerHalfWidth - Width * 0.5f, 0.f);
		const float OffsetX = Index == 0 ? 0.f : Random.FRandRange(-MaxOffset, MaxOffset);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.bDeferConstruction = true;

		const FTransform Transform(FVector(Start.X + OffsetX, Start.Y, Height));
		if (AYJJRope* Rope = World->SpawnActor<AYJJRope>(AYJJRope::StaticClass(), Transform, Params))
		{
			Rope->Width = Width;
			Rope->FinishSpawning(Transform);
		}

		const float NextHeight = Height + Random.FRandRange(TestRopeGapRange.X, TestRopeGapRange.Y);
		if (Table && Index >= Table->SafeGapCount && Index < TestRopeCount - 1)
		{
			SpawnMobInGap(Random, *Table, Start, Height, NextHeight);
		}
		Height = NextHeight;
	}
}

void AYJJJumpGameModeBase::SpawnMobInGap(FRandomStream& Random, const UYJJSpawnTableData& Table, const FVector& Start, float GapBottom, float GapTop)
{
	if (Random.FRand() > Table.MobChancePerGap)
	{
		return;
	}

	const FYJJMobSpawnEntry* Entry = Table.PickEntry(Random, GapBottom - static_cast<float>(Start.Z));
	if (!Entry)
	{
		return;
	}

	// 좌우로 다니는 몹은 벽을 넘지 않게
	const UYJJMobData* Data = Entry->Mob;
	const float Travel = Data->Movement == EYJJMobMovement::Patrol ? Data->MoveRange : 0.f;
	const float MaxOffset = FMath::Max(TestTowerHalfWidth - Data->Radius - Travel, 0.f);
	const float Z = (GapBottom + GapTop) * 0.5f;
	const FTransform Transform(FVector(Start.X + Random.FRandRange(-MaxOffset, MaxOffset), Start.Y, Z));

	if (AYJJMob* Mob = GetWorld()->SpawnActorDeferred<AYJJMob>(AYJJMob::StaticClass(), Transform))
	{
		Mob->Data = Entry->Mob;
		Mob->MovePhase = Random.FRandRange(0.f, UE_TWO_PI);
		Mob->FinishSpawning(Transform);
	}
}
