# CineCam Tools: notes for Claude Code

Unreal Engine 5.8 plugin of camera tools for teaching lighting, value and composition at SCAD ITGM/GAME. First tool is **Value Scope**: overlays drawn on the final, post-tonemap image so students judge value in engine the way they'd judge it with a Photoshop histogram.

**Version 1.0** (tag `v1.0`) is everything through step 4.3. Read these as needed:

| File | What's in it |
|---|---|
| `docs/DECISIONS.md` | Every decision Tim made, dated, by step, and the engine facts we had to find out |
| `docs/ARCHITECTURE.md` | Paths, layout, how a frame gets its overlay, editor hooks, shared math, cvars, build, known risks |
| `docs/BACKLOG.md` | Feature requests (FR-NNN) and the 1.1 plan |
| `docs/CHANGELOG.md` | What each version added, in plain words |
| `docs/checklists/1.0.md` | Every 1.0 checklist as tested |
| `README.md` | Student-facing |

## Where we left off (2026-09-26)

- 1.0 is done: docs split, `VersionName` 1.0, committed, tagged `v1.0` and pushed.
- Next: Tim adds feature requests to `docs/BACKLOG.md` as they come up. We discuss them, then pick a 1.1 plan from them. Nothing is planned for 1.1 yet.

## Working with Tim

- Tim teaches at SCAD and tests every step himself in the editor. He says "go" before any new step; discuss, diagram and agree on each step's design first.
- Commit and push only when he asks ("commit it", "push it", "commit and push"). Don't offer to commit on your own. End commit messages with the Co-Authored-By line.
- The build refuses to run while the editor is open (Live Coding). Check first (`Get-Process UnrealEditor`); if it's running, ask Tim to close it, and he replies "editor closed, build it". Shader-only (`.usf`) changes don't need a rebuild: `recompileshaders changed` in the editor console.
- Run `package_plugin.bat` in the background after each build; the monolithic game build catches things the editor build hides. Don't edit source while it runs, and commit only after it passes.
- Keep answers short and plain. No em-dashes anywhere students read.

## Backlog

- When Tim (or a student through him) asks for something that isn't the current step, add it to `docs/BACKLOG.md` with the next FR number and status "new". Don't start it.
- When we discuss an entry, update its Notes with what was decided and the date, and its Status and Size.
- Never delete an entry. A "no" keeps its reason.
- Planning 1.1: pull entries into the "1.1 plan" section as steps, with a design talk, a checklist in `docs/checklists/1.1.md`, and a commit per step, like 1.0. Record decisions in `docs/DECISIONS.md`. Bump `Version` in the uplugin to 2 and `VersionName` to "1.1" when 1.1 ships, add a CHANGELOG entry, and tag `v1.1` when Tim says.

## How we verify

- HighResShot: `HighResShot 1` writes `C:\_projects\pluginWorkbench\Saved\Screenshots\WindowsEditor\HighresScreenshot000NN.png` (with the High Resolution Screenshot window it can be `HighresScreenshot_<date-time>.png`). Tim reports the file; compare against `value_report.py` (notan, false color, clip %, `--compare-histogram`).
- `r.ValueScope.DumpHistogram` writes `Saved/ValueScope/*.csv`, including `_highresshot.csv` for the HighResShot frame itself. Byte-identical to `value_report.py --histogram-csv` of that PNG in Game View.
- Read the editor log directly: `C:\_projects\pluginWorkbench\Saved\Logs\pluginWorkbench.log` (commands show as `Cmd:`; ours log as `LogValueScope`).
- Toolbar settings are saved in `C:\_projects\pluginWorkbench\Saved\Config\WindowsEditor\EditorPerProjectUserSettings.ini`, section `ValueScopeEditorSettings`: read it when what's drawn doesn't match what Tim expects.
- Panels bake into HighResShot, so a shot shows exactly what was drawn. Canvas text and the spot meter do not.

## Gotchas learned

- Tim's window screenshot tool makes canvas text vanish (focus change). Not a bug; ask him to read numbers off the screen instead.
- Exact histogram comparisons need Game View (G) with nothing selected: in editor viewports with icons, the editor primitive composite runs after our hook and changes pixels (dump line says "other passes follow ours").
- Two live frames of the same camera differ by Lumen and temporal noise (about 0.2 levels), so only same-frame comparisons can be exact.
- `r.ValueScope.Mode 0` turns everything off, the component and toolbar included. When "the component does nothing", check the log for a leftover cvar.
- Clipping only happens at level 250 and up (or 5 and down); a bright-looking frame often isn't clipped because the tonemapper rolls highlights off. Zone X in false color starts at 232.
- CineCameraActor hides Auto Activate for Player; use Level Blueprint BeginPlay > Get Player Controller > Set View Target with Blend.
- Tim's display has no HDR output path. Test HDR through the High Resolution Screenshot window's Capture HDR option.
- Ctrl+Alt+M didn't register as a key binding on Tim's machine. Prefer two-key chords.
- The shell here mangles backslashes and quotes in heredocs. For multi-line code edits, write a Python patch script to the scratchpad with the Write tool and run it, or use the Edit tool. Patch scripts should check each pattern matches exactly once before writing anything.

## Rules that should not drift

- **Post-tonemap only.** Pre-tonemap luminance is what the built-in Eye Adaptation view already shows, and it doesn't match Photoshop.
- **Luma weights 0.30 / 0.59 / 0.11**, Photoshop's Luminosity histogram. Not Rec.709.
- **Shader, component defaults and `value_report.py` share constants.** Change all or none.
- **Shaders module stays PostConfigInit, runtime module stays Default.** No UObjects in the shaders module. Editor-only code (Slate, UnrealEd) goes in `CineCamToolsEditor`, never the runtime module; the runtime module only exposes hooks (`ValueScopeEditorHook.h`).
- **Built-in presets have one definition:** `UValueScopeComponent::GetBuiltInPreset`. Both menus list presets through `ValueScopePresetList`.
- **Precedence, first match wins:** console cvars, then the Value Scope component on the view actor (even disabled), then the level viewport toolbar.
- **Value tools off on HDR output.** Anything new that reads pixel values goes in `ReadsValues`.
- **Docs and on-screen text:** plain language, no em-dashes. Students read this.
