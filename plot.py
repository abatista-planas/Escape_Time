#!/usr/bin/env python3
"""Optional viewer: render an escape-time .dat file as a shaded picture.

Usage:
    python3 plot.py fractal.dat [--cmap hot_r] [--log] [--save_img out.png]
"""
import argparse
import sys
from math import isqrt

import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.cm import ScalarMappable
from matplotlib.colors import LightSource, Normalize


def load_data(path):
    """Read a raw little-endian uint32 file and reshape it to a square grid."""
    try:
        data = np.fromfile(path, dtype="<u4")
    except OSError as exc:
        sys.exit(f"Error: cannot read '{path}': {exc}")
    if data.size == 0:
        sys.exit(f"Error: '{path}' is empty.")
    side = isqrt(int(data.size))
    if side * side != data.size:
        sys.exit(f"Error: '{path}' holds {data.size} values, not a perfect square.")
    return data.reshape(side, side).astype(np.float64)


def show_data(ax, data, cmap_name, use_log=False, save_img="", title=""):
    if use_log:
        data = np.log1p(data)  # log(1 + n) keeps zero-iteration pixels finite

    cmap = mpl.colormaps[cmap_name]
    norm = Normalize(vmin=float(data.min()), vmax=float(data.max()))

    light = LightSource(azdeg=315, altdeg=45)
    rgb = light.shade(data, cmap, norm=norm)

    ax.imshow(rgb, interpolation="bilinear")
    if save_img:
        plt.imsave(save_img, rgb)

    # A ScalarMappable gives the colorbar the right data range without the
    # old "imshow then remove" hack.
    plt.colorbar(ScalarMappable(norm=norm, cmap=cmap), ax=ax)
    ax.set_title(title, size="x-large")


def main():
    parser = argparse.ArgumentParser(description="Render an escape-time fractal.")
    parser.add_argument("fname", help="raw .dat file produced by Escape_Time")
    parser.add_argument("--cmap", default="hot_r", help="Matplotlib colormap")
    parser.add_argument("--log", action="store_true", help="use a log intensity scale")
    parser.add_argument("--save_img", default="", help="also save the picture to this path")
    parser.add_argument("--title", default="Julia set (escape time)", help="plot title")
    args = parser.parse_args()

    data = load_data(args.fname)
    fig, ax = plt.subplots()
    show_data(ax, data, args.cmap, use_log=args.log, save_img=args.save_img, title=args.title)
    fig.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
