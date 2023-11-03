# Bad Apple/Video playback app

The calculator has a 128x64 px big B/W OLED display, which is plenty enough to
display recognizable videos like - bad apple (ofc).

As we don't have too much flash, we use [heatshrink][1].

Convert videos using the description in the res folders README.

## Restrictions
- b/w
- compressed videos at most 0x500000 B (~5.1MB)

## Upload video

One needs to manually upload the file into the flash at offset 0x250000 using

```bash
python3 esptool.py write_flash 0x250000 res/badapple.hs
```

Luckily Zephyr allows us to partition the flash, so we can be sure that the
`storage-partition` that we use here won't be overwritten between reflashes.

[1]: https://github.com/atomicobject/heatshrink
