#!/usr/bin/env python3
"""
Copyright (c) 2023 Sophie Tyalie
SPDX-License-Identifier: Apache-2.0

Small utility to convert a provided video to a lz4 compressed binary bitmap
using a threashold and output display size.

requires
  - imageio
  - av
  - heatshrink2
"""

from argparse import ArgumentParser
import lz4.frame
import numpy as np
from PIL import Image
import imageio.v3 as iio
import heatshrink2

def dim_parser(v: str):
    return tuple(map(int, v.split("x")))

def get_parser():
    parser = ArgumentParser(description="Utility to compress videos for embedded use");
    parser.add_argument("-i", "--input", help="Input file", required=True)
    parser.add_argument("-d", "--dimensions", type=dim_parser, help="Output dimensions", required=True)
    parser.add_argument("-t", "--threshold", type=int, default=-1, help="Threshold value [0-255]")
    parser.add_argument("-o", "--output", help="output file name", required=True)

    return parser

def convert_frame(frame, dimensions, threshold):
    img = Image.fromarray(frame)
    img = img.resize(dimensions)

    # using B/W without dithering
    if 0 <= threshold and threshold <= 0xff:
        # see https://stackoverflow.com/a/50090612
        img = img.convert('L').point(lambda x: 255 if x > threshold else 0, mode='1')
    else:  # with dithering
        img = img.convert('1')

    return img

def do_video_conversion(video_file, dimensions, threshold):
    """resize image and apply threshold"""

    for _, frame in enumerate(iio.imiter(video_file)):
        yield convert_frame(frame, dimensions, threshold)

def video_to_stream(frames):
    raw_data = bytearray()

    for frame in frames:
        raw_data += np.packbits(frame).tobytes()

    return raw_data

def apply_lz4(data):
    return lz4.frame.compress(
        data,
        block_size=1, #lz4.frame.BLOCKSIZE_MAX64KB,
        compression_level=lz4.frame.COMPRESSIONLEVEL_MAX
    )

def apply_heatshrink(data):
    return heatshrink2.compress(data)


if __name__ == "__main__":
    args = get_parser().parse_args()

    frame_iter = do_video_conversion(args.input, args.dimensions, args.threshold)
    raw_data = video_to_stream(frame_iter)

    comp_method = None
    if args.output.endswith(".lz4"):
        comp_method = apply_lz4
    elif args.output.endswith(".hs"):
        comp_method = apply_heatshrink

    if comp_method is None:
        print("Unknown compression method choosen [*.lz4,*.hs] - ending")
        exit(-1)

    with open(args.output, "wb") as fp:
        fp.write(comp_method(raw_data))

