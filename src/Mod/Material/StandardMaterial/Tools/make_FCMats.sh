#!/bin/bash
# That script converts Materials.ods file into FCMat files. The result files are saved in FCMAT_OUTPUT_DIR

FCMAT_OUTPUT_DIR=FCMats
MATERIALS_FILE=Materials

# Check for unoconv
which unoconv 2>&1 > /dev/null || ( echo "unoconv not found. Please install it first!" && exit 1 )

# Convert Materials.ods to Materials.csv
if [ -f "$MATERIALS_FILE.ods" ]
then
	echo "Creating "$MATERIALS_FILE.csv
	# 124 is field delimiter |
	# 34 is text delimiter "
	# 76 is encoding UTF-8
	unoconv -e FilterOptions=124,34,76 -f csv $MATERIALS_FILE.ods
else
	echo "Material.ods not found. Please run make_ods.sh first!"
	exit 1
fi

# Helper function to retrieve string from X,Y position in csv file
function get_xy() {
	VALUE_XY=$(cat $MATERIALS_FILE.csv | awk -v x=$X -v y=$Y -F\| 'NR==y {print $x}' | sed 's/\"//g')
}

# Determine number of columns and rows in the Materials.csv file
NUMBER_OF_COLUMNS=$(awk -F "|" "NR==1 { print NF }" $MATERIALS_FILE.csv )
NUMBER_OF_ROWS=$(cat $MATERIALS_FILE.csv | wc -l)

echo "Setting output directory:" $FCMAT_OUTPUT_DIR
if [ -d "$FCMAT_OUTPUT_DIR" ]
then
       rm "$FCMAT_OUTPUT_DIR"/* 2>&1 > /dev/null
else
	mkdir $FCMAT_OUTPUT_DIR
fi

for MAT_NO in $(seq 2 $NUMBER_OF_ROWS)
do
	X=1
	Y=$MAT_NO
	get_xy
	FCMAT_FILE=$VALUE_XY
	echo "Generating material file: " $FCMAT_OUTPUT_DIR/$FCMAT_FILE.FCMat
	touch $FCMAT_OUTPUT_DIR/$FCMAT_FILE.FCMat

	if [ -f "../$FCMAT_FILE.FCMat" ]
	then
		head -n 5 ../$FCMAT_FILE.FCMat > $FCMAT_OUTPUT_DIR/$FCMAT_FILE.FCMat
        else
                cp new_material_header $FCMAT_OUTPUT_DIR/$FCMAT_FILE.FCMat
	fi

	echo "[FCMat]" >> $FCMAT_OUTPUT_DIR/$FCMAT_FILE.FCMat

	for X in $(seq 2 $NUMBER_OF_COLUMNS)
	do
		Y=1
		get_xy
		PROPERTY=$VALUE_XY
		Y=$MAT_NO
		get_xy
		VALUE=$VALUE_XY
		if [ -n "$VALUE_XY" ]
		then
			echo $PROPERTY = $VALUE >> $FCMAT_OUTPUT_DIR/$FCMAT_FILE.FCMat;
		fi
	done
done
