import unreal as u
worlds=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
print('Simulation world:',worlds)
assert worlds is not None
u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
