#! /bin/bash

# Example file to drive the performance profiling python test from the shell.
# Built to support the Topological Naming Problem fixes from early 2024;
# this will likely need tweaking for your use.

notnp=<Path to your first executable to profile goes here>  #<dir>/bin/FreeCAD${cmd}
tnp=<Path to your second executable to profile goes here>   #<dir/bin/FreeCAD${cmd}
results="results.txt" # File to append measurements to.  We do not clear so xargs works on script
save="--save" # or ""  Whether to save the resulting file from the tnp executable for further testing.

perf record -o "$1.perf" $notnp -t TestPerf --pass "$@"
perf record -o "$1.tnp.perf" $tnp -t TestPerf --pass "$save" "$@"

# For interactive walking of the details using perf:
#perf report -i $1.perf

# After the two test runs above, process the resulting files to get the numbers.
# For perf, use the 'script' command to pull the info out of the file, and then do a little
# old school unix manipulation
times=($(perf script -F time -i "$1.perf" | sed -e's/://' -e'1p;$!d'))  # first, last timestamps
totaltime=$(echo ${times[1]} - ${times[0]} | bc)
memory=$(grep .size: "$1.mprofile" | cut -f2 -d\  )
memory2=$(perf script --header -i "$1.perf" | grep "data size" | cut -f2 -d:)

timestnp=($(perf script -F time -i "$1.tnp.perf" | sed -e's/://' -e'1p;$!d'))  # first, last times
totaltimetnp=$(echo ${timestnp[1]} - ${timestnp[0]} | bc)
memorytnp=$(grep .size: "$1.tnp.mprofile" | cut -f2 -d\  )
memory2tnp=$(perf script --header -i "$1.tnp.perf" | grep "data size" | cut -f2 -d:)

# To calculate in this script instead of externally, you could do something like this:
# delta=$(echo ${totaltimetnp} - ${totaltime} | bc)
# percent=$(echo  "scale=6; ${delta} / ${totaltime} * 100" | bc)

# Summarize the run of one document into a CSV line suitable for importing into a spreadsheet.
echo $totaltime,$totaltimetnp,$memory,$memorytnp,$memory2,$memory2tnp,"$1" >>"${results}"

