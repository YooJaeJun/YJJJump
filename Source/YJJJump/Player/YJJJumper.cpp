#include "Player/YJJJumper.h"
#include "Combat/YJJCombat.h"
#include "Combat/YJJMob.h"
#include "Combat/YJJMobSubsystem.h"
#include "Rope/YJJRope.h"
#include "Rope/YJJRopeSimulation.h"
#include "Rope/YJJRopeSubsystem.h"
#include "Visual/YJJHitEffect.h"
#include "Visual/YJJSpriteUtils.h"
#include "YJJAssetPaths.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogYJJJumper, Log, All);

static TAutoConsoleVariable<int32> CVarAutoPlay(
	TEXT("yjj.AutoPlay"), 0,
	TEXT("1이면 봇이 드래그 발사와 퍼펙트 타이밍을 대신 입력한다 (스모크 테스트용)"));

static TAutoConsoleVariable<float> CVarAutoPlayDuration(
	TEXT("yjj.AutoPlayDuration"), 0.f,
	TEXT("0보다 크면 오토플레이를 이 시간(초) 동안 하고 스크린샷을 남긴 뒤 게임을 종료한다"));

namespace
{
	// 화면 깊이(Y) 순서. 카메라는 +Y에서 -Y를 본다
	constexpr float BodyDepth = 10.f;
	constexpr float EffectDepth = 20.f;
	constexpr float BackgroundDepth = -2000.f;

	const FLinearColor PerfectColor(1.f, 0.85f, 0.1f);
	const FLinearColor GoodColor(0.3f, 1.f, 0.4f);
	const FLinearColor MissColor(1.f, 0.35f, 0.3f);
}

AYJJJumper::AYJJJumper()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Body = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 배경은 카메라를 따라다니도록 매 프레임 월드 위치를 직접 잡는다
	Background = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Background"));
	Background->SetupAttachment(Root);
	Background->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Background->SetUsingAbsoluteLocation(true);
	Background->SetUsingAbsoluteRotation(true);
	Background->SetUsingAbsoluteScale(true);

	// 옆에서 보는 직교 카메라. X는 탑 중앙에 고정하고 Z만 따라간다
	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(Root);
	CameraArm->SetUsingAbsoluteLocation(true);
	CameraArm->SetUsingAbsoluteRotation(true);
	CameraArm->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	CameraArm->TargetArmLength = 3000.f;
	CameraArm->bDoCollisionTest = false;
	CameraArm->bEnableCameraLag = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraArm);
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	Camera->SetOrthoWidth(CameraViewHeight);

	BodySprite = TSoftObjectPtr<UPaperSprite>(FSoftObjectPath(YJJAssetPaths::PlayerSprite));
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
	CameraZ = Start.Z + CameraLookAhead;
	Camera->SetOrthoWidth(CameraViewHeight);

	UPaperSprite* BodyAsset = BodySprite.LoadSynchronous();
	YJJSpriteUtils::SetSpriteWithWidth(Body, BodyAsset, BodyWidth);
	BodyBaseScale = static_cast<float>(Body->GetRelativeScale3D().X);
	BodySpriteHeight = YJJSpriteUtils::GetSpriteSize(BodyAsset).Y * BodyBaseScale;

	UPaperSprite* BackgroundAsset = BackgroundSprite.LoadSynchronous();
	Background->SetSprite(BackgroundAsset);
	BackgroundSpriteSize = YJJSpriteUtils::GetSpriteSize(BackgroundAsset);

	// 시작은 공중. 아래 로프에 떨어져 자연스럽게 멈춘다
	SimState = FYJJJumperState();
	SimState.Position = FVector2f(Start.X, Start.Z);
	SimState.MaxHeightReached = Start.Z;
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

float AYJJJumper::GetAttackPower() const
{
	return YJJCombat::ComputeAttackPower(SimState, CombatTuning);
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

	if (CVarAutoPlay.GetValueOnGameThread() != 0)
	{
		TickAutoPlay(DeltaSeconds);
	}

	TickSimulation(DeltaSeconds);
	RecoverFromFall();
	SyncVisuals(DeltaSeconds);
	TickPopups(DeltaSeconds);

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

	// 몹 위치는 프레임 단위로 갱신되므로 프레임마다 한 번 모은다
	TArray<FYJJMobTarget> Targets;
	if (const UYJJMobSubsystem* Mobs = GetMobs())
	{
		Mobs->GatherTargets(Targets);
	}

	// 프레임레이트와 무관하게 같은 결과가 나오도록 고정 스텝
	StepAccumulator = FMath::Min(StepAccumulator + DeltaSeconds, 0.25f);
	while (StepAccumulator >= YJJRopeSim::FixedStep)
	{
		StepAccumulator -= YJJRopeSim::FixedStep;

		FYJJSimEvents Events;
		YJJRopeSim::Step(SimState, Tuning, Ropes->GetSegments(), CenterX, Events);
		HandleEvents(Events);
		ResolveMobHits(Targets);
	}
}

void AYJJJumper::ResolveMobHits(TArray<FYJJMobTarget>& Targets)
{
	for (FYJJMobTarget& Target : Targets)
	{
		if (Target.Health <= 0.f)
		{
			continue;
		}

		if (const double* LastHit = LastHitTimeByMob.Find(Target.Id))
		{
			if (SimState.SimTime - *LastHit < CombatTuning.HitCooldown)
			{
				continue;
			}
		}

		const FYJJHitResult Result = YJJCombat::ResolveHit(SimState, CombatTuning, Target);
		if (!Result.bHit)
		{
			continue;
		}

		LastHitTimeByMob.Add(Target.Id, SimState.SimTime);
		Target.Health -= Result.Damage;

		const FVector MobLocation(Target.Position.X, PlaneY + EffectDepth, Target.Position.Y);
		if (AYJJMob* Mob = GetMobs()->FindMob(Target.Id))
		{
			Mob->ApplyHit(Result.Damage);
		}

		if (Result.bKilled)
		{
			AddPopup(MobLocation, FString::Printf(TEXT("+%d"), Result.ScoreGained), PerfectColor, 1.4f);
		}
		else
		{
			AYJJHitEffect::Spawn(GetWorld(), MobLocation, 90.f, FLinearColor(0.8f, 0.8f, 1.f));
			AddPopup(MobLocation, FString::Printf(TEXT("%.1f"), Result.Damage), FLinearColor(0.8f, 0.85f, 1.f), 1.f);
		}

		// 한 스텝에 한 마리만
		break;
	}
}

void AYJJJumper::HandleEvents(const FYJJSimEvents& Events)
{
	if (!Events.bGraded)
	{
		return;
	}

	OnBounceGraded.Broadcast(Events.Grade, SimState.PerfectChain);

	const FVector Above = GetActorLocation() + FVector(0.f, EffectDepth, 160.f);
	switch (Events.Grade)
	{
	case EYJJBounceGrade::Perfect:
		AddPopup(Above, SimState.PerfectChain > 1 ? FString::Printf(TEXT("PERFECT x%d"), SimState.PerfectChain) : FString(TEXT("PERFECT")), PerfectColor, 1.6f);
		break;
	case EYJJBounceGrade::Good:
		AddPopup(Above, TEXT("GOOD"), GoodColor, 1.3f);
		break;
	default:
		AddPopup(Above, TEXT("MISS"), MissColor, 1.1f);
		break;
	}
}

void AYJJJumper::RecoverFromFall()
{
	const UYJJRopeSubsystem* Ropes = GetRopes();
	if (!Ropes || !Ropes->HasRopes() || SimState.Mode != EYJJJumperMode::Airborne)
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

	// 카메라는 위쪽을 더 보이게 따라간다
	CameraZ = FMath::FInterpTo(CameraZ, SimState.Position.Y + CameraLookAhead, DeltaSeconds, CameraFollowSpeed);
	CameraArm->SetWorldLocation(FVector(CenterX, PlaneY, CameraZ));

	// 배경은 카메라 뒤에서 보이는 영역을 넉넉히 덮는다
	const FVector2f Visible = UpdateCameraFraming();
	if (BackgroundSpriteSize.X > KINDA_SMALL_NUMBER && BackgroundSpriteSize.Y > KINDA_SMALL_NUMBER)
	{
		Background->SetWorldLocationAndRotation(FVector(CenterX, PlaneY + BackgroundDepth, CameraZ), FRotator::ZeroRotator);
		Background->SetWorldScale3D(FVector(Visible.X * 1.1f / BackgroundSpriteSize.X, 1.f, Visible.Y * 1.1f / BackgroundSpriteSize.Y));
	}
}

FVector2f AYJJJumper::UpdateCameraFraming()
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

	// 세로 시야를 기준으로 했을 때 탑이 좌우로 다 들어오면 세로 고정, 아니면(세로 폰) 가로 고정
	const float NeedWidth = (Tuning.WallHalfWidth + CameraSideMargin) * 2.f;
	Camera->bOverrideAspectRatioAxisConstraint = true;
	if (CameraViewHeight * Aspect >= NeedWidth)
	{
		Camera->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainYFOV);
		Camera->SetOrthoWidth(CameraViewHeight);
		return FVector2f(CameraViewHeight * Aspect, CameraViewHeight);
	}

	Camera->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV);
	Camera->SetOrthoWidth(NeedWidth);
	return FVector2f(NeedWidth, NeedWidth / Aspect);
}

void AYJJJumper::AnimateBody()
{
	// 로프 위에서는 처진 만큼 찌그러지고, 공중에서는 속도만큼 늘어난다
	float SquashX = 1.f;
	float SquashZ = 1.f;
	if (SimState.Mode == EYJJJumperMode::Airborne)
	{
		const float Stretch = FMath::Clamp(FMath::Abs(SimState.Velocity.Y) / 5000.f, 0.f, 0.3f);
		SquashZ = 1.f + Stretch;
		SquashX = 1.f - Stretch * 0.5f;
	}
	else
	{
		const float Squash = FMath::Clamp(SimState.Deflection / FMath::Max(Tuning.MaxDeflection, 1.f), 0.f, 1.f) * 0.35f;
		SquashZ = 1.f - Squash;
		SquashX = 1.f + Squash * 0.7f;
	}

	// 좌우로 움직이면 그쪽을 본다
	if (FMath::Abs(SimState.Velocity.X) > 50.f)
	{
		Facing = SimState.Velocity.X > 0.f ? 1.f : -1.f;
	}

	Body->SetRelativeScale3D(FVector(BodyBaseScale * SquashX * Facing, BodyBaseScale, BodyBaseScale * SquashZ));
	// 발이 로프(액터 위치)에 닿도록 중심을 올린다
	Body->SetRelativeLocation(FVector(0.f, BodyDepth, BodySpriteHeight * 0.5f * SquashZ));
}

void AYJJJumper::AddPopup(const FVector& Location, const FString& Text, const FLinearColor& Color, float Scale)
{
	FYJJPopup& Popup = Popups.AddDefaulted_GetRef();
	Popup.WorldLocation = Location;
	Popup.Text = Text;
	Popup.Color = Color;
	Popup.Scale = Scale;
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

	GEngine->AddOnScreenDebugMessage(1001, 0.f, FColor::White, FString::Printf(TEXT("Mode %s  Launch %.0f  Deflection %.0f  Chain %d"),
		*UEnum::GetDisplayValueAsText(SimState.Mode).ToString(), SimState.LastLaunchSpeed, SimState.Deflection, SimState.PerfectChain));

	// 드래그 중이면 예상 발사 방향 표시
	if (SimState.Mode == EYJJJumperMode::Dragging)
	{
		const float UpSpeed = (SimState.Deflection - YJJRopeSim::RestDeflection(Tuning)) * FMath::Sqrt(Tuning.RopeStiffness);
		const FVector From = GetActorLocation();
		const FVector Dir = FVector(SimState.PendingHorizontalSpeed, 0.f, UpSpeed) * 0.15f;
		DrawDebugDirectionalArrow(GetWorld(), From, From + Dir, 60.f, FColor::Cyan, false, 0.f, 0, 6.f);
	}
}

float AYJJJumper::ChooseAutoAim() const
{
	// 지금보다 높은 로프 중 가장 가까운 것의 중앙을 노린다
	const UYJJRopeSubsystem* Ropes = GetRopes();
	if (!Ropes)
	{
		return 0.f;
	}

	const FYJJRopeSegment* Target = nullptr;
	for (const FYJJRopeSegment& Rope : Ropes->GetSegments())
	{
		if (Rope.Height > SimState.Position.Y + 50.f && (!Target || Rope.Height < Target->Height))
		{
			Target = &Rope;
		}
	}
	if (!Target)
	{
		return 0.f;
	}

	const float TargetX = (Target->MinX + Target->MaxX) * 0.5f;
	return FMath::Clamp((TargetX - SimState.Position.X) / 700.f, -1.f, 1.f);
}

void AYJJJumper::TickAutoPlay(float DeltaSeconds)
{
	AutoPlayTime += DeltaSeconds;

	if (SimState.Mode == EYJJJumperMode::Resting)
	{
		// 끝까지 당기고 목표 쪽으로 발사 (드래그 픽셀 -> 좌우 속도는 반대 방향)
		const float Aim = ChooseAutoAim();
		const float DragX = -Aim * Tuning.MaxHorizontalSpeed / FMath::Max(Tuning.DragPixelsToHorizontalSpeed, 0.01f);
		if (YJJRopeSim::BeginDrag(SimState))
		{
			YJJRopeSim::UpdateDrag(SimState, Tuning, FVector2f(DragX, 1000.f));
			YJJRopeSim::ReleaseDrag(SimState, Tuning);
		}
	}
	else if (SimState.Mode == EYJJJumperMode::OnRope && SimState.bReachedBottom && !SimState.bGraded)
	{
		// 최저점 직후 프레임에 입력 -> 퍼펙트
		FYJJSimEvents Events;
		YJJRopeSim::PressBounce(SimState, Tuning, ChooseAutoAim(), Events);
		HandleEvents(Events);
	}

	if (AutoPlayTime >= AutoPlayNextReport)
	{
		AutoPlayNextReport += 2.f;
		UE_LOG(LogYJJJumper, Display, TEXT("[AutoPlay] t=%.0f height=%.1fm best=%.1fm score=%d kills=%d chain=%d mode=%s mobs=%d"),
			AutoPlayTime, GetHeightMeters(), GetBestHeightMeters(), SimState.Score, SimState.Kills, SimState.PerfectChain,
			*UEnum::GetValueAsString(SimState.Mode), GetMobs() ? GetMobs()->Num() : 0);
	}

	// 정해진 시간 동안 스크린샷 3장을 남기고 종료
	const float Duration = CVarAutoPlayDuration.GetValueOnGameThread();
	if (Duration > 0.f)
	{
		const float ShotTimes[] = { Duration * 0.15f, Duration * 0.5f, Duration * 0.95f };
		if (AutoPlayShotIndex < UE_ARRAY_COUNT(ShotTimes) && AutoPlayTime >= ShotTimes[AutoPlayShotIndex])
		{
			const FString Path = FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/AutoPlay_%d.png"), AutoPlayShotIndex);
			FScreenshotRequest::RequestScreenshot(Path, true, false);
			UE_LOG(LogYJJJumper, Display, TEXT("[AutoPlay] screenshot %s"), *Path);
			AutoPlayShotIndex++;
		}
		if (AutoPlayTime >= Duration + 1.f)
		{
			UE_LOG(LogYJJJumper, Display, TEXT("[AutoPlay] finished best=%.1fm score=%d kills=%d"), GetBestHeightMeters(), SimState.Score, SimState.Kills);
			FPlatformMisc::RequestExit(false, TEXT("yjj.AutoPlayDuration"));
		}
	}
}
