"""생성한 PNG를 언리얼 에셋으로 만든다 (에디터 Python, 커맨드릿에서 실행).

만드는 것:
  /Game/YJJJump/Textures/T_*            픽셀아트 텍스처
  /Game/YJJJump/Sprites/SPR_*           Paper2D 스프라이트
  /Game/YJJJump/Maps/L_RopeTest         테스트 맵 (빈 맵, 게임모드가 탑을 생성)

몬스터·스폰 수치는 에셋이 아니라 Content/YJJJump/Data/*.csv (게임이 직접 읽는다).

여러 번 실행해도 같은 결과가 나오도록 있으면 갱신, 없으면 생성한다.
실행은 Tools/Art/BuildArt.ps1 참고.
"""

import os

import unreal

ROOT = "/Game/YJJJump"
TEXTURE_DIR = ROOT + "/Textures"
SPRITE_DIR = ROOT + "/Sprites"
MAP_DIR = ROOT + "/Maps"
AUDIO_DIR = ROOT + "/Audio"

ART_DIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "Art", "Generated")

# 커맨드릿에서는 Paper2D 기본 머티리얼이 비어 있어서 직접 지정한다
SPRITE_MATERIAL = "/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial"

# 이름: 부드럽게 늘려야 하는 텍스처인지
TEXTURES = {
    "T_Beetle": False,
    "T_Beetle_Dash": False,
    "T_MobBaldSparrow_Hit": False,
    "T_MobBaldEagle_Hit": False,
    "T_Poof0": False,
    "T_Poof1": False,
    "T_Feather": False,
    "T_DizzyStar": False,
    "T_MobBaldSparrow": False,
    "T_MobBaldSparrow_Flap": False,
    "T_MobBaldEagle": False,
    "T_MobBaldEagle_Flap": False,
    "T_Impact": False,
    "T_CloudButt": False,
    "T_Rope": False,
    "T_RopePeg": False,
    "T_SkyBackground": True,
    "T_UI_Panel": False,
    "T_UI_Coin": False,
    "T_UI_Skull": False,
    "T_UI_Arrow": False,
    "T_UI_Flame": False,
}

# 이전 버전(귀여운 아트)에서 쓰던 에셋. 있으면 지운다
OBSOLETE = [
    "/Game/YJJJump/Textures/T_Player",
    "/Game/YJJJump/Textures/T_MobCloudSlime",
    "/Game/YJJJump/Textures/T_MobBird",
    "/Game/YJJJump/Textures/T_MobThunderCloud",
    "/Game/YJJJump/Sprites/SPR_Player",
    "/Game/YJJJump/Sprites/SPR_MobCloudSlime",
    "/Game/YJJJump/Sprites/SPR_MobBird",
    "/Game/YJJJump/Sprites/SPR_MobThunderCloud",
    "/Game/YJJJump/Data/Mobs/DA_Mob_CloudSlime",
    "/Game/YJJJump/Data/Mobs/DA_Mob_Bird",
    "/Game/YJJJump/Data/Mobs/DA_Mob_ThunderCloud",
    # 몬스터·스폰 데이터 에셋은 CSV 데이터 테이블로 옮겼다
    "/Game/YJJJump/Data/Mobs/DA_Mob_BaldSparrow",
    "/Game/YJJJump/Data/Mobs/DA_Mob_BaldEagle",
    "/Game/YJJJump/Data/DA_SpawnTable_Sky",
]

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
assets = unreal.EditorAssetLibrary


def log(message):
    unreal.log("[YJJArt] " + message)


def get_or_create(name, directory, asset_class, factory):
    path = directory + "/" + name
    if assets.does_asset_exist(path):
        return assets.load_asset(path)
    asset = asset_tools.create_asset(name, directory, asset_class, factory)
    if asset is None:
        raise RuntimeError("Failed to create " + path)
    return asset


def import_texture(name, smooth):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(ART_DIR, name + ".png"))
    task.set_editor_property("destination_path", TEXTURE_DIR)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    asset_tools.import_asset_tasks([task])

    texture = assets.load_asset(TEXTURE_DIR + "/" + name)
    if texture is None:
        raise RuntimeError("Failed to import " + name)

    # 픽셀아트: 압축/밉맵 없이 최근접 필터
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR if smooth else unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property("srgb", True)
    log("texture " + name)
    return texture


def make_sprite(texture_name, texture, material):
    sprite_name = "SPR_" + texture_name[2:]
    # 팩토리의 InitialTexture는 Python에 노출되지 않는다.
    # 빈 스프라이트를 만들고 SourceTexture와 크기를 직접 넣는다
    # (같은 텍스처를 다시 넣으면 PostEditChange가 크기를 다시 잡지 않으므로 크기는 항상 명시)
    sprite = get_or_create(sprite_name, SPRITE_DIR, unreal.PaperSprite, unreal.PaperSpriteFactory())

    sprite.set_editor_property("sprite_collision_domain", unreal.SpriteCollisionMode.NONE)
    sprite.set_editor_property("source_texture", texture)
    sprite.set_editor_property("source_uv", unreal.Vector2D(0.0, 0.0))
    sprite.set_editor_property("source_dimension", unreal.Vector2D(float(texture.blueprint_get_size_x()), float(texture.blueprint_get_size_y())))
    sprite.set_editor_property("default_material", material)
    sprite.set_editor_property("pixels_per_unreal_unit", 1.0)
    log("sprite " + sprite_name)
    return sprite


def import_sounds():
    # Tools/Audio/generate_audio.py가 직접 합성한 소리 (상업적 사용 자유)
    audio_dir = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "Audio", "Generated")
    for file_name in sorted(os.listdir(audio_dir)):
        if not file_name.endswith(".wav"):
            continue
        name = file_name[:-4]
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(audio_dir, file_name))
        task.set_editor_property("destination_path", AUDIO_DIR)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", False)
        asset_tools.import_asset_tasks([task])

        sound = assets.load_asset(AUDIO_DIR + "/" + name)
        if sound is None:
            raise RuntimeError("Failed to import " + file_name)
        sound.set_editor_property("looping", name.startswith("BGM"))
        log("sound " + name)


def make_test_map():
    path = MAP_DIR + "/L_RopeTest"
    if assets.does_asset_exist(path):
        log("map exists " + path)
        return
    get_or_create("L_RopeTest", MAP_DIR, unreal.World, unreal.WorldFactory())
    log("map " + path)


def main():
    material = unreal.load_asset(SPRITE_MATERIAL)
    if material is None:
        raise RuntimeError("Missing " + SPRITE_MATERIAL)

    for path in OBSOLETE:
        if assets.does_asset_exist(path):
            assets.delete_asset(path)
            log("deleted " + path)

    sprites = {}
    for name, smooth in TEXTURES.items():
        texture = import_texture(name, smooth)
        sprite = make_sprite(name, texture, material)
        sprites[sprite.get_name()] = sprite

    import_sounds()
    make_test_map()

    assets.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
    log("done")


main()
