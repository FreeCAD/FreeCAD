
# Define a function for filtering lint output
# stdin: content to be filtered
# stdout: filtered content
# usage: cat unfiltered.log | lintfilter > filtered.log
#lintfilter() {
    lastLineHadPath=true
    chunk=""
    filterLinesFile="filterLines.txt"
    # Let's define a chunk / entry
    # Each chunk starts with lines containing /home/runner
    # A chunk ends, when the next line does not contain /home/runner
    while IFS= read -r line; do
        if echo $line | grep -q "/home/runner"; then
            if ! $lastLineHadPath; then
                # Must be new chunk!
                #cat filterLines.txt | xargs grep -q $chunk
                # filterLines.txt must not have empty newlines!!
                # as newlines will be matched all over
                if echo $chunk | grep -q -f filterLines.txt; then
                    # Also has affected lines
                    # Flush chunk to stdout!
                    echo "${chunk}"
                fi
            fi
            # New chunk, let's reset
            chunk=""
            lastLineHadPath=true
        else
            lastLineHadPath=false
        fi
        # Concatenate line to chunk buffer
        if [ "$chunk" = "" ]; then
            chunk="${chunk}${line}"
        else
            chunk="${chunk}"$'\n'"${line}"
        fi
    done #< "$1"
#}