#!/bin/bash
# That script creates Materials.ods file from all FCMat files in current directory

FCMAT_DIR="../"
MATERIALS_FILE=Materials

# Remove existing $MATERIALS_FILE.csv
if [ -f "$MATERIALS_FILE.csv" ]
then
	rm $MATERIALS_FILE.csv
fi

# Create new Materials.csv from all FCMat files
ls $FCMAT_DIR*.FCMat | xargs -I [] ./FCMat2csv.sh [] && ls *.csv | xargs -I [] cat [] | awk "NR==1; NR%2 == 0" > $MATERIALS_FILE.csv

# Check for unoconv
which unoconv 2>&1 > /dev/null || ( echo "unoconv not found. Please install it first!" && exit 1 )

# Convert Materials.csv to Materials.ods
echo "Creating "$MATERIALS_FILE.ods
# 124 is field delimiter |
# 34 is text delimiter "
# 76 is encoding UTF-8
unoconv -i FilterOptions=124,34,76 -f ods $MATERIALS_FILE.csv

# Remove all temporary files
rm *.csv
