#!/usr/bin/env bash

set -euo pipefail

fps="10"
force=0

print_help() {
    cat <<EOF
Usage:
  $(basename "$0") [--force] [--fps FPS] INPUT OUTPUT.vid WIDTH HEIGHT

Creates an uncompressed GVID v1 video for gigaSDK:
  pixel format : RGB565 big-endian
  audio        : removed (play audio separately through Audio_Mixer)
  scaling      : exact WIDTH x HEIGHT

The firmware supports frames up to 320x240. Smaller frames are centred by the
player, which clears the screen to black before drawing the first frame.

Examples:
  $(basename "$0") input.mp4 intro.vid 320 240
  $(basename "$0") --fps 12 input.mov small.vid 200 128
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            print_help
            exit 0
            ;;
        -f|--force)
            force=1
            shift
            ;;
        -r|--fps)
            if [[ $# -lt 2 ]]; then
                echo "Error: --fps requires a value" >&2
                exit 2
            fi
            fps=$2
            shift 2
            ;;
        --)
            shift
            break
            ;;
        -*)
            echo "Error: unknown option: $1" >&2
            print_help >&2
            exit 2
            ;;
        *)
            break
            ;;
    esac
done

if [[ $# -ne 4 ]]; then
    print_help >&2
    exit 2
fi

input_file=$1
output_file=$2
width=$3
height=$4

if [[ ! -f "$input_file" ]]; then
    echo "Error: input file does not exist: $input_file" >&2
    exit 1
fi
if ! command -v ffmpeg >/dev/null 2>&1; then
    echo "Error: ffmpeg is not installed or is not available in PATH" >&2
    exit 127
fi
if ! command -v python3 >/dev/null 2>&1; then
    echo "Error: python3 is not installed or is not available in PATH" >&2
    exit 127
fi
if [[ ! "$width" =~ ^[0-9]+$ ]] || [[ ! "$height" =~ ^[0-9]+$ ]] ||
   (( width < 1 || width > 320 || height < 1 || height > 240 )); then
    echo "Error: WIDTH and HEIGHT must be within 1..320 and 1..240" >&2
    exit 2
fi
if [[ -e "$output_file" && $force -eq 0 ]]; then
    echo "Error: output exists (use --force): $output_file" >&2
    exit 1
fi

raw_file=$(mktemp --suffix=.rgb565be)
output_tmp="${output_file}.tmp.$$"
cleanup() {
    rm -f "$raw_file" "$output_tmp"
}
trap cleanup EXIT

ffmpeg \
    -hide_banner \
    -loglevel error \
    -i "$input_file" \
    -an \
    -vf "fps=${fps},scale=${width}:${height}:flags=lanczos" \
    -pix_fmt rgb565be \
    -f rawvideo \
    -y "$raw_file"

python3 - "$raw_file" "$output_tmp" "$width" "$height" "$fps" "$output_file" <<'PY'
import shutil
import struct
import sys
from pathlib import Path

raw_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
width = int(sys.argv[3])
height = int(sys.argv[4])
fps_milli = round(float(sys.argv[5]) * 1000)
display_path = sys.argv[6]

if fps_milli <= 0:
    raise SystemExit("Error: FPS must be greater than zero")

frame_bytes = width * height * 2
raw_size = raw_path.stat().st_size
if raw_size == 0 or raw_size % frame_bytes:
    raise SystemExit("Error: ffmpeg produced an incomplete RGB565 frame")

frame_count = raw_size // frame_bytes
header = struct.pack(
    "<4sBBHHHIII",
    b"GVID", 1, 1, 24, width, height, fps_milli, frame_count, 0,
)

with output_path.open("wb") as output, raw_path.open("rb") as raw:
    output.write(header)
    shutil.copyfileobj(raw, output, length=1024 * 1024)

duration = frame_count * 1000 / fps_milli
print(f"Created:    {display_path}")
print(f"Resolution: {width}x{height}")
print(f"Frame rate: {fps_milli / 1000:g} FPS")
print(f"Frames:     {frame_count}")
print(f"Duration:   {duration:.3f} s")
print(f"Data size:  {raw_size} bytes")
PY

mv -f "$output_tmp" "$output_file"
trap - EXIT
rm -f "$raw_file"
