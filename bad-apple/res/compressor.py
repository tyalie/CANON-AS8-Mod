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
import av
import heatshrink2

def dim_parser(v: str):
    return tuple(map(int, v.split("x")))

def get_parser():
    parser = ArgumentParser(description="Utility to compress videos for embedded use");
    parser.add_argument("-i", "--input", help="Input file", required=True)
    parser.add_argument("-d", "--dimensions", type=dim_parser, help="Output dimensions", required=True)
    parser.add_argument("-t", "--threshold", type=int, default=-1, help="Threshold value [0-255]")
    parser.add_argument("-o", "--output", help="output file name", required=True)
    parser.add_argument("--aspect", action="store_true", help="Preserve aspect ratio by adding borders")
    parser.add_argument("--debug", default=None, help="Debug video out (mp4)")

    return parser

def add_border(img, dimensions):
    ratio = dimensions[0] / dimensions[1]

    if ratio * img.height == img.width:
        return img

    dimensions = (img.width, int(img.width / ratio)) # add horizontal borders
    if ratio * img.height > img.width:
        # add vertical borders
        dimensions = (int(ratio * img.height), img.height)

    new_img = Image.new(img.mode, dimensions)
    new_pos = (dimensions[0] // 2 - img.width//2, dimensions[1] // 2 - img.height//2)
    new_img.paste(img, new_pos)
    return new_img

def convert_frame(frame, preserve_aspect, dimensions, threshold):
    img = Image.fromarray(frame)

    if preserve_aspect:
        img = add_border(img, dimensions)

    img = img.resize(dimensions)

    # using B/W without dithering
    if 0 <= threshold and threshold <= 0xff:
        # see https://stackoverflow.com/a/50090612
        img = img.convert('L').point(lambda x: 255 if x > threshold else 0, mode='1')
    else:  # with dithering
        img = img.convert('1')

    return img

def do_video_conversion(video_file, preserve_aspect, dimensions, threshold):
    """resize image and apply threshold"""

    for _, frame in enumerate(iio.imiter(video_file)):
        yield convert_frame(frame, preserve_aspect, dimensions, threshold)

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

    frame_iter = do_video_conversion(args.input, args.aspect, args.dimensions, args.threshold)

    if args.debug is not None:
        frame_iter = list(map(np.array, frame_iter))
        new_frames = list(map(lambda i: i*255, frame_iter))
        mpv_writer = iio.imwrite(args.debug, new_frames, codec="mpeg4", in_pixel_format="gray8")

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

