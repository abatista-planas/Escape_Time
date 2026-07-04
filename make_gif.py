#!/usr/bin/env python3
"""Render a looping GIF of Julia sets by sweeping the constant c around a circle.

For each frame the constant is c = radius * (cos t + i sin t) with t going once
around the circle, so the animation loops seamlessly. Each frame is produced by
the compiled Escape_Time program and coloured with a fixed palette.

Usage:
    python3 make_gif.py --bin ./build/Escape_Time --out assets/Fractal.gif
"""
import argparse
import os
import subprocess
import tempfile
from math import cos, isqrt, pi, sin

import matplotlib as mpl
import numpy as np
from PIL import Image


def load(path):
    data = np.fromfile(path, dtype="<u4")
    side = isqrt(int(data.size))
    return data.reshape(side, side)


def main():
    ap = argparse.ArgumentParser(description="Make a looping Julia-set GIF.")
    ap.add_argument("--bin", default="./Escape_Time", help="path to the Escape_Time binary")
    ap.add_argument("--size", type=int, default=480, help="image size in pixels")
    ap.add_argument("--frames", type=int, default=60, help="number of frames in the loop")
    ap.add_argument("--radius", type=float, default=0.7885, help="|c| of the swept constant")
    ap.add_argument("--cmap", default="magma", help="Matplotlib colormap")
    ap.add_argument("--mode", type=int, default=2, help="compute mode (0/1/2)")
    ap.add_argument("--duration", type=int, default=80, help="milliseconds per frame")
    ap.add_argument("--out", default="assets/Fractal.gif", help="output GIF path")
    args = ap.parse_args()

    cmap = mpl.colormaps[args.cmap]
    palette = (np.array([cmap(i / 255)[:3] for i in range(256)]) * 255).astype(np.uint8)
    flat_palette = palette.flatten().tolist()

    frames = []
    with tempfile.TemporaryDirectory() as tmp:
        raw = os.path.join(tmp, "frame.dat")
        for k in range(args.frames):
            t = 2 * pi * k / args.frames
            cx, cy = args.radius * cos(t), args.radius * sin(t)
            subprocess.run(
                [args.bin, str(cx), str(cy), str(args.size), raw, str(args.mode)],
                check=True, stdout=subprocess.DEVNULL,
            )
            data = np.log1p(load(raw).astype(np.float64))  # log keeps detail in the halo
            hi = data.max()
            idx = (data / hi * 255).astype(np.uint8) if hi > 0 else data.astype(np.uint8)
            img = Image.fromarray(idx, mode="P")
            img.putpalette(flat_palette)
            frames.append(img)
            print(f"\rframe {k + 1}/{args.frames}", end="", flush=True)
    print()

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    frames[0].save(
        args.out, save_all=True, append_images=frames[1:],
        duration=args.duration, loop=0, optimize=True,
    )
    print(f"Wrote {args.out} ({args.frames} frames, {args.size}x{args.size})")


if __name__ == "__main__":
    main()
