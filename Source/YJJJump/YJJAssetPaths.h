#pragma once

// 코드에서 기본값으로 참조하는 에셋 경로.
// 에셋은 Tools/Art 스크립트가 생성한다 (Tools/README.md)
namespace YJJAssetPaths
{
	inline constexpr const TCHAR* PlayerSprite = TEXT("/Game/YJJJump/Sprites/SPR_Beetle.SPR_Beetle");
	inline constexpr const TCHAR* PlayerDashSprite = TEXT("/Game/YJJJump/Sprites/SPR_Beetle_Dash.SPR_Beetle_Dash");
	inline constexpr const TCHAR* Poof0Sprite = TEXT("/Game/YJJJump/Sprites/SPR_Poof0.SPR_Poof0");
	inline constexpr const TCHAR* Poof1Sprite = TEXT("/Game/YJJJump/Sprites/SPR_Poof1.SPR_Poof1");
	inline constexpr const TCHAR* FeatherSprite = TEXT("/Game/YJJJump/Sprites/SPR_Feather.SPR_Feather");
	inline constexpr const TCHAR* DizzyStarSprite = TEXT("/Game/YJJJump/Sprites/SPR_DizzyStar.SPR_DizzyStar");
	inline constexpr const TCHAR* TranslucentSpriteMaterial = TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial");
	inline constexpr const TCHAR* RopeSprite = TEXT("/Game/YJJJump/Sprites/SPR_Rope.SPR_Rope");
	inline constexpr const TCHAR* RopePegSprite = TEXT("/Game/YJJJump/Sprites/SPR_RopePeg.SPR_RopePeg");
	inline constexpr const TCHAR* ImpactSprite = TEXT("/Game/YJJJump/Sprites/SPR_Impact.SPR_Impact");
	inline constexpr const TCHAR* SkySprite = TEXT("/Game/YJJJump/Sprites/SPR_SkyBackground.SPR_SkyBackground");
	inline constexpr const TCHAR* CloudDecorSprite = TEXT("/Game/YJJJump/Sprites/SPR_CloudButt.SPR_CloudButt");

	// HUD에서 그리는 텍스처
	inline constexpr const TCHAR* BaldEagleTexture = TEXT("/Game/YJJJump/Textures/T_MobBaldEagle.T_MobBaldEagle");
	inline constexpr const TCHAR* UIPanelTexture = TEXT("/Game/YJJJump/Textures/T_UI_Panel.T_UI_Panel");
	inline constexpr const TCHAR* UICoinTexture = TEXT("/Game/YJJJump/Textures/T_UI_Coin.T_UI_Coin");
	inline constexpr const TCHAR* UISkullTexture = TEXT("/Game/YJJJump/Textures/T_UI_Skull.T_UI_Skull");
	inline constexpr const TCHAR* UIArrowTexture = TEXT("/Game/YJJJump/Textures/T_UI_Arrow.T_UI_Arrow");
	inline constexpr const TCHAR* UIFlameTexture = TEXT("/Game/YJJJump/Textures/T_UI_Flame.T_UI_Flame");

	// HUD 글꼴. TTF 파일(Content 기준)을 직접 읽고, 없는 글자는 엔진 Roboto로
	inline constexpr const TCHAR* GameFontFile = TEXT("YJJJump/Fonts/Jua-Regular.ttf");
	inline constexpr const TCHAR* FallbackFont = TEXT("/Engine/EngineFonts/Roboto.Roboto");
	inline constexpr const TCHAR* FallbackFontTypeface = TEXT("Bold");

	// 소리 (Tools/Audio/generate_audio.py)
	inline constexpr const TCHAR* BgmSky = TEXT("/Game/YJJJump/Audio/BGM_Sky.BGM_Sky");
	inline constexpr const TCHAR* SfxLaunch = TEXT("/Game/YJJJump/Audio/SFX_Launch.SFX_Launch");
	inline constexpr const TCHAR* SfxLand = TEXT("/Game/YJJJump/Audio/SFX_Land.SFX_Land");
	inline constexpr const TCHAR* SfxPerfect = TEXT("/Game/YJJJump/Audio/SFX_Perfect.SFX_Perfect");
	inline constexpr const TCHAR* SfxGood = TEXT("/Game/YJJJump/Audio/SFX_Good.SFX_Good");
	inline constexpr const TCHAR* SfxMiss = TEXT("/Game/YJJJump/Audio/SFX_Miss.SFX_Miss");
	inline constexpr const TCHAR* SfxDash = TEXT("/Game/YJJJump/Audio/SFX_Dash.SFX_Dash");
	inline constexpr const TCHAR* SfxHit = TEXT("/Game/YJJJump/Audio/SFX_Hit.SFX_Hit");
	inline constexpr const TCHAR* SfxKill = TEXT("/Game/YJJJump/Audio/SFX_Kill.SFX_Kill");
	inline constexpr const TCHAR* SfxLevelUp = TEXT("/Game/YJJJump/Audio/SFX_LevelUp.SFX_LevelUp");
}
