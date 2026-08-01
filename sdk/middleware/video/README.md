# GVID video playback

The video module streams uncompressed RGB565 frames from FatFs directly to
the ILI9341 display. A complete frame does not have to fit in RAM.

The `.vid` file contains a 24-byte GVID v1 header followed by big-endian
RGB565 frames. Keep filenames compatible with FatFs 8.3 naming.

Initialize the module after the display:

```c
ILI9341_Init();
Video_Init(&hspi2);
```

Play a file while keeping the audio DMA mixer supplied:

```c
static void Video_ServiceAudio(void)
{
  if (Audio_MixerIsRunning())
  {
    (void)Audio_MixerProcess();
  }
}

(void)Video_PlayFile("intro.vid", Video_ServiceAudio, NULL);
```

`Video_DrawRgb565Frame()` draws a single in-memory frame. Both playback APIs
centre frames smaller than 320x240. `Video_PlayFile()` also clears the screen
to black once before the first undersized frame, leaving a stable black border.

Convert source video with:

```sh
sdk/tools/video-prepare.sh --fps 10 input.mp4 intro.vid 320 240
sdk/tools/video-prepare.sh --fps 12 input.mp4 small.vid 200 128
```

There is no audio track in GVID. A full 320x240 frame occupies 153600 bytes;
at 10 FPS the SD card and display path must sustain about 1.5 MB/s.
