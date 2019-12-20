#!/bin/bash

## INIT
FILE_NAME_1="uncached_reindex.log.csv"
FILE_NAME_2="cached_reindex.log.csv"

## DRAW PLOT
OUTPUT_PNG="./png/synctime.png"
gnuplot -persist <<-EOFMarker
set terminal pngcairo size 1500,750 enhanced font "Ubuntu,12";
set output "$OUTPUT_PNG";

set title "Synchronization Time of Sugarchain Yumekawa";
set xlabel "Elapsed Time";
set ylabel "Block Height";

set datafile separator ","

set xrange [0:*];
set xtics 0, 3600 rotate by 45 right;

set xdata time
set timefmt "%s"
set format x "%H:%M:%S"

set yrange [0:*];
set ytics 0, 100000;
set format y '%.0f'

set grid xtics;

set key top left invert;
set key box opaque;
set key noenhanced;

plot \
"$FILE_NAME_1" using 2:3 axis x1y1 w l title "$FILE_NAME_1" lc rgb "blue" lw 1.0, \
"$FILE_NAME_2" using 2:3 axis x1y1 w l title "$FILE_NAME_2" lc rgb "red" lw 1.0, \
# caution at the end: no "\"
EOFMarker

## ECHO
echo ""
echo -e "  \e[32m..PRINTING TO FILE $OUTPUT_PNG\e[39m"
echo ""

## COPY TO CLIPBOARD
xclip -selection clipboard -t image/png -i $OUTPUT_PNG

## OPEN PNG
feh --scale-down $OUTPUT_PNG
