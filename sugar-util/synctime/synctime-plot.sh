#!/bin/bash

## INIT
FILE_NAME_1=$1

## DRAW PLOT
OUTPUT_PNG="./png/synctime.png"
gnuplot -persist <<-EOFMarker
set terminal pngcairo size 1500,750 enhanced font "VL P Gothic,11";
set output "$OUTPUT_PNG";

set title "Synchronization Time of Sugarchain Yumekawa";
set xlabel "Elapsed Time";
set ylabel "Block Height";

set datafile separator ","

set yrange [0:*];
set ytics 0, 100000;
set format y '%.0f'

# data example
# 2019-12-06 17:14:47,0:10:00,81558
set xdata time
set timefmt "%H%M%S"
set format x "%H:%M:%S"
# set xrange [0:*]
# set xrange ["$l_start_dt":"$l_end_dt"]

# set xrange [0:*];
set xtics 0, 1;

plot \
"$FILE_NAME_1" using 2:3 axis x1y1 w l title "$FILE_NAME_1" lc rgb "#ff0000" lw 1.0,
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
