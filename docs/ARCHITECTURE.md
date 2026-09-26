# CineCam Tools: how it's built

For whoever maintains it next. Why things are this way is in `DECISIONS.md`.

## Paths

| What | Path |
|---|---|
| This repo (the plugin root) | `C:\SCAD\Projects\cineCamTools` |
| Workbench host project (UE 5.8.3, C++) | `C:\_projects\pluginWorkbench` |
| Plugin inside workbench (junction to this repo) | `C:\_projects\pluginWorkbench\Plugins\CineCamTools` |
| Engine | `%UE_ROOT%`, default `C:\Program Files\Epic Games\UE_5.8` |

The workbench is only a host. Never edit plugin files through the workbench path and never commit workbench files here.

## Layout

```
CineCamTools.uplugin
Source/CineCamToolsShaders/   PostConfigInit. Shader dir mapping. FValueScopePS (permutations
                              VALUE_SCOPE_HISTOGRAM, VALUE_SCOPE_WAVEFORM), FValueScopeHistogramCS,
                              FValueScopeHistogramMaxCS, FValueScopeWaveformCS, FValueScopeSpotMeterCS.
Source/CineCamTools/          Default (runtime). UValueScopeComponent (+ built-in presets),
                              UValueScopePreset, FValueScopeViewExtension, module.
                              Public/ValueScopeEditorHook.h: the hooks the editor module fills.
Source/CineCamToolsEditor/    Editor only. FValueScopeComponentDetails (preset picker, Save as Preset),
                              ValueScopePresetList (the one preset list both menus use),
                              UValueScopeEditorSettings + ValueScopeToolbar (level viewport toolbar,
                              toolbar resolver, cursor provider), FValueScopeCommands (pin keys).
Shaders/Private/ValueScope.usf  Every shader entry point: MainPS, HistogramCS, HistogramMaxCS,
                              WaveformCS, SpotMeterCS.
Tools/value_report.py         CPU reference for the shader math, runs on a PNG.
Scripts/                      link_workbench.bat, build_workbench.bat, package_plugin.bat
Config/FilterPlugin.ini       Stock template from BuildPlugin, nothing listed yet.
docs/                         DECISIONS, ARCHITECTURE (this), CHANGELOG, BACKLOG, checklists/
```

## How a frame gets its overlay

1. **Settings live in three places.** `UValueScopeComponent` on a camera holds `FValueScopeSettings` (all `Interp`, so Sequencer keys them). `UValueScopeEditorSettings` holds the toolbar's copy. The `r.ValueScope.*` cvars override both.
2. **Resolve, game thread.** `FValueScopeViewExtension::BeginRenderViewFamily` calls `ResolveView` for each view. Not `SetupView`: see the ViewActor fact in `DECISIONS.md`. In order:
   - The component on `InView.ViewActor`, if any. It wins even when disabled.
   - With no component, the editor resolver: is this view's `State` a level viewport client's `ViewState`, and is the toolbar on?
   - The cvars on top.
   - HDR check (`IsHDROutput`): if the output is HDR, strip every value tool (`ReadsValues`) and mark the view for the orange notice.
   - Spot meter points: the live point (cursor from the editor cursor provider if it's over this view's image, else the center) plus this view's pins.
   - Results go into `Pending` (for the render thread, with a HighResShot flag from `GIsHighResScreenshot`) and `CanvasSettings` / `CanvasSpots` (for canvas text), all keyed by the view's `State`.
3. **Subscribe, render thread.** `SubscribeToPostProcessingPass` takes the view's `Pending` entry for `EPostProcessingPass::Tonemap` and adds `AfterTonemap_RenderThread`.
4. **Draw, render thread.** `AfterTonemap_RenderThread`, in order:
   - `HistogramCS` if Histogram or Clip Percentages is on. Queues a GPU readback into a 4-slot ring per view.
   - `SpotMeterCS` if the meter is on: one group per point, sum and count of `LumaLevel()` in the box. Own 4-slot readback ring.
   - `HistogramMaxCS` if the histogram panel is on (tallest bin in 1 to 254).
   - `WaveformCS` if Waveform is on (512 columns x 256 levels).
   - `FValueScopePS` full screen: mode, zebras, thirds, waveform panel, histogram panel, plumbing frame. If `Inputs.OverrideOutput` is valid it writes there, because that's the backbuffer when Tonemap is the last pass.
5. **Canvas, game thread.** Through `UDebugDrawService` ("Rendering" show flag, so every editor and game viewport): override notice (yellow, bottom left), HDR notice (orange, above it), clip percentages (under the histogram panel), spot meter boxes and readouts. All from the latest readbacks, 2 to 3 frames late. Canvas units are pixels / DPI. Skipped during HighResShot, so none of it is ever baked.
6. **Repaint.** Cvar changes broadcast `FEditorSupportDelegates::RedrawAllViewports`; toolbar changes call `RedrawLevelEditingViewports`, so non-Realtime viewports repaint.

What bakes into HighResShot and Movie Render Queue: modes, zebras, thirds, histogram and waveform panels, plumbing frame. What never does: all canvas text and boxes.

## Editor hooks

The runtime module never includes editor code. `Public/ValueScopeEditorHook.h` has game-thread hooks the editor module fills at startup and clears at shutdown. Packaged games never set them.

| Hook | Filled by | Used for |
|---|---|---|
| `SetEditorViewportResolver` | `ValueScopeToolbar` | Toolbar settings for level viewports with no camera component |
| `SetEditorCursorProvider` | `ValueScopeToolbar` | Mouse position for the spot meter |
| `AddSpotPin`, `ClearSpotPins` | `FValueScopeCommands` actions, toolbar menu | Pins A to D, stored per view state in the extension |

## Shared math

- `LumaLevel()` in the shader is the one definition of a pixel's level 0 to 255: quantize to 8 bits, then `(30R + 59G + 11B + 50) / 100`. Histogram, waveform, zebras, clip percentages and the spot meter all use it. `value_report.py` uses the same formula.
- Zones: `floor(luma * 10.999)`, 0 to 10. False color paints them; the spot meter reports them from its averaged level.
- Built-in presets: `UValueScopeComponent::GetBuiltInPreset`. The Details picker, Blueprint and the toolbar all call it, and both menus list presets through `ValueScopePresetList`.
- Sizes: `Scale = max(1, round(view height / 540))`. Zebra stripes, line widths, text scale and the spot meter box (half size `2 * Scale`) follow it.

## Cvars

- `r.ValueScope.Mode`: -1 camera or toolbar, 0 everything off (component and toolbar included), 1 plumbing, 2 notan, 3 false color.
- `r.ValueScope.Zebras`, `.Thirds`, `.Histogram`, `.ClipPercent`, `.Waveform`, `.SpotMeter`: -1 camera or toolbar, 0 force off, 1 force on.
- `r.ValueScope.DumpHistogram`: writes each view's latest histogram to `Saved/ValueScope/*.csv`, including `_highresshot.csv` for the last HighResShot frame.
- `r.ValueScope.OverrideMessage 0` hides the yellow notice (so does `DisableAllScreenMessages`).

## Build

- Day to day: `Scripts\build_workbench.bat`. Builds `pluginWorkbenchEditor`, which compiles the plugin through the junction. Refuses to run while the editor is open (Live Coding).
- Packaging check: `Scripts\package_plugin.bat` (RunUAT BuildPlugin, no host). The monolithic game build catches missing includes the modular editor build hides.
- C++ errors come from the build. Shader errors only appear when the editor launches: Output Log, search `ValueScope.usf`. Shader-only changes: `recompileshaders changed` in the editor console, no rebuild.
- New UPROPERTYs, shader parameters, modules or classes need a full editor restart; Live Coding can't take them.

## Known risks

1. In editor viewports with icons showing, the editor primitive composite runs after our Tonemap hook and changes pixels (not just the icons), so the histogram differs from a HighResShot of that view by a few percent. Game View (G), PIE, games and Movie Render Queue don't run it. No post-process hook exists after it. The histogram dump line says "other passes follow ours" when this applies.
2. Live HDR output is untested: the test machine's display has no HDR output path. The Capture HDR screenshot path was tested. See `BACKLOG.md`.
3. With Realtime off, the spot meter reading can lag the cursor by a frame or two (readbacks arrive after the repaint).

## Extending the toolbar

A working 5.8 example of extending the level viewport toolbar: `Engine/Plugins/Developer/RenderDocPlugin/Source/RenderDocPlugin/Private/SRenderDocPluginEditorExtension.cpp` (also PixWinPlugin and GPUReshape). The toggle-plus-arrow pattern is in `Editor/UnrealEd/Private/ViewportToolbar/UnrealEdViewportToolbar.cpp` (Surface snapping).
