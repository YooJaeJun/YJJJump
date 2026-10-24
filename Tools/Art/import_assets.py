"""생성한 PNG를 언리얼 에셋으로 만든다 (에디터 Python, 커맨드릿에서 실행).

만드는 것:
  /Game/YJJJump/Textures/T_*            픽셀아트 텍스처
  /Game/YJJJump/Sprites/SPR_*           Paper2D 스프라이트
  /Game/YJJJump/Data/Mobs/DA_Mob_*      몹 데이터
  /Game/YJJJump/Data/DA_SpawnTable_Sky  하늘 구간 스폰 테이블
  /Game/YJJJump/Maps/L_RopeTest         테스트 맵 (빈 맵, 게임모드가 탑을 생성)

여러 번 실행해도 같은 결과가 나오도록 있으면 갱신, 없으면 생성한다.
실행은 Tools/Art/BuildArt.ps1 참고.
"""

import os

import unreal

ROOT = "/Game/YJJJump"
TEXTURE_DIR = ROOT + "/Textures"
SPRITE_DIR = ROOT + "/Sprites"
MOB_DIR = ROOT + "/Data/Mobs"
DATA_DIR = ROOT + "/Data"
MAP_DIR = ROOT + "/Maps"

ART_DIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "Art", "Generated")

# 커맨드릿에서는 Paper2D 기본 머티리얼이 비어 있어서 직접 지정한다
SPRITE_MATERIAL = "/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial"

# 이름: 부드럽게 늘려야 하는 텍스처인지
TEXTURES = {
    "T_Player": False,
    "T_MobCloudSlime": False,
    "T_MobBird": False,
    "T_MobThunderCloud": False,
    "T_Impact": False,
    "T_Rope": False,
    "T_RopePeg": False,
    "T_SkyBackground": True,
}

MOBS = [
    dict(name="DA_Mob_CloudSlime", sprite="SPR_MobCloudSlime", display="Cloud Slime",
         health=1.0, radius=55.0, score=100, width=130.0, movement="STATIC", move_range=0.0, period=2.5),
    dict(name="DA_Mob_Bird", sprite="SPR_MobBird", display="Sky Bird",
         health=1.8, radius=50.0, score=150, width=130.0, movement="PATROL", move_range=260.0, period=4.0),
    dict(name="DA_Mob_ThunderCloud", sprite="SPR_MobThunderCloud", display="Thunder Cloud",
         health=3.5, radius=70.0, score=300, width=170.0, movement="HOVER", move_range=90.0, period=3.0),
]

# 몹 이름: (가중치, 등장 최소 높이 cm)
SKY_SPAWNS = {
    "DA_Mob_CloudSlime": (3.0, 0.0),
    "DA_Mob_Bird": (2.0, 800.0),
    "DA_Mob_ThunderCloud": (1.2, 2000.0),
}

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
    # 빈 스프라이트를 만들고 SourceTexture를 넣으면 PostEditChange가 크기를 잡는다
    sprite = get_or_create(sprite_name, SPRITE_DIR, unreal.PaperSprite, unreal.PaperSpriteFactory())

    sprite.set_editor_property("source_dimension", unreal.Vector2D(0.0, 0.0))
    sprite.set_editor_property("source_texture", texture)
    sprite.set_editor_property("default_material", material)
    sprite.set_editor_property("pixels_per_unreal_unit", 1.0)
    log("sprite " + sprite_name)
    return sprite


def make_mob(spec, sprites):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.YJJMobData)
    mob = get_or_create(spec["name"], MOB_DIR, unreal.YJJMobData, factory)

    mob.set_editor_property("display_name", spec["display"])
    mob.set_editor_property("max_health", spec["health"])
    mob.set_editor_property("radius", spec["radius"])
    mob.set_editor_property("score_value", spec["score"])
    mob.set_editor_property("sprite", sprites[spec["sprite"]])
    mob.set_editor_property("sprite_width", spec["width"])
    mob.set_editor_property("movement", getattr(unreal.YJJMobMovement, spec["movement"]))
    mob.set_editor_property("move_range", spec["move_range"])
    mob.set_editor_property("move_period", spec["period"])
    log("mob " + spec["name"])
    return mob


def make_spawn_table(mobs):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.YJJSpawnTableData)
    table = get_or_create("DA_SpawnTable_Sky", DATA_DIR, unreal.YJJSpawnTableData, factory)

    entries = []
    for name, (weight, min_height) in SKY_SPAWNS.items():
        entry = unreal.YJJMobSpawnEntry()
        entry.set_editor_property("mob", mobs[name])
        entry.set_editor_property("weight", weight)
        entry.set_editor_property("min_height", min_height)
        entries.append(entry)

    table.set_editor_property("entries", entries)
    table.set_editor_property("mob_chance_per_gap", 0.65)
    table.set_editor_property("safe_gap_count", 2)
    log("spawn table DA_SpawnTable_Sky")
    return table


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

    sprites = {}
    for name, smooth in TEXTURES.items():
        texture = import_texture(name, smooth)
        sprite = make_sprite(name, texture, material)
        sprites[sprite.get_name()] = sprite

    mobs = {spec["name"]: make_mob(spec, sprites) for spec in MOBS}
    make_spawn_table(mobs)
    make_test_map()

    assets.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
    log("done")


main()
