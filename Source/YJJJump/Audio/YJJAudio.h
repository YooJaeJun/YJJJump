#pragma once

#include "CoreMinimal.h"

// 효과음/BGM 재생. 소리는 Tools/Audio/generate_audio.py가 직접 합성한 것 (상업적 사용 자유)
// 볼륨·피치는 부르는 쪽이 Tuning.csv Sound.* 값으로 넘긴다
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

	YJJJUMP_API void Play(const UObject* WorldContext, ESfx Sfx, float Volume, float Pitch);

	// 하늘 구간 BGM (루프, 볼륨은 Sound.BgmVolume)
	YJJJUMP_API void StartBgm(const UObject* WorldContext);

	// 효과음 에셋 경로 (YJJAssetPaths)
	YJJJUMP_API const TCHAR* SfxPath(ESfx Sfx);
}
