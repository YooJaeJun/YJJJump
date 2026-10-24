#include "Player/YJJumper.h"
#include "Rope/YJRope.h"
#include "Rope/YJRopeSimulation.h"
#include "Rope/YJRopeSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

AYJJumper::AYJJumper()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetRelativeScale3D(FVector(0.6f));
	Body->SetRelativeLocation(FVector(0.f, 0.f, 30.f));	// 발이 로프에 닿도록
	if (SphereMesh.Succeeded())
	{
		Body->SetStaticMesh(SphereMesh.Object);
	}

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
	Camera->SetOrthoWidth(CameraOrthoWidth);

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void AYJJumper::BeginPlay()
{
	Super::BeginPlay();

	const FVector3f Start(GetActorLocation());
	CenterX = Start.X;
	PlaneY = Start.Y;
	StartHeight = Start.Z;
	CameraZ = Start.Z + CameraLookAhead;
	Camera->SetOrthoWidth(CameraOrthoWidth);

	// 시작은 공중. 아래 로프에 떨어져 자연스럽게 멈춘다
	SimState = FYJJumperState();
	SimState.Position = FVector2f(Start.X, Start.Z);
	SimState.MaxHeightReached = Start.Z;
}

void AYJJumper::EnsureInputObjects()
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

void AYJJumper::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	EnsureInputObjects();
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(PressAction, ETriggerEvent::Started, this, &AYJJumper::OnPressStarted);
		Input->BindAction(PressAction, ETriggerEvent::Completed, this, &AYJJumper::OnPressCompleted);
	}
}

void AYJJumper::NotifyControllerChanged()
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

void AYJJumper::OnPressStarted(const FInputActionValue& Value)
{
	if (YJRopeSim::BeginDrag(SimState))
	{
		bDragging = GetPointerPosition(DragStartPixels);
		if (!bDragging)
		{
			// 포인터가 없으면 (키보드) 드래그 불가
			SimState.Mode = EYJJumperMode::Resting;
		}
		return;
	}

	FYJSimEvents Events;
	YJRopeSim::PressBounce(SimState, Tuning, ComputeAimX(), Events);
	HandleEvents(Events);
}

void AYJJumper::OnPressCompleted(const FInputActionValue& Value)
{
	if (bDragging)
	{
		bDragging = false;
		YJRopeSim::ReleaseDrag(SimState, Tuning);
	}
}

bool AYJJumper::GetPointerPosition(FVector2f& OutPosition) const
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

float AYJJumper::ComputeAimX() const
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

void AYJJumper::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDragging)
	{
		FVector2f Pointer;
		if (GetPointerPosition(Pointer))
		{
			YJRopeSim::UpdateDrag(SimState, Tuning, Pointer - DragStartPixels);
		}
	}

	TickSimulation(DeltaSeconds);
	RecoverFromFall();
	SyncVisuals(DeltaSeconds);

	if (bShowDebugHud)
	{
		DrawDebugHud();
	}
}

UYJRopeSubsystem* AYJJumper::GetRopes() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UYJRopeSubsystem>() : nullptr;
}

void AYJJumper::TickSimulation(float DeltaSeconds)
{
	const UYJRopeSubsystem* Ropes = GetRopes();
	if (!Ropes || !Ropes->HasRopes())
	{
		return;
	}

	// 프레임레이트와 무관하게 같은 결과가 나오도록 고정 스텝
	StepAccumulator = FMath::Min(StepAccumulator + DeltaSeconds, 0.25f);
	while (StepAccumulator >= YJRopeSim::FixedStep)
	{
		StepAccumulator -= YJRopeSim::FixedStep;

		FYJSimEvents Events;
		YJRopeSim::Step(SimState, Tuning, Ropes->GetSegments(), CenterX, Events);
		HandleEvents(Events);
	}
}

void AYJJumper::HandleEvents(const FYJSimEvents& Events)
{
	if (Events.bGraded)
	{
		OnBounceGraded.Broadcast(Events.Grade, SimState.PerfectChain);

		if (bShowDebugHud && GEngine)
		{
			const FColor Color = Events.Grade == EYJBounceGrade::Perfect ? FColor::Yellow
				: Events.Grade == EYJBounceGrade::Good ? FColor::Green : FColor::Red;
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, Color,
				FString::Printf(TEXT("%s  x%d"), *UEnum::GetDisplayValueAsText(Events.Grade).ToString(), SimState.PerfectChain), true, FVector2D(2.f, 2.f));
		}
	}
}

void AYJJumper::RecoverFromFall()
{
	const UYJRopeSubsystem* Ropes = GetRopes();
	if (!Ropes || !Ropes->HasRopes() || SimState.Mode != EYJJumperMode::Airborne)
	{
		return;
	}

	if (SimState.Position.Y > Ropes->GetLowestHeight() - FallRecoverDistance)
	{
		return;
	}

	const FYJRopeSegment* Rope = YJRopeSim::FindRope(Ropes->GetSegments(), SimState.LastRopeId);
	if (!Rope)
	{
		Rope = &Ropes->GetSegments()[0];
	}
	YJRopeSim::PlaceOnRope(SimState, Tuning, *Rope, (Rope->MinX + Rope->MaxX) * 0.5f);
}

void AYJJumper::SyncVisuals(float DeltaSeconds)
{
	SetActorLocation(FVector(SimState.Position.X, PlaneY, SimState.Position.Y));

	// 로프 V자 처짐
	const bool bOnRope = SimState.Mode != EYJJumperMode::Airborne;
	const int32 ContactRopeId = bOnRope ? SimState.RopeId : INDEX_NONE;
	if (const UYJRopeSubsystem* Ropes = GetRopes())
	{
		if (VisualRopeId != ContactRopeId)
		{
			if (AYJRope* Previous = Ropes->FindRopeActor(VisualRopeId))
			{
				Previous->ClearContact();
			}
			VisualRopeId = ContactRopeId;
		}
		if (AYJRope* Current = Ropes->FindRopeActor(ContactRopeId))
		{
			Current->SetContact(SimState.Position.X, SimState.Deflection);
		}
	}

	// 카메라는 위쪽을 더 보이게 따라간다
	CameraZ = FMath::FInterpTo(CameraZ, SimState.Position.Y + CameraLookAhead, DeltaSeconds, CameraFollowSpeed);
	CameraArm->SetWorldLocation(FVector(CenterX, PlaneY, CameraZ));
}

void AYJJumper::DrawDebugHud() const
{
	if (!GEngine)
	{
		return;
	}

	const float HeightM = (SimState.Position.Y - StartHeight) / 100.f;
	const float BestM = (SimState.MaxHeightReached - StartHeight) / 100.f;
	GEngine->AddOnScreenDebugMessage(1001, 0.f, FColor::White, FString::Printf(TEXT("Height %.1fm  (Best %.1fm)"), HeightM, BestM));
	GEngine->AddOnScreenDebugMessage(1002, 0.f, FColor::White, FString::Printf(TEXT("Mode %s  Launch %.0f  Chain %d"),
		*UEnum::GetDisplayValueAsText(SimState.Mode).ToString(), SimState.LastLaunchSpeed, SimState.PerfectChain));

	// 드래그 중이면 예상 발사 방향 표시
	if (SimState.Mode == EYJJumperMode::Dragging)
	{
		const float UpSpeed = (SimState.Deflection - YJRopeSim::RestDeflection(Tuning)) * FMath::Sqrt(Tuning.RopeStiffness);
		const FVector From = GetActorLocation();
		const FVector Dir = FVector(SimState.PendingHorizontalSpeed, 0.f, UpSpeed) * 0.15f;
		DrawDebugDirectionalArrow(GetWorld(), From, From + Dir, 60.f, FColor::Cyan, false, 0.f, 0, 6.f);
	}
}
