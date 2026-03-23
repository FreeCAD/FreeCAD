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
# chapter 5, example 1.
#
# This example builds an obelisk using the Face Set node.
#

import sys

from pivy.coin import *
from pivy.sogui import *

##############################################################
## CODE FOR The Inventor Mentor STARTS HERE

##  Eight polygons. The first four are triangles 
##  The second four are quadrilaterals for the sides.
vertices = (
   ( 0, 30, 0), (-2,27, 2), ( 2,27, 2),            #front tri
   ( 0, 30, 0), (-2,27,-2), (-2,27, 2),            #left  tri
   ( 0, 30, 0), ( 2,27,-2), (-2,27,-2),            #rear  tri
   ( 0, 30, 0), ( 2,27, 2), ( 2,27,-2),            #right tri
   (-2, 27, 2), (-4,0, 4), ( 4,0, 4), ( 2,27, 2),  #front quad
   (-2, 27,-2), (-4,0,-4), (-4,0, 4), (-2,27, 2),  #left  quad
   ( 2, 27,-2), ( 4,0,-4), (-4,0,-4), (-2,27,-2),  #rear  quad
   ( 2, 27, 2), ( 4,0, 4), ( 4,0,-4), ( 2,27,-2)   #right quad
)

# Number of vertices in each polygon:
numvertices = (3, 3, 3, 3, 4, 4, 4, 4)

# Normals for each polygon:
norms = ( 
   (0, .555,  .832), (-.832, .555, 0), #front, left tris
   (0, .555, -.832), ( .832, .555, 0), #rear, right tris
   
   (0, .0739,  .9973), (-.9972, .0739, 0),#front, left quads
   (0, .0739, -.9973), ( .9972, .0739, 0),#rear, right quads
)

# set this variable to 0 if you want to use the other method
IV_STRICT = 1

def makeObeliskFaceSet():
   obelisk = SoSeparator()

   if IV_STRICT:
       # This is the preferred code for Inventor 2.1
 
       # Using the new SoVertexProperty node is more efficient
       myVertexProperty = SoVertexProperty()

       # Define the normals used:
       myVertexProperty.normal.setValues(0, 8, norms)
       myVertexProperty.normalBinding = SoNormalBinding.PER_FACE

       # Define material for obelisk
       myVertexProperty.orderedRGBA = SbColor(.4,.4,.4).getPackedValue()

       # Define coordinates for vertices
       myVertexProperty.vertex.setValues(0, 28, vertices)

       # Define the FaceSet
       myFaceSet = SoFaceSet()
       myFaceSet.numVertices.setValues(0, 8, numvertices)
 
       myFaceSet.vertexProperty = myVertexProperty
       obelisk.addChild(myFaceSet)

   else:
       # Define the normals used:
       myNormals = SoNormal()
       myNormals.vector.setValues(0, 8, norms)
       obelisk.addChild(myNormals)
       myNormalBinding = SoNormalBinding()
       myNormalBinding.value = SoNormalBinding.PER_FACE
       obelisk.addChild(myNormalBinding)

       # Define material for obelisk
       myMaterial = SoMaterial()
       myMaterial.diffuseColor = (.4, .4, .4)
       obelisk.addChild(myMaterial)

       # Define coordinates for vertices
       myCoords = SoCoordinate3()
       myCoords.point.setValues(0, 28, vertices)
       obelisk.addChild(myCoords)

       # Define the FaceSet
       myFaceSet = SoFaceSet()
       myFaceSet.numVertices.setValues(0, 8, numvertices)
       obelisk.addChild(myFaceSet)

   return obelisk

## CODE FOR The Inventor Mentor ENDS HERE
##############################################################

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    root = SoSeparator()

    root.addChild(makeObeliskFaceSet())

    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("Face Set: Obelisk")
    myViewer.show()
    myViewer.viewAll()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
