#!/bin/bash
# Helper script that converts FCMat file into csv file

FILE=$1
FILE_BASENAME=$(basename $FILE)

if [ "$FILE_BASENAME" = "TEMPLATE.FCMat" ]
then
	echo "Skipping TEMPLATE.FCMat";
	exit 0
fi

if [ -f "$1.temp" ]
then
	rm $1.temp
fi

if [ -f "$1.csv" ]
then
	rm $1.csv
fi

echo "Processing "${FILE_BASENAME%%.FCMat}
echo "FileName = "${FILE_BASENAME%%.FCMat} > $FILE_BASENAME.temp

while read -r PROPERTY || [[ -n "$PROPERTY" ]]; do
	VALUE=$(cat $FILE | awk '{if ($1!="") print $0; else "X" }' | grep -w $PROPERTY | cut -f3- -d " ")
	echo $PROPERTY' = '$VALUE >> $FILE_BASENAME.temp
done < headers

echo "FileName" | tr '\n' "|" > $FILE_BASENAME.csv
cat headers | tr '\n' "|" | sed 's/|$/\n/' >> $FILE_BASENAME.csv
cat $FILE_BASENAME.temp | sed 's/^[\[,;].*//' | sed '/^$/d' | cut -d"=" -f2- | cut -d " " -f2- | sed 's/^/\"/' | sed 's/$/\"/' | tr '\n' "|" | sed 's/|$/\n/'  >> $FILE_BASENAME.csv

rm $FILE_BASENAME.temp
