#!/bin/sh
./mjpg_streamer -i "./input_uvc.so -f 1280x760" -o "./output_http.so -w ./www"

