# CineCam Tools

Unreal Engine 5.8 camera tools for studying lighting, value and composition.

## Value Scope

Draws overlays on the final image a camera sees, after tonemapping. That's the same image a Photoshop histogram would read, so what you see in engine matches what you'd measure in Photoshop.

Modes (pick one):

- **Plumbing check.** Leaves the image alone and draws a magenta frame on the edges. If you see the frame, the tool is working.
- **Notan.** Reduces the frame to black, grey and white. If your shot reads as two blobs with no grey, it has no middle values.
- **False color.** Paints each of the 11 zones (0 to X) its own color, from purple in the deepest shadow through grey at middle (zone V) to red at pure white. Shows where every value in the frame sits.

Overlays (turn on with any mode, including Off):

- **Clip zebras.** Red stripes where the image is blown to white, blue stripes where it is crushed to black. Detail under the stripes is gone and can't be brought back in Photoshop. Zone X in false color only means near white (level 232 and up). Only the zebras show real clipping (level 250 and up, or 5 and down).
- **Thirds guide.** Rule of thirds lines for checking composition.
- **Histogram.** Top right. The same luma histogram Photoshop shows in its Histogram panel (Luminosity), measured from the frame before any overlay. Black on the left, white on the right. Pure black (0) and pure white (255) show in blue and red, with a colored strip at that edge whenever anything clips. Their height doesn't set the scale, so a clipped spike can't flatten the rest.
- **Clip percentages.** Text under the histogram: how much of the frame is crushed to black and blown to white, counted at the same levels the zebras use. "under 0.1%" still means some clipping; only "0%" means none. It's drawn as screen text, so it never shows up in screenshots.
- **Waveform.** Top left. Each column of the frame, left to right, with its values from dark (bottom) to bright (top); brighter spots mean more pixels at that value. Shows *where* in the frame the darks and lights sit, which the histogram can't. Clipped values trace in blue and red, like the zebras.
- **Spot meter.** A small box that reads the value under it, as a level (0 to 255) and a zone (0 to X, the same zones false color paints). In the editor viewport it follows your mouse; in Play and in games it reads the center of the frame. It's screen text, so it never shows up in screenshots.

## Use

1. Select a CineCameraActor. Add Component > Value Scope. Pick a mode.
2. It shows whenever you look through that camera: piloting it in the editor viewport, PIE, Sequencer, Movie Render Queue.
3. In the editor viewport, use the **Value Scope** button on the viewport toolbar (right side). Click it to turn Value Scope on or off; the arrow next to it opens the menu: Presets, Mode, and each overlay. Picking anything there also turns it on. More Settings opens Editor Preferences > Plugins > Value Scope for the notan thresholds and clip levels. The setting is shared by every level viewport and remembered next time you open the project. While you look through a camera that has its own Value Scope, the camera's settings show instead, the same as in PIE.
4. The console overrides both, which is handy for quick checks:
   - `r.ValueScope.Mode 1` plumbing check
   - `r.ValueScope.Mode 2` notan
   - `r.ValueScope.Mode 3` false color
   - `r.ValueScope.Zebras 1` clip zebras, `0` to turn off
   - `r.ValueScope.Thirds 1` thirds guide, `0` to turn off
   - `r.ValueScope.Histogram 1` histogram, `0` to turn off
   - `r.ValueScope.ClipPercent 1` clip percentages, `0` to turn off
   - `r.ValueScope.Waveform 1` waveform, `0` to turn off
   - `r.ValueScope.SpotMeter 1` spot meter, `0` to turn off
   - `r.ValueScope.Mode 0` everything off
   - `-1` on any of these hands control back to the camera and toolbar

   Console settings win over the camera and the toolbar. While any are set, a yellow line at the bottom left of the screen says which. If the camera or toolbar seems to do nothing, check for that line. `r.ValueScope.OverrideMessage 0` hides it, and so does `DisableAllScreenMessages`.

Turn it off before final renders. Movie Render Queue will bake it in.

Value Scope only works on SDR output. With HDR output on (or a HighResShot with Capture HDR), the pixels aren't 0 to 255 values anymore, so it turns the value tools off and an orange line at the bottom left says so. The thirds guide still works.

Every Value Scope setting can be keyed in Sequencer, the same way you key the camera's focal length: add the camera to a Level Sequence, then + Track on its ValueScope component. Use it to switch views per shot, or key Enabled off before a final render.

### Presets

At the top of the Value Scope settings, **Apply Preset** sets everything at once:

| Preset | Shows |
|---|---|
| Notan | Black, grey, white. Nothing else. |
| Value Check | False color zones and the histogram. |
| Exposure | The image unchanged, with clip zebras, histogram, clip percentages and waveform. |
| Composition | Notan and the thirds guide. |

A preset is copied into the camera, so you can still change and key any setting afterward. **Save as Preset** saves the camera's current settings as a Value Scope Preset asset. Presets saved in the project show in every camera's Apply Preset list, under the built-in ones. Give yours a Description; it shows when you hover over it in the list. Changing a preset asset later doesn't change cameras that already used it.

In Blueprint, a Value Scope component has Apply Built In Preset and Apply Preset.

## Check a frame outside the engine

```
pip install pillow numpy
python Tools/value_report.py shot.png --crop-letterbox --out previews
```

Prints clip percentages, a zone breakdown and the notan split, and saves notan and false color images.

## Developers

Setup, build scripts and the plumbing checklist are in `CLAUDE.md`.
