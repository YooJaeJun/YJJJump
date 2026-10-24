#pragma once

// 코드에서 기본값으로 참조하는 에셋 경로.
// 에셋은 Tools/Art 스크립트가 생성한다 (Tools/README.md)
namespace YJJAssetPaths
{
	inline constexpr const TCHAR* PlayerSprite = TEXT("/Game/YJJJump/Sprites/SPR_Beetle.SPR_Beetle");
	inline constexpr const TCHAR* RopeSprite = TEXT("/Game/YJJJump/Sprites/SPR_Rope.SPR_Rope");
	inline constexpr const TCHAR* RopePegSprite = TEXT("/Game/YJJJump/Sprites/SPR_RopePeg.SPR_RopePeg");
	inline constexpr const TCHAR* ImpactSprite = TEXT("/Game/YJJJump/Sprites/SPR_Impact.SPR_Impact");
	inline constexpr const TCHAR* SkySprite = TEXT("/Game/YJJJump/Sprites/SPR_SkyBackground.SPR_SkyBackground");
	inline constexpr const TCHAR* CloudDecorSprite = TEXT("/Game/YJJJump/Sprites/SPR_CloudButt.SPR_CloudButt");
	inline constexpr const TCHAR* SkySpawnTable = TEXT("/Game/YJJJump/Data/DA_SpawnTable_Sky.DA_SpawnTable_Sky");

	// HUD에서 그리는 텍스처 (튜토리얼 끝 연출)
	inline constexpr const TCHAR* BaldEagleTexture = TEXT("/Game/YJJJump/Textures/T_MobBaldEagle.T_MobBaldEagle");
}
