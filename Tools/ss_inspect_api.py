import unreal as u
for cls,names in [(u.InputSettings,['add_axis_mapping','add_action_mapping','save_key_mappings']),(u.BlueprintEditorLibrary,['reparent_blueprint','compile_blueprint']),(u.AutomationLibrary,['take_high_res_screenshot']),(u.LevelEditorSubsystem,['editor_play_simulate','editor_request_end_play'])]:
    for n in names:
        print(n, getattr(cls,n,None).__doc__)
print('character',u.load_class(None,'/Script/SixtySeconds.SSCharacter'))
