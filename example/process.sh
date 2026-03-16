#!/bin/bash

TEMP1=$(mktemp)

dirbase="$(dirname -- "$1")"
name="$(basename -- "$1")"

if [[ "$name" == *.* ]]; then
  ext="${name##*.}"                # text after last dot
  base="${name%.*}"                # everything before last dot
  outname="${base}.processed.${ext}"
else
  outname="${name}.processed"
fi

outfile="${dirbase}/${outname}"

cat $1 | ./la_strip | ./la_dedup > $TEMP1

echo "; Processed with LA tools" > $outfile
echo "; GRBL device profile, absolute coords" >> $outfile
cat $1 | ./la_bounds >> $outfile
cat >> $outfile<< EOF
G00         ; Rapid Travel
G17         ; XY plane
G40         ; Cancel Cutter compensation (?)
G21         ; Set units to mm
G54         ; Work Offset
G90         ; Absolute positioning
M4
M8
EOF

cat $TEMP1 >> $outfile
cat $TEMP1 >> $outfile

cat >> $outfile<< EOF
G90
M9
G1S0
M5
G90
G0 X0 Y0
M2

EOF

rm $TEMP1
