import unreal as u
c=u.load_class(None,'/Script/SixtySeconds.SSCharacter')
print([x.get_name() for x in u.get_default_object(c).get_components_by_class(u.ActorComponent)])
b=u.load_asset('/Game/Blueprints/Character/BP_Character')
print('BP',b.generated_class())
print([x.get_name() for x in u.get_default_object(b.generated_class()).get_components_by_class(u.ActorComponent)])
