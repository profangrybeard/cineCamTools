# CineCam Tools changelog

## 1.0 (2026-09-26)

The first full version of **Value Scope**: tools for judging value (how light or dark things are) in Unreal, on the final image, the way you'd judge it in Photoshop. For Unreal Engine 5.8.

**See value**

- **Notan:** the frame in three values (black, grey, white), with adjustable split points.
- **False color:** the frame in 11 zones (0 to X), each its own color.
- **Clip zebras:** stripes over crushed blacks (blue) and blown whites (red).
- **Thirds guide:** rule of thirds lines.

**Measure value**

- **Histogram:** uses Photoshop's Luminosity math. Checked bin for bin against screenshots, and by shape against Photoshop itself.
- **Clip percentages:** how much of the frame is crushed and blown.
- **Waveform:** where in the frame each value sits, left to right.
- **Spot meter:** the level (0 to 255) and zone under a small box. It follows the mouse in the editor and reads the center in Play.
- **Pins:** pin up to four spots (A to D) with Alt+M and compare them: "B is 3 zones under A".

**Use it anywhere**

- **On a camera:** add the Value Scope component to a CineCamera. It shows whenever you look through that camera: piloting in the editor, Play, Sequencer, Movie Render Queue.
- **In Sequencer:** every setting can be keyed, so a shot can switch views at any frame.
- **Presets:** Notan, Value Check, Exposure and Composition built in. Save your own as a preset asset and share it with the class.
- **Viewport toolbar:** a Value Scope button on every level viewport, with presets and every overlay in its menu, remembered between sessions.
- **Console:** `r.ValueScope.*` commands override everything for quick checks, with an on-screen reminder while they're set.

**Good to know**

- The histogram, waveform, zebras and false color bake into screenshots and Movie Render Queue renders. Turn them off before final renders. Text and the spot meter never bake.
- On an HDR display, the value tools turn off and say why: HDR pixels aren't 0 to 255 values. The thirds guide still works.
- `Tools/value_report.py` checks a screenshot outside the engine: notan, false color, clip percentages and histogram, with the same math.
