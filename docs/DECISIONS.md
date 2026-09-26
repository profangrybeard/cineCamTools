# CineCam Tools: decisions

What Tim decided and why, by step, newest first. Engine facts we had to find out the hard way are at the bottom. When a decision changes, add the new one with its date and say what it replaces; don't delete the old one.

## Step 5: 1.0 (2026-09-26)

- Version 1.0 is everything through step 4.3. `VersionName` "1.0", `Version` 1. Each release after bumps `Version` by one (1.1 is 2); Unreal compares that number, not the name.
- Git tag `v1.0` only. No GitHub release page.
- Docs split by job: `CLAUDE.md` (working notes for Claude), `docs/DECISIONS.md` (this), `docs/ARCHITECTURE.md`, `docs/CHANGELOG.md`, `docs/checklists/` (test records), `README.md` (students).
- Feature requests go in `docs/BACKLOG.md`, one entry each, never deleted. A "no" stays with its reason. The 1.1 plan is built from it, in steps like 1 to 4.

## Step 4: spot meter (2026-09-26)

- **HDR output:** turn every value tool off (modes, zebras, histogram, clip %, waveform, meter). Keep the thirds guide and the plumbing frame, which don't read values. Orange canvas line bottom left, above the override notice. Log once. Wrong numbers teach the wrong thing.
- **Spot meter:** reads the cursor in editor viewports, the frame center otherwise (PIE, games, cursor elsewhere). Fixed box, 9 x 9 at 1080p, scaled with view height. Averages `LumaLevel()`. Readout is level and zone, canvas only, never baked. On the component (keyable), in the toolbar, and `r.ValueScope.SpotMeter`. No built-in preset turns it on. The cursor reaches the runtime module through an editor hook, like the toolbar.
- **Zones in the readout** use false color's formula, so the readout and the false color under the box always agree.
- **Pins:** up to 4 (A to D) per viewport, not saved, reading live. B to D show the difference from A in levels and zones, never stops (stops would need light before the tonemapper, which breaks the post-tonemap rule). Rebindable command "Pin Spot Meter Point", plus Pin and Clear Pins in the toolbar menu.
- **Pin key is Alt+M.** Ctrl+Alt+M was the first default and didn't work on Tim's machine. Alt+M is also Control Rig's in its edit mode and DMX's in its own editor, neither of which is the normal level editor.
- **Toolbar label** never says "Off" while enabled: mode name, "Overlays" (Mode Off with overlays, like the Exposure preset), or "On". "Value Scope: Off" read as the scope being off.
- Order was 4.1 HDR notice, 4.2 live meter, 4.3 pins, each tested and committed alone.

## Step 3: keying, presets, toolbar (2026-09-25 and 26)

- **Precedence, first match wins:** console cvars (yellow notice), then the Value Scope component on the view actor (fed by Sequencer keys and applied presets), then the editor toolbar.
- **A component wins even when disabled**, so piloting a camera looks exactly like PIE. A keyed Enabled off shows nothing.
- **Sequencer:** every component setting and Enabled are `Interp`, like the CineCamera's lens settings.
- **Presets are copied**, not linked: editable and keyable after, and editing a preset asset later doesn't change cameras that used it.
- **Built-in presets live in code**, one definition (`GetBuiltInPreset`). Anything a preset doesn't list is off, and clip levels reset to 0.02 / 0.98:

  | Preset | Mode | Overlays |
  |---|---|---|
  | Notan | Notan (0.25 / 0.75) | none |
  | Value Check | False Color | Histogram |
  | Exposure | Off | Clip Zebras, Histogram, Clip Percentages, Waveform |
  | Composition | Notan | Thirds Guide |

- **Teachers' presets** are `UValueScopePreset` data assets with a Description. Save as Preset makes one from a camera.
- **Picker:** a full-width row at the top of the Value Scope category, Apply Preset (built-ins, then project presets) and Save as Preset. Full width because the value column cut off the second button.
- **Toolbar:** its own settings layer, not the cvars. One setting shared by all level viewports, saved per user and per project, off at first launch. Level editor viewports only; never material editor, thumbnails, asset previews or PIE. It also applies while piloting a camera with no component. Thresholds and clip levels are edited only in Editor Preferences (More Settings). Picking anything in its menu turns it on. Click toggles, arrow opens the menu.
- **Editor code lives in `CineCamToolsEditor`**, an editor-only module. The runtime module only exposes hooks the editor fills.
- The built-in preset enum is `EValueScopeBuiltInPreset`, because `EValueScopePreset` clashed with `UValueScopePreset` in Python.

## Step 2: histogram and waveform (2026-09-25)

- Histogram and waveform are luma only (Photoshop Luminosity). RGB is later, if ever.
- Histogram height scales to the tallest bin in levels 1 to 254. Levels 0 and 255 draw as clip markers, so a spike there can't flatten the rest.
- Histogram panel top right, waveform top left. About 30% of view width, scaled by view height, dim backing, top margin clear of the editor viewport toolbar. The override notice moved to the bottom left so the waveform doesn't cover it.
- Panels are drawn by the overlay pass, so they bake into HighResShot and Movie Render Queue like zebras. Clip % text is canvas (from the GPU readback), so it doesn't. That split is fine.
- Waveform: image columns across, levels 0 to 255 up, brightness = pixel count.
- Clipping is whole levels everywhere: level at or below round(Black Clip x 255) is crushed, at or above round(White Clip x 255) is blown.
- Each panel has a component toggle and an `r.ValueScope.*` cvar the override notice reports.

## Foundations: plumbing and step 1

These are the rules that should not drift.

- **Post-tonemap only.** Pre-tonemap luminance is what the built-in Eye Adaptation view already shows, and it doesn't match Photoshop.
- **Luma weights 0.30 / 0.59 / 0.11**, Photoshop's Luminosity histogram. Not Rec.709. A pixel's level is `(30R + 59G + 11B + 50) / 100` on the 8-bit color: whole-number math, so the shader and `value_report.py` agree exactly.
- **Shader, component defaults and `value_report.py` share constants** (weights, notan thresholds, clip levels, zone palette). Change all or none.
- **Modes:** off, plumbing check (magenta frame), notan (3 values), false color (11 zones, 0 to X).
- **Overlays** (zebras, thirds) draw on top of any mode, including Off. Sizes scale with view height, 1x per 540 px.
- **Cvars:** -1 uses the camera, 0 forces off, 1 forces on. `r.ValueScope.Mode 0` turns everything off. While any cvar overrides, a yellow on-screen notice says so, never in screenshots.
- **Modules:** shaders module at PostConfigInit, runtime module at Default. No UObjects in the shaders module.
- **Docs and on-screen text:** plain language, no em-dashes. Students read this.

## Engine facts (UE 5.8.3, CL 58210709)

- `ScreenPass.h` and `PostProcess/PostProcessMaterialInputs.h` are in `Renderer/Public`.
- `SubscribeToPostProcessingPass(EPostProcessingPass, const FSceneView&, FPostProcessingPassDelegateArray&, bool)` is the live overload; the one without the view is deprecated since 5.5. `FAfterPassCallbackDelegate(Array)` are aliases for `FPostProcessingPassDelegate(Array)`.
- `Inputs.GetInput(...)` returns `FScreenPassTextureSlice`. `FScreenPassTexture::CopyFromSlice` returns `FScreenPassTexture`. `Inputs.OverrideOutput` is an `FScreenPassRenderTarget`, and when valid it's the backbuffer, so we must write there.
- Per-view data can't be keyed by view pointer: the renderer copies each `FSceneView` into a new `FViewInfo` (`SceneRendering.cpp`). We key by `InView.State`, which carries over and is unique per view. Views with no State get no overlay.
- `InView.ViewActor` is null during `SetupView` in a level viewport, even while piloting: `FEditorViewportClient::CalcSceneView` calls `SetupView` (`EditorViewportClient.cpp:1650`) before `FLevelEditorViewportClient::CalcSceneView` sets `ViewActor` (`LevelEditorViewport.cpp:2563`). We resolve in `BeginRenderViewFamily` (`SceneRenderBuilder.cpp:511`), after every view is complete. Found 2026-09-26: earlier checks all ran in PIE, where `LocalPlayer.cpp` sets ViewActor first.
- HDR output after Tonemap is PQ or scRGB, not 0 to 1. `IsHDREnabled()` (`RenderCore.cpp:433`) is `GRHISupportsHDROutput && r.HDR.EnableHDROutput`, so without HDR output support the cvar does nothing. We also check `FSceneViewFamily::bIsHDR` and HighResShot's `bCaptureHDR`.
- A replaced tonemapper (post process material at "Replacing the Tonemapper") still runs `AddAfterPass(EPass::Tonemap)`, so our hook runs either way.
- The level viewport toolbar is `LevelEditor.ViewportToolbar`, section "Right". A toggle-plus-arrow entry is `FToolMenuEntry::InitSubMenu` with an action and `EUserInterfaceActionType::ToggleButton`, like the engine's Surface snapping button.
- `UDataAssetFactory::ConfigureProperties` shows a class picker; `IAssetTools::CreateAssetWithDialog(..., bCallConfigureProperties = false)` skips it.
- Level viewport clients' `ViewState` and `StereoViewStates` are public on `FEditorViewportClient`, which is how the toolbar tells a level viewport from every other view.
