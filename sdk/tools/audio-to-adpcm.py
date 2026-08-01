#!/usr/bin/env python3
"""Convert audio to the streaming GIMA v1 IMA ADPCM format used by gigaSDK."""

from __future__ import annotations

import argparse
import os
import shutil
import struct
import subprocess
import sys
from pathlib import Path


SAMPLE_RATE = 22050
HEADER_SIZE = 32
MAGIC = b"GIMA"
VERSION = 1

INDEX_TABLE = (-1, -1, -1, -1, 2, 4, 6, 8,
               -1, -1, -1, -1, 2, 4, 6, 8)
STEP_TABLE = (
       7,     8,     9,    10,    11,    12,    13,    14,
      16,    17,    19,    21,    23,    25,    28,    31,
      34,    37,    41,    45,    50,    55,    60,    66,
      73,    80,    88,    97,   107,   118,   130,   143,
     157,   173,   190,   209,   230,   253,   279,   307,
     337,   371,   408,   449,   494,   544,   598,   658,
     724,   796,   876,   963,  1060,  1166,  1282,  1411,
    1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
    3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
    7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
   15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
   32767,
)


class EncoderState:
    def __init__(self, predictor: int) -> None:
        self.predictor = predictor
        self.step_index = 0

    def encode(self, sample: int) -> int:
        step = STEP_TABLE[self.step_index]
        difference = sample - self.predictor
        nibble = 0

        if difference < 0:
            nibble = 8
            difference = -difference

        reconstructed = step >> 3
        if difference >= step:
            nibble |= 4
            difference -= step
            reconstructed += step
        step >>= 1
        if difference >= step:
            nibble |= 2
            difference -= step
            reconstructed += step
        step >>= 1
        if difference >= step:
            nibble |= 1
            reconstructed += step

        if nibble & 8:
            self.predictor -= reconstructed
        else:
            self.predictor += reconstructed
        self.predictor = max(-32768, min(32767, self.predictor))
        self.step_index = max(
            0, min(88, self.step_index + INDEX_TABLE[nibble]))
        return nibble


class NibbleWriter:
    def __init__(self, output) -> None:
        self.output = output
        self.pending: int | None = None
        self.bytes_written = 0

    def write(self, nibble: int) -> None:
        if self.pending is None:
            self.pending = nibble & 0x0F
        else:
            self.output.write(bytes((self.pending | ((nibble & 0x0F) << 4),)))
            self.pending = None
            self.bytes_written += 1

    def finish(self) -> None:
        if self.pending is not None:
            self.output.write(bytes((self.pending,)))
            self.pending = None
            self.bytes_written += 1


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Convert audio to GIMA v1: streaming IMA ADPCM at 22050 Hz. "
            "Stereo is used by default; --mono is recommended for effects."
        )
    )
    parser.add_argument("input", type=Path, help="input audio file")
    parser.add_argument("output", type=Path, help="output .gim file")
    parser.add_argument("--mono", action="store_true",
                        help="encode one channel; playback duplicates it to L/R")
    parser.add_argument("-f", "--force", action="store_true",
                        help="overwrite an existing output file")
    return parser.parse_args()


def build_header(channels: int, frames: int, predictors: list[int],
                 data_bytes: int) -> bytes:
    right_predictor = predictors[1] if channels == 2 else 0
    return struct.pack(
        "<4sBBHIIhBBhBBII",
        MAGIC, VERSION, channels, HEADER_SIZE, SAMPLE_RATE, frames,
        predictors[0], 0, 0, right_predictor, 0, 0, data_bytes, 0,
    )


def main() -> int:
    args = parse_args()
    channels = 1 if args.mono else 2

    if not args.input.is_file():
        print(f"error: input file does not exist: {args.input}", file=sys.stderr)
        return 1
    if args.output.exists() and not args.force:
        print(f"error: output exists (use --force): {args.output}", file=sys.stderr)
        return 1
    if shutil.which("ffmpeg") is None:
        print("error: ffmpeg is not installed or is not in PATH", file=sys.stderr)
        return 127

    command = [
        "ffmpeg", "-hide_banner", "-loglevel", "error", "-i", str(args.input),
        "-map", "0:a:0", "-vn", "-ar", str(SAMPLE_RATE), "-ac", str(channels),
        "-c:a", "pcm_s16le", "-f", "s16le", "pipe:1",
    ]
    process = subprocess.Popen(command, stdout=subprocess.PIPE)
    assert process.stdout is not None

    frame_size = channels * 2
    frames = 0
    predictors: list[int] = []
    states: list[EncoderState] = []
    remainder = b""

    try:
        with args.output.open("wb+") as output:
            output.write(bytes(HEADER_SIZE))
            nibbles = NibbleWriter(output)

            while True:
                chunk = process.stdout.read(16384)
                if not chunk:
                    break
                chunk = remainder + chunk
                complete_size = len(chunk) - (len(chunk) % frame_size)
                remainder = chunk[complete_size:]

                for offset in range(0, complete_size, frame_size):
                    samples = struct.unpack_from(
                        "<" + ("h" * channels), chunk, offset)
                    if frames == 0:
                        predictors = list(samples)
                        states = [EncoderState(sample) for sample in samples]
                    else:
                        for channel, sample in enumerate(samples):
                            nibbles.write(states[channel].encode(sample))
                    frames += 1

            nibbles.finish()
            return_code = process.wait()
            if return_code != 0:
                raise RuntimeError(f"ffmpeg exited with status {return_code}")
            if frames == 0:
                raise RuntimeError("input contains no audio frames")

            output.seek(0)
            output.write(build_header(
                channels, frames, predictors, nibbles.bytes_written))

    except Exception as error:
        if process.poll() is None:
            process.kill()
            process.wait()
        try:
            args.output.unlink()
        except FileNotFoundError:
            pass
        print(f"error: {error}", file=sys.stderr)
        return 1

    pcm_bytes = frames * channels * 2
    encoded_bytes = HEADER_SIZE + nibbles.bytes_written
    duration = frames / SAMPLE_RATE
    print(f"Created:  {args.output}")
    print(f"Format:   GIMA v1, IMA ADPCM, {channels} channel(s), {SAMPLE_RATE} Hz")
    print(f"Duration: {duration:.3f} s")
    print(f"Size:     {encoded_bytes} bytes ({encoded_bytes / pcm_bytes:.1%} of PCM)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
