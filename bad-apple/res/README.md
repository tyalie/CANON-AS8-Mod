# Source folder for the image files

In normal operation this directory contains two files
1. `Bad Apple.mp4` - The original bad apple source file
2. `badapple.hs` - The heatshrink compressed 128x64 b/w version

One can easily reproduce the result by installing the dependencies mentioned in the
`requirements.txt` and executing the following command

```bash
./compressor.py -i Bad\ Apple.mp4 -d 128x64 -o badapple.hs
```

One can choose between dithering and threshold conversion of the video into B/W by
using the `--threshold` parameter with a value between 0 and 255 (omit it for
dithering).
