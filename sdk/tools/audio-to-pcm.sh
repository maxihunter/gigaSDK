#!/usr/bin/env bash

set -euo pipefail

readonly SAMPLE_RATE=22050
readonly CHANNELS=2
readonly SAMPLE_FORMAT="s16le"

print_help() {
    cat <<EOF
Usage:
  $(basename "$0") [--force] <input-audio> <output.pcm>

Converts any audio format supported by ffmpeg to the raw format used by
gstr-bios and PCM5102A:

  sample rate : ${SAMPLE_RATE} Hz
  channels    : stereo
  samples     : signed 16-bit little-endian
  frame order : L0, R0, L1, R1, ...
  container   : none (raw PCM)

The bytes are fed to the I2S peripheral unchanged, so the file must stay raw:
any header would be played back as noise. gstr-bios plays "music.pcm" from the
root of the card at boot, and its FatFs build has long file names disabled, so
the name on the card has to fit into 8.3.

Options:
  -f, --force  overwrite the output file
  -h, --help   show this help

Example:
  $(basename "$0") music.mp3 MUSIC.PCM
EOF
}

overwrite_flag="-n"

case "${1:-}" in
    -h|--help)
        print_help
        exit 0
        ;;
    -f|--force)
        overwrite_flag="-y"
        shift
        ;;
esac

if [[ $# -ne 2 ]]; then
    print_help >&2
    exit 2
fi

input_file=$1
output_file=$2

if ! command -v ffmpeg >/dev/null 2>&1; then
    echo "Error: ffmpeg is not installed or is not available in PATH." >&2
    exit 127
fi

if [[ ! -f "$input_file" ]]; then
    echo "Error: input file does not exist: $input_file" >&2
    exit 1
fi

# FatFs in gstr-bios is built without long file name support, so the card entry
# must be a plain 8.3 name.
output_name=$(basename "$output_file")
if [[ ! "$output_name" =~ ^[^.]{1,8}\.[^.]{1,3}$ ]]; then
    echo "Warning: '$output_name' is not an 8.3 name and gstr-bios will not" >&2
    echo "         find it; rename the file when copying it to the card." >&2
fi

ffmpeg \
    -hide_banner \
    -loglevel warning \
    "$overwrite_flag" \
    -i "$input_file" \
    -map 0:a:0 \
    -vn \
    -ar "$SAMPLE_RATE" \
    -ac "$CHANNELS" \
    -c:a pcm_s16le \
    -f "$SAMPLE_FORMAT" \
    "$output_file"

output_size=$(stat -c '%s' "$output_file")
if (( output_size % 4 != 0 )); then
    echo "Error: output size is not aligned to a stereo PCM frame." >&2
    exit 1
fi

frame_count=$((output_size / 4))
duration_ms=$((frame_count * 1000 / SAMPLE_RATE))

echo "Created: $output_file"
echo "Format:  PCM S16LE, stereo, ${SAMPLE_RATE} Hz"
echo "Frames:  $frame_count"
echo "Duration: ${duration_ms} ms"
