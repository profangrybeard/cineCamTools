# CineCam Tools: notes for Claude Code

Unreal Engine 5.8 plugin of camera tools for teaching lighting, value and composition at SCAD ITGM/GAME. First tool is **Value Scope**: overlays drawn on the final, post-tonemap image so students judge value in engine the way they'd judge it with a Photoshop histogram.

## Where we left off (2026-09-26)

- 3.1 and the pilot fix are committed and pushed (`d684851`).
- 3.2 presets is committed and pushed (`3e7bd28`).
- Step 3 is done and pushed (`3ea53d1`).
- Step 4 (spot meter) agreed 2026-09-26, see "Step 4 checklist". 4.1 HDR notice (plus toolbar label fix) passed on 2026-09-26, two checks skipped for lack of an HDR display, committed as "Value Scope step 4.1: HDR notice". Push when Tim says. Next: 4.2 live spot meter, write its checklist and wait for go.
- 3.2 design (agreed 2026-09-26): Preset row at the top of the component's Value Scope category, Apply Preset dropdown (built-ins, then every `UValueScopePreset` asset) and Save as Preset button; new editor module `CineCamToolsEditor` (3.3's toolbar goes there too).

  | Preset | Mode | Overlays |
  |---|---|---|
  | Notan | Notan (0.25 / 0.75) | none |
  | Value Check | False Color | Histogram |
  | Exposure | Off | Clip Zebras, Histogram, Clip Percentages, Waveform |
  | Composition | Notan | Thirds Guide |

## Working with Tim

- Tim teaches at SCAD and tests every step himself in the editor. He says "go" before any new step; discuss, diagram and agree on each step's design first.
- Commit and push only when he asks ("commit it", "push it", "commit and push"). End commit messages with the Co-Authored-By line.
- The build refuses to run while the editor is open (Live Coding). Check first (`Get-Process UnrealEditor`); if it's running, ask Tim to close it, and he replies "editor closed, build it". Shader-only (`.usf`) changes don't need a rebuild: `recompileshaders changed` in the editor console.
- Run `package_plugin.bat` in the background after each build; the monolithic game build catches things the editor build hides.
- Keep answers short and plain. No em-dashes anywhere students read.

## How we verify

- HighResShot pairs: `HighResShot 1` writes `C:\_projects\pluginWorkbench\Saved\Screenshots\WindowsEditor\HighresScreenshot000NN.png`. Tim reports the number; compare against `value_report.py` (notan, false color, clip %, `--compare-histogram`).
- `r.ValueScope.DumpHistogram` writes `Saved/ValueScope/*.csv`, including `_highresshot.csv` for the HighResShot frame itself. Byte-identical to `value_report.py --histogram-csv` of that PNG in Game View.
- Read the editor log directly: `C:\_projects\pluginWorkbench\Saved\Logs\pluginWorkbench.log` (commands show as `Cmd:`; ours log as `LogValueScope`).
- Panels bake into HighResShot, so a shot shows exactly what was drawn. Canvas text (notice, clip %) does not.

## Gotchas learned

- Tim's window screenshot tool makes canvas text vanish (focus change). Not a bug; ask him to read numbers off the screen instead.
- Exact histogram comparisons need Game View (G) with nothing selected: in editor viewports with icons, the editor primitive composite runs after our hook and changes pixels (dump line says "other passes follow ours").
- Two live frames of the same camera differ by Lumen and temporal noise (about 0.2 levels), so only same-frame comparisons can be exact.
- `r.ValueScope.Mode 0` turns everything off, the component included. When "the component does nothing", check the log for a leftover cvar.
- Clipping only happens at level 250 and up (or 5 and down); a bright-looking frame often isn't clipped because the tonemapper rolls highlights off. Zone X in false color starts at 232.
- CineCameraActor hides Auto Activate for Player; use Level Blueprint BeginPlay > Get Player Controller > Set View Target with Blend.
- The shell here mangles backslashes and quotes in heredocs. For multi-line code edits, write a Python patch script to the scratchpad with the Write tool and run it, or use the Edit tool.

## Current goal: roadmap, step 4

Plumbing and steps 1 to 3 are done. Tim said go on step 4: spot meter, HDR notice first. Work "Step 4 checklist" in order: 4.1 HDR notice, 4.2 live spot meter, 4.3 pins. Each tested and committed on its own.

Step 4 decisions (Tim, 2026-09-26):

- HDR output: turn every value tool off (modes, zebras, histogram, clip %, waveform, meter), keep the thirds guide and plumbing frame (geometry only), canvas notice bottom left above the override notice, log once. Wrong numbers teach the wrong thing.
- Spot meter samples the cursor in editor viewports, the frame center otherwise (PIE, games, cursor elsewhere). Fixed box, about 9x9 px at 1080p, scaled with view height. Averages `LumaLevel()`. Readout is level and zone, canvas only, never baked. `bSpotMeter` on the component (keyable), toolbar menu, `r.ValueScope.SpotMeter`. No built-in preset turns it on. Cursor reaches the runtime module through an editor hook, like the toolbar resolver.
- Pins: up to 4 (A to D), per viewport, not saved, live. Differences from A in levels and zones, never stops (that would need pre-tonemap light). Pinned with a rebindable editor command "Pin Spot Meter Point" (check the default key for conflicts), plus Pin and Clear Pins in the toolbar menu.

Step 3 decisions (Tim, 2026-09-25):

- Who decides what a view shows, first match wins: console cvars (yellow notice), then the Value Scope component on the view target (fed by Sequencer keys and applied presets), then the editor toolbar (level editor viewports only; never material editor, thumbnails or previews).
- Piloting a camera that has a component: the component wins, so it matches PIE. The toolbar covers the free viewport.
- Presets are copied into the component when chosen (editable and keyable after), not linked live. Built-in presets live in code (Notan, Value Check, Exposure, Composition); teachers can make their own as `UValueScopePreset` data assets.
- Toolbar: its own settings layer (not the cvars), one setting shared by all level viewports, remembered between sessions. Lives in a new editor-only module `CineCamToolsEditor`, extending `LevelEditor.ViewportToolbar`.

Step 2 decisions (Tim, 2026-09-25):

- Histogram and waveform are luma only (Photoshop Luminosity). RGB is later, if ever.
- Histogram height scales to the tallest bin in 1 to 254. Levels 0 and 255 draw as clip markers so a spike there can't flatten the rest.
- Histogram panel top right, waveform top left. About 30% of view width, scaled by view height, dim backing, top margin clear of the editor viewport toolbar. The override notice moves to bottom left so the waveform doesn't cover it.
- Panels are drawn by the overlay pass, so they bake into HighResShot and Movie Render Queue like zebras. Clip % text is canvas (from GPU readback), so it doesn't. That split is fine.
- Waveform: image columns across, levels 0 to 255 up, brightness = pixel count.
- Component toggles Histogram, Clip Percentages, Waveform, each with an `r.ValueScope.*` cvar the override notice reports.

## Paths

| What | Path |
|---|---|
| This repo (the plugin root) | `C:\SCAD\Projects\cineCamTools` |
| Workbench host project (UE 5.8.3, C++) | `C:\_projects\pluginWorkbench` |
| Plugin inside workbench (junction to this repo) | `C:\_projects\pluginWorkbench\Plugins\CineCamTools` |
| Engine | `%UE_ROOT%`, default `C:\Program Files\Epic Games\UE_5.8` |

The workbench is only a host. Never edit plugin files through the workbench path and never commit workbench files here. If the workbench has its own git repo, its `.gitignore` should exclude `Plugins/`.

## Layout

```
CineCamTools.uplugin
Source/CineCamToolsShaders/   PostConfigInit. Shader dir mapping; FValueScopePS (permutations
                              VALUE_SCOPE_HISTOGRAM, VALUE_SCOPE_WAVEFORM), FValueScopeHistogramCS,
                              FValueScopeHistogramMaxCS, FValueScopeWaveformCS.
Source/CineCamTools/          Default. UValueScopeComponent (+ built-in presets), UValueScopePreset,
                              FValueScopeViewExtension, module.
Source/CineCamToolsEditor/    Editor only. FValueScopeComponentDetails (preset picker, Save as Preset),
                              ValueScopePresetList (the one preset list both menus use),
                              UValueScopeEditorSettings + ValueScopeToolbar (level viewport toolbar).
Shaders/Private/ValueScope.usf  All shader entry points: MainPS, HistogramCS, HistogramMaxCS, WaveformCS.
Tools/value_report.py         CPU reference for the shader math, runs on a PNG.
Scripts/                      link_workbench.bat, build_workbench.bat, package_plugin.bat
Config/FilterPlugin.ini       Stock template from BuildPlugin, nothing listed yet.
```

## How it works

1. `UValueScopeComponent` sits on a CineCameraActor and holds `FValueScopeSettings` (all `Interp`, so Sequencer can key them).
2. `FValueScopeViewExtension::BeginRenderViewFamily` (game thread, per view via `ResolveView`; not `SetupView`, see "Resolved") reads the component off `InView.ViewActor` (a component wins even when disabled); with no component, asks the editor resolver (`ValueScopeEditorHook.h`, set by `CineCamToolsEditor`) whether the view's State is a level viewport client's ViewState and the toolbar is on; then applies the `r.ValueScope.*` cvar overrides, and stores settings in `Pending` (plus a HighResShot flag from `GIsHighResScreenshot`) keyed by the view's `State` pointer. It also keeps a game-thread copy in `CanvasSettings` for the canvas text.
3. `SubscribeToPostProcessingPass` (render thread) takes those settings for `EPostProcessingPass::Tonemap` and adds `AfterTonemap_RenderThread`.
4. That callback, in order: `HistogramCS` (if Histogram or Clip Percentages; queues a GPU readback into a 4-slot ring per view), `HistogramMaxCS` (if the histogram panel is on), `WaveformCS` (if Waveform), then `FValueScopePS` full screen, which draws the mode, zebras, thirds, waveform panel, histogram panel and plumbing frame. If `Inputs.OverrideOutput` is valid it must write there, because it's the backbuffer when Tonemap is the last pass.
5. Canvas text goes through `UDebugDrawService` ("Rendering" show flag, so every editor and game viewport): the console override notice (bottom left) and the clip percentages (under the histogram panel, from the latest readback). Canvas units are pixels / DPI. Skipped during HighResShot.
6. Cvar changes broadcast `FEditorSupportDelegates::RedrawAllViewports` so non-Realtime editor viewports repaint.

Shared math: `LumaLevel()` in the shader is the one definition of a pixel's level 0 to 255: quantize to 8 bits, then (30R + 59G + 11B + 50) / 100. Histogram, waveform, zebras and clip percentages all use it; `value_report.py` uses the same formula.

Modes: 0 off, 1 plumbing check (image untouched, 6px magenta frame on the view rect edges), 2 notan, 3 false color (11 zones, palette shared with `value_report.py`).

Overlays, on top of any mode including Off: clip zebras (diagonal stripes, red at or above White Clip, blue at or below Black Clip, tested on the source luma) and the thirds guide. Stripe period and line width scale with view height (1x per 540 px).

Cvars: `r.ValueScope.Mode` -1 component, 0 everything off, 1 to 3 force mode. `r.ValueScope.Zebras`, `r.ValueScope.Thirds`, `r.ValueScope.Histogram`, `r.ValueScope.ClipPercent` and `r.ValueScope.Waveform` -1 component, 0 force off, 1 force on. `r.ValueScope.DumpHistogram` writes each view's latest histogram (from the GPU readback) to `Saved/ValueScope/*.csv`. With no component on the view target, only what a cvar forces is drawn. While any override is set, a yellow on-screen notice says so (`r.ValueScope.OverrideMessage 0` hides it, and so does `DisableAllScreenMessages`; it never shows in HighResShot).

## Build

- Day to day: `Scripts\build_workbench.bat`. Builds `pluginWorkbenchEditor`, which compiles the plugin through the junction.
- Packaging check only: `Scripts\package_plugin.bat` (RunUAT BuildPlugin, no host).
- C++ errors come from the build. Shader errors only appear when the editor launches: Output Log, search `ValueScope.usf`.

## Plumbing checklist

Work top to bottom. Stop and report at the first failure.

- [x] `Scripts\link_workbench.bat` made the junction.
- [x] `build_workbench.bat` compiles with zero errors.
- [x] Editor opens, Output Log shows `CineCamTools: Value Scope view extension registered.` and no `ValueScope.usf` errors.
- [x] `r.ValueScope.Mode 1` in the viewport: magenta frame sits exactly on the viewport edges, image otherwise unchanged. Try Screen Percentage 50 too; frame must stay on the edges.
- [x] `r.ValueScope.Mode 2`: viewport goes to 3 values.
- [x] `r.ValueScope.Mode -1`, add Value Scope to a CineCamera, make it the view target in PIE: overlay shows through that camera only. CineCameraActor hides Auto Activate for Player, so use Level Blueprint: BeginPlay > Get Player Controller > Set View Target with Blend.
- [x] `HighResShot 1` with the scope off, then `python Tools\value_report.py <shot> --out previews`. The notan preview should match the in-engine notan.

## Step 1 checklist

Full editor restart after building (new UPROPERTYs and shader params, Live Coding can't take them). Work in the editor viewport unless noted.

- [x] Build clean, editor opens, no `ValueScope.usf` errors.
- [x] `r.ValueScope.Mode 3`: false color. Grey (zone V) where mid values are, no image colors left.
- [x] `r.ValueScope.Mode -1`, `r.ValueScope.Zebras 1`: image normal except stripes on clipped areas. Blow out something (raise exposure) to see red; crush something to see blue.
- [x] `r.ValueScope.Thirds 1`: two vertical and two horizontal lines at thirds, visible on sky and on floor. Screen Percentage 50: lines stay at thirds.
- [x] `r.ValueScope.Mode 0`: everything off, even with Zebras and Thirds at 1.
- [x] Component: Value Scope on the CineCamera with False Color, Clip Zebras and Thirds Guide on, PIE through it. All three show. Editor viewport stays clean with cvars back at -1.
- [x] Override notice: set any `r.ValueScope.*` cvar and a yellow line names it, in the editor viewport and PIE. All at -1: gone. `r.ValueScope.OverrideMessage 0` hides it, `DisableAllScreenMessages` hides it, `EnableAllScreenMessages` brings it back.
- [x] `Scripts\package_plugin.bat` passes. The editor build is modular and hides missing includes; the packaged game build is monolithic and does not (it caught a missing `Modules/ModuleManager.h`).
- [x] `HighResShot 1` with scope off, then with `r.ValueScope.Mode 3`. Script false color preview matches the shader's pixel by pixel (within rounding).

## Step 2 checklist

2.1 readback plumbing (histogram measured and read back, nothing drawn yet):

- [x] Build clean, editor opens, no `ValueScope.usf` errors.
- [x] `r.ValueScope.Histogram 1`, then `r.ValueScope.DumpHistogram`. Log line per view, pixels counted equals width x height (no MISMATCH). CSVs in `Saved/ValueScope`.
- [x] Game View (G), nothing selected, histogram still on (it doesn't change the image): `HighResShot 1`, then `r.ValueScope.DumpHistogram`. Dump line says "ours is the last pass". Compare the `_highresshot.csv` (same frame as the PNG): `python Tools\value_report.py <shot> --compare-histogram <csv>` shows about 100%. Result: CSVs byte-identical, 100.00% (binning is whole-number math on both sides: (30R + 59G + 11B + 50) / 100; float math split the exact .5 ties). Live dumps are other frames; Lumen and temporal noise move pixels 1 or 2 levels per frame, so those only reach 92 to 97% in dark flat scenes.
- [x] `package_plugin.bat` passes.

2.2 histogram panel (drawn by the overlay pass from the same frame's buffer; `HistogramMaxCS` finds the tallest bin in 1 to 254; `VALUE_SCOPE_HISTOGRAM` permutation compiles the panel out when off):

- [x] Build clean, editor opens, no `ValueScope.usf` errors.
- [x] `r.ValueScope.Histogram 1`: panel top right, dim backing, clear of the viewport toolbar, faint lines at 64 / 128 / 192. Screen Percentage 50: same place and size.
- [x] Photoshop check: Game View, `r.ValueScope.Histogram 0`, `HighResShot 1`. Open the PNG in Photoshop, Histogram panel, Luminosity. Same shape as the in-engine panel (Photoshop may scale height differently when 0 or 255 spike). Also read back from shot 11: panel bars vs script histogram, same peak level, correlation 0.9987.
- [x] Clip markers: raise exposure until something blows out, red bar and red strip at the right edge. Lower it, blue at the left. Reset exposure.
- [x] With `r.ValueScope.Mode 2` or `3`, zebras and thirds on: panel draws on top, and its shape doesn't change (it measures the image, not the overlay).
- [x] Component: Histogram on the CineCamera's Value Scope, PIE through it: panel shows.
- [x] `package_plugin.bat` passes.

2.3 clip percentages (canvas text from the readback, under the panel; clipping is whole levels everywhere: `LumaLevel()` in the shader, same formula in the script, level <= round(Black Clip x 255) and >= round(White Clip x 255)):

- [x] Build clean, editor opens, no `ValueScope.usf` errors.
- [x] `r.ValueScope.Histogram 1`, `r.ValueScope.ClipPercent 1`: "Crushed" (blue) and "Blown" (red) just under the panel, left edge and middle. Screen Percentage 50: still under it.
- [x] `r.ValueScope.Histogram 0`, clip text on: text moves up to where the panel's top would be.
- [x] Push exposure until something clips. Game View, `HighResShot 1`: the PNG has no text in it, and `python Tools\value_report.py <shot>` crushed and blown match the on-screen numbers to 0.1%. Result: shot 12 blown 22.2%, shot 13 crushed 10.2%, both matched on screen; no text in either PNG.
- [x] Same exposure, `r.ValueScope.Zebras 1`: stripes cover exactly the clipped areas the numbers count (red with Blown, blue with Crushed).
- [x] Component: Clip Percentages on the CineCamera's Value Scope, PIE through it: text shows. Clip levels are editable when Zebras or Clip Percentages is on.
- [x] `package_plugin.bat` passes.

2.4 waveform (GPU only, `WaveformCS` counts pixels into 512 columns x 256 levels with `LumaLevel()`; overlay pass draws it top left; `VALUE_SCOPE_WAVEFORM` permutation; override notice moved to bottom left):

- [x] Build clean, editor opens, no `ValueScope.usf` errors.
- [x] `r.ValueScope.Waveform 1`: panel top left, same size as the histogram, clear of the toolbar. Dark bottom, bright top, faint lines at 64 / 128 / 192. Screen Percentage 50: same place and size.
- [x] Reads the frame: sky across the top of the frame shows as a band high in the waveform's left-to-right span where the sky is; the floor lower. Pan the camera and the trace follows.
- [x] Clipped levels trace in blue (bottom) and red (top) when exposure is pushed, matching the zebras.
- [x] Override notice now bottom left, right of the axis gizmo, not under the waveform.
- [x] All panels together (Histogram, Clip Percent, Waveform, Mode 3, zebras, thirds): nothing overlaps badly.
- [x] Component: Waveform on the CineCamera's Value Scope, PIE through it: panel shows.
- [x] `package_plugin.bat` passes.

## Step 3 checklist

3.1 Sequencer keying (every component setting and Enabled are `Interp`, like the CineCamera's lens settings):

- [x] Build clean, editor opens.
- [x] Level Sequence with the CineCameraActor added: on its ValueScope component, + Track lists the Value Scope settings (Enabled, Mode, Clip Zebras, Histogram, and the rest).
- [x] Key Mode Notan at frame 0 and False Color at frame 60. Scrub while piloting the camera (or through a Camera Cut): the view switches at 60.
- [x] Key a bool (Clip Zebras on at 30) and a float (Shadow Threshold 0.25 to 0.5 over 0 to 60): both follow the timeline.
- [x] Movie Render Queue, a frame each side of 60: the renders show the switch.
- [x] `package_plugin.bat` passes.

Pilot fix (settings now resolved in `BeginRenderViewFamily`, because `ViewActor` was null in `SetupView` for piloted editor viewports):

- [x] Build clean.
- [x] Editor opens, all `r.ValueScope.*` cvars at -1 (`r.ValueScope.Thirds` was left at 1).
- [x] Pilot the CineCamera with the component: its histogram, waveform and thirds show. Eject: they go away.
- [x] Sequencer: scrub the 3.1 Mode keys while piloting, the view switches at 60.
- [x] PIE through the camera: still works.
- [x] Piloting, Game View, `HighResShot 1`: panels are baked into the PNG.
- [x] `package_plugin.bat` passes.

3.2 presets (`UValueScopeComponent::GetBuiltInPreset` is the one definition of the built-ins; `UValueScopePreset` data asset; `FValueScopeComponentDetails` in `CineCamToolsEditor` applies through the Settings property handle, so it's one undo step and covers multi-select and Blueprint templates):

- [x] Build clean.
- [x] Full editor restart (new module, new UCLASS). Opens with no errors, and no LogPython "same name" warning (the enum was `EValueScopePreset`, which clashed with `UValueScopePreset` in Python; now `EValueScopeBuiltInPreset`).
- [x] Value Scope component Details: a full-width row at the top of the Value Scope category with Apply Preset and Save as Preset side by side, both visible with the Details panel docked narrow. (First try put them in the value column and Save as Preset was cut off.)
- [x] Apply Preset lists Notan, Value Check, Exposure, Composition (hover shows each tooltip), then Project presets saying "None yet".
- [x] Apply each built-in while piloting: settings match the table in "Where we left off" and the view changes to match. Clip levels reset to 0.02 / 0.98.
- [x] Ctrl+Z after applying restores the previous settings in one step.
- [x] Edit a setting after applying (for example Histogram on with Notan): it sticks, and keys in Sequencer as before.
- [x] Save as Preset: name dialog opens straight away (no class picker), makes the asset with the camera's settings. Give it a Description.
- [x] The new asset shows under Project presets, with its Description as the tooltip. Apply it to a second camera: same settings.
- [x] Content Browser > Miscellaneous > Data Asset lists Value Scope Preset, and one made that way also shows in the list.
- [x] Change the preset asset: cameras that already used it don't change.
- [x] Two cameras selected, Apply Preset: both change.
- [x] Level Blueprint BeginPlay > Apply Built In Preset (Exposure) on the camera's Value Scope, PIE through it: Exposure shows.
- [x] `package_plugin.bat` passes.

3.3 toolbar (`UValueScopeEditorSettings`, config EditorPerProjectUserSettings; `ValueScopeToolbar` extends `LevelEditor.ViewportToolbar` "Right" with a toggle + submenu entry like the engine's Surface snapping; the runtime module only sees `ValueScope::SetEditorViewportResolver`). Decisions (Tim, 2026-09-26): the toolbar also applies while piloting a camera with no component; thresholds and clip levels only in Editor Preferences (More Settings); one setting shared by all level viewports. Picking anything in the menu also turns it on. Off at first launch.

- [x] Build clean.
- [x] Full editor restart. Opens with no errors. Value Scope button on the right of the level viewport toolbar, not highlighted, labeled "Value Scope".
- [x] Click it: turns on (highlighted, label "Value Scope: Notan"), free viewport shows notan. Click again: off.
- [x] Arrow menu: Enabled, Presets (built-ins, then project presets from 3.2), Mode (Off, Notan, False Color as radio), five overlay checkboxes, More Settings.
- [x] Each built-in preset: the viewport matches the table. Each overlay toggle and mode works. Picking one while off turns it on.
- [x] More Settings opens Editor Preferences > Plugins > Value Scope. Change Shadow Threshold there: the viewport updates.
- [x] Four-viewport layout: all level viewports show the same thing, including a non-Realtime one after a change.
- [x] Pilot the camera that has a Value Scope component: the component's settings show, not the toolbar's. Disable the component's Enabled: nothing shows (component wins). Eject: the toolbar's back.
- [x] Pilot a camera with no component: the toolbar shows.
- [x] Not in: material editor preview, Blueprint editor viewport, Content Browser thumbnails, Static Mesh editor. PIE shows only the component (like a game), never the toolbar.
- [x] Console still wins: toolbar on in notan, `r.ValueScope.Mode 3`: false color and the yellow notice. `-1`: back to the toolbar.
- [x] Restart the editor: the toolbar setting is remembered.
- [x] Toolbar on, Game View, `HighResShot 1`: the shot has the overlay baked in, same as a camera component.
- [x] `package_plugin.bat` passes.

For 3.3, start from a working 5.8 example of extending the level viewport toolbar: `Engine/Plugins/Developer/RenderDocPlugin/Source/RenderDocPlugin/Private/SRenderDocPluginEditorExtension.cpp` (also PixWinPlugin and GPUReshape). The menu is `LevelEditor.ViewportToolbar` (`SLevelViewport.cpp:2316`).

## Step 4 checklist

4.1 HDR notice (`IsHDROutput`: `IsHDREnabled()`, the family's `bIsHDR`, or HighResShot with Capture HDR; `ReadsValues` strips Notan, False Color, zebras, histogram, clip %, waveform; `HDRBlocked` set drives the canvas line):

- [x] Build clean.
- [x] Editor opens, no errors. SDR as usual: toolbar in Exposure preset plus Thirds, everything draws, no orange line. (Nothing changed for SDR.)
- [ ] `r.HDR.EnableHDROutput 1`. If the image changes to HDR: value tools vanish, thirds stay, orange line bottom left ("Value Scope is off: HDR output..."), one LogValueScope warning in the log. With a console override set too, the yellow line sits under the orange one. `r.HDR.EnableHDROutput 0`: everything back. If nothing changes on this monitor, skip this check (no HDR output path) and note it. SKIPPED 2026-09-26: Tim's internal display reports "HDR games, apps and more: Not supported" in Windows, so there's no HDR swapchain. Test on an HDR display when one is available.
- [x] High Resolution Screenshot window (viewport menu), tick Capture HDR, take a shot with the toolbar in Exposure plus Thirds: the saved image has the thirds lines and no value overlays. Untick it, shot again: overlays are back. Result: `HighresScreenshot_2026.09.26-12.42.43.png` (Capture HDR) thirds only, one LogValueScope warning; `HighresScreenshot00014.png` (off) histogram and thirds back (toolbar had Histogram + Thirds on).
- [ ] `DisableAllScreenMessages` hides the orange line; `EnableAllScreenMessages` brings it back. SKIPPED 2026-09-26: the line only shows while HDR is detected live, and Tim's display has no HDR output. Same code path as the override notice's check, which passed in step 1.
- [x] Toolbar label (folded in 2026-09-26): with Mode Off it said "Value Scope: Off" while enabled, which read as the scope being off. Now: Notan or False Color by mode, "Overlays" with Mode Off and any overlay (Exposure preset), "On" with nothing selected. Disabled: plain "Value Scope".
- [x] `package_plugin.bat` passes.

4.2 live spot meter, 4.3 pins: checklists written when each starts.

## Rules that should not drift

- **Post-tonemap only.** Pre-tonemap luminance is what the built-in Eye Adaptation view already shows, and it doesn't match Photoshop.
- **Luma weights 0.30 / 0.59 / 0.11**, Photoshop's Luminosity histogram. Not Rec.709.
- **Shader, component defaults and `value_report.py` share constants.** Change all or none.
- **Shaders module stays PostConfigInit, runtime module stays Default.** No UObjects in the shaders module. Editor-only code (Slate, UnrealEd) goes in `CineCamToolsEditor`, never the runtime module.
- **Built-in presets have one definition:** `UValueScopeComponent::GetBuiltInPreset`. The picker, Blueprint and the toolbar all call it, and both menus list presets through `ValueScopePresetList`.
- **Precedence, first match wins:** console cvars, then the Value Scope component on the view actor (even disabled), then the level viewport toolbar.
- **Docs and on-screen text:** plain language, no em-dashes. Students read this.

## Known risks, most likely first

1. In editor viewports with icons showing, the editor primitive composite runs after our Tonemap hook and changes pixels (not just the icons), so the histogram differs from a HighResShot of that view by a few percent. Game View (G), PIE, games and Movie Render Queue don't run it, and there the histogram matches the PNG. No post-process hook exists after it. The dump line says "other passes follow ours" when this applies.

When a risk is resolved, fix the code, delete the item here, and say what 5.8 actually does.

Resolved against the 5.8.3 install (CL 58210709):

- `ScreenPass.h` and `PostProcess/PostProcessMaterialInputs.h` are both in `Renderer/Public`. The Private and Internal include fallbacks are gone from both Build.cs files.
- `SubscribeToPostProcessingPass(EPostProcessingPass, const FSceneView&, FPostProcessingPassDelegateArray&, bool)` is the live overload. The one without the view is deprecated since 5.5. `FAfterPassCallbackDelegate(Array)` are aliases for `FPostProcessingPassDelegate(Array)`.
- `Inputs.GetInput(...)` returns `FScreenPassTextureSlice`. `FScreenPassTexture::CopyFromSlice(GraphBuilder, Slice)` returns `FScreenPassTexture`. `Inputs.OverrideOutput` is an `FScreenPassRenderTarget`.
- `Pending` can't be keyed by view pointer. The renderer copies each `FSceneView` into a new `FViewInfo` (`SceneRendering.cpp`, `Views.Emplace_GetRef(InViewFamily->Views[i])`), so `SubscribeToPostProcessingPass` sees a different address than the game thread did. It's now keyed by `InView.State`, which is copied over and unique per view. Views with no State get no overlay.
- `InView.ViewActor` is null during `SetupView` in a level viewport, even while piloting: `FEditorViewportClient::CalcSceneView` calls `SetupView` (`EditorViewportClient.cpp:1650`), and only afterward does `FLevelEditorViewportClient::CalcSceneView` set `ViewActor` to the locked actor (`LevelEditorViewport.cpp:2563`). A free (unpiloted) viewport has no ViewActor at all. We resolve settings in `BeginRenderViewFamily` instead (`SceneRenderBuilder.cpp:511`, game thread, after all views are complete, before the renderer copies them). Scene captures also go through `CreateSceneRenderer`, so they behave as before. Found 2026-09-26: step 1 to 3.1 component checks all ran in PIE, where `LocalPlayer.cpp` sets ViewActor before `SetupView`.
- HDR output after Tonemap is PQ or scRGB, not 0 to 1. `IsHDREnabled()` (`RenderCore.cpp:433`) is `GRHISupportsHDROutput && r.HDR.EnableHDROutput`, so on a machine without HDR output support the cvar does nothing. We also check `FSceneViewFamily::bIsHDR` and HighResShot's `bCaptureHDR` (writes linear HDR). When any is true, value tools are turned off with a notice (step 4.1).

## Roadmap

Done, step 1: false color zones, clip zebras, thirds guide, console override notice.

Done, step 2: GPU histogram with readback (matches Photoshop Luminosity exactly), histogram panel, clip percentages, waveform.

Done, step 3: Sequencer keying, presets (four built-ins, preset data assets, Save as Preset), level viewport toolbar. Also fixed: piloted cameras' components weren't found (settings now resolved in BeginRenderViewFamily).

Next (step 4): not planned yet.
