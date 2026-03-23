#!/bin/bash
#
# ./replace-headers.sh
#
# The purpose of this script is to update old header information from e.g.
# cpp/h files in Coin. The script indicates that it is working by showing
# a . for each file. When the script is done it will show the amount of files
# updated.
#
# Usage: Simply run the script from the directory you wish to update. It will
#        do recursive scanning.
#
# Variables:
#   DEBUG      - Setting this to "true" displays the change without updating,
#                and "false" to do the changes in the files.
#   STARTSWITH - The start pattern(regex) to look for.
#   ENDSWITH   - The pattern(regex) to look for after the start pattern.
#   HEADER     - The new header information to be used.
#   IGNORE     - The file path pattern(regex) to ignore.
#
# Note: This script does not take backup of changed files. Use with caution!
#
DEBUG=false
HEADER="/**************************************************************************\\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\\**************************************************************************/"
STARTSWITH="^/\*\*\*"
ENDSWITH="^\\\*\*\*"
IGNORE="(/boost/|/\.hg/)"
TEMP=".replace-headers.tmp"
COUNTER=0

echo "Updating headers... (This may take several minutes)"

for file in $(find . -type f)
#for file in $@
do
  # indicate work in progress...
  echo -n .

  # skip hidden and binary files
  [[ "$file" =~ "/\." ]] && continue
  [[ "$file" =~ "$IGNORE" ]] && continue
  [[ "$(file $file)" =~ .*:*text ]] || continue

  # fetch the header line numbers
  LINES=$(wc -l $file | sed -e 's/[^0-9]*\([0-9]*\) .*/\1/')
  FROM=$(grep -n "$STARTSWITH" $file | head -n 1 | sed 's/:.*//')
  [ "$FROM" = "" ] && continue
  LENGTH=$(cat $file | tail -n $(($LINES-$FROM)) | grep -n "$ENDSWITH" | head -n 1 | sed 's/:.*//')
  [ "$LENGTH" = "" ] && continue

  if ($DEBUG)
  then
    #debug
    clear
    echo "Showing \"$file\" from line $FROM-$LENGTH:"
    cat $file | head -n $(($FROM+$LENGTH)) | tail -n $(($LENGTH+1))
    #echo
    #echo "Hit ENTER to continue."
    #read
  else
    # update file
    if [[ $(($FROM-1)) -gt 0 ]]; then cat $file | head -n $(($FROM-1)) > $TEMP; fi
    echo "$HEADER" >> $TEMP
    cat $file | tail -n $(($LINES-$FROM-$LENGTH)) >> $TEMP
    mv $TEMP $file
  fi

  # increase amount of updated files
  COUNTER=$(($COUNTER+1))
done

echo
echo "Done! $COUNTER file(s) updated."
