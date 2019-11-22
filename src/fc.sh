#!/bin/bash

# You should have to change just next 2 (uncommented) lines.
CASROOT=/opt/OpenCASCADE5.2/ros
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CASROOT/Linux/lib
export XERCESCROOT=/usr/local

export CSF_MDTVFontDirectory=$CASROOT/src/FontMFT/
export CSF_MDTVTexturesDirectory=$CASROOT/src/Textures/
export CSF_UnitsDefinition=$CASROOT/src/UnitsAPI/Units.dat
export CSF_UnitsLexicon=$CASROOT/src/UnitsAPI/Lexi_Expr.dat
export CSF_GraphicShr=$CASROOT/Linux/lib/libTKOpenGl.so

# These two env. variables are overwritten by FreeCAD
export CSF_PluginDefaults=$CASROOT/src/XCAFResources
export CSF_StandardDefaults=$CASROOT/src/StdResource

export TEMP=$HOME/temp

./FreeCAD $1 $2 $3 $4 $5

