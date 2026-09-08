# Research laboratory blockout

Open `/Game/SSLabPrototype/L_SSResearchLab` in Unreal Editor.

Four rooms surround a 4 m wide corridor: laboratory, medical room, storage,
and breakroom. A southern safe room contains terminals, bedrolls, and crates.
Door gaps are 2.4 m wide. The map contains 139 primitive mesh actors and ten
new materials. Ceilings are intentionally open for blockout inspection.

Play uses a map-specific GameMode and engine DefaultPawn exploration controls:
WASD and mouse, with vertical flight. BP_SSLabWalker is currently an exploration
pawn despite its name; gravity-based walking is not implemented in this asset.
Supplies are decorative placeholders. Pickup, inventory, 60-second timer,
shelter phase switching and UI are not connected in this map.

Existing TestMap, project default map, C++ and existing Blueprints are unchanged.
Blueprint compilation and editor simulation were used for basic verification;
manual keyboard navigation and screenshot visual QA remain unverified.

Tools: ss_remote.py executes an explicitly supplied Python file in the one
discovered SixtySeconds editor. ss_build_lab.py refuses an existing target map
and refuses switching away from unsaved assets. ss_finish_lab.py is a one-time
follow-up setup script, not an idempotent updater. Do not rerun it on a finished
map. No script should be used to overwrite a hand-edited map.
