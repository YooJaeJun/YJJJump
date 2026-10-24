#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Rope/YJJRopeTypes.h"
#include "YJJJumper.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class UYJJRopeSubsystem;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FYJJOnBounceGraded, EYJJBounceGrade, Grade, int32, PerfectChain);

// 로프 반동으로 점프하는 플레이어.
// 이동은 물리 엔진 없이 YJJRopeSim 고정 스텝 시뮬레이션으로만 한다.
UCLASS()
class YJJJUMP_API AYJJJumper : public APawn
{
	GENERATED_BODY()

public:
	AYJJJumper();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

	UFUNCTION(BlueprintPure, Category = "Jumper")
	const FYJJJumperState& GetSimState() const { return SimState; }

	UPROPERTY(BlueprintAssignable, Category = "Jumper")
	FYJJOnBounceGraded OnBounceGraded;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jumper")
	FYJJRopeTuning Tuning;

	// 가장 낮은 로프보다 이만큼 떨어지면 마지막 로프로 복귀
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jumper")
	float FallRecoverDistance = 2000.f;

	// 캐릭터 기준 이 비율(뷰포트 너비)만큼 옆을 누르면 좌우 최대 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jumper")
	float FullAimScreenRatio = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraOrthoWidth = 2400.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraLookAhead = 300.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraFollowSpeed = 5.f;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebugHud = true;

private:
	void EnsureInputObjects();
	void OnPressStarted(const FInputActionValue& Value);
	void OnPressCompleted(const FInputActionValue& Value);

	bool GetPointerPosition(FVector2f& OutPosition) const;
	float ComputeAimX() const;

	void TickSimulation(float DeltaSeconds);
	void HandleEvents(const FYJJSimEvents& Events);
	void RecoverFromFall();
	void SyncVisuals(float DeltaSeconds);
	void DrawDebugHud() const;

	UYJJRopeSubsystem* GetRopes() const;

	UPROPERTY(VisibleAnywhere, Category = "Jumper")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Jumper")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// 프로토타입이라 에셋 없이 런타임에 만든다. 나중에 에셋으로 교체
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PressAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> InputContext;

	UPROPERTY(VisibleInstanceOnly, Category = "Jumper")
	FYJJJumperState SimState;

	float StepAccumulator = 0.f;
	float CenterX = 0.f;
	float PlaneY = 0.f;
	float StartHeight = 0.f;
	float CameraZ = 0.f;

	bool bDragging = false;
	FVector2f DragStartPixels = FVector2f::ZeroVector;

	int32 VisualRopeId = INDEX_NONE;
};
