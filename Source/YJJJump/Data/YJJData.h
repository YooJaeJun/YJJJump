#pragma once

#include "CoreMinimal.h"
#include "Combat/YJJCombatTypes.h"
#include "Data/YJJDataTypes.h"
#include "HAL/IConsoleManager.h"
#include "Rope/YJJRopeTypes.h"

class UDataTable;

/**
 * 게임 수치는 코드가 아니라 Content/YJJJump/Data/*.csv 에 있다 (엑셀로 열어서 고치면 된다).
 * 처음 쓸 때 CSV를 엔진 DataTable로 읽고, 콘솔 yjj.ReloadData 로 게임 중에 다시 읽는다.
 *
 *   Tuning.csv     손맛·연출 상수 (이름 = 구역.필드, 값, 설명)
 *   Colors.csv     색 (이름, 16진수)
 *   Mobs.csv       몬스터 (행 이름 = 몬스터 ID)
 *   Spawns.csv     구간별 몬스터 등장 가중치
 *   HudLayout.csv  HUD 요소 배치
 *
 * 화면 글자는 Localization.csv (FYJJLoc).
 */
class YJJJUMP_API FYJJData
{
public:
	static const FYJJData& Get();

	// CSV를 다시 읽는다 (콘솔 yjj.ReloadData). 로컬라이제이션도 같이 읽는다
	static void Reload();

	// Content/YJJJump/Data/ 안의 파일 경로
	static FString DataPath(const TCHAR* FileName);

	const FYJJMobRow* FindMob(FName MobId) const;

	// 없는 요소면 경고 후 0으로 채운 배치
	const FYJJHudLayoutRow& Layout(FName Element) const;

	// 구간과 높이에 맞는 스폰 항목 중 가중치로 하나 고른다
	const FYJJSpawnRow* PickSpawn(FName Biome, FRandomStream& Random, float HeightAboveStart) const;

	FYJJRopeTuning Rope;
	FYJJCombatTuning Combat;
	FYJJPlayerTuning Player;
	FYJJCameraTuning Camera;
	FYJJSoundTuning Sound;
	FYJJMobFxTuning MobFx;
	FYJJSkyTuning Sky;
	FYJJStageTuning Stage;
	FYJJFxTuning Fx;
	FYJJHudTuning Hud;
	FYJJAutoPlayTuning AutoPlay;
	FYJJPalette Colors;

	TMap<FName, FYJJMobRow> Mobs;
	TArray<FYJJSpawnRow> Spawns;
	TMap<FName, FYJJHudLayoutRow> HudLayout;

	// 하늘 구간 이름 (Spawns.csv Biome 열)
	static const FName SkyBiome;

private:
	void Load();

	// CSV 파일 -> DataTable (행 구조체 RowStruct). 실패하면 빈 표
	static UDataTable* LoadTable(const TCHAR* FileName, UScriptStruct* RowStruct);

	// Tuning.csv/Colors.csv 같은 이름-값 표를 맵으로
	static TMap<FString, FString> LoadKeyValues(const TCHAR* FileName);

	// 구조체 필드마다 "Prefix필드" 이름의 값을 찾아 채운다. 빠진 값은 경고
	static void FillStruct(const UScriptStruct* Struct, void* Target, const FString& Prefix, const TMap<FString, FString>& Values, const TCHAR* FileName, TSet<FString>& UsedKeys);

	// 16진수 색 (RRGGBB / RRGGBBAA, 감마 변환 없음)
	static bool ParseHexColor(const FString& Text, FLinearColor& OutColor);

	static TUniquePtr<FYJJData> Instance;
	static FAutoConsoleCommand ReloadCommand;

	FYJJHudLayoutRow MissingLayout;
	mutable TSet<FName> ReportedMissingLayout;
};
