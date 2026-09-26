# CineCam Tools backlog

Feature requests, big or small, from anyone. Add them fast; we sort them later. Entries are never deleted: a "no" stays with its reason, so the same idea doesn't come back without new information.

**Status:** new, discussing, planned 1.1, later, no, done (with version).
**Size** is Claude's guess once discussed: S (an afternoon), M (a step like 4.2), L (a whole step like 3 or 4).

To add one, copy the template to the bottom of "Requests" with the next number.

```
### FR-000  Short name
- Asked: YYYY-MM-DD, who (and where it came from, if a student or class)
- Status: new
- Size: ?
- Notes: what it is and why. Open questions. What we decided, with dates.
```

## 1.1 plan

Nothing pulled in yet. When we plan 1.1, the chosen requests get grouped into steps here (1.1 step 1, step 2, ...), each with a design talk, a checklist in `checklists/1.1.md`, and a commit of its own, like 1.0.

## Requests

### FR-001  Reference compare
- Asked: 2026-09-26, Claude (step 4 discussion)
- Status: new
- Size: ?
- Notes: load a painting or film still and show it next to the frame (side by side or a wipe), with its own histogram and notan next to ours, so students match value structure to a reference. Open: where the image comes from (texture asset? file on disk?), whether it's per camera or per viewport, whether it bakes into renders.

### FR-002  Class handoff sheet
- Asked: 2026-09-26, Claude (step 4 discussion)
- Status: new
- Size: ?
- Notes: one click saves a HighResShot plus a value report (notan, false color, histogram, clip %) as one PNG sheet students can submit. What `value_report.py` does now, moved into the engine. Open: layout of the sheet, where it saves, whether it includes camera and lens info.

### FR-003  Composition overlays
- Asked: 2026-09-26, Claude (step 4 discussion)
- Status: new
- Size: ?
- Notes: golden ratio and spiral, diagonals, center cross, title and action safe, aspect masks (2.39, 1.85, 4:3). Same pass and toolbar as the thirds guide. Open: which ones matter most for class, whether the spiral needs flip and rotate.

### FR-004  Color scope
- Asked: 2026-09-26, Claude (step 4 discussion)
- Status: new
- Size: ?
- Notes: vectorscope and saturation false color, for color work once value is solid. Step 2 decided RGB is "later, if ever", so this is Tim's call on whether it's time. Stays post-tonemap.

### FR-005  Test live HDR output
- Asked: 2026-09-26, Claude (4.1 test)
- Status: new
- Size: S
- Notes: two 4.1 checks were skipped because the test machine's display has no HDR output path ("HDR games, apps and more: Not supported" in Windows): the live `r.HDR.EnableHDROutput 1` check and `DisableAllScreenMessages` hiding the orange line. Run them on an HDR display. See `checklists/1.0.md`, step 4.1.

### FR-006  Spot meter exact check against a PNG
- Asked: 2026-09-26, Claude (4.2 test)
- Status: new
- Size: S
- Notes: 4.2's exact check passed on Tim's report, but Claude never averaged the same box in a HighResShot PNG. Do it once, like the histogram's byte-identical check: note the center reading in Game View, `HighResShot 1`, compare. Could also add `--spot x,y` to `value_report.py`.
