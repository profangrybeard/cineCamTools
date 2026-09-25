# CineCam Tools

Unreal Engine 5.8 camera tools for studying lighting, value and composition.

## Value Scope

Draws overlays on the final image a camera sees, after tonemapping. That's the same image a Photoshop histogram would read, so what you see in engine matches what you'd measure in Photoshop.

Right now it has two modes:

- **Plumbing check.** Leaves the image alone and draws a magenta frame on the edges. If you see the frame, the tool is working.
- **Notan.** Reduces the frame to black, grey and white. If your shot reads as two blobs with no grey, it has no middle values.

More modes (false color, clip zebras, histogram) come later.

## Use

1. Select a CineCameraActor. Add Component > Value Scope. Pick a mode.
2. It shows whenever that camera is the view target: PIE, Sequencer in game, Movie Render Queue.
3. In the editor viewport, use the console:
   - `r.ValueScope.Mode 1` plumbing check
   - `r.ValueScope.Mode 2` notan
   - `r.ValueScope.Mode -1` back to per-camera

Turn it off before final renders. Movie Render Queue will bake it in.

## Check a frame outside the engine

```
pip install pillow numpy
python Tools/value_report.py shot.png --crop-letterbox --out previews
```

Prints clip percentages, a zone breakdown and the notan split, and saves notan and false color images.

## Developers

Setup, build scripts and the plumbing checklist are in `CLAUDE.md`.
