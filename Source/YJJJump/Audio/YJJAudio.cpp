#include "Audio/YJJAudio.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace YJJAudio
{
	namespace
	{
		const TCHAR* SfxPath(ESfx Sfx)
		{
			switch (Sfx)
			{
			case ESfx::Launch: return TEXT("/Game/YJJJump/Audio/SFX_Launch.SFX_Launch");
			case ESfx::Land: return TEXT("/Game/YJJJump/Audio/SFX_Land.SFX_Land");
			case ESfx::Perfect: return TEXT("/Game/YJJJump/Audio/SFX_Perfect.SFX_Perfect");
			case ESfx::Good: return TEXT("/Game/YJJJump/Audio/SFX_Good.SFX_Good");
			case ESfx::Miss: return TEXT("/Game/YJJJump/Audio/SFX_Miss.SFX_Miss");
			case ESfx::Dash: return TEXT("/Game/YJJJump/Audio/SFX_Dash.SFX_Dash");
			case ESfx::Hit: return TEXT("/Game/YJJJump/Audio/SFX_Hit.SFX_Hit");
			case ESfx::Kill: return TEXT("/Game/YJJJump/Audio/SFX_Kill.SFX_Kill");
			case ESfx::LevelUp: return TEXT("/Game/YJJJump/Audio/SFX_LevelUp.SFX_LevelUp");
			}
			return nullptr;
		}
	}

	void Play(const UObject* WorldContext, ESfx Sfx, float Volume, float Pitch)
	{
		const TCHAR* Path = SfxPath(Sfx);
		if (USoundBase* Sound = Path ? LoadObject<USoundBase>(nullptr, Path) : nullptr)
		{
			UGameplayStatics::PlaySound2D(WorldContext, Sound, Volume, Pitch);
		}
	}

	void StartBgm(const UObject* WorldContext, float Volume)
	{
		if (USoundBase* Bgm = LoadObject<USoundBase>(nullptr, TEXT("/Game/YJJJump/Audio/BGM_Sky.BGM_Sky")))
		{
			UGameplayStatics::SpawnSound2D(WorldContext, Bgm, Volume);
		}
	}
}
