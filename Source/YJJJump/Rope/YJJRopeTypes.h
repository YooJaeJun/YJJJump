#pragma once

#include "CoreMinimal.h"
#include "YJJRopeTypes.generated.h"

// 게임 평면: X = 좌우, Z = 위아래 (Y는 깊이, 사용하지 않음)

UENUM(BlueprintType)
enum class EYJJJumperMode : uint8
{
	Airborne,	// 공중
	OnRope,		// 로프 위에서 출렁이는 중
	Resting,	// 로프 위에 멈춤 - 드래그 가능
	Dragging,	// 드래그로 로프를 당기는 중
};

UENUM(BlueprintType)
enum class EYJJBounceGrade : uint8
{
	None,
	Miss,
	Good,
	Perfect,
};

// 반동 손맛 튜닝 값. 단위: cm, 초
USTRUCT(BlueprintType)
struct FYJJRopeTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float Gravity = 2400.f;

	// 로프 스프링 강성 (1/s^2). 발사 속도 ≈ 처짐 * sqrt(강성)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope")
	float RopeStiffness = 180.f;

	// 로프 감쇠 (1/s). 클수록 튕길 때마다 에너지를 많이 잃음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope")
	float RopeDamping = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope")
	float MaxDeflection = 260.f;

	// 로프 위에서 좌우 속도 감쇠 (1/s)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope")
	float RopeHorizontalFriction = 8.f;

	// 이 속도/처짐 이하로 떨어지면 정지 상태로 전환
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope")
	float RestSpeedThreshold = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope")
	float RestDeflectionTolerance = 6.f;

	// 드래그: 화면 픽셀 -> 로프 처짐(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drag")
	float DragPixelsToDeflection = 0.6f;

	// 드래그: 좌우 픽셀 -> 발사 좌우 속도(cm/s). 당긴 반대 방향으로 날아감
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drag")
	float DragPixelsToHorizontalSpeed = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drag")
	float MaxHorizontalSpeed = 1400.f;

	// 최저점 기준 판정 윈도우 (초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	float PerfectWindow = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	float GoodWindow = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	float PerfectMultiplier = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	float GoodMultiplier = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	float MissMultiplier = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	float MaxLaunchSpeed = 400000.f;

	// 속도감: 발사 속도가 이 값의 몇 배인지(SpeedScale)만큼 중력은 s배, 로프 강성은 s^2배.
	// 한 번 비행하는 시간은 거의 그대로인데 속도와 높이는 s배로 커져서 점점 미친 듯이 빨라진다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speed")
	float SpeedReference = 2500.f;

	// 이번 비행에서 잡은 몬스터 1마리당 다음 발사 속도 보너스 (최대 KillComboMax마리)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speed")
	float KillComboLaunchBonus = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speed")
	int32 KillComboMax = 10;

	// 좌우 벽 (중심 X 기준 반폭). 벽에 부딪히면 튕겨 나옴
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World")
	float WallHalfWidth = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World")
	float WallRestitution = 0.6f;
};

// 수평 로프 한 줄
struct FYJJRopeSegment
{
	int32 Id = INDEX_NONE;
	float MinX = 0.f;
	float MaxX = 0.f;
	float Height = 0.f;
};

// 점퍼 한 명의 시뮬레이션 상태. 서버/클라 모두 이 값으로 같은 결과를 낸다
USTRUCT(BlueprintType)
struct FYJJJumperState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EYJJJumperMode Mode = EYJJJumperMode::Airborne;

	UPROPERTY(BlueprintReadOnly)
	FVector2f Position = FVector2f::ZeroVector;	// (X, Z)

	UPROPERTY(BlueprintReadOnly)
	FVector2f Velocity = FVector2f::ZeroVector;

	// 로프 접촉 정보 (OnRope/Resting/Dragging)
	UPROPERTY(BlueprintReadOnly)
	int32 RopeId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	float RopeHeight = 0.f;

	// 아래로 처진 양 (+) 과 그 속도
	UPROPERTY(BlueprintReadOnly)
	float Deflection = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float DeflectionSpeed = 0.f;

	// 시뮬레이션 시간 (초)
	UPROPERTY(BlueprintReadOnly)
	double SimTime = 0.0;

	// 타이밍 판정
	double LastPressTime = -1.0;	// 아직 판정에 쓰이지 않은 입력
	double BottomTime = -1.0;		// 이번 접촉의 최저점 시각
	bool bReachedBottom = false;
	bool bGraded = false;			// 이번 반동에서 판정이 끝났는지
	float PendingLaunchMultiplier = 1.f;
	float PendingHorizontalSpeed = 0.f;
	bool bHasPendingHorizontal = false;

	UPROPERTY(BlueprintReadOnly)
	EYJJBounceGrade LastGrade = EYJJBounceGrade::None;

	UPROPERTY(BlueprintReadOnly)
	int32 PerfectChain = 0;

	UPROPERTY(BlueprintReadOnly)
	float LastLaunchSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float MaxHeightReached = 0.f;

	// 마지막으로 착지한 로프 (낙하 시 복귀용)
	int32 LastRopeId = INDEX_NONE;

	// 이번 반동(착지~발사)의 판정, 그리고 마지막 발사에 적용된 판정 (공격력 계산용)
	EYJJBounceGrade CurrentBounceGrade = EYJJBounceGrade::None;

	UPROPERTY(BlueprintReadOnly)
	EYJJBounceGrade LaunchGrade = EYJJBounceGrade::None;

	// 속도 배율 (발사 속도 / SpeedReference, 최소 1). 중력과 로프를 이만큼 빠르게 돌린다
	UPROPERTY(BlueprintReadOnly)
	float SpeedScale = 1.f;

	// 이번 비행에서 잡은 몬스터 수 (다음 발사 보너스)
	UPROPERTY(BlueprintReadOnly)
	int32 KillCombo = 0;

	// 전투 결과 (서버 권한 값)
	UPROPERTY(BlueprintReadOnly)
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Kills = 0;

	// 성장. 튜토리얼 동안은 Lv 999Gp(구골플렉스) 장수풍뎅이
	UPROPERTY(BlueprintReadOnly)
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly)
	int32 XP = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bTutorial = false;

	// 탭한 몬스터에게 뿔 돌진 중 (돌진 중엔 중력 정지, 끝나면 원래 속도로 계속 비행)
	int32 DashMobId = INDEX_NONE;
	float DashTimeLeft = 0.f;
	float DashDuration = 0.f;
	FVector2f DashFrom = FVector2f::ZeroVector;
	FVector2f DashTo = FVector2f::ZeroVector;
	FVector2f DashSavedVelocity = FVector2f::ZeroVector;

	bool IsDashing() const { return DashMobId != INDEX_NONE; }
};

// 한 스텝에서 일어난 일 (연출/UI용)
struct FYJJSimEvents
{
	bool bLanded = false;
	bool bLaunched = false;
	bool bRested = false;
	bool bGraded = false;
	EYJJBounceGrade Grade = EYJJBounceGrade::None;
};
