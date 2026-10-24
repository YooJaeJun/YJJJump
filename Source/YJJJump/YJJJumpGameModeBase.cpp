// Copyright Epic Games, Inc. All Rights Reserved.


#include "YJJJumpGameModeBase.h"
#include "Audio/YJJAudio.h"
#include "Combat/YJJMobData.h"
#include "Combat/YJJSkySpawner.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "PaperSprite.h"
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
	DecorSprite = TSoftObjectPtr<UPaperSprite>(FSoftObjectPath(YJJAssetPaths::CloudDecorSprite));
}

void AYJJJumpGameModeBase::StartPlay()
{
	if (bSpawnStage && !TActorIterator<AYJJRope>(GetWorld()))
	{
		SpawnStage();
	}

	Super::StartPlay();

	YJJAudio::StartBgm(this);
}

void AYJJJumpGameModeBase::SpawnStage()
{
	UWorld* World = GetWorld();

	FVector Start = FVector::ZeroVector;
	if (TActorIterator<APlayerStart> It(World); It)
	{
		Start = It->GetActorLocation();
	}

	// 로프는 딱 하나. 벽에서 벽까지 걸려 있어서 어디로 떨어져도 받아 준다
	const FTransform RopeTransform(FVector(Start.X, Start.Y, Start.Z - 150.0));
	if (AYJJRope* Rope = World->SpawnActorDeferred<AYJJRope>(AYJJRope::StaticClass(), RopeTransform))
	{
		Rope->Width = StageHalfWidth * 2.f;
		Rope->FinishSpawning(RopeTransform);
	}

	if (AYJJSkySpawner* Sky = World->SpawnActorDeferred<AYJJSkySpawner>(AYJJSkySpawner::StaticClass(), RopeTransform))
	{
		Sky->SpawnTable = SpawnTable.LoadSynchronous();
		Sky->DecorSprite = DecorSprite.LoadSynchronous();
		Sky->Seed = StageSeed;
		Sky->BaseHeight = static_cast<float>(RopeTransform.GetLocation().Z);
		Sky->HalfWidth = StageHalfWidth;
		Sky->FinishSpawning(RopeTransform);
	}
}
