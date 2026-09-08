# L_Scramble 레벨 기본 형태 자동 생성
# 레이아웃:
#   [창고]─[복도]─[실험실]
#              │
#           [로비] ← 시작
#              │
#           [은신처입구]
#
# 에디터 콘솔(~ 키)에서 실행:
# py "D:/Unreal Projects/SixtySeconds/.claude/scripts/auto_create_scramble_map.py"

import unreal

ell = unreal.EditorLevelLibrary
cube_mesh = unreal.load_asset("/Engine/BasicShapes/Cube")
plane_mesh = unreal.load_asset("/Engine/BasicShapes/Plane")

def spawn_box(location, scale, label=""):
    actor = ell.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
    if actor:
        actor.static_mesh_component.set_static_mesh(cube_mesh)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        if label:
            actor.set_actor_label(label)
    return actor

def spawn_floor(location, scale, label=""):
    actor = ell.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
    if actor:
        actor.static_mesh_component.set_static_mesh(plane_mesh)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        if label:
            actor.set_actor_label(label)
    return actor

# ── 단위: 1유닛 = 100cm ──
# 방 크기: 800x800 / 복도: 200x400

# 바닥 전체 (대형 플레인)
spawn_floor((0, 0, 0), (30, 30, 1), "Floor_Main")

# ── 로비 (시작점) ──
# 벽 4면
spawn_box((0, 400, 200),   (8, 1, 4), "Lobby_Wall_N")
spawn_box((0, -400, 200),  (8, 1, 4), "Lobby_Wall_S")
spawn_box((400, 0, 200),   (1, 8, 4), "Lobby_Wall_E")
spawn_box((-400, 0, 200),  (1, 8, 4), "Lobby_Wall_W")

# ── 위쪽 복도 (로비 → 창고/실험실) ──
spawn_box((0, 800, 200),   (2, 4, 4), "Corridor_N_Wall_L")
spawn_box((300, 800, 200), (2, 4, 4), "Corridor_N_Wall_R")

# ── 창고 (좌상단) ──
spawn_box((-800, 1200, 200), (6, 1, 4), "Storage_Wall_N")
spawn_box((-800, 800, 200),  (6, 1, 4), "Storage_Wall_S")
spawn_box((-500, 1000, 200), (1, 4, 4), "Storage_Wall_E")
spawn_box((-1100, 1000, 200),(1, 4, 4), "Storage_Wall_W")

# ── 실험실 (우상단) ──
spawn_box((800, 1200, 200),  (6, 1, 4), "Lab_Wall_N")
spawn_box((800, 800, 200),   (6, 1, 4), "Lab_Wall_S")
spawn_box((1100, 1000, 200), (1, 4, 4), "Lab_Wall_E")
spawn_box((500, 1000, 200),  (1, 4, 4), "Lab_Wall_W")

# ── 아래쪽 복도 (로비 → 은신처) ──
spawn_box((150, -700, 200),  (2, 4, 4), "Corridor_S_Wall_L")
spawn_box((-150, -700, 200), (2, 4, 4), "Corridor_S_Wall_R")

# ── 은신처 입구 ──
spawn_box((0, -1100, 200),   (4, 1, 4), "Shelter_Wall_N")
spawn_box((0, -1300, 200),   (4, 1, 4), "Shelter_Wall_S")
spawn_box((200, -1200, 200), (1, 2, 4), "Shelter_Wall_E")
spawn_box((-200, -1200, 200),(1, 2, 4), "Shelter_Wall_W")

# ── 플레이어 스타트 (로비 중앙) ──
ell.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 100))
unreal.log("[SS] 플레이어 스타트 배치 완료")

# 저장
ell.save_all_dirty_levels()
unreal.log("[SS] L_Scramble 맵 생성 완료!")
