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

A histogram comes later.

## Use

1. Select a CineCameraActor. Add Component > Value Scope. Pick a mode.
2. It shows whenever that camera is the view target: PIE, Sequencer in game, Movie Render Queue.
3. In the editor viewport, use the console:
   - `r.ValueScope.Mode 1` plumbing check
   - `r.ValueScope.Mode 2` notan
   - `r.ValueScope.Mode 3` false color
   - `r.ValueScope.Zebras 1` clip zebras, `0` to turn off
   - `r.ValueScope.Thirds 1` thirds guide, `0` to turn off
   - `r.ValueScope.Mode 0` everything off
   - `-1` on any of these goes back to per-camera

   Console settings win over the camera's settings. While any are set, a yellow line on screen says which. If the camera's settings seem to do nothing, check for that line. `r.ValueScope.OverrideMessage 0` hides it, and so does `DisableAllScreenMessages`.

Turn it off before final renders. Movie Render Queue will bake it in.

## Check a frame outside the engine

```
pip install pillow numpy
python Tools/value_report.py shot.png --crop-letterbox --out previews
```

Prints clip percentages, a zone breakdown and the notan split, and saves notan and false color images.

## Developers

Setup, build scripts and the plumbing checklist are in `CLAUDE.md`.
