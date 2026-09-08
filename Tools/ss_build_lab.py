"""Create a separate, non-destructive research-lab blockout in the active editor."""
import unreal as u
ROOT = '/Game/SSLabPrototype'
MAP = ROOT + '/L_SSResearchLab'
assets = u.EditorAssetLibrary
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
if assets.does_asset_exist(MAP):
    raise RuntimeError('Map already exists; refusing to overwrite')
if u.EditorLoadingAndSavingUtils.get_dirty_map_packages() or u.EditorLoadingAndSavingUtils.get_dirty_content_packages():
    raise RuntimeError('Unsaved editor changes; refusing to switch maps')
assert levels.new_level(MAP)
cube = u.load_asset('/Engine/BasicShapes/Cube')
cylinder = u.load_asset('/Engine/BasicShapes/Cylinder')
palette = {}
for name, rgb, glow in [('Wall',(0.43,0.51,0.56),0),('Floor',(0.065,0.09,0.12),0),('Steel',(0.12,0.18,0.22),0),('White',(0.72,0.78,0.79),0),('Cyan',(0.03,0.65,0.9),3),('Red',(0.9,0.035,0.015),3),('Water',(0.025,0.35,0.8),0),('Food',(0.48,0.36,0.13),0),('Medical',(0.8,0.12,0.1),0),('Screen',(0.015,0.24,0.34),2)]:
    m = u.AssetToolsHelpers.get_asset_tools().create_asset('M_SS'+name, ROOT+'/Materials', u.Material, u.MaterialFactoryNew())
    c = u.MaterialEditingLibrary.create_material_expression(m,u.MaterialExpressionConstant3Vector)
    c.set_editor_property('constant',u.LinearColor(*rgb,1))
    u.MaterialEditingLibrary.connect_material_property(c,'',u.MaterialProperty.MP_BASE_COLOR)
    if glow:
        e = u.MaterialEditingLibrary.create_material_expression(m,u.MaterialExpressionConstant3Vector)
        e.set_editor_property('constant',u.LinearColor(*(v*glow for v in rgb),1))
        u.MaterialEditingLibrary.connect_material_property(e,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    u.MaterialEditingLibrary.recompile_material(m)
    assets.save_loaded_asset(m)
    palette[name]=m
count=0
def box(name,loc,size,mat='Wall',mesh=None):
    global count
    a=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*loc))
    a.set_actor_label('SS_'+name)
    a.set_folder_path('SS_Lab/'+name.split('_')[0])
    a.static_mesh_component.set_static_mesh(mesh or cube)
    a.static_mesh_component.set_material(0,palette[mat])
    a.set_actor_scale3d(u.Vector(*(v/100 for v in size)))
    count+=1
    return a
def light(x,y,z=300,color=(0.65,0.83,1.0),power=4500):
    a=actors.spawn_actor_from_class(u.PointLight,u.Vector(x,y,z))
    a.set_actor_label('SS_Light')
    a.point_light_component.set_editor_property('intensity',power)
    a.point_light_component.set_editor_property('attenuation_radius',1100)
    a.point_light_component.set_light_color(u.LinearColor(*color,1))
def text(label,x,y,z=220,yaw=90):
    a=actors.spawn_actor_from_class(u.TextRenderActor,u.Vector(x,y,z),u.Rotator(0,yaw,0))
    a.set_actor_label('SS_Sign_'+label)
    a.text_render.set_text(label)
    a.text_render.set_editor_property('world_size',38)
def table(x,y,kind='Lab'):
    box(kind+'_Desk',(x,y,90),(240,100,12),'White')
    for dx in [-100,100]:
        box(kind+'_DeskLeg',(x+dx,y,42),(12,80,84),'Steel')
    box(kind+'_Monitor',(x,y+20,135),(75,12,50),'Steel')
    box(kind+'_Display',(x,y+12,135),(65,2,40),'Screen')
def supplies(x,y,kind):
    for i in range(4):
        box(kind+'_Supply',(x+i*35,y,115),(24,24,38),kind,cylinder if kind in ['Food','Water'] else None)
def room(name,x,y,kind):
    # Each room opens onto the central corridor through a 240 cm gap.
    box(name+'_Floor',(x,y,-15),(1300,1500,30),'Floor')
    box(name+'_OuterWall',(x+(650 if x>0 else -650),y,180),(20,1500,360))
    for dy in [-750,750]:
        box(name+'_Wall',(x,y+dy,180),(1300,20,360))
    inner=x-650 if x>0 else x+650
    for dy in [-435,435]:
        box(name+'_DoorWall',(inner,y+dy,180),(20,630,360))
    box(name+'_Lintel',(inner,y,325),(20,240,70),'Steel')
    for dy in [-125,125]:
        box(name+'_DoorLight',(inner,y+dy,145),(26,8,280),'Cyan')
    table(x,y+400,name)
    table(x,y-400,name)
    supplies(x-55,y-400,kind)
    box(name+'_Island',(x+150,y,55),(160,280,110),'Steel')
    for dx in [-300,300]:
        light(x+dx,y)
        box(name+'_Light',(x+dx,y,350),(180,40,8),'Cyan')
    text(name.upper(),x-200,y+730)
box('Corridor_Floor',(0,0,-15),(400,3800,30),'Floor')
for x in [-160,160]:
    box('Corridor_Guide',(x,0,2),(8,3780,2),'Cyan')
for y in [-1450,-650,150,950,1750]: light(0,y)
for x,name,kind in [(-850,'Laboratory','Water'),(850,'Medical','Medical')]: room(name,x,950,kind)
for x,name,kind in [(-850,'Storage','Food'),(850,'Breakroom','Water')]: room(name,x,-650,kind)
box('Corridor_End',(0,1900,180),(400,20,360))
for x in [-200,200]: box('Corridor_EndWall',(x,1800,180),(20,200,360))
# Safe room: open northern doorway, terminals, bedrolls and stockpile.
box('Shelter_Floor',(0,-2450,-15),(1600,1100,30),'Floor')
box('Shelter_Back',(0,-3000,180),(1600,20,360))
for x in [-800,800]: box('Shelter_Side',(x,-2450,180),(20,1100,360))
for x in [-500,500]: box('Shelter_Entrance',(x,-1900,180),(600,20,360))
box('Shelter_Lintel',(0,-1900,330),(400,30,60),'Steel')
for x in [-760,760]: box('Shelter_Emergency',(x,-2450,15),(8,1050,12),'Red')
table(-500,-2780,'Shelter')
table(500,-2780,'Shelter')
for x in [-140,0,140]:
    box('Shelter_Bedroll',(x,-2800,18),(110,230,30),'Wall')
    box('Shelter_Pillow',(x,-2870,38),(90,55,20),'White')
for y in [-2150,-2300]:
    box('Shelter_Crate',(620,y,35),(100,100,70),'Food')
light(0,-2500,power=6500)
text('SAFE ROOM',-210,-2980)
text('RETURN TO SAFE ROOM',-140,-1870,290,-90)
# First-person walking prototype uses the engine's existing character movement.
factory=u.BlueprintFactory()
factory.set_editor_property('parent_class',u.Character)
pawn=u.AssetToolsHelpers.get_asset_tools().create_asset('BP_SSLabWalker',ROOT,u.Blueprint,factory)
u.KismetEditorUtilities.compile_blueprint(pawn) if hasattr(u,'KismetEditorUtilities') else None
assets.save_loaded_asset(pawn)
start=actors.spawn_actor_from_class(u.PlayerStart,u.Vector(0,1250,110),u.Rotator(0,-90,0))
start.set_actor_label('SS_PlayerStart')
# Retain a usable engine spectator control scheme for immediate layout exploration.
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',u.GameModeBase)
u.EditorLevelLibrary.set_level_viewport_camera_info(u.Vector(0,1600,230),u.Rotator(-8,-90,0))
assert levels.save_current_level()
print('SS_LAB_CREATED '+MAP+' meshes='+str(count))
