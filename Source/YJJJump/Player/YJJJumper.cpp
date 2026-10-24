#include "Player/YJJJumper.h"
#include "Audio/YJJAudio.h"
#include "Combat/YJJCombat.h"
#include "Combat/YJJMob.h"
#include "Combat/YJJMobSubsystem.h"
#include "Rope/YJJRope.h"
#include "Rope/YJJRopeSimulation.h"
#include "Rope/YJJRopeSubsystem.h"
#include "Visual/YJJHitEffect.h"
#include "Visual/YJJSpriteFx.h"
#include "Visual/YJJSpriteUtils.h"
#include "YJJAssetPaths.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogYJJJumper, Log, All);

static TAutoConsoleVariable<int32> CVarAutoPlay(
	TEXT("yjj.AutoPlay"), 0,
	TEXT("1이면 봇이 드래그 발사, 몬스터 탭, 퍼펙트 타이밍을 대신 입력한다 (스모크 테스트용)"));

static TAutoConsoleVariable<float> CVarAutoPlayDuration(
	TEXT("yjj.AutoPlayDuration"), 0.f,
	TEXT("0보다 크면 오토플레이를 이 시간(초) 동안 하고 스크린샷을 남긴 뒤 게임을 종료한다"));

// 튜토리얼은 기획 보류 중이라 기본은 꺼져 있고 Lv 1부터 시작한다
static TAutoConsoleVariable<int32> CVarTutorial(
	TEXT("yjj.Tutorial"), 0,
	TEXT("1이면 Lv 999Gp 튜토리얼부터 시작 (보류 중인 기능)"));

namespace
{
	// 화면 깊이(Y) 순서. 카메라는 +Y에서 -Y를 본다
	constexpr float BodyDepth = 10.f;
	constexpr float EffectDepth = 20.f;
	constexpr float BackgroundDepth = -2000.f;
	constexpr float CameraDistance = 3000.f;

	const FLinearColor PerfectColor(1.f, 0.85f, 0.1f);
	const FLinearColor GoodColor(0.3f, 1.f, 0.4f);
	const FLinearColor MissColor(1.f, 0.35f, 0.3f);
	const FLinearColor LevelUpColor(0.4f, 1.f, 1.f);

	constexpr int32 TutorialKillsNeeded = 3;
}

AYJJJumper::AYJJJumper()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Body = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 옆(+Y)에서 -Y를 보는 직교 카메라. X는 탑 중앙에 고정하고 Z만 따라간다.
	// 초고속에선 한 프레임에 수십 m를 움직이므로 스프링암 없이 매 프레임 직접 위치를 잡는다
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->SetUsingAbsoluteLocation(true);
	Camera->SetUsingAbsoluteRotation(true);
	Camera->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	Camera->SetOrthoWidth(CameraViewHeight);

	// 배경은 카메라 자식으로 붙여 화면에 고정 (카메라 앞쪽 = 로컬 +X, 스프라이트 면이 카메라를 보도록 Yaw 90)
	Background = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Background"));
	Background->SetupAttachment(Camera);
	Background->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Background->SetRelativeLocation(FVector(CameraDistance - BackgroundDepth, 0.f, 0.f));
	Background->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));

	// 돌진 잔상 (반투명 돌진 포즈 몇 장)
	for (int32 Index = 0; Index < 4; ++Index)
	{
		UPaperSpriteComponent* Ghost = CreateDefaultSubobject<UPaperSpriteComponent>(*FString::Printf(TEXT("DashGhost%d"), Index));
		Ghost->SetupAttachment(Root);
		Ghost->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ghost->SetUsingAbsoluteLocation(true);
		Ghost->SetUsingAbsoluteRotation(true);
		Ghost->SetUsingAbsoluteScale(true);
		Ghost->SetVisibility(false);
		Ghosts.Add(Ghost);
	}

	BodySprite = TSoftObjectPtr<UPaperSprite>(FSoftObjectPath(YJJAssetPaths::PlayerSprite));
	DashSprite = TSoftObjectPtr<UPaperSprite>(FSoftObjectPath(YJJAssetPaths::PlayerDashSprite));
	BackgroundSprite = TSoftObjectPtr<UPaperSprite>(FSoftObjectPath(YJJAssetPaths::SkySprite));

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void AYJJJumper::BeginPlay()
{
	Super::BeginPlay();

	const FVector3f Start(GetActorLocation());
	CenterX = Start.X;
	PlaneY = Start.Y;
	StartHeight = Start.Z;
	CameraZ = Start.Z;

	UPaperSprite* BodyAsset = BodySprite.LoadSynchronous();
	YJJSpriteUtils::SetSpriteWithWidth(Body, BodyAsset, BodyWidth);
	BodyBaseScale = static_cast<float>(Body->GetRelativeScale3D().X);
	BodySpriteHeight = YJJSpriteUtils::GetSpriteSize(BodyAsset).Y * BodyBaseScale;
	BodySpriteAsset = BodyAsset;

	// 돌진 포즈는 같은 픽셀 배율 기준으로 머리 크기가 평소보다 작게 그려져 있어 그만큼 키운다
	DashSpriteAsset = DashSprite.LoadSynchronous();
	DashBaseScale = BodyBaseScale * DashPoseScale;
	UMaterialInterface* GhostMaterial = LoadObject<UMaterialInterface>(nullptr, YJJAssetPaths::TranslucentSpriteMaterial);
	for (UPaperSpriteComponent* Ghost : Ghosts)
	{
		Ghost->SetSprite(DashSpriteAsset);
		if (GhostMaterial)
		{
			Ghost->SetMaterial(0, GhostMaterial);
		}
	}

	UPaperSprite* BackgroundAsset = BackgroundSprite.LoadSynchronous();
	Background->SetSprite(BackgroundAsset);
	BackgroundSpriteSize = YJJSpriteUtils::GetSpriteSize(BackgroundAsset);

	// 시작은 공중. 아래 로프에 떨어져 자연스럽게 멈춘다
	SimState = FYJJJumperState();
	SimState.Position = FVector2f(Start.X, Start.Z);
	SimState.MaxHeightReached = Start.Z;

	// 튜토리얼 동안은 Lv 999Gp(구골플렉스) 장수풍뎅이
	const bool bSkipTutorial = CVarTutorial.GetValueOnGameThread() == 0;
	SimState.bTutorial = !bSkipTutorial;
	TutorialStep = bSkipTutorial ? EYJJTutorialStep::Done : EYJJTutorialStep::Drag;
}

void AYJJJumper::EnsureInputObjects()
{
	if (PressAction)
	{
		return;
	}

	// 기본 ValueType = Boolean
	PressAction = NewObject<UInputAction>(this, TEXT("IA_JumperPress"));
	InputContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Jumper"));
	InputContext->MapKey(PressAction, EKeys::LeftMouseButton);
	InputContext->MapKey(PressAction, EKeys::TouchKeys[0]);
	InputContext->MapKey(PressAction, EKeys::SpaceBar);
}

void AYJJJumper::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	EnsureInputObjects();
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(PressAction, ETriggerEvent::Started, this, &AYJJJumper::OnPressStarted);
		Input->BindAction(PressAction, ETriggerEvent::Completed, this, &AYJJJumper::OnPressCompleted);
	}
}

void AYJJJumper::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	EnsureInputObjects();
	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputContext, 0);
		}
	}
}

void AYJJJumper::OnPressStarted(const FInputActionValue& Value)
{
	if (YJJRopeSim::BeginDrag(SimState))
	{
		bDragging = GetPointerPosition(DragStartPixels);
		if (!bDragging)
		{
			// 포인터가 없으면 (키보드) 드래그 불가
			SimState.Mode = EYJJJumperMode::Resting;
		}
		return;
	}

	// 공중에서 몬스터를 탭하면 들이받기, 아니면 로프 타이밍 입력
	FVector2f Pointer;
	FVector2f TapGame;
	if (GetPointerPosition(Pointer) && ScreenToGame(Pointer, TapGame) && TryTapAttack(TapGame))
	{
		return;
	}

	FYJJSimEvents Events;
	YJJRopeSim::PressBounce(SimState, Tuning, ComputeAimX(), Events);
	HandleEvents(Events);
}

void AYJJJumper::OnPressCompleted(const FInputActionValue& Value)
{
	if (bDragging)
	{
		bDragging = false;
		YJJRopeSim::ReleaseDrag(SimState, Tuning);
	}
}

bool AYJJJumper::GetPointerPosition(FVector2f& OutPosition) const
{
	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC)
	{
		return false;
	}

	float X = 0.f;
	float Y = 0.f;
	if (PC->GetMousePosition(X, Y))
	{
		OutPosition = FVector2f(X, Y);
		return true;
	}

	bool bPressed = false;
	PC->GetInputTouchState(ETouchIndex::Touch1, X, Y, bPressed);
	if (bPressed)
	{
		OutPosition = FVector2f(X, Y);
		return true;
	}
	return false;
}

bool AYJJJumper::ScreenToGame(const FVector2f& Screen, FVector2f& OutGame) const
{
	const APlayerController* PC = Cast<APlayerController>(Controller);
	FVector Origin;
	FVector Direction;
	if (!PC || !PC->DeprojectScreenPositionToWorld(Screen.X, Screen.Y, Origin, Direction) || FMath::IsNearlyZero(Direction.Y))
	{
		return false;
	}

	// 게임 평면(Y = PlaneY)과 만나는 점
	const double T = (PlaneY - Origin.Y) / Direction.Y;
	const FVector Hit = Origin + Direction * T;
	OutGame = FVector2f(static_cast<float>(Hit.X), static_cast<float>(Hit.Z));
	return true;
}

float AYJJJumper::ComputeAimX() const
{
	const APlayerController* PC = Cast<APlayerController>(Controller);
	FVector2f Pointer;
	if (!PC || !GetPointerPosition(Pointer))
	{
		return 0.f;
	}

	FVector2D CharacterScreen;
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PC->GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth <= 0 || !PC->ProjectWorldLocationToScreen(GetActorLocation(), CharacterScreen))
	{
		return 0.f;
	}

	const float FullAimPixels = FMath::Max(ViewportWidth * FullAimScreenRatio, 1.f);
	return FMath::Clamp((Pointer.X - static_cast<float>(CharacterScreen.X)) / FullAimPixels, -1.f, 1.f);
}

bool AYJJJumper::TryTapAttack(const FVector2f& TapGamePosition)
{
	const UYJJMobSubsystem* Mobs = GetMobs();
	if (!Mobs || SimState.Mode != EYJJJumperMode::Airborne)
	{
		return false;
	}

	TArray<FYJJMobTarget> Targets;
	Mobs->GatherTargets(Targets);

	// 탭한 지점에 가장 가까운 몬스터
	const FYJJMobTarget* Tapped = nullptr;
	for (const FYJJMobTarget& Target : Targets)
	{
		if (YJJCombat::IsTapOnMob(CombatTuning, TapGamePosition, Target)
			&& (!Tapped || FVector2f::DistSquared(TapGamePosition, Target.Position) < FVector2f::DistSquared(TapGamePosition, Tapped->Position)))
		{
			Tapped = &Target;
		}
	}
	if (!Tapped)
	{
		return false;
	}

	if (!StartDash(*Tapped))
	{
		AddPopup(GameToWorld(Tapped->Position, EffectDepth), TEXT("멀어 임마"), FLinearColor(0.85f, 0.85f, 0.85f), 0.9f, 0.6f);
	}
	return true;
}

bool AYJJJumper::StartDash(const FYJJMobTarget& Target)
{
	if (!YJJCombat::BeginDash(SimState, CombatTuning, Target))
	{
		return false;
	}

	const FVector2f Delta = Target.Position - SimState.Position;
	if (FMath::Abs(Delta.X) > 1.f)
	{
		Facing = Delta.X > 0.f ? 1.f : -1.f;
	}
	LastDashDir = Delta.IsNearlyZero() ? FVector2f(Facing, 0.f) : Delta.GetSafeNormal();
	TrailHistory.Reset();
	YJJAudio::Play(this, YJJAudio::ESfx::Dash, 0.7f, FMath::FRandRange(0.95f, 1.1f));

	// 출발 순간 뒤쪽에 "슝" 먼지
	FYJJSpriteFxParams Dust;
	Dust.Frames = YJJSpriteUtils::LoadPoofFrames();
	Dust.FrameTime = 0.1f;
	Dust.Lifetime = 0.25f;
	Dust.Width = 110.f;
	Dust.StartScale = 0.5f;
	Dust.EndScale = 1.1f;
	Dust.Velocity = FVector(-LastDashDir.X, 0.f, -LastDashDir.Y) * 300.f;
	AYJJSpriteFx::Spawn(GetWorld(), GetActorLocation() + FVector(0.f, BodyDepth - 5.f, BodySpriteHeight * 0.5f), Dust);
	return true;
}

float AYJJJumper::GetAttackPower() const
{
	return YJJCombat::ComputeAttackPower(SimState, CombatTuning);
}

float AYJJJumper::GetAttackRange() const
{
	return YJJCombat::GetAttackRange(SimState, CombatTuning);
}

int32 AYJJJumper::GetXPToNextLevel() const
{
	return YJJCombat::XPToNextLevel(CombatTuning, SimState.Level);
}

bool AYJJJumper::GetLaunchPreview(FVector2f& OutVelocity) const
{
	if (SimState.Mode != EYJJJumperMode::Dragging)
	{
		return false;
	}

	// 실제 시뮬레이션을 복사해서 발사 순간까지 돌려 본다
	const UYJJRopeSubsystem* Ropes = GetRopes();
	if (!Ropes)
	{
		return false;
	}

	FYJJJumperState Preview = SimState;
	YJJRopeSim::ReleaseDrag(Preview, Tuning);
	for (int32 Step = 0; Step < 240 && Preview.Mode == EYJJJumperMode::OnRope; ++Step)
	{
		FYJJSimEvents Ignored;
		YJJRopeSim::Step(Preview, Tuning, Ropes->GetSegments(), CenterX, Ignored);
	}

	if (Preview.Mode != EYJJJumperMode::Airborne)
	{
		return false;
	}
	OutVelocity = Preview.Velocity;
	return true;
}

float AYJJJumper::GetHeightMeters() const
{
	return (SimState.Position.Y - StartHeight) / 100.f;
}

float AYJJJumper::GetBestHeightMeters() const
{
	return (SimState.MaxHeightReached - StartHeight) / 100.f;
}

float AYJJJumper::GetSpeedKmh() const
{
	// cm/s -> km/h
	const FVector2f Velocity = SimState.IsDashing() ? SimState.DashSavedVelocity : SimState.Velocity;
	return Velocity.Size() * 0.036f;
}

void AYJJJumper::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDragging)
	{
		FVector2f Pointer;
		if (GetPointerPosition(Pointer))
		{
			YJJRopeSim::UpdateDrag(SimState, Tuning, Pointer - DragStartPixels);
		}
	}

	// 처치 순간 히트스톱: 장수풍뎅이가 잠깐 멈추고 몬스터 처치 연출만 움직인다
	if (HitStopTime > 0.f)
	{
		HitStopTime = FMath::Max(HitStopTime - DeltaSeconds, 0.f);
	}
	else
	{
		DashPoseTime = FMath::Max(DashPoseTime - DeltaSeconds, 0.f);
		if (CVarAutoPlay.GetValueOnGameThread() != 0)
		{
			TickAutoPlay(DeltaSeconds);
		}
		TickSimulation(DeltaSeconds);
	}
	RecoverFromFall();
	SyncVisuals(DeltaSeconds);
	TickPopups(DeltaSeconds);
	LevelStolenBannerTime = FMath::Max(LevelStolenBannerTime - DeltaSeconds, 0.f);

	if (bShowDebugText)
	{
		DrawDebugText();
	}
}

UYJJRopeSubsystem* AYJJJumper::GetRopes() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UYJJRopeSubsystem>() : nullptr;
}

UYJJMobSubsystem* AYJJJumper::GetMobs() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UYJJMobSubsystem>() : nullptr;
}

void AYJJJumper::TickSimulation(float DeltaSeconds)
{
	const UYJJRopeSubsystem* Ropes = GetRopes();
	if (!Ropes || !Ropes->HasRopes())
	{
		return;
	}

	// 프레임레이트와 무관하게 같은 결과가 나오도록 고정 스텝
	StepAccumulator = FMath::Min(StepAccumulator + DeltaSeconds, 0.25f);
	while (StepAccumulator >= YJJRopeSim::FixedStep)
	{
		StepAccumulator -= YJJRopeSim::FixedStep;

		if (SimState.IsDashing())
		{
			// 돌진 중엔 로프/중력 대신 몬스터에게 직선 이동
			SimState.SimTime += YJJRopeSim::FixedStep;
			if (YJJCombat::StepDash(SimState, YJJRopeSim::FixedStep))
			{
				FinishDash();
			}
			continue;
		}

		FYJJSimEvents Events;
		YJJRopeSim::Step(SimState, Tuning, Ropes->GetSegments(), CenterX, Events);
		HandleEvents(Events);
	}
}

void AYJJJumper::FinishDash()
{
	const int32 MobId = SimState.DashMobId;
	UYJJMobSubsystem* Mobs = GetMobs();
	AYJJMob* Mob = Mobs ? Mobs->FindMob(MobId) : nullptr;

	FYJJMobTarget Target;
	if (Mob)
	{
		Target = Mob->MakeTarget();
	}

	const FYJJHitResult Result = YJJCombat::FinishDash(SimState, CombatTuning, Mob ? &Target : nullptr);
	// 들이받은 자세를 잠깐 유지 (히트스톱 동안 + 약간 더)
	DashPoseTime = 0.12f;
	if (!Result.bHit)
	{
		return;
	}

	const FVector HitLocation = GameToWorld(Target.Position, EffectDepth);
	Mob->ApplyHit(Result.Damage);

	if (Result.bKilled)
	{
		AddShake(0.25f);
		YJJAudio::Play(this, YJJAudio::ESfx::Kill, 0.9f, FMath::FRandRange(0.95f, 1.08f));
		HitStopTime = HitStopDuration;
		// 히트스톱 동안 남은 고정 스텝을 버려서 멈췄다 이어질 때 순간이동하지 않게
		StepAccumulator = 0.f;
		AddPopup(HitLocation, FString::Printf(TEXT("+%d"), Result.ScoreGained), PerfectColor, 1.3f);
		if (Result.LevelsGained > 0)
		{
			AddShake(0.6f);
			YJJAudio::Play(this, YJJAudio::ESfx::LevelUp, 0.8f);
			AYJJHitEffect::Spawn(GetWorld(), GetActorLocation() + FVector(0.f, EffectDepth, 60.f), 260.f, LevelUpColor);
			AddPopup(GetActorLocation() + FVector(0.f, EffectDepth, 220.f), FString::Printf(TEXT("레벨 업!! Lv %d"), SimState.Level), LevelUpColor, 1.8f, 1.4f);
		}
	}
	else
	{
		AddShake(0.2f);
		YJJAudio::Play(this, YJJAudio::ESfx::Hit, 0.8f);
		AYJJHitEffect::Spawn(GetWorld(), HitLocation, 90.f, FLinearColor(0.8f, 0.8f, 1.f));
		AddPopup(HitLocation, FString::Printf(TEXT("-%.1f  안 죽네?"), Result.Damage), FLinearColor(0.8f, 0.85f, 1.f), 1.f);
	}
}

void AYJJJumper::HandleEvents(const FYJJSimEvents& Events)
{
	UpdateTutorial(Events);

	if (Events.bLaunched)
	{
		// 빨라질수록 "띠요옹"도 높아진다
		YJJAudio::Play(this, YJJAudio::ESfx::Launch, 0.8f, 0.9f + 0.06f * FMath::Min(SimState.SpeedScale, 8.f));
	}
	if (Events.bLanded)
	{
		YJJAudio::Play(this, YJJAudio::ESfx::Land, 0.7f);
	}

	if (!Events.bGraded)
	{
		return;
	}

	OnBounceGraded.Broadcast(Events.Grade, SimState.PerfectChain);
	switch (Events.Grade)
	{
	case EYJJBounceGrade::Perfect:
		YJJAudio::Play(this, YJJAudio::ESfx::Perfect, 0.8f, 1.f + 0.04f * FMath::Min(SimState.PerfectChain, 10));
		break;
	case EYJJBounceGrade::Good:
		YJJAudio::Play(this, YJJAudio::ESfx::Good, 0.7f);
		break;
	default:
		YJJAudio::Play(this, YJJAudio::ESfx::Miss, 0.6f);
		break;
	}

	const FVector Above = GetActorLocation() + FVector(0.f, EffectDepth, 180.f);
	switch (Events.Grade)
	{
	case EYJJBounceGrade::Perfect:
		AddShake(0.3f);
		AddPopup(Above, SimState.PerfectChain >= 3 ? FString::Printf(TEXT("개쩔어!! x%d"), SimState.PerfectChain) : FString(TEXT("퍼펙트!!")), PerfectColor, 1.7f);
		break;
	case EYJJBounceGrade::Good:
		AddPopup(Above, TEXT("굿"), GoodColor, 1.3f);
		break;
	default:
		AddPopup(Above, TEXT("응 아니야"), MissColor, 1.2f);
		break;
	}
}

void AYJJJumper::UpdateTutorial(const FYJJSimEvents& Events)
{
	switch (TutorialStep)
	{
	case EYJJTutorialStep::Drag:
		if (Events.bLaunched)
		{
			TutorialStep = EYJJTutorialStep::TapMob;
		}
		break;
	case EYJJTutorialStep::TapMob:
		if (SimState.Kills >= TutorialKillsNeeded)
		{
			TutorialStep = EYJJTutorialStep::Perfect;
		}
		break;
	case EYJJTutorialStep::Perfect:
		if (Events.bGraded && Events.Grade == EYJJBounceGrade::Perfect)
		{
			TutorialStep = EYJJTutorialStep::LevelStolen;
		}
		break;
	case EYJJTutorialStep::LevelStolen:
		// 다음 착지 순간 대머리 독수리가 레벨을 털어 간다
		if (Events.bLanded)
		{
			ApplyLevelStolen();
		}
		break;
	default:
		break;
	}
}

void AYJJJumper::ApplyLevelStolen()
{
	TutorialStep = EYJJTutorialStep::Done;
	LevelStolenBannerTime = 4.5f;

	SimState.bTutorial = false;
	SimState.Level = 1;
	SimState.XP = 0;
	SimState.Score = 0;
	SimState.Kills = 0;
	SimState.PerfectChain = 0;
	SimState.LaunchGrade = EYJJBounceGrade::None;

	if (const UYJJRopeSubsystem* Ropes = GetRopes())
	{
		if (const FYJJRopeSegment* Rope = YJJRopeSim::FindRope(Ropes->GetSegments(), SimState.RopeId))
		{
			YJJRopeSim::PlaceOnRope(SimState, Tuning, *Rope, CenterX);
		}
	}
	SimState.MaxHeightReached = SimState.Position.Y;
	AddShake(2.f);
}

void AYJJJumper::RecoverFromFall()
{
	const UYJJRopeSubsystem* Ropes = GetRopes();
	if (!Ropes || !Ropes->HasRopes() || SimState.Mode != EYJJJumperMode::Airborne || SimState.IsDashing())
	{
		return;
	}

	if (SimState.Position.Y > Ropes->GetLowestHeight() - FallRecoverDistance)
	{
		return;
	}

	const FYJJRopeSegment* Rope = YJJRopeSim::FindRope(Ropes->GetSegments(), SimState.LastRopeId);
	if (!Rope)
	{
		Rope = &Ropes->GetSegments()[0];
	}
	YJJRopeSim::PlaceOnRope(SimState, Tuning, *Rope, (Rope->MinX + Rope->MaxX) * 0.5f);
}

FVector AYJJJumper::GameToWorld(const FVector2f& Game, float Depth) const
{
	return FVector(Game.X, PlaneY + Depth, Game.Y);
}

void AYJJJumper::SyncVisuals(float DeltaSeconds)
{
	SetActorLocation(FVector(SimState.Position.X, PlaneY, SimState.Position.Y));
	AnimateBody();

	// 로프 V자 처짐
	const bool bOnRope = SimState.Mode != EYJJJumperMode::Airborne;
	const int32 ContactRopeId = bOnRope ? SimState.RopeId : INDEX_NONE;
	if (const UYJJRopeSubsystem* Ropes = GetRopes())
	{
		if (VisualRopeId != ContactRopeId)
		{
			if (AYJJRope* Previous = Ropes->FindRopeActor(VisualRopeId))
			{
				Previous->ClearContact();
			}
			VisualRopeId = ContactRopeId;
		}
		if (AYJJRope* Current = Ropes->FindRopeActor(ContactRopeId))
		{
			Current->SetContact(SimState.Position.X, SimState.Deflection);
		}
	}

	const FVector2f Visible = UpdateCameraFraming(DeltaSeconds);

	// 진행 방향을 미리 보여준다 (떨어질 땐 로프 쪽을 더 보이게)
	const FVector2f Velocity = SimState.IsDashing() ? SimState.DashSavedVelocity : SimState.Velocity;
	const float LookAhead = SimState.Mode == EYJJJumperMode::Airborne
		? FMath::Clamp(Velocity.Y * CameraLookAheadTime, -Visible.Y * 0.38f, Visible.Y * 0.38f)
		: Visible.Y * 0.3f;
	const float TargetZ = SimState.Position.Y + LookAhead;
	// 아주 빠르면 카메라도 바로 붙는다
	const float FollowSpeed = CameraFollowSpeed * FMath::Max(SimState.SpeedScale, 1.f);
	CameraZ = FMath::FInterpTo(CameraZ, TargetZ, DeltaSeconds, FollowSpeed);

	// 처치/퍼펙트 때 화면 흔들림
	FVector ShakeOffset = FVector::ZeroVector;
	if (ShakeTime > 0.f)
	{
		ShakeTime = FMath::Max(ShakeTime - DeltaSeconds, 0.f);
		const float Amplitude = ShakeStrength * (ShakeTime / 0.25f) * Visible.Y * 0.012f;
		ShakeOffset = FVector(FMath::FRandRange(-1.f, 1.f) * Amplitude, 0.f, FMath::FRandRange(-1.f, 1.f) * Amplitude);
	}
	Camera->SetWorldLocation(FVector(CenterX, PlaneY + CameraDistance, CameraZ) + ShakeOffset);

	// 배경은 보이는 영역을 덮고, 높이 올라갈수록 어두워진다 (하늘 -> 우주)
	if (BackgroundSpriteSize.X > KINDA_SMALL_NUMBER && BackgroundSpriteSize.Y > KINDA_SMALL_NUMBER)
	{
		// 초고속 프레임에서도 빈 곳이 보이지 않도록 보이는 영역보다 훨씬 크게 덮는다
		Background->SetRelativeScale3D(FVector(Visible.X * 4.f / BackgroundSpriteSize.X, 1.f, Visible.Y * 4.f / BackgroundSpriteSize.Y));

		const float HeightM = (CameraZ - StartHeight) / 100.f;
		const FLinearColor Low(1.f, 1.f, 1.f);
		const FLinearColor High(0.35f, 0.35f, 0.65f);
		const FLinearColor Space(0.07f, 0.05f, 0.14f);
		const FLinearColor Tint = HeightM < 300.f
			? FLinearColor::LerpUsingHSV(Low, High, FMath::Clamp(HeightM / 300.f, 0.f, 1.f))
			: FLinearColor::LerpUsingHSV(High, Space, FMath::Clamp((HeightM - 300.f) / 700.f, 0.f, 1.f));
		Background->SetSpriteColor(Tint);
	}
}

FVector2f AYJJJumper::UpdateCameraFraming(float DeltaSeconds)
{
	float Aspect = 9.f / 16.f;
	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		int32 Width = 0;
		int32 Height = 0;
		PC->GetViewportSize(Width, Height);
		if (Width > 0 && Height > 0)
		{
			Aspect = static_cast<float>(Width) / Height;
		}
	}

	// 빨라질수록 줌 아웃해서 속도감을 키운다
	const float TargetZoom = FMath::Clamp(1.f + 0.2f * FMath::Log2(FMath::Max(SimState.SpeedScale, 1.f)), 1.f, CameraMaxZoomOut);
	CameraZoom = FMath::FInterpTo(CameraZoom, TargetZoom, DeltaSeconds, 2.f);

	// 세로 시야를 기준으로 했을 때 탑이 좌우로 다 들어오면 세로 고정, 아니면(세로 폰) 가로 고정
	const float ViewHeight = CameraViewHeight * CameraZoom;
	const float NeedWidth = (Tuning.WallHalfWidth + CameraSideMargin) * 2.f * CameraZoom;
	Camera->bOverrideAspectRatioAxisConstraint = true;
	if (ViewHeight * Aspect >= NeedWidth)
	{
		Camera->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainYFOV);
		Camera->SetOrthoWidth(ViewHeight);
		return FVector2f(ViewHeight * Aspect, ViewHeight);
	}

	Camera->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV);
	Camera->SetOrthoWidth(NeedWidth);
	return FVector2f(NeedWidth, NeedWidth / Aspect);
}

void AYJJJumper::AddShake(float Strength)
{
	// 흔들리는 중이면 남은 세기를 절반 이어받아 겹친다
	ShakeStrength = (ShakeTime > 0.f ? ShakeStrength * 0.5f : 0.f) + Strength;
	ShakeTime = 0.25f;
}

void AYJJJumper::AnimateBody()
{
	// 돌진 중(그리고 타격 직후 잠깐)은 전용 돌진 포즈로 뿔을 목표 쪽으로 향하고 잔상을 남긴다
	const bool bDashPose = SimState.IsDashing() || DashPoseTime > 0.f;
	if (bDashPose && DashSpriteAsset)
	{
		if (Body->GetSprite() != DashSpriteAsset)
		{
			Body->SetSprite(DashSpriteAsset);
		}
		const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(LastDashDir.Y * Facing, LastDashDir.X * Facing));
		// 출발 순간 크게 늘어났다가 돌진하는 동안 살짝 줄어든다
		const float DashAlpha = SimState.IsDashing() ? 1.f - SimState.DashTimeLeft / FMath::Max(SimState.DashDuration, KINDA_SMALL_NUMBER) : 1.f;
		const float Stretch = FMath::Lerp(1.25f, 1.05f, DashAlpha);
		Body->SetRelativeScale3D(FVector(DashBaseScale * Stretch * Facing, DashBaseScale, DashBaseScale / Stretch));
		Body->SetRelativeRotation(FRotator(Pitch, 0.f, 0.f));
		Body->SetRelativeLocation(FVector(0.f, BodyDepth, BodySpriteHeight * 0.5f));

		TrailHistory.Insert(Body->GetComponentTransform(), 0);
		TrailHistory.SetNum(FMath::Min(TrailHistory.Num(), 16));
		TrailFade = 0.2f;
		UpdateTrail();
		return;
	}

	if (BodySpriteAsset && Body->GetSprite() != BodySpriteAsset)
	{
		Body->SetSprite(BodySpriteAsset);
	}
	UpdateTrail();

	// 로프 위에서는 처진 만큼 찌그러지고, 공중에서는 속도만큼 늘어난다
	float SquashX = 1.f;
	float SquashZ = 1.f;
	const float Pitch = 0.f;
	if (SimState.Mode == EYJJJumperMode::Airborne)
	{
		const float Stretch = FMath::Clamp(FMath::Abs(SimState.Velocity.Y) / (5000.f * SimState.SpeedScale), 0.f, 0.3f);
		SquashZ = 1.f + Stretch;
		SquashX = 1.f - Stretch * 0.5f;
	}
	else
	{
		const float Squash = FMath::Clamp(SimState.Deflection / FMath::Max(Tuning.MaxDeflection, 1.f), 0.f, 1.f) * 0.4f;
		SquashZ = 1.f - Squash;
		SquashX = 1.f + Squash * 0.7f;
	}

	// 좌우로 움직이면 그쪽을 본다
	if (!SimState.IsDashing() && FMath::Abs(SimState.Velocity.X) > 50.f)
	{
		Facing = SimState.Velocity.X > 0.f ? 1.f : -1.f;
	}

	Body->SetRelativeScale3D(FVector(BodyBaseScale * SquashX * Facing, BodyBaseScale, BodyBaseScale * SquashZ));
	Body->SetRelativeRotation(FRotator(Pitch, 0.f, 0.f));
	// 발이 로프(액터 위치)에 닿도록 중심을 올린다
	Body->SetRelativeLocation(FVector(0.f, BodyDepth, BodySpriteHeight * 0.5f * SquashZ));
}

void AYJJJumper::UpdateTrail()
{
	// 잔상: 몇 프레임 전 몸 위치에 반투명 돌진 포즈를 남긴다. 돌진이 끝나면 서서히 사라짐
	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
	TrailFade = FMath::Max(TrailFade - DeltaSeconds, 0.f);
	const float Fade = TrailFade / 0.2f;
	if (Fade <= 0.f)
	{
		TrailHistory.Reset();
	}

	for (int32 Index = 0; Index < Ghosts.Num(); ++Index)
	{
		UPaperSpriteComponent* Ghost = Ghosts[Index];
		const int32 Sample = (Index + 1) * 2;
		const bool bVisible = Fade > 0.f && TrailHistory.IsValidIndex(Sample);
		Ghost->SetVisibility(bVisible);
		if (bVisible)
		{
			FTransform Transform = TrailHistory[Sample];
			Transform.AddToTranslation(FVector(0.f, -2.f * (Index + 1), 0.f));
			Ghost->SetWorldTransform(Transform);
			Ghost->SetSpriteColor(FLinearColor(0.55f, 0.8f, 1.f, (0.5f - Index * 0.11f) * Fade));
		}
	}
}

void AYJJJumper::AddPopup(const FVector& Location, const FString& Text, const FLinearColor& Color, float Scale, float Lifetime)
{
	// 고속에서 처치가 몰려도 화면이 글자로 덮이지 않게 최근 몇 개만
	constexpr int32 MaxPopups = 6;
	if (Popups.Num() >= MaxPopups)
	{
		Popups.RemoveAt(0);
	}
	FYJJPopup& Popup = Popups.AddDefaulted_GetRef();
	Popup.WorldLocation = Location;
	Popup.Text = Text;
	Popup.Color = Color;
	Popup.Scale = Scale;
	Popup.Lifetime = Lifetime;
}

void AYJJJumper::TickPopups(float DeltaSeconds)
{
	for (FYJJPopup& Popup : Popups)
	{
		Popup.Age += DeltaSeconds;
		Popup.WorldLocation.Z += 120.f * DeltaSeconds;
	}
	Popups.RemoveAll([](const FYJJPopup& Popup) { return Popup.Age >= Popup.Lifetime; });
}

void AYJJJumper::DrawDebugText() const
{
	if (!GEngine)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(1001, 0.f, FColor::White, FString::Printf(TEXT("Mode %s  Launch %.0f  Scale %.2f  Deflection %.0f  Chain %d"),
		*UEnum::GetDisplayValueAsText(SimState.Mode).ToString(), SimState.LastLaunchSpeed, SimState.SpeedScale, SimState.Deflection, SimState.PerfectChain));
}

void AYJJJumper::TickAutoPlay(float DeltaSeconds)
{
	AutoPlayTime += DeltaSeconds;

	if (SimState.Mode == EYJJJumperMode::Resting)
	{
		// 끝까지 당겨서 똑바로 발사
		if (YJJRopeSim::BeginDrag(SimState))
		{
			YJJRopeSim::UpdateDrag(SimState, Tuning, FVector2f(0.f, 1000.f));
			YJJRopeSim::ReleaseDrag(SimState, Tuning);
		}
	}
	else if (SimState.Mode == EYJJJumperMode::OnRope && SimState.bReachedBottom && !SimState.bGraded)
	{
		// 최저점 직후 프레임에 입력 -> 퍼펙트. 로프 쪽으로 살짝 중앙 복귀
		FYJJSimEvents Events;
		YJJRopeSim::PressBounce(SimState, Tuning, FMath::Clamp((CenterX - SimState.Position.X) / 900.f, -0.5f, 0.5f), Events);
		HandleEvents(Events);
	}
	else if (SimState.Mode == EYJJJumperMode::Airborne && SimState.Velocity.Y < 0.f && SimState.SpeedScale >= 3.f
		&& SimState.LastPressTime < 0.0 && !SimState.IsDashing()
		&& (SimState.Position.Y - SimState.RopeHeight) / -SimState.Velocity.Y < 0.02f)
	{
		// 너무 빨라서 로프에 닿아 있는 시간이 한 프레임보다 짧다 -> 닿기 직전에 미리 누른다 (사람도 이렇게 해야 함)
		FYJJSimEvents Events;
		YJJRopeSim::PressBounce(SimState, Tuning, FMath::Clamp((CenterX - SimState.Position.X) / 900.f, -0.5f, 0.5f), Events);
		HandleEvents(Events);
	}
	else if (SimState.Mode == EYJJJumperMode::Airborne && !SimState.IsDashing() && AutoPlayTime >= AutoPlayNextTap)
	{
		// 사거리 안의 가장 가까운 몬스터를 탭
		if (const UYJJMobSubsystem* Mobs = GetMobs())
		{
			TArray<FYJJMobTarget> Targets;
			Mobs->GatherTargets(Targets);
			const FYJJMobTarget* Best = nullptr;
			for (const FYJJMobTarget& Target : Targets)
			{
				if (YJJCombat::CanDashTo(SimState, CombatTuning, Target)
					&& (!Best || FVector2f::DistSquared(SimState.Position, Target.Position) < FVector2f::DistSquared(SimState.Position, Best->Position)))
				{
					Best = &Target;
				}
			}
			if (Best && StartDash(*Best))
			{
				AutoPlayNextTap = AutoPlayTime + 0.12f;
			}
		}
	}

	if (AutoPlayTime >= AutoPlayNextReport)
	{
		AutoPlayNextReport += 2.f;
		UE_LOG(LogYJJJumper, Display, TEXT("[AutoPlay] t=%.0f height=%.1fm best=%.1fm speed=%.0fkm/h scale=%.2f Lv=%d%s xp=%d score=%d kills=%d chain=%d tutorial=%s mobs=%d"),
			AutoPlayTime, GetHeightMeters(), GetBestHeightMeters(), GetSpeedKmh(), SimState.SpeedScale, SimState.Level, SimState.bTutorial ? TEXT("(999Gp)") : TEXT(""),
			SimState.XP, SimState.Score, SimState.Kills, SimState.PerfectChain, *UEnum::GetValueAsString(TutorialStep), GetMobs() ? GetMobs()->Num() : 0);
	}

	// 정해진 시간 동안 스크린샷 4장을 남기고 종료
	const float Duration = CVarAutoPlayDuration.GetValueOnGameThread();
	if (Duration > 0.f)
	{
		const float ShotTimes[] = { Duration * 0.1f, Duration * 0.35f, Duration * 0.65f, Duration * 0.95f };
		if (AutoPlayShotIndex < UE_ARRAY_COUNT(ShotTimes) && AutoPlayTime >= ShotTimes[AutoPlayShotIndex])
		{
			const FString Path = FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/AutoPlay_%d.png"), AutoPlayShotIndex);
			FScreenshotRequest::RequestScreenshot(Path, true, false);
			UE_LOG(LogYJJJumper, Display, TEXT("[AutoPlay] screenshot %s"), *Path);
			AutoPlayShotIndex++;
		}
		if (AutoPlayTime >= Duration + 1.f)
		{
			UE_LOG(LogYJJJumper, Display, TEXT("[AutoPlay] finished best=%.1fm Lv=%d score=%d kills=%d"), GetBestHeightMeters(), SimState.Level, SimState.Score, SimState.Kills);
			FPlatformMisc::RequestExit(false, TEXT("yjj.AutoPlayDuration"));
		}
	}
}
