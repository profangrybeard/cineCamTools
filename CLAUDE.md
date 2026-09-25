# CineCam Tools: notes for Claude Code

Unreal Engine 5.8 plugin of camera tools for teaching lighting, value and composition at SCAD ITGM/GAME. First tool is **Value Scope**: overlays drawn on the final, post-tonemap image so students judge value in engine the way they'd judge it with a Photoshop histogram.

## Current goal: plumbing only

This is a clean start. The job right now is proving the pipeline works end to end, not adding features. Do not start roadmap items until every box in "Plumbing checklist" is checked and Tim says go.

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
2. `FValueScopeViewExtension::SetupView` (game thread) reads the component off `InView.ViewActor`, or the `r.ValueScope.Mode` cvar override, and stores settings in `Pending` keyed by the view's `State` pointer.
3. `SubscribeToPostProcessingPass` (render thread) takes those settings for `EPostProcessingPass::Tonemap` and adds `AfterTonemap_RenderThread`.
4. That callback draws `FValueScopePS` full screen. If `Inputs.OverrideOutput` is valid it must write there, because it's the backbuffer when Tonemap is the last pass.

Modes: 0 off, 1 plumbing check (image untouched, 6px magenta frame on the view rect edges), 2 notan.

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

## Rules that should not drift

- **Post-tonemap only.** Pre-tonemap luminance is what the built-in Eye Adaptation view already shows, and it doesn't match Photoshop.
- **Luma weights 0.30 / 0.59 / 0.11**, Photoshop's Luminosity histogram. Not Rec.709.
- **Shader, component defaults and `value_report.py` share constants.** Change all or none.
- **Shaders module stays PostConfigInit, runtime module stays Default.** No UObjects in the shaders module.
- **Docs and on-screen text:** plain language, no em-dashes. Students read this.

## Known risks, most likely first

1. `InView.ViewActor` may be null in editor viewports. That's why the cvar exists.
2. HDR output after Tonemap is PQ or scRGB, not 0 to 1. Out of scope. Skip or warn later.

When a risk is resolved, fix the code, delete the item here, and say what 5.8 actually does.

Resolved against the 5.8.3 install (CL 58210709):

- `ScreenPass.h` and `PostProcess/PostProcessMaterialInputs.h` are both in `Renderer/Public`. The Private and Internal include fallbacks are gone from both Build.cs files.
- `SubscribeToPostProcessingPass(EPostProcessingPass, const FSceneView&, FPostProcessingPassDelegateArray&, bool)` is the live overload. The one without the view is deprecated since 5.5. `FAfterPassCallbackDelegate(Array)` are aliases for `FPostProcessingPassDelegate(Array)`.
- `Inputs.GetInput(...)` returns `FScreenPassTextureSlice`. `FScreenPassTexture::CopyFromSlice(GraphBuilder, Slice)` returns `FScreenPassTexture`. `Inputs.OverrideOutput` is an `FScreenPassRenderTarget`.
- `Pending` can't be keyed by view pointer. The renderer copies each `FSceneView` into a new `FViewInfo` (`SceneRendering.cpp`, `Views.Emplace_GetRef(InViewFamily->Views[i])`), so `SubscribeToPostProcessingPass` sees a different address than `SetupView`. It's now keyed by `InView.State`, which is copied over and unique per view. Views with no State get no overlay.

## Roadmap (not now)

False color zones, clip zebras, thirds guide, luma histogram (compute + readback), on-screen clip percentages, presets data asset, waveform, Sequencer keying, editor toolbar toggle.
