#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Combat/YJJCombatTypes.h"
#include "Rope/YJJRopeTypes.h"
#include "YJJJumper.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UPaperSprite;
class UPaperSpriteComponent;
class UInputAction;
class UInputMappingContext;
class UYJJRopeSubsystem;
class UYJJMobSubsystem;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FYJJOnBounceGraded, EYJJBounceGrade, Grade, int32, PerfectChain);

// 화면에 잠깐 떠오르는 글자 (판정, 점수 등). HUD가 그린다
struct FYJJPopup
{
	FVector WorldLocation = FVector::ZeroVector;
	FString Text;
	FLinearColor Color = FLinearColor::White;
	float Scale = 1.f;
	float Age = 0.f;
	float Lifetime = 0.8f;
};

// 로프 반동으로 점프하며 몹을 사냥하는 플레이어.
// 이동/전투는 물리 엔진 없이 YJJRopeSim, YJJCombat 고정 스텝 계산으로만 한다.
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

	UFUNCTION(BlueprintPure, Category = "Jumper")
	float GetAttackPower() const;

	// 시작 지점 기준 높이 (m)
	UFUNCTION(BlueprintPure, Category = "Jumper")
	float GetHeightMeters() const;

	UFUNCTION(BlueprintPure, Category = "Jumper")
	float GetBestHeightMeters() const;

	const TArray<FYJJPopup>& GetPopups() const { return Popups; }

	// 드래그 중이면 놓았을 때의 예상 발사 속도 (X, Z)
	bool GetLaunchPreview(FVector2f& OutVelocity) const;

	UPROPERTY(BlueprintAssignable, Category = "Jumper")
	FYJJOnBounceGraded OnBounceGraded;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jumper")
	FYJJRopeTuning Tuning;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jumper")
	FYJJCombatTuning CombatTuning;

	// 가장 낮은 로프보다 이만큼 떨어지면 마지막 로프로 복귀
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jumper")
	float FallRecoverDistance = 2000.f;

	// 캐릭터 기준 이 비율(뷰포트 너비)만큼 옆을 누르면 좌우 최대 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jumper")
	float FullAimScreenRatio = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UPaperSprite> BodySprite;

	UPROPERTY(EditAnywhere, Category = "Visual")
	float BodyWidth = 110.f;

	UPROPERTY(EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UPaperSprite> BackgroundSprite;

	// 최소한 이만큼 위아래가 보인다 (cm)
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraViewHeight = 2400.f;

	// 탑 양옆 벽 바깥으로 더 보여줄 여백 (cm)
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraSideMargin = 150.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraLookAhead = 300.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraFollowSpeed = 5.f;

	// 화면 왼쪽 위 디버그 텍스트 (HUD와 별개)
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebugText = false;

private:
	void EnsureInputObjects();
	void OnPressStarted(const FInputActionValue& Value);
	void OnPressCompleted(const FInputActionValue& Value);

	bool GetPointerPosition(FVector2f& OutPosition) const;
	float ComputeAimX() const;

	void TickSimulation(float DeltaSeconds);
	void HandleEvents(const FYJJSimEvents& Events);
	void ResolveMobHits(TArray<FYJJMobTarget>& Targets);
	void RecoverFromFall();
	void SyncVisuals(float DeltaSeconds);
	// 화면 비율에 맞춰 탑 너비와 세로 시야를 둘 다 보장. 보이는 영역(cm)을 돌려준다
	FVector2f UpdateCameraFraming();
	void AnimateBody();
	void TickPopups(float DeltaSeconds);
	void AddPopup(const FVector& Location, const FString& Text, const FLinearColor& Color, float Scale = 1.f);
	void DrawDebugText() const;

	// 테스트용 자동 플레이 (yjj.AutoPlay). 드래그 발사와 퍼펙트 타이밍을 대신 입력한다
	void TickAutoPlay(float DeltaSeconds);
	float ChooseAutoAim() const;
	float AutoPlayTime = 0.f;
	float AutoPlayNextReport = 0.f;
	int32 AutoPlayShotIndex = 0;

	UYJJRopeSubsystem* GetRopes() const;
	UYJJMobSubsystem* GetMobs() const;

	UPROPERTY(VisibleAnywhere, Category = "Jumper")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Jumper")
	TObjectPtr<UPaperSpriteComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UPaperSpriteComponent> Background;

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

	TArray<FYJJPopup> Popups;
	TMap<int32, double> LastHitTimeByMob;

	float StepAccumulator = 0.f;
	float CenterX = 0.f;
	float PlaneY = 0.f;
	float StartHeight = 0.f;
	float CameraZ = 0.f;
	float BodyBaseScale = 1.f;
	float BodySpriteHeight = 0.f;
	float Facing = 1.f;
	FVector2f BackgroundSpriteSize = FVector2f::ZeroVector;

	bool bDragging = false;
	FVector2f DragStartPixels = FVector2f::ZeroVector;

	int32 VisualRopeId = INDEX_NONE;
};
