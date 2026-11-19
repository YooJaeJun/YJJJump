#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"

/**
 * 화면에 나오는 글자는 전부 Content/YJJJump/Data/Localization.csv 에 있다.
 *   열: Key, ko, en, Note  (언어를 늘리려면 열을 하나 더 붙이면 된다. 예: ja)
 * 언어: 기본 ko. 명령줄 -yjjlang=en 또는 콘솔 yjj.Lang en 으로 바꾼다.
 * 그 언어 칸이 비어 있으면 ko, 그것도 없으면 [Key] 를 보여 준다 (빠진 글자가 눈에 띄게).
 * 서식 자리는 {0} {1} ... (FString::Format)
 */
class YJJJUMP_API FYJJLoc
{
public:
	static const FString& Get(const TCHAR* Key);
	static FString Format(const TCHAR* Key, const FStringFormatOrderedArguments& Args);

	// 지금 언어 (ko, en ...)
	static const FString& Language();
	static void SetLanguage(const FString& NewLanguage);

	// CSV를 다시 읽는다 (yjj.ReloadData 도 부른다)
	static void Reload();

private:
	static FYJJLoc& Instance();

	void Load();
	void PickLanguage();

	// 키 -> (언어 -> 글자)
	TMap<FString, TMap<FString, FString>> Texts;
	TArray<FString> Languages;
	FString CurrentLanguage;
	// 빠진 키는 "[Key]" 를 만들어 두고 돌려준다 (경고는 한 번만)
	TMap<FString, FString> MissingTexts;

	static TUniquePtr<FYJJLoc> Singleton;
	static FAutoConsoleCommand LanguageCommand;

	static constexpr const TCHAR* FileName = TEXT("Localization.csv");
	static constexpr const TCHAR* DefaultLanguage = TEXT("ko");
	static constexpr const TCHAR* NoteColumn = TEXT("Note");
	static constexpr const TCHAR* CommentPrefix = TEXT("#");
	static constexpr const TCHAR* LanguageSwitch = TEXT("yjjlang=");
};
