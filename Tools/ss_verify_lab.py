import unreal as u, json
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
allactors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
meshes=[a for a in allactors if isinstance(a,u.StaticMeshActor)]
assert world.get_path_name()=='/Game/SSLabPrototype/L_SSResearchLab.L_SSResearchLab'
assert len(meshes)==139
assert all(a.static_mesh_component.get_editor_property('static_mesh') for a in meshes)
gm=world.get_world_settings().get_editor_property('default_game_mode')
print('VERIFY',len(meshes),'meshes', 'pawn', u.get_default_object(gm).get_editor_property('default_pawn_class'))
u.get_editor_subsystem(u.LevelEditorSubsystem).editor_play_simulate()
