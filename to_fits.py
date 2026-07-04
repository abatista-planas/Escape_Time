#!/usr/bin/env python3
"""Convert the raw escape-time data file into a FITS image.

Usage:
    python3 to_fits.py fractal.dat [fractal.fits] [--overwrite]
"""
import argparse
import os
import sys
from math import isqrt

import numpy as np
from astropy.io import fits


def load_grid(path):
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
    return data.reshape(side, side)


def main():
    parser = argparse.ArgumentParser(description="Convert an escape-time .dat file to FITS.")
    parser.add_argument("input", help="raw .dat produced by Escape_Time")
    parser.add_argument("output", nargs="?", help="output .fits (default: input name with .fits)")
    parser.add_argument("--overwrite", action="store_true", help="overwrite an existing FITS file")
    args = parser.parse_args()

    grid = load_grid(args.input)
    out = args.output or (os.path.splitext(args.input)[0] + ".fits")
    if os.path.abspath(out) == os.path.abspath(args.input):
        sys.exit(f"Error: output '{out}' would overwrite the input file.")

    # Iteration counts are small (<= MAX_ITER), so int32 is plenty and maps
    # directly to a native FITS type (BITPIX = 32).
    hdu = fits.PrimaryHDU(grid.astype(np.int32))
    hdu.header["COMMENT"] = "Escape-time Julia set iteration counts"
    hdu.writeto(out, overwrite=args.overwrite)
    print(f"Wrote {grid.shape[0]}x{grid.shape[1]} FITS image to {out}")


if __name__ == "__main__":
    main()
