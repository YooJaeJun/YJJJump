#pragma once

#include "CoreMinimal.h"
#include "Combat/YJJCombatTypes.h"
#include "Engine/DataTable.h"
#include "YJJDataTypes.generated.h"

class UPaperSprite;

// 데이터 테이블(Content/YJJJump/Data/*.csv)이 채우는 구조체들.
// 값은 전부 CSV에 있고, 여기 기본값은 0 (빠진 값이 눈에 띄도록). 각 값의 뜻은 CSV의 Note 열에 적는다.
// Tuning.csv 의 이름은 "구역.필드" (예: Camera.ViewHeight), Colors.csv 는 필드 이름 그대로.

// Tuning.csv / Colors.csv 한 줄
USTRUCT()
struct FYJJKeyValueRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString Value;

	UPROPERTY(EditAnywhere)
	FString Note;
};

// Mobs.csv 한 줄 (행 이름 = 몬스터 ID)
USTRUCT()
struct FYJJMobRow : public FTableRowBase
{
	GENERATED_BODY()

	// Localization.csv 키
	UPROPERTY(EditAnywhere)
	FString NameKey;

	UPROPERTY(EditAnywhere)
	float MaxHealth = 0.f;

	// 히트 판정 반지름 (cm)
	UPROPERTY(EditAnywhere)
	float Radius = 0.f;

	UPROPERTY(EditAnywhere)
	int32 ScoreValue = 0;

	UPROPERTY(EditAnywhere)
	int32 XPValue = 0;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UPaperSprite> Sprite;

	// 날갯짓 두 번째 프레임 (Sprite와 번갈아 보여준다)
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UPaperSprite> FlapSprite;

	UPROPERTY(EditAnywhere)
	float FlapInterval = 0.f;

	// 처치당한 프레임 (X 눈, 혹 등)
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UPaperSprite> HitSprite;

	// 처치될 때 흩날리는 깃털 수
	UPROPERTY(EditAnywhere)
	int32 DeathFeathers = 0;

	// 화면에 보이는 너비 (cm)
	UPROPERTY(EditAnywhere)
	float SpriteWidth = 0.f;

	UPROPERTY(EditAnywhere)
	EYJJMobMovement Movement = EYJJMobMovement::Static;

	// 왕복 거리의 절반 (cm)
	UPROPERTY(EditAnywhere)
	float MoveRange = 0.f;

	// 왕복 1회 시간 (초)
	UPROPERTY(EditAnywhere)
	float MovePeriod = 0.f;

	UPROPERTY(EditAnywhere)
	FString Note;
};

// Spawns.csv 한 줄: 구간(바이옴)마다 나오는 몬스터와 가중치
USTRUCT()
struct FYJJSpawnRow : public FTableRowBase
{
	GENERATED_BODY()

	// 구간 이름 (지금은 Sky만. 우주, 천국 ... 이 늘어난다)
	UPROPERTY(EditAnywhere)
	FName Biome;

	// Mobs.csv 행 이름
	UPROPERTY(EditAnywhere)
	FName Mob;

	UPROPERTY(EditAnywhere)
	float Weight = 0.f;

	// 시작 지점에서 이 높이(cm) 이상부터 등장
	UPROPERTY(EditAnywhere)
	float MinHeight = 0.f;

	UPROPERTY(EditAnywhere)
	FString Note;
};

// HudLayout.csv 한 줄: HUD 요소 하나의 배치 (세로 1280 / 가로 720 기준 픽셀)
// 어느 모서리 기준인지는 요소마다 다르다 (Note 열 참고)
USTRUCT()
struct FYJJHudLayoutRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float X = 0.f;
	UPROPERTY(EditAnywhere) float Y = 0.f;
	UPROPERTY(EditAnywhere) float W = 0.f;
	UPROPERTY(EditAnywhere) float H = 0.f;
	// 반복되는 요소 사이 간격
	UPROPERTY(EditAnywhere) float Gap = 0.f;
	// 패널 안쪽 여백
	UPROPERTY(EditAnywhere) float Inset = 0.f;
	UPROPERTY(EditAnywhere) float IconSize = 0.f;
	UPROPERTY(EditAnywhere) float IconX = 0.f;
	UPROPERTY(EditAnywhere) float IconY = 0.f;
	UPROPERTY(EditAnywhere) float TextX = 0.f;
	UPROPERTY(EditAnywhere) float TextY = 0.f;
	// 글자 크기 (1080p 기준 포인트)
	UPROPERTY(EditAnywhere) float TextSize = 0.f;
	UPROPERTY(EditAnywhere) float Outline = 0.f;
	// 화면 높이 비율 위치 (0~1, 쓰는 요소만)
	UPROPERTY(EditAnywhere) float ScreenY = 0.f;

	UPROPERTY(EditAnywhere)
	FString Note;
};

// Tuning.csv "Player.*": 장수풍뎅이 조작과 연출
USTRUCT()
struct FYJJPlayerTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float FallRecoverDistance = 0.f;
	UPROPERTY(EditAnywhere) float FullAimScreenRatio = 0.f;
	UPROPERTY(EditAnywhere) float MaxStepAccumulation = 0.f;
	UPROPERTY(EditAnywhere) int32 LaunchPreviewMaxSteps = 0;
	UPROPERTY(EditAnywhere) int32 TutorialKillsNeeded = 0;
	UPROPERTY(EditAnywhere) float LevelStolenBannerTime = 0.f;

	// 몸
	UPROPERTY(EditAnywhere) float BodyWidth = 0.f;
	UPROPERTY(EditAnywhere) float BodyDepth = 0.f;
	UPROPERTY(EditAnywhere) float EffectDepth = 0.f;
	UPROPERTY(EditAnywhere) float FacingSpeedThreshold = 0.f;
	UPROPERTY(EditAnywhere) float FacingDashThreshold = 0.f;
	UPROPERTY(EditAnywhere) float AirStretchSpeed = 0.f;
	UPROPERTY(EditAnywhere) float AirStretchMax = 0.f;
	UPROPERTY(EditAnywhere) float AirSquashRatio = 0.f;
	UPROPERTY(EditAnywhere) float RopeSquashMax = 0.f;
	UPROPERTY(EditAnywhere) float RopeSquashWidenRatio = 0.f;

	// 돌진 포즈와 잔상
	UPROPERTY(EditAnywhere) float DashPoseScale = 0.f;
	UPROPERTY(EditAnywhere) float DashPoseHoldTime = 0.f;
	UPROPERTY(EditAnywhere) float DashStretchStart = 0.f;
	UPROPERTY(EditAnywhere) float DashStretchEnd = 0.f;
	UPROPERTY(EditAnywhere) int32 TrailHistoryLength = 0;
	UPROPERTY(EditAnywhere) float TrailFadeTime = 0.f;
	UPROPERTY(EditAnywhere) int32 GhostSampleStride = 0;
	UPROPERTY(EditAnywhere) float GhostDepthStep = 0.f;
	UPROPERTY(EditAnywhere) float GhostAlphaStart = 0.f;
	UPROPERTY(EditAnywhere) float GhostAlphaStep = 0.f;

	// 돌진 출발 먼지
	UPROPERTY(EditAnywhere) float DustFrameTime = 0.f;
	UPROPERTY(EditAnywhere) float DustLifetime = 0.f;
	UPROPERTY(EditAnywhere) float DustWidth = 0.f;
	UPROPERTY(EditAnywhere) float DustStartScale = 0.f;
	UPROPERTY(EditAnywhere) float DustEndScale = 0.f;
	UPROPERTY(EditAnywhere) float DustSpeed = 0.f;
	UPROPERTY(EditAnywhere) float DustDepthOffset = 0.f;

	// 타격
	UPROPERTY(EditAnywhere) float HitStopDuration = 0.f;
	UPROPERTY(EditAnywhere) float LevelUpEffectHeight = 0.f;
	UPROPERTY(EditAnywhere) float LevelUpEffectWidth = 0.f;
	UPROPERTY(EditAnywhere) float HitEffectWidth = 0.f;

	// 화면 흔들림 세기
	UPROPERTY(EditAnywhere) float KillShake = 0.f;
	UPROPERTY(EditAnywhere) float LevelUpShake = 0.f;
	UPROPERTY(EditAnywhere) float HitShake = 0.f;
	UPROPERTY(EditAnywhere) float PerfectShake = 0.f;
	UPROPERTY(EditAnywhere) float LevelStolenShake = 0.f;

	// 떠오르는 글자
	UPROPERTY(EditAnywhere) int32 MaxPopups = 0;
	UPROPERTY(EditAnywhere) float PopupRiseSpeed = 0.f;
	UPROPERTY(EditAnywhere) float PopupLifetime = 0.f;
	UPROPERTY(EditAnywhere) float GradePopupHeight = 0.f;
	UPROPERTY(EditAnywhere) float LevelUpPopupHeight = 0.f;
	UPROPERTY(EditAnywhere) float KillPopupScale = 0.f;
	UPROPERTY(EditAnywhere) float LevelUpPopupScale = 0.f;
	UPROPERTY(EditAnywhere) float LevelUpPopupLifetime = 0.f;
	UPROPERTY(EditAnywhere) float HitPopupScale = 0.f;
	UPROPERTY(EditAnywhere) float PerfectPopupScale = 0.f;
	UPROPERTY(EditAnywhere) float GoodPopupScale = 0.f;
	UPROPERTY(EditAnywhere) float MissPopupScale = 0.f;
	UPROPERTY(EditAnywhere) float TooFarPopupScale = 0.f;
	UPROPERTY(EditAnywhere) float TooFarPopupLifetime = 0.f;
	UPROPERTY(EditAnywhere) int32 PerfectComboPopupMin = 0;
};

// Tuning.csv "Camera.*"
USTRUCT()
struct FYJJCameraTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float Distance = 0.f;
	UPROPERTY(EditAnywhere) float ViewHeight = 0.f;
	UPROPERTY(EditAnywhere) float MaxZoomOut = 0.f;
	UPROPERTY(EditAnywhere) float ZoomPerSpeedDoubling = 0.f;
	UPROPERTY(EditAnywhere) float ZoomInterpSpeed = 0.f;
	UPROPERTY(EditAnywhere) float SideMargin = 0.f;
	UPROPERTY(EditAnywhere) float DefaultAspect = 0.f;
	UPROPERTY(EditAnywhere) float LookAheadTime = 0.f;
	UPROPERTY(EditAnywhere) float LookAheadMaxRatio = 0.f;
	UPROPERTY(EditAnywhere) float RestLookAheadRatio = 0.f;
	UPROPERTY(EditAnywhere) float FollowSpeed = 0.f;
	UPROPERTY(EditAnywhere) float ShakeDuration = 0.f;
	UPROPERTY(EditAnywhere) float ShakeAmplitudeRatio = 0.f;
	UPROPERTY(EditAnywhere) float ShakeCarryOver = 0.f;
	UPROPERTY(EditAnywhere) float BackgroundDepth = 0.f;
	UPROPERTY(EditAnywhere) float BackgroundCoverage = 0.f;
	// 배경 색이 하늘 -> 높은 하늘 -> 우주로 바뀌는 높이 (m)
	UPROPERTY(EditAnywhere) float SkyFadeMeters = 0.f;
	UPROPERTY(EditAnywhere) float SpaceFadeMeters = 0.f;
};

// Tuning.csv "Sound.*": 효과음 볼륨과 피치
USTRUCT()
struct FYJJSoundTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float BgmVolume = 0.f;
	UPROPERTY(EditAnywhere) float LaunchVolume = 0.f;
	UPROPERTY(EditAnywhere) float LaunchPitchBase = 0.f;
	UPROPERTY(EditAnywhere) float LaunchPitchPerScale = 0.f;
	UPROPERTY(EditAnywhere) float LaunchPitchMaxScale = 0.f;
	UPROPERTY(EditAnywhere) float LandVolume = 0.f;
	UPROPERTY(EditAnywhere) float PerfectVolume = 0.f;
	UPROPERTY(EditAnywhere) float PerfectPitchPerChain = 0.f;
	UPROPERTY(EditAnywhere) int32 PerfectPitchMaxChain = 0;
	UPROPERTY(EditAnywhere) float GoodVolume = 0.f;
	UPROPERTY(EditAnywhere) float MissVolume = 0.f;
	UPROPERTY(EditAnywhere) float DashVolume = 0.f;
	UPROPERTY(EditAnywhere) float DashPitchMin = 0.f;
	UPROPERTY(EditAnywhere) float DashPitchMax = 0.f;
	UPROPERTY(EditAnywhere) float HitVolume = 0.f;
	UPROPERTY(EditAnywhere) float KillVolume = 0.f;
	UPROPERTY(EditAnywhere) float KillPitchMin = 0.f;
	UPROPERTY(EditAnywhere) float KillPitchMax = 0.f;
	UPROPERTY(EditAnywhere) float LevelUpVolume = 0.f;
};

// Tuning.csv "MobFx.*": 몬스터 피격/처치 연출
USTRUCT()
struct FYJJMobFxTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float IdleBob = 0.f;
	UPROPERTY(EditAnywhere) float MinFlapInterval = 0.f;
	UPROPERTY(EditAnywhere) float HitPunchDecay = 0.f;
	UPROPERTY(EditAnywhere) float HitPunchScale = 0.f;

	UPROPERTY(EditAnywhere) float DeathFreezeTime = 0.f;
	UPROPERTY(EditAnywhere) float DeathFlyTime = 0.f;
	UPROPERTY(EditAnywhere) float DeathShake = 0.f;
	UPROPERTY(EditAnywhere) float DeathFreezeScaleX = 0.f;
	UPROPERTY(EditAnywhere) float DeathFreezeScaleZ = 0.f;
	UPROPERTY(EditAnywhere) float DeathFlashInterval = 0.f;
	UPROPERTY(EditAnywhere) float DeathFlyDistance = 0.f;
	UPROPERTY(EditAnywhere) float DeathFlyRise = 0.f;
	UPROPERTY(EditAnywhere) float DeathFlyDrop = 0.f;
	UPROPERTY(EditAnywhere) float DeathSpinDegrees = 0.f;
	UPROPERTY(EditAnywhere) float DeathShrinkStart = 0.f;
	UPROPERTY(EditAnywhere) float DeathShrinkEnd = 0.f;

	UPROPERTY(EditAnywhere) float ImpactDepth = 0.f;
	UPROPERTY(EditAnywhere) float ImpactWidthRatio = 0.f;

	UPROPERTY(EditAnywhere) float FeatherDepth = 0.f;
	UPROPERTY(EditAnywhere) float FeatherAngleJitter = 0.f;
	UPROPERTY(EditAnywhere) float FeatherLifeMin = 0.f;
	UPROPERTY(EditAnywhere) float FeatherLifeMax = 0.f;
	UPROPERTY(EditAnywhere) float FeatherWidthRatio = 0.f;
	UPROPERTY(EditAnywhere) float FeatherUpBias = 0.f;
	UPROPERTY(EditAnywhere) float FeatherSpeedMin = 0.f;
	UPROPERTY(EditAnywhere) float FeatherSpeedMax = 0.f;
	UPROPERTY(EditAnywhere) float FeatherGravity = 0.f;
	UPROPERTY(EditAnywhere) float FeatherSpinMax = 0.f;
	UPROPERTY(EditAnywhere) float FeatherEndScale = 0.f;

	UPROPERTY(EditAnywhere) int32 StarCount = 0;
	UPROPERTY(EditAnywhere) float StarDepth = 0.f;
	UPROPERTY(EditAnywhere) float StarWidthRatio = 0.f;
	UPROPERTY(EditAnywhere) float StarSpacingRatio = 0.f;
	UPROPERTY(EditAnywhere) float StarHeightRatio = 0.f;
	UPROPERTY(EditAnywhere) float StarFollowRatio = 0.f;
	UPROPERTY(EditAnywhere) float StarSpread = 0.f;
	UPROPERTY(EditAnywhere) float StarRise = 0.f;
	UPROPERTY(EditAnywhere) float StarSpin = 0.f;
	UPROPERTY(EditAnywhere) float StarEndScale = 0.f;

	UPROPERTY(EditAnywhere) float PoofDepth = 0.f;
	UPROPERTY(EditAnywhere) float PoofFrameTime = 0.f;
	UPROPERTY(EditAnywhere) float PoofLifetime = 0.f;
	UPROPERTY(EditAnywhere) float PoofWidthRatio = 0.f;
	UPROPERTY(EditAnywhere) float PoofStartScale = 0.f;
	UPROPERTY(EditAnywhere) float PoofEndScale = 0.f;
};

// Tuning.csv "Sky.*": 하늘 칸 채우기 (몬스터, 장식 구름)
USTRUCT()
struct FYJJSkyTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float CellHeight = 0.f;
	UPROPERTY(EditAnywhere) float MobChancePerCell = 0.f;
	UPROPERTY(EditAnywhere) float SecondMobChance = 0.f;
	UPROPERTY(EditAnywhere) float MinSpawnHeight = 0.f;
	UPROPERTY(EditAnywhere) float ScalePerMeter = 0.f;

	UPROPERTY(EditAnywhere) float SpawnBelow = 0.f;
	UPROPERTY(EditAnywhere) float SpawnAbove = 0.f;
	UPROPERTY(EditAnywhere) float LookAheadTime = 0.f;
	UPROPERTY(EditAnywhere) float DespawnMargin = 0.f;
	UPROPERTY(EditAnywhere) int32 MaxCellsPerTick = 0;
	UPROPERTY(EditAnywhere) float RespawnDelay = 0.f;
	UPROPERTY(EditAnywhere) float RespawnMinDistance = 0.f;

	UPROPERTY(EditAnywhere) float MobDepthJitter = 0.f;
	UPROPERTY(EditAnywhere) float MobPatrolMaxRatio = 0.f;
	UPROPERTY(EditAnywhere) float MobHeightMin = 0.f;
	UPROPERTY(EditAnywhere) float MobHeightMax = 0.f;

	UPROPERTY(EditAnywhere) float DecorChancePerCell = 0.f;
	UPROPERTY(EditAnywhere) float DecorDepth = 0.f;
	UPROPERTY(EditAnywhere) float DecorDepthJitter = 0.f;
	UPROPERTY(EditAnywhere) float DecorSpreadRatio = 0.f;
	UPROPERTY(EditAnywhere) float DecorWidthMin = 0.f;
	UPROPERTY(EditAnywhere) float DecorWidthMax = 0.f;
	UPROPERTY(EditAnywhere) float DecorFlipChance = 0.f;
};

// Tuning.csv "Stage.*": 스테이지 구성
USTRUCT()
struct FYJJStageTuning
{
	GENERATED_BODY()

	// 같은 시드면 같은 하늘 (랭킹 공정성)
	UPROPERTY(EditAnywhere) int32 Seed = 0;
	UPROPERTY(EditAnywhere) float RopeDrop = 0.f;
	UPROPERTY(EditAnywhere) float RopeThickness = 0.f;
	UPROPERTY(EditAnywhere) float PegWidth = 0.f;
	UPROPERTY(EditAnywhere) float PegDepth = 0.f;
};

// Tuning.csv "Fx.*": 공용 이펙트
USTRUCT()
struct FYJJFxTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float ImpactLifetime = 0.f;
	UPROPERTY(EditAnywhere) float ImpactGrowStart = 0.f;
	UPROPERTY(EditAnywhere) float ImpactGrowEnd = 0.f;
	UPROPERTY(EditAnywhere) float ImpactGrowExponent = 0.f;
	UPROPERTY(EditAnywhere) float ImpactShrinkFrom = 0.f;
	UPROPERTY(EditAnywhere) float ImpactSpinDegrees = 0.f;
	UPROPERTY(EditAnywhere) float SpriteFxLifeSpanPadding = 0.f;
	UPROPERTY(EditAnywhere) float SpriteFxMinFrameTime = 0.f;
};

// Tuning.csv "Hud.*": 배치(HudLayout.csv) 말고 HUD 동작 값
USTRUCT()
struct FYJJHudTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float ReferenceHeight = 0.f;
	UPROPERTY(EditAnywhere) float ReferenceWidth = 0.f;
	UPROPERTY(EditAnywhere) float MinScale = 0.f;
	UPROPERTY(EditAnywhere) int32 MinFontSize = 0;
	UPROPERTY(EditAnywhere) float PanelTexturePixels = 0.f;
	UPROPERTY(EditAnywhere) float PanelCornerPixels = 0.f;
	UPROPERTY(EditAnywhere) float PanelCorner = 0.f;

	UPROPERTY(EditAnywhere) float ChainHeatMax = 0.f;
	UPROPERTY(EditAnywhere) float ChainGrowSize = 0.f;
	UPROPERTY(EditAnywhere) float PowerIntegerFrom = 0.f;

	UPROPERTY(EditAnywhere) float SpeedHeatMax = 0.f;
	UPROPERTY(EditAnywhere) float SpeedGrowSize = 0.f;
	UPROPERTY(EditAnywhere) float SpeedFlameStep = 0.f;
	UPROPERTY(EditAnywhere) int32 SpeedFlameMax = 0;

	UPROPERTY(EditAnywhere) float SpeedLineFrom = 0.f;
	UPROPERTY(EditAnywhere) float SpeedLineRange = 0.f;
	UPROPERTY(EditAnywhere) float SpeedLineSeedRate = 0.f;
	UPROPERTY(EditAnywhere) float SpeedLineCountMin = 0.f;
	UPROPERTY(EditAnywhere) float SpeedLineCountMax = 0.f;
	UPROPERTY(EditAnywhere) float SpeedLineLengthMin = 0.f;
	UPROPERTY(EditAnywhere) float SpeedLineLengthMax = 0.f;
	UPROPERTY(EditAnywhere) float SpeedLineLengthBase = 0.f;
	UPROPERTY(EditAnywhere) float SpeedLineAlphaBase = 0.f;
	UPROPERTY(EditAnywhere) float SpeedLineAlphaGain = 0.f;
	UPROPERTY(EditAnywhere) float SpeedLineWidthMin = 0.f;
	UPROPERTY(EditAnywhere) float SpeedLineWidthMax = 0.f;

	UPROPERTY(EditAnywhere) int32 RangeSegments = 0;
	UPROPERTY(EditAnywhere) float RangeLineWidth = 0.f;

	UPROPERTY(EditAnywhere) int32 PreviewDots = 0;
	UPROPERTY(EditAnywhere) float PreviewTimeScale = 0.f;
	UPROPERTY(EditAnywhere) float PreviewTimeMin = 0.f;
	UPROPERTY(EditAnywhere) float PreviewTimeMax = 0.f;
	UPROPERTY(EditAnywhere) float PreviewDotStart = 0.f;
	UPROPERTY(EditAnywhere) float PreviewDotEnd = 0.f;
	UPROPERTY(EditAnywhere) float PreviewDotBorder = 0.f;

	UPROPERTY(EditAnywhere) float PopupTextSize = 0.f;
	UPROPERTY(EditAnywhere) float PopupOutline = 0.f;
	UPROPERTY(EditAnywhere) float PopupFadeFrom = 0.f;
	UPROPERTY(EditAnywhere) float PopupPunch = 0.f;
	UPROPERTY(EditAnywhere) float PopupPunchSpeed = 0.f;

	UPROPERTY(EditAnywhere) float BannerEagleTime = 0.f;
	UPROPERTY(EditAnywhere) float BannerEagleBobSpeed = 0.f;
	UPROPERTY(EditAnywhere) float BannerEagleBobSize = 0.f;
	UPROPERTY(EditAnywhere) float BannerEagleAspect = 0.f;
};

// Tuning.csv "AutoPlay.*": 테스트용 자동 플레이 봇
USTRUCT()
struct FYJJAutoPlayTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float DragPixels = 0.f;
	UPROPERTY(EditAnywhere) float AimRange = 0.f;
	UPROPERTY(EditAnywhere) float AimMax = 0.f;
	UPROPERTY(EditAnywhere) float PreTouchSpeedScale = 0.f;
	UPROPERTY(EditAnywhere) float PreTouchTime = 0.f;
	UPROPERTY(EditAnywhere) float TapInterval = 0.f;
	UPROPERTY(EditAnywhere) float ReportInterval = 0.f;
	UPROPERTY(EditAnywhere) float ExitDelay = 0.f;
	// 스크린샷 찍는 시점 (플레이 시간 비율)
	UPROPERTY(EditAnywhere) float Shot1 = 0.f;
	UPROPERTY(EditAnywhere) float Shot2 = 0.f;
	UPROPERTY(EditAnywhere) float Shot3 = 0.f;
	UPROPERTY(EditAnywhere) float Shot4 = 0.f;
};

// Colors.csv: 게임에서 쓰는 색 (16진수 RRGGBB 또는 RRGGBBAA, 감마 변환 없이 그대로)
USTRUCT()
struct FYJJPalette
{
	GENERATED_BODY()

	// 판정/연출
	UPROPERTY(EditAnywhere) FLinearColor Perfect = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor Good = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor Miss = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor LevelUp = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor TooFar = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor HitEffect = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor HitPopup = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor DashGhost = FLinearColor::Transparent;

	// 배경 하늘 틴트
	UPROPERTY(EditAnywhere) FLinearColor SkyLow = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor SkyHigh = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor SkySpace = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor DecorTint = FLinearColor::Transparent;

	// 몬스터
	UPROPERTY(EditAnywhere) FLinearColor MobHitFlash = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor MobDeathFlash = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor MobDeathImpact = FLinearColor::Transparent;

	// HUD
	UPROPERTY(EditAnywhere) FLinearColor Ink = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor Yellow = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor Cream = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor Cyan = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor Orange = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor Red = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor HudText = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor HintPanel = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor ExpBarBack = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor AttackIdle = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor SpeedPanel = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor SpeedLine = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor RangeCircle = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor PreviewDot = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere) FLinearColor BannerDim = FLinearColor::Transparent;
};
