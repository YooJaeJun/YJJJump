// Copyright Epic Games, Inc. All Rights Reserved.


#include "YJJJumpGameModeBase.h"
#include "Audio/YJJAudio.h"
#include "Combat/YJJSkySpawner.h"
#include "Data/YJJData.h"
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
	const FYJJData& Data = FYJJData::Get();
	const float HalfWidth = Data.Rope.WallHalfWidth;

	FVector Start = FVector::ZeroVector;
	if (TActorIterator<APlayerStart> It(World); It)
	{
		Start = It->GetActorLocation();
	}

	// 로프는 딱 하나. 벽에서 벽까지 걸려 있어서 어디로 떨어져도 받아 준다
	const FTransform RopeTransform(FVector(Start.X, Start.Y, Start.Z - Data.Stage.RopeDrop));
	if (AYJJRope* Rope = World->SpawnActorDeferred<AYJJRope>(AYJJRope::StaticClass(), RopeTransform))
	{
		Rope->Width = HalfWidth * 2.f;
		Rope->FinishSpawning(RopeTransform);
	}

	if (AYJJSkySpawner* Sky = World->SpawnActorDeferred<AYJJSkySpawner>(AYJJSkySpawner::StaticClass(), RopeTransform))
	{
		Sky->DecorSprite = DecorSprite.LoadSynchronous();
		Sky->Seed = Data.Stage.Seed;
		Sky->BaseHeight = static_cast<float>(RopeTransform.GetLocation().Z);
		Sky->HalfWidth = HalfWidth;
		Sky->Biome = FYJJData::SkyBiome;
		Sky->FinishSpawning(RopeTransform);
	}
}
