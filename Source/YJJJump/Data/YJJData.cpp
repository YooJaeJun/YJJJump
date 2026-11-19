#include "Data/YJJData.h"
#include "Data/YJJLoc.h"
#include "YJJJump.h"
#include "Engine/DataTable.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

TUniquePtr<FYJJData> FYJJData::Instance;

const FName FYJJData::SkyBiome(TEXT("Sky"));

FAutoConsoleCommand FYJJData::ReloadCommand(
	TEXT("yjj.ReloadData"),
	TEXT("Content/YJJJump/Data/*.csv 다시 읽기 (로컬라이제이션 포함)"),
	FConsoleCommandDelegate::CreateStatic(&FYJJData::Reload));

const FYJJData& FYJJData::Get()
{
	if (!Instance)
	{
		Instance = MakeUnique<FYJJData>();
		Instance->Load();
	}
	return *Instance;
}

void FYJJData::Reload()
{
	if (!Instance)
	{
		Instance = MakeUnique<FYJJData>();
	}
	Instance->Load();
	FYJJLoc::Reload();
	UE_LOG(LogYJJJump, Display, TEXT("[Data] reloaded"));
}

FString FYJJData::DataPath(const TCHAR* FileName)
{
	return FPaths::ProjectContentDir() / TEXT("YJJJump/Data") / FileName;
}

UDataTable* FYJJData::LoadTable(const TCHAR* FileName, UScriptStruct* RowStruct)
{
	UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), NAME_None, RF_Transient);
	Table->RowStruct = RowStruct;

	FString Csv;
	if (!FFileHelper::LoadFileToString(Csv, *DataPath(FileName)))
	{
		UE_LOG(LogYJJJump, Error, TEXT("[Data] %s 를 읽지 못했다 (%s)"), FileName, *DataPath(FileName));
		return Table;
	}
	for (const FString& Problem : Table->CreateTableFromCSVString(Csv))
	{
		UE_LOG(LogYJJJump, Warning, TEXT("[Data] %s: %s"), FileName, *Problem);
	}
	return Table;
}

TMap<FString, FString> FYJJData::LoadKeyValues(const TCHAR* FileName)
{
	TMap<FString, FString> Values;
	const UDataTable* Table = LoadTable(FileName, FYJJKeyValueRow::StaticStruct());
	Table->ForeachRow<FYJJKeyValueRow>(TEXT("FYJJData"), [&Values](const FName& Key, const FYJJKeyValueRow& Row)
	{
		Values.Add(Key.ToString(), Row.Value.TrimStartAndEnd());
	});
	return Values;
}

bool FYJJData::ParseHexColor(const FString& Text, FLinearColor& OutColor)
{
	FString Hex = Text.TrimStartAndEnd();
	Hex.RemoveFromStart(TEXT("#"));
	const bool bHasAlpha = Hex.Len() == 8;
	if (!bHasAlpha && Hex.Len() != 6)
	{
		return false;
	}
	for (const TCHAR Char : Hex)
	{
		if (!FChar::IsHexDigit(Char))
		{
			return false;
		}
	}

	// FColor::FromHex 는 RRGGBBAA. 알파가 없으면 불투명
	const FColor Parsed = FColor::FromHex(bHasAlpha ? Hex : Hex + TEXT("FF"));
	OutColor = Parsed.ReinterpretAsLinear();
	return true;
}

void FYJJData::FillStruct(const UScriptStruct* Struct, void* Target, const FString& Prefix, const TMap<FString, FString>& Values, const TCHAR* FileName, TSet<FString>& UsedKeys)
{
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FProperty* Property = *It;
		const FString Key = Prefix + Property->GetName();
		const FString* Value = Values.Find(Key);
		if (!Value)
		{
			UE_LOG(LogYJJJump, Warning, TEXT("[Data] %s 에 %s 가 없다 (0으로 둔다)"), FileName, *Key);
			continue;
		}
		UsedKeys.Add(Key);

		void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Target);
		const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
		if (StructProperty && StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
		{
			if (!ParseHexColor(*Value, *static_cast<FLinearColor*>(ValuePtr)))
			{
				UE_LOG(LogYJJJump, Warning, TEXT("[Data] %s %s: 색 '%s' 를 읽지 못했다 (RRGGBB 또는 RRGGBBAA)"), FileName, *Key, **Value);
			}
			continue;
		}
		if (!Property->ImportText_Direct(**Value, ValuePtr, nullptr, PPF_None))
		{
			UE_LOG(LogYJJJump, Warning, TEXT("[Data] %s %s: 값 '%s' 를 읽지 못했다"), FileName, *Key, **Value);
		}
	}
}

void FYJJData::Load()
{
	// 이름-값 표: 구역마다 구조체 하나
	{
		const TCHAR* TuningFile = TEXT("Tuning.csv");
		const TMap<FString, FString> Tuning = LoadKeyValues(TuningFile);
		TSet<FString> Used;
		FillStruct(FYJJRopeTuning::StaticStruct(), &Rope, TEXT("Rope."), Tuning, TuningFile, Used);
		FillStruct(FYJJCombatTuning::StaticStruct(), &Combat, TEXT("Combat."), Tuning, TuningFile, Used);
		FillStruct(FYJJPlayerTuning::StaticStruct(), &Player, TEXT("Player."), Tuning, TuningFile, Used);
		FillStruct(FYJJCameraTuning::StaticStruct(), &Camera, TEXT("Camera."), Tuning, TuningFile, Used);
		FillStruct(FYJJSoundTuning::StaticStruct(), &Sound, TEXT("Sound."), Tuning, TuningFile, Used);
		FillStruct(FYJJMobFxTuning::StaticStruct(), &MobFx, TEXT("MobFx."), Tuning, TuningFile, Used);
		FillStruct(FYJJSkyTuning::StaticStruct(), &Sky, TEXT("Sky."), Tuning, TuningFile, Used);
		FillStruct(FYJJStageTuning::StaticStruct(), &Stage, TEXT("Stage."), Tuning, TuningFile, Used);
		FillStruct(FYJJFxTuning::StaticStruct(), &Fx, TEXT("Fx."), Tuning, TuningFile, Used);
		FillStruct(FYJJHudTuning::StaticStruct(), &Hud, TEXT("Hud."), Tuning, TuningFile, Used);
		FillStruct(FYJJAutoPlayTuning::StaticStruct(), &AutoPlay, TEXT("AutoPlay."), Tuning, TuningFile, Used);
		for (const TPair<FString, FString>& Pair : Tuning)
		{
			if (!Used.Contains(Pair.Key))
			{
				UE_LOG(LogYJJJump, Warning, TEXT("[Data] %s 의 %s 는 쓰는 곳이 없다 (오타?)"), TuningFile, *Pair.Key);
			}
		}
		UE_LOG(LogYJJJump, Display, TEXT("[Data] %s: %d values"), TuningFile, Tuning.Num());
	}
	{
		const TCHAR* ColorsFile = TEXT("Colors.csv");
		const TMap<FString, FString> ColorValues = LoadKeyValues(ColorsFile);
		TSet<FString> Used;
		FillStruct(FYJJPalette::StaticStruct(), &Colors, FString(), ColorValues, ColorsFile, Used);
	}

	Mobs.Reset();
	LoadTable(TEXT("Mobs.csv"), FYJJMobRow::StaticStruct())->ForeachRow<FYJJMobRow>(TEXT("FYJJData"), [this](const FName& Key, const FYJJMobRow& Row)
	{
		Mobs.Add(Key, Row);
	});

	Spawns.Reset();
	LoadTable(TEXT("Spawns.csv"), FYJJSpawnRow::StaticStruct())->ForeachRow<FYJJSpawnRow>(TEXT("FYJJData"), [this](const FName& Key, const FYJJSpawnRow& Row)
	{
		if (!Mobs.Contains(Row.Mob))
		{
			UE_LOG(LogYJJJump, Warning, TEXT("[Data] Spawns.csv %s: Mobs.csv 에 없는 몬스터 '%s'"), *Key.ToString(), *Row.Mob.ToString());
			return;
		}
		Spawns.Add(Row);
	});

	HudLayout.Reset();
	ReportedMissingLayout.Reset();
	LoadTable(TEXT("HudLayout.csv"), FYJJHudLayoutRow::StaticStruct())->ForeachRow<FYJJHudLayoutRow>(TEXT("FYJJData"), [this](const FName& Key, const FYJJHudLayoutRow& Row)
	{
		HudLayout.Add(Key, Row);
	});

	UE_LOG(LogYJJJump, Display, TEXT("[Data] mobs %d, spawns %d, hud %d"), Mobs.Num(), Spawns.Num(), HudLayout.Num());
}

const FYJJMobRow* FYJJData::FindMob(FName MobId) const
{
	return Mobs.Find(MobId);
}

const FYJJHudLayoutRow& FYJJData::Layout(FName Element) const
{
	if (const FYJJHudLayoutRow* Row = HudLayout.Find(Element))
	{
		return *Row;
	}
	if (!ReportedMissingLayout.Contains(Element))
	{
		ReportedMissingLayout.Add(Element);
		UE_LOG(LogYJJJump, Warning, TEXT("[Data] HudLayout.csv 에 %s 줄이 없다"), *Element.ToString());
	}
	return MissingLayout;
}

const FYJJSpawnRow* FYJJData::PickSpawn(FName Biome, FRandomStream& Random, float HeightAboveStart) const
{
	auto IsCandidate = [Biome, HeightAboveStart](const FYJJSpawnRow& Row)
	{
		return Row.Biome == Biome && HeightAboveStart >= Row.MinHeight && Row.Weight > 0.f;
	};

	float TotalWeight = 0.f;
	for (const FYJJSpawnRow& Row : Spawns)
	{
		if (IsCandidate(Row))
		{
			TotalWeight += Row.Weight;
		}
	}
	if (TotalWeight <= 0.f)
	{
		return nullptr;
	}

	float Roll = Random.FRandRange(0.f, TotalWeight);
	for (const FYJJSpawnRow& Row : Spawns)
	{
		if (IsCandidate(Row))
		{
			Roll -= Row.Weight;
			if (Roll <= 0.f)
			{
				return &Row;
			}
		}
	}
	return nullptr;
}
