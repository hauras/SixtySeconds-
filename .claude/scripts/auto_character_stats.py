# USSCharacterStats 컴포넌트를 가진 BP_SSCharacter Blueprint 생성
# 에디터 콘솔(~ 키)에서 실행: py "D:/Unreal Projects/SixtySeconds/.claude/scripts/auto_character_stats.py"

import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_asset_lib = unreal.EditorAssetLibrary

# BP_SSCharacter 생성 (부모: ASSGameMode 대신 Character 기반으로 추후 교체 예정)
factory = unreal.BlueprintFactory()
factory.set_editor_property("parent_class", unreal.Character)

bp_path = "/Game/Blueprints/Character"
bp_name = "BP_SSCharacter"

if not editor_asset_lib.does_asset_exist(f"{bp_path}/{bp_name}"):
    bp = asset_tools.create_asset(bp_name, bp_path, unreal.Blueprint, factory)
    unreal.log(f"[SS] {bp_name} 생성 완료: {bp_path}/{bp_name}")
else:
    bp = unreal.load_asset(f"{bp_path}/{bp_name}")
    unreal.log(f"[SS] {bp_name} 이미 존재, 로드함")

# USSCharacterStats 컴포넌트 추가
subsystem = unreal.get_editor_subsystem(unreal.SubobjectDataSubsystem)
root_data = subsystem.k2_gather_subobject_data_for_blueprint(bp)

stats_class = unreal.load_class(None, "/Script/SixtySeconds.SSCharacterStats")
if stats_class:
    handle = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
        parent_handle=root_data[0],
        new_class=stats_class,
        blueprint_context=bp
    ))
    unreal.log("[SS] USSCharacterStats 컴포넌트 추가 완료")
else:
    unreal.log_warning("[SS] SSCharacterStats 클래스를 찾지 못했습니다. 빌드 후 다시 시도하세요.")

editor_asset_lib.save_asset(bp.get_path_name())
unreal.log("[SS] BP_SSCharacter 저장 완료")
