# CineCam Tools: notes for Claude Code

Unreal Engine 5.8 plugin of camera tools for teaching lighting, value and composition at SCAD ITGM/GAME. First tool is **Value Scope**: overlays drawn on the final, post-tonemap image so students judge value in engine the way they'd judge it with a Photoshop histogram.

## Current goal: roadmap, step 2

Plumbing and step 1 are done. Tim said go on step 2. Work "Step 2 checklist" in order: readback plumbing (2.1) first, then histogram panel, clip text, waveform.

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
Source/CineCamToolsShaders/   PostConfigInit. Shader dir mapping + FValueScopePS.
Source/CineCamTools/          Default. UValueScopeComponent, FValueScopeViewExtension, module.
Shaders/Private/ValueScope.usf
Tools/value_report.py         CPU reference for the shader math, runs on a PNG.
Scripts/                      link_workbench.bat, build_workbench.bat, package_plugin.bat
```

## How it works

1. `UValueScopeComponent` sits on a CineCameraActor and holds `FValueScopeSettings`.
2. `FValueScopeViewExtension::SetupView` (game thread) reads the component off `InView.ViewActor`, applies the `r.ValueScope.Mode` / `.Zebras` / `.Thirds` cvar overrides, and stores settings in `Pending` keyed by the view's `State` pointer.
3. `SubscribeToPostProcessingPass` (render thread) takes those settings for `EPostProcessingPass::Tonemap` and adds `AfterTonemap_RenderThread`.
4. That callback draws `FValueScopePS` full screen. If `Inputs.OverrideOutput` is valid it must write there, because it's the backbuffer when Tonemap is the last pass.

Modes: 0 off, 1 plumbing check (image untouched, 6px magenta frame on the view rect edges), 2 notan, 3 false color (11 zones, palette shared with `value_report.py`).

Overlays, on top of any mode including Off: clip zebras (diagonal stripes, red at or above White Clip, blue at or below Black Clip, tested on the source luma) and the thirds guide. Stripe period and line width scale with view height (1x per 540 px).

Cvars: `r.ValueScope.Mode` -1 component, 0 everything off, 1 to 3 force mode. `r.ValueScope.Zebras`, `r.ValueScope.Thirds` and `r.ValueScope.Histogram` -1 component, 0 force off, 1 force on. `r.ValueScope.DumpHistogram` writes each view's latest histogram (from the GPU readback) to `Saved/ValueScope/*.csv`. With no component on the view target, only what a cvar forces is drawn. While any override is set, a yellow on-screen notice says so (`r.ValueScope.OverrideMessage 0` hides it, and so does `DisableAllScreenMessages`; it never shows in HighResShot).

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

2.3 clip text, 2.4 waveform: checklists written when each starts.

## Rules that should not drift

- **Post-tonemap only.** Pre-tonemap luminance is what the built-in Eye Adaptation view already shows, and it doesn't match Photoshop.
- **Luma weights 0.30 / 0.59 / 0.11**, Photoshop's Luminosity histogram. Not Rec.709.
- **Shader, component defaults and `value_report.py` share constants.** Change all or none.
- **Shaders module stays PostConfigInit, runtime module stays Default.** No UObjects in the shaders module.
- **Docs and on-screen text:** plain language, no em-dashes. Students read this.

## Known risks, most likely first

1. `InView.ViewActor` may be null in editor viewports. That's why the cvar exists.
2. In editor viewports with icons showing, the editor primitive composite runs after our Tonemap hook and changes pixels (not just the icons), so the histogram differs from a HighResShot of that view by a few percent. Game View (G), PIE, games and Movie Render Queue don't run it, and there the histogram matches the PNG. No post-process hook exists after it. The dump line says "other passes follow ours" when this applies.
3. HDR output after Tonemap is PQ or scRGB, not 0 to 1. Out of scope. Skip or warn later.

When a risk is resolved, fix the code, delete the item here, and say what 5.8 actually does.

Resolved against the 5.8.3 install (CL 58210709):

- `ScreenPass.h` and `PostProcess/PostProcessMaterialInputs.h` are both in `Renderer/Public`. The Private and Internal include fallbacks are gone from both Build.cs files.
- `SubscribeToPostProcessingPass(EPostProcessingPass, const FSceneView&, FPostProcessingPassDelegateArray&, bool)` is the live overload. The one without the view is deprecated since 5.5. `FAfterPassCallbackDelegate(Array)` are aliases for `FPostProcessingPassDelegate(Array)`.
- `Inputs.GetInput(...)` returns `FScreenPassTextureSlice`. `FScreenPassTexture::CopyFromSlice(GraphBuilder, Slice)` returns `FScreenPassTexture`. `Inputs.OverrideOutput` is an `FScreenPassRenderTarget`.
- `Pending` can't be keyed by view pointer. The renderer copies each `FSceneView` into a new `FViewInfo` (`SceneRendering.cpp`, `Views.Emplace_GetRef(InViewFamily->Views[i])`), so `SubscribeToPostProcessingPass` sees a different address than `SetupView`. It's now keyed by `InView.State`, which is copied over and unique per view. Views with no State get no overlay.

## Roadmap

Done, step 1: false color zones, clip zebras, thirds guide, console override notice.

Step 2 (next): luma histogram (compute + readback), on-screen clip percentages, waveform. First GPU readback, so it gets its own plumbing check.

Later: presets data asset, Sequencer keying, editor toolbar toggle.
