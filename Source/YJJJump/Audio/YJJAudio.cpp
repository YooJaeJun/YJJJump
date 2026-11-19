#include "Audio/YJJAudio.h"
#include "Data/YJJData.h"
#include "YJJAssetPaths.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

const TCHAR* YJJAudio::SfxPath(ESfx Sfx)
{
	switch (Sfx)
	{
	case ESfx::Launch: return YJJAssetPaths::SfxLaunch;
	case ESfx::Land: return YJJAssetPaths::SfxLand;
	case ESfx::Perfect: return YJJAssetPaths::SfxPerfect;
	case ESfx::Good: return YJJAssetPaths::SfxGood;
	case ESfx::Miss: return YJJAssetPaths::SfxMiss;
	case ESfx::Dash: return YJJAssetPaths::SfxDash;
	case ESfx::Hit: return YJJAssetPaths::SfxHit;
	case ESfx::Kill: return YJJAssetPaths::SfxKill;
	case ESfx::LevelUp: return YJJAssetPaths::SfxLevelUp;
	}
	return nullptr;
}

void YJJAudio::Play(const UObject* WorldContext, ESfx Sfx, float Volume, float Pitch)
{
	const TCHAR* Path = SfxPath(Sfx);
	if (USoundBase* Sound = Path ? LoadObject<USoundBase>(nullptr, Path) : nullptr)
	{
		UGameplayStatics::PlaySound2D(WorldContext, Sound, Volume, Pitch);
	}
}

void YJJAudio::StartBgm(const UObject* WorldContext)
{
	if (USoundBase* Bgm = LoadObject<USoundBase>(nullptr, YJJAssetPaths::BgmSky))
	{
		UGameplayStatics::SpawnSound2D(WorldContext, Bgm, FYJJData::Get().Sound.BgmVolume);
	}
}
