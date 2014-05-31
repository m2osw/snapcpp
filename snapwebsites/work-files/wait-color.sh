#!/bin/sh
# Convert the wait-color-??.png files in an "animated" PNG

cd work-files
for f in wait-color-[0-9][0-9].png
do
	g=`basename $f .png`
	convert -resize 64x64 $f $g-64x64.png
done
convert wait-color-[0-9][0-9]-64x64.png +append wait-color-64x64.png
