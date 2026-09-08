import unreal, json
print(json.dumps({'world': unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name(), 'dirty_maps': [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()], 'dirty_content': [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]}))
