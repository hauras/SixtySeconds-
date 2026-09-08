# SixtySeconds — Codex project instructions

## Project
- Unreal Engine 5.7, C++ and Blueprint, Windows single-player.
- Project file: SixtySeconds.uproject.
- GitHub origin: https://github.com/hauras/SixtySeconds-.git
- Installed engine: D:/Program Files/Epic Games/UE_5.7.
- Read CLAUDE.md for existing conventions. Its .claude workflows do not run automatically in Codex.
- Preserve existing uncommitted changes. Do not commit or publish them merely as part of connection setup.

## Design decisions accepted in the planning conversation
- 60-second 3D scramble followed by a day-based 2D shelter UI.
- Initial implementation retains one map and GameMode; switch input and UI at the phase boundary.
- USSRunSubsystem owns persistent run data. Shelter simulation must not depend on the scramble character Actor.
- Use custom stats and status rules, without GAS. Shelter effects advance once per turn, not every frame.
- Prefer Satiety and Hydration for meters where zero means starvation or dehydration.
- Keep companion identity, trust, and investigation evidence separate.
- A companion snapshot supports clue generation; a data difference alone does not prove replacement.
- These are target design decisions, not claims that the existing code implements them.

## Editing and verification
- Inspect the existing implementation before changes and use the SS project prefix.
- Never hand-edit generated headers, Intermediate, Binaries, or engine source.
- Create or modify .uasset and .umap files through Unreal-supported tools, not text editing.
- Inspect editor process state before running command-line editor automation; do not open a second unattended editor on the same project while it is open.
- Verify relevant C++ changes with the installed engine build tools when permissions and prerequisites allow.
- Python editor automation and live MCP control are not yet configured or verified.
- Report separately what was edited, compiled, tested in editor, and published.

## Git
- Use the existing origin; do not initialize a replacement repository.
- Do not discard user changes, force-push, or rewrite history without explicit authorization.
- Under the sandbox account, Git may require the per-command option:
  git -c safe.directory="D:/Unreal Projects/SixtySeconds" -C "D:/Unreal Projects/SixtySeconds" <command>
- Avoid changing global Git safe.directory settings for this account mismatch.