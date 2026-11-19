#include "Data/YJJLoc.h"
#include "Data/YJJData.h"
#include "YJJJump.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Serialization/Csv/CsvParser.h"

TUniquePtr<FYJJLoc> FYJJLoc::Singleton;

FAutoConsoleCommand FYJJLoc::LanguageCommand(
	TEXT("yjj.Lang"),
	TEXT("화면 글자 언어 바꾸기 (예: yjj.Lang en). Localization.csv 머리줄의 언어 이름"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		if (Args.Num() > 0)
		{
			FYJJLoc::SetLanguage(Args[0]);
		}
		UE_LOG(LogYJJJump, Display, TEXT("[Loc] language: %s"), *FYJJLoc::Language());
	}));

FYJJLoc& FYJJLoc::Instance()
{
	if (!Singleton)
	{
		Singleton = MakeUnique<FYJJLoc>();
		Singleton->Load();
		Singleton->PickLanguage();
	}
	return *Singleton;
}

void FYJJLoc::Load()
{
	Texts.Reset();
	Languages.Reset();
	MissingTexts.Reset();

	const FString Path = FYJJData::DataPath(FileName);
	FString Csv;
	if (!FFileHelper::LoadFileToString(Csv, *Path))
	{
		UE_LOG(LogYJJJump, Error, TEXT("[Loc] %s 를 읽지 못했다"), *Path);
		return;
	}

	const FCsvParser Parser(MoveTemp(Csv));
	const FCsvParser::FRows& Rows = Parser.GetRows();
	if (Rows.IsEmpty())
	{
		return;
	}

	// 머리줄: Key, ko, en, ..., Note
	TArray<int32> LanguageColumns;
	for (int32 Column = 1; Column < Rows[0].Num(); ++Column)
	{
		const FString Header = FString(Rows[0][Column]).TrimStartAndEnd();
		if (!Header.IsEmpty() && !Header.Equals(NoteColumn, ESearchCase::IgnoreCase))
		{
			Languages.Add(Header.ToLower());
			LanguageColumns.Add(Column);
		}
	}

	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TArray<const TCHAR*>& Row = Rows[RowIndex];
		const FString Key = Row.IsEmpty() ? FString() : FString(Row[0]).TrimStartAndEnd();
		if (Key.IsEmpty() || Key.StartsWith(CommentPrefix))
		{
			continue;
		}
		if (Texts.Contains(Key))
		{
			UE_LOG(LogYJJJump, Warning, TEXT("[Loc] 키 중복: %s"), *Key);
		}

		TMap<FString, FString>& Entry = Texts.FindOrAdd(Key);
		for (int32 Index = 0; Index < LanguageColumns.Num(); ++Index)
		{
			const int32 Column = LanguageColumns[Index];
			if (Column < Row.Num() && *Row[Column])
			{
				Entry.Add(Languages[Index], Row[Column]);
			}
		}
	}
	UE_LOG(LogYJJJump, Display, TEXT("[Loc] %d keys, languages: %s"), Texts.Num(), *FString::Join(Languages, TEXT(", ")));
}

void FYJJLoc::PickLanguage()
{
	// 기본은 한국어. 엔진 문화권은 OS와 상관없이 en으로 잡히는 경우가 있어서 따르지 않는다
	FString Wanted = DefaultLanguage;
	FParse::Value(FCommandLine::Get(), LanguageSwitch, Wanted);
	Wanted = Wanted.ToLower();
	CurrentLanguage = Languages.Contains(Wanted) ? Wanted : FString(DefaultLanguage);
}

const FString& FYJJLoc::Get(const TCHAR* Key)
{
	FYJJLoc& Self = Instance();
	if (const TMap<FString, FString>* Entry = Self.Texts.Find(Key))
	{
		if (const FString* Text = Entry->Find(Self.CurrentLanguage))
		{
			return *Text;
		}
		if (const FString* Fallback = Entry->Find(DefaultLanguage))
		{
			return *Fallback;
		}
	}

	if (const FString* Shown = Self.MissingTexts.Find(Key))
	{
		return *Shown;
	}
	UE_LOG(LogYJJJump, Warning, TEXT("[Loc] %s 에 %s 가 없다"), FileName, Key);
	return Self.MissingTexts.Add(Key, FString::Printf(TEXT("[%s]"), Key));
}

FString FYJJLoc::Format(const TCHAR* Key, const FStringFormatOrderedArguments& Args)
{
	return FString::Format(*Get(Key), Args);
}

const FString& FYJJLoc::Language()
{
	return Instance().CurrentLanguage;
}

void FYJJLoc::SetLanguage(const FString& NewLanguage)
{
	FYJJLoc& Self = Instance();
	const FString Wanted = NewLanguage.ToLower();
	if (!Self.Languages.Contains(Wanted))
	{
		UE_LOG(LogYJJJump, Warning, TEXT("[Loc] 없는 언어 '%s' (있는 언어: %s)"), *NewLanguage, *FString::Join(Self.Languages, TEXT(", ")));
		return;
	}
	Self.CurrentLanguage = Wanted;
}

void FYJJLoc::Reload()
{
	FYJJLoc& Self = Instance();
	const FString Previous = Self.CurrentLanguage;
	Self.Load();
	Self.CurrentLanguage = Self.Languages.Contains(Previous) ? Previous : FString(DefaultLanguage);
}
