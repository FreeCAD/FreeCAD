filterLinesFile="filterLines.txt"
while IFS= read -r line; do
    if echo $line | grep -q -f filterLines.txt; then
        echo $line
    fi
done
