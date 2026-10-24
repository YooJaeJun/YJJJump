// Copyright Epic Games, Inc. All Rights Reserved.


#include "YJJJumpGameModeBase.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Player/YJJJumper.h"
#include "Player/YJJPlayerController.h"
#include "Rope/YJJRope.h"

AYJJJumpGameModeBase::AYJJJumpGameModeBase()
{
	DefaultPawnClass = AYJJJumper::StaticClass();
	PlayerControllerClass = AYJJPlayerController::StaticClass();
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

		Height += Random.FRandRange(TestRopeGapRange.X, TestRopeGapRange.Y);
	}
}
