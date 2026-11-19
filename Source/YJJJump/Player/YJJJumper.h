#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Combat/YJJCombatTypes.h"
#include "HAL/IConsoleManager.h"
#include "Rope/YJJRopeTypes.h"
#include "YJJJumper.generated.h"

class UCameraComponent;
class UPaperSprite;
class UPaperSpriteComponent;
class UInputAction;
class UInputMappingContext;
class UYJJRopeSubsystem;
class UYJJMobSubsystem;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FYJJOnBounceGraded, EYJJBounceGrade, Grade, int32, PerfectChain);

// 튜토리얼: Lv 999Gp 장수풍뎅이로 조작을 배운 뒤 대머리 독수리에게 레벨을 털려 Lv 1이 된다
UENUM(BlueprintType)
enum class EYJJTutorialStep : uint8
{
	Drag,			// 줄을 당겼다 놓기
	TapMob,			// 몬스터 탭해서 들이받기
	Perfect,		// 줄에 닿는 순간 탭
	LevelStolen,	// 다음 착지 때 레벨 털림 연출
	Done,
};

// 화면에 잠깐 떠오르는 글자 (판정, 점수 등). HUD가 그린다
struct FYJJPopup
{
	FVector WorldLocation = FVector::ZeroVector;
	FString Text;
	FLinearColor Color = FLinearColor::White;
	float Scale = 1.f;
	float Age = 0.f;
	float Lifetime = 0.f;
};

// 로프 하나로 튕겨 오르내리며 몬스터를 탭해서 들이받는 장수풍뎅이.
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

	UFUNCTION(BlueprintPure, Category = "Jumper")
	float GetAttackRange() const;

	// 시작 지점 기준 높이 (m)
	UFUNCTION(BlueprintPure, Category = "Jumper")
	float GetHeightMeters() const;

	UFUNCTION(BlueprintPure, Category = "Jumper")
	float GetBestHeightMeters() const;

	UFUNCTION(BlueprintPure, Category = "Jumper")
	float GetSpeedKmh() const;

	UFUNCTION(BlueprintPure, Category = "Jumper")
	EYJJTutorialStep GetTutorialStep() const { return TutorialStep; }

	// 튜토리얼 끝 "레벨 털림" 배너가 떠 있는 남은 시간
	float GetLevelStolenBannerTime() const { return LevelStolenBannerTime; }

	int32 GetXPToNextLevel() const;

	const TArray<FYJJPopup>& GetPopups() const { return Popups; }

	// 돌진/타격 중 (이때는 상승·하강 연출을 끈다)
	bool IsAttacking() const { return SimState.IsDashing() || DashPoseTime > 0.f || HitStopTime > 0.f; }

	// 드래그 중이면 놓았을 때의 예상 발사 속도 (X, Z)
	bool GetLaunchPreview(FVector2f& OutVelocity) const;

	UPROPERTY(BlueprintAssignable, Category = "Jumper")
	FYJJOnBounceGraded OnBounceGraded;

	// 시뮬레이션 튜닝. BeginPlay에서 Tuning.csv(Rope.*, Combat.*) 값을 복사한다 (서버/클라가 같은 값)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Jumper")
	FYJJRopeTuning Tuning;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Jumper")
	FYJJCombatTuning CombatTuning;

	UPROPERTY(EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UPaperSprite> BodySprite;

	// 뿔 돌진 포즈
	UPROPERTY(EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UPaperSprite> DashSprite;

	UPROPERTY(EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UPaperSprite> BackgroundSprite;

	// 화면 왼쪽 위 디버그 텍스트 (HUD와 별개)
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebugText = false;

private:
	void EnsureInputObjects();
	void OnPressStarted(const FInputActionValue& Value);
	void OnPressCompleted(const FInputActionValue& Value);

	bool GetPointerPosition(FVector2f& OutPosition) const;
	bool ScreenToGame(const FVector2f& Screen, FVector2f& OutGame) const;
	float ComputeAimX() const;

	// 탭한 지점의 몬스터에게 돌진 시도. 몬스터를 가리켰으면 true (사거리 밖이어도)
	bool TryTapAttack(const FVector2f& TapGamePosition);
	bool StartDash(const FYJJMobTarget& Target);

	void TickSimulation(float DeltaSeconds);
	void HandleEvents(const FYJJSimEvents& Events);
	void FinishDash();
	void RecoverFromFall();
	void UpdateTutorial(const FYJJSimEvents& Events);
	void ApplyLevelStolen();

	void SyncVisuals(float DeltaSeconds);
	void AnimateBody();
	// 화면 비율에 맞춰 탑 너비와 세로 시야를 둘 다 보장. 보이는 영역(cm)을 돌려준다
	FVector2f UpdateCameraFraming(float DeltaSeconds);
	void AddShake(float Strength);
	void TickPopups(float DeltaSeconds);
	void AddPopup(const FVector& Location, const FString& Text, const FLinearColor& Color, float Scale, float Lifetime);
	FVector GameToWorld(const FVector2f& Game, float Depth) const;
	void DrawDebugText() const;

	UYJJRopeSubsystem* GetRopes() const;
	UYJJMobSubsystem* GetMobs() const;

	// 테스트용 자동 플레이 (yjj.AutoPlay). 드래그 발사, 몬스터 탭, 퍼펙트 타이밍을 대신 입력한다
	void TickAutoPlay(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Jumper")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Jumper")
	TObjectPtr<UPaperSpriteComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UPaperSpriteComponent> Background;

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

	EYJJTutorialStep TutorialStep = EYJJTutorialStep::Done;
	float LevelStolenBannerTime = 0.f;
	float HitStopTime = 0.f;

	// 돌진 연출
	void UpdateTrail();

	UPROPERTY(Transient)
	TObjectPtr<UPaperSprite> BodySpriteAsset;

	UPROPERTY(Transient)
	TObjectPtr<UPaperSprite> DashSpriteAsset;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TArray<TObjectPtr<UPaperSpriteComponent>> Ghosts;

	TArray<FTransform> TrailHistory;
	FVector2f LastDashDir = FVector2f(1.f, 0.f);
	float DashBaseScale = 1.f;
	float DashPoseTime = 0.f;	// 돌진이 끝난 뒤에도 잠깐 포즈 유지 (타격 순간)
	float TrailFade = 0.f;

	float StepAccumulator = 0.f;
	float CenterX = 0.f;
	float PlaneY = 0.f;
	float StartHeight = 0.f;
	float CameraZ = 0.f;
	float CameraZoom = 1.f;
	float ShakeTime = 0.f;
	float ShakeStrength = 0.f;
	float BodyBaseScale = 1.f;
	float BodySpriteHeight = 0.f;
	float Facing = 1.f;
	FVector2f BackgroundSpriteSize = FVector2f::ZeroVector;

	bool bDragging = false;
	FVector2f DragStartPixels = FVector2f::ZeroVector;

	int32 VisualRopeId = INDEX_NONE;

	float AutoPlayTime = 0.f;
	float AutoPlayNextReport = 0.f;
	float AutoPlayNextTap = 0.f;
	int32 AutoPlayShotIndex = 0;

	// 돌진 잔상 컴포넌트 수 (생성자에서 만들어서 데이터 테이블 대신 상수)
	static constexpr int32 GhostCount = 4;

	static TAutoConsoleVariable<int32> CVarAutoPlay;
	static TAutoConsoleVariable<float> CVarAutoPlayDuration;
	// 튜토리얼은 기획 보류 중이라 기본은 꺼져 있고 Lv 1부터 시작한다
	static TAutoConsoleVariable<int32> CVarTutorial;
};
