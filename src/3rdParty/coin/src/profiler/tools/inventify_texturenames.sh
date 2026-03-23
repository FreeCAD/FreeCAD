#!/bin/bash

# Crude hack to take a directory of gifs, to a scene graph, and then inline the
# scene graph files in a cpp file. Filename for the cpp-file must be supplied
# as the first argument. REQUIRES that the example in the Texture2
# documentation, for inlining textures, are built and exist in path as
# 'inline_texture'.

tmplate='#Inventor V2.1 ascii \n
# Texture @TEX@ embedded in a scene graph \n
    Texture2{ \n
        filename @TEX@  \n
    } \n
'

ctmplate_begin='static const char * @NODE@[] = {'
ctmplate_end='NULL};'

if [ ! -f inline_texture ]
then
   coin-config --build inline_texture inline_texture.cpp
fi

OUTPUT=$1
shift

rm $OUTPUT

for i in $@;
  do
    fnam=${i%.svg}
    echo "Generating source for $i"
    if ! [ -f $fnam.gif ]
    then
       echo "Converting to gif"
       #Not sure if this is the best method
       convert -colors 2 -scale 128x128 $i $fnam.gif
    fi
    tmpname=$(echo -e $fnam | sed -e 's/\./\\\./g' | sed -e 's/\//\\\//g').gif
    buffername=$(basename $fnam)

    echo -e $tmplate | sed -e "s/@TEX@/${tmpname}/g" > $fnam-texture.tmp

    ./inline_texture < $fnam-texture.tmp > $fnam-texture.iv
    rm $fnam-texture.tmp
    echo "$ctmplate_begin" | sed s/@NODE@/${buffername}/g >> $OUTPUT
    cat $fnam-texture.iv | sed 's/^/"/g' | sed 's/$/\\n",/g' >> $OUTPUT
    echo -e "$ctmplate_end" >> $OUTPUT
    rm $fnam-texture.iv
  done
