#!/bin/bash

print_help() {
	echo "Usage:"
	echo "$0 <input_filename> <output_filename>"
}

# Check the parameters first
if [ -z "$1" ]; then
	echo "Error: Input parameters are not set properly"
	print_help
	exit
fi
if [ -z "$2" ]; then
	echo "Error: Input parameters are not set properly"
	print_help
	exit
fi

# For st7735
ffmpeg -i myvideo.mov -vf scale=200:128 -vcodec rawvideo -f rawvideo -pix_fmt rgb565 myvideo.raw

# For ili9431 with jpeg decoder
ffmpeg -i video.mp4 -c:a mp3 -c:v mjpeg -s 320x240 -r 25 -q 15 video.avi
