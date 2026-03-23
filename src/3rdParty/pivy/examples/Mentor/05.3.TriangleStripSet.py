#!/usr/bin/env python

###
# Copyright (c) 2002-2007 Systems in Motion
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

###
# This is an example from the Inventor Mentor,
# chapter 5, example 3.
#
# This example creates a TriangleStripSet. It creates
# a pennant-shaped flag.
#

import sys

from pivy.coin import *
from pivy.sogui import *

##############################################################
## CODE FOR The Inventor Mentor STARTS HERE

#
# Positions of all of the vertices:
#
vertexPositions = (
   (  0,   12,    0 ), (   0,   15,    0),
   (2.1, 12.1,  -.2 ), ( 2.1, 14.6,  -.2),
   (  4, 12.5,  -.7 ), (   4, 14.5,  -.7),
   (4.5, 12.6,  -.8 ), ( 4.5, 14.4,  -.8),
   (  5, 12.7,   -1 ), (   5, 14.4,   -1),
   (4.5, 12.8, -1.4 ), ( 4.5, 14.6, -1.4),
   (  4, 12.9, -1.6 ), (   4, 14.8, -1.6),
   (3.3, 12.9, -1.8 ), ( 3.3, 14.9, -1.8),
   (  3,   13, -2.0 ), (   3, 14.9, -2.0), 
   (3.3, 13.1, -2.2 ), ( 3.3, 15.0, -2.2),
   (  4, 13.2, -2.5 ), (   4, 15.0, -2.5),
   (  6, 13.5, -2.2 ), (   6, 14.8, -2.2),
   (  8, 13.4,   -2 ), (   8, 14.6,   -2),
   ( 10, 13.7, -1.8 ), (  10, 14.4, -1.8),
   ( 12,   14, -1.3 ), (  12, 14.5, -1.3),
   ( 15, 14.9, -1.2 ), (  15,   15, -1.2),

   (-.5, 15,   0 ), ( -.5, 0,   0),   # the flagpole
   (  0, 15,  .5 ), (   0, 0,  .5),
   (  0, 15, -.5 ), (   0, 0, -.5),
   (-.5, 15,   0 ), ( -.5, 0,   0)
)


# Number of vertices in each strip.
numVertices = (
   32, # flag
   8   # pole
)
 
# Colors for the 12 faces
colors = (
   ( .5, .5,  1 ), # purple flag
   ( .4, .4, .4 ), # grey flagpole
)

# set this variable to 0 if you want to use the other method
IV_STRICT = 1

# Routine to create a scene graph representing a pennant.
def makePennant():
    result = SoSeparator()

    # A shape hints tells the ordering of polygons. 
    # This insures double sided lighting.
    myHints = SoShapeHints()
    myHints.vertexOrdering = SoShapeHints.COUNTERCLOCKWISE
    result.addChild(myHints)

    if IV_STRICT:
        # This is the preferred code for Inventor 2.1 

        # Using the new SoVertexProperty node is more efficient
        myVertexProperty = SoVertexProperty()

        # Define colors for the strips
        for i in range(2):
            myVertexProperty.orderedRGBA.set1Value(i, SbColor(colors[i]).getPackedValue())
            myVertexProperty.materialBinding = SoMaterialBinding.PER_PART

        # Define coordinates for vertices
        myVertexProperty.vertex.setValues(0, 40, vertexPositions)

        # Define the TriangleStripSet, made of two strips.
        myStrips = SoTriangleStripSet()
        myStrips.numVertices.setValues(0, 2, numVertices)
 
        myStrips.vertexProperty = myVertexProperty
        result.addChild(myStrips)

    else:
        # Define colors for the strips
        myMaterials = SoMaterial()
        myMaterials.diffuseColor.setValues(0, 2, colors)
        result.addChild(myMaterials)
        myMaterialBinding = SoMaterialBinding()
        myMaterialBinding.value = SoMaterialBinding.PER_PART
        result.addChild(myMaterialBinding)

        # Define coordinates for vertices
        myCoords = SoCoordinate3()
        myCoords.point.setValues(0, 40, vertexPositions)
        result.addChild(myCoords)

        # Define the TriangleStripSet, made of two strips.
        myStrips = SoTriangleStripSet()
        myStrips.numVertices.setValues(0, 2, numVertices)
        result.addChild(myStrips)

    return result

## CODE FOR The Inventor Mentor ENDS HERE
##############################################################

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    root = makePennant()

    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("Triangle Strip Set: Pennant")
    myViewer.show()
    myViewer.viewAll()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
