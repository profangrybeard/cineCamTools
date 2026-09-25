#!/usr/bin/env python3
"""
Value report for a rendered frame. CPU reference for the Value Scope shader.

Luma weights, notan thresholds, clip levels and the zone palette match
Shaders/Private/ValueScope.usf and the defaults in ValueScopeComponent.h.
If you change one, change the others. The false color preview matches the
shader's False Color mode, and the clip percentages count the pixels the
shader's zebras stripe.

Usage:
    python value_report.py frame.png
    python value_report.py frame.png --crop-letterbox --out previews/

Needs: pip install pillow numpy
"""
import argparse
import os
import sys

import numpy as np
from PIL import Image

# Keep all of these in sync with ValueScope.usf and ValueScopeComponent.h.
LUMA_WEIGHTS = np.array([0.30, 0.59, 0.11])  # Photoshop Luminosity histogram
SHADOW_THRESHOLD = 0.25
HIGHLIGHT_THRESHOLD = 0.75
BLACK_CLIP = 0.02   # about level 5
WHITE_CLIP = 0.98   # about level 250
ZONE_PALETTE = np.array([
    [0.25, 0.00, 0.40],  # 0
    [0.10, 0.10, 0.65],  # I
    [0.00, 0.35, 0.80],  # II
    [0.00, 0.55, 0.55],  # III
    [0.15, 0.60, 0.20],  # IV
    [0.50, 0.50, 0.50],  # V
    [0.70, 0.70, 0.30],  # VI
    [0.95, 0.80, 0.10],  # VII
    [1.00, 0.55, 0.00],  # VIII
    [1.00, 0.30, 0.30],  # IX
    [1.00, 0.00, 0.00],  # X
])
ROMAN = ["0", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X"]


def crop_letterbox(img, max_mean=2 / 255, max_std=4 / 255):
    """Drop flat black bars at top and bottom. Tolerates stray pixels from compression or UI."""
    row_mean = img.mean(axis=(1, 2))
    row_std = img.std(axis=(1, 2))
    is_bar = (row_mean <= max_mean) & (row_std <= max_std)
    top = 0
    while top < len(is_bar) and is_bar[top]:
        top += 1
    bottom = len(is_bar)
    while bottom > top and is_bar[bottom - 1]:
        bottom -= 1
    return img[top:bottom] if bottom > top else img


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("image")
    ap.add_argument("--crop-letterbox", action="store_true")
    ap.add_argument("--out", default=None, help="folder for notan and false color PNGs")
    ap.add_argument("--histogram-csv", default=None, help="write the 256-bin luma histogram as level,count")
    ap.add_argument("--compare-histogram", default=None,
                    help="compare to an engine dump from r.ValueScope.DumpHistogram")
    args = ap.parse_args()

    img = np.asarray(Image.open(args.image).convert("RGB")).astype(np.float64) / 255.0
    if args.crop_letterbox:
        img = crop_letterbox(img)

    luma = np.clip(img @ LUMA_WEIGHTS, 0, 1)
    zones = np.clip(np.floor(luma * 10.999), 0, 10).astype(int)

    print(f"{args.image}  {img.shape[1]}x{img.shape[0]}")
    print(f"min level {round(luma.min() * 255)}   max level {round(luma.max() * 255)}   "
          f"median level {round(np.median(luma) * 255)}")
    print(f"crushed black (<= level {round(BLACK_CLIP * 255)}): {100 * (luma <= BLACK_CLIP).mean():5.1f}%")
    print(f"blown white   (>= level {round(WHITE_CLIP * 255)}): {100 * (luma >= WHITE_CLIP).mean():5.1f}%")
    print("zones:")
    for z in range(11):
        pct = 100 * (zones == z).mean()
        print(f"  {ROMAN[z]:>4}  {pct:5.1f}%  {'#' * int(round(pct / 2))}")
    print(f"notan split: dark {100 * (luma < SHADOW_THRESHOLD).mean():.1f}%  "
          f"mid {100 * ((luma >= SHADOW_THRESHOLD) & (luma <= HIGHLIGHT_THRESHOLD)).mean():.1f}%  "
          f"light {100 * (luma > HIGHLIGHT_THRESHOLD).mean():.1f}%")

    # Same binning as HistogramCS in ValueScope.usf: round(luma * 255) in whole numbers.
    # 30R + 59G + 11B is luma * 255 * 100 exactly, so no float rounding at the .5 ties.
    q = np.rint(img * 255).astype(np.int64)
    levels = np.minimum(255, (30 * q[..., 0] + 59 * q[..., 1] + 11 * q[..., 2] + 50) // 100)
    bins = np.bincount(levels.ravel(), minlength=256)

    if args.histogram_csv:
        with open(args.histogram_csv, "w", newline="") as f:
            f.write("level,count\n")
            for level, count in enumerate(bins):
                f.write(f"{level},{count}\n")
        print(f"wrote histogram to {args.histogram_csv}")

    if args.compare_histogram:
        engine = np.zeros(256, dtype=np.int64)
        with open(args.compare_histogram) as f:
            next(f)
            for line in f:
                level, count = line.strip().split(",")
                engine[int(level)] = int(count)
        # Share of pixels that land in the same level in both. 100% is an exact match.
        # Two separate renders differ by dithering, so expect just under 100%.
        overlap = np.minimum(engine / engine.sum(), bins / bins.sum()).sum()
        print(f"histogram compare: engine {engine.sum()} px, image {bins.sum()} px, overlap {100 * overlap:.2f}%")

    if args.out:
        os.makedirs(args.out, exist_ok=True)
        stem = os.path.splitext(os.path.basename(args.image))[0]
        notan = np.where(luma < SHADOW_THRESHOLD, 0.0, np.where(luma > HIGHLIGHT_THRESHOLD, 1.0, 0.5))
        Image.fromarray((notan * 255).astype(np.uint8)).save(os.path.join(args.out, f"{stem}_notan.png"))
        Image.fromarray((ZONE_PALETTE[zones] * 255).astype(np.uint8)).save(
            os.path.join(args.out, f"{stem}_falsecolor.png"))
        print(f"wrote previews to {args.out}")


if __name__ == "__main__":
    sys.exit(main())
