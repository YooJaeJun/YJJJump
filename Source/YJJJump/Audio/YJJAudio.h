#pragma once

#include "CoreMinimal.h"

// 효과음/BGM 재생. 소리는 Tools/Audio/generate_audio.py가 직접 합성한 것 (상업적 사용 자유)
namespace YJJAudio
{
	enum class ESfx : uint8
	{
		Launch,		// 로프 "띠요옹"
		Land,		// 로프에 "퉁"
		Perfect,
		Good,
		Miss,
		Dash,		// 뿔 돌진 "슈웅"
		Hit,		// 못 잡음 "퍽"
		Kill,		// 처치 "빠악! 뿅"
		LevelUp,
	};

	YJJJUMP_API void Play(const UObject* WorldContext, ESfx Sfx, float Volume = 1.f, float Pitch = 1.f);

	// 하늘 구간 BGM (루프)
	YJJJUMP_API void StartBgm(const UObject* WorldContext, float Volume = 0.45f);
}
