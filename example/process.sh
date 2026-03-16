#!/bin/bash

# This Bash script demonstrates an example workflow that I currently use.
# It accepts a single argument which is a path to input gcode file.
# It strips all comments and codes unrecognized by la (la_strip)
# Then it deduplicates all cuts (la_dedup)
# It produces the output file with custom header and footer commands.
# The header contains a comment with job XY bounds. (la_bounds)
# The output file contains two passes of the deduplicated input job.

# Exit if la is not installed
for prog in la_strip la_dedup la_bounds; do
    if ! command -v "$prog" &> /dev/null; then
        echo "Error: la is not installed (missing $prog)" >&2
        exit 1
    fi
done

# Preparing a temporary file and output file name
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


# The processing chain of commands
cat $1 | la_strip | la_dedup > $TEMP1

# Header
echo "; Processed with LA tools" > $outfile
echo "; GRBL device profile, absolute coords" >> $outfile
cat $1 | la_bounds >> $outfile
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

# A number of passes in the output file is adjusted here
cat $TEMP1 >> $outfile
cat $TEMP1 >> $outfile

# Footer
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
