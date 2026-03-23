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
# This is an example from the Inventor Mentor
# chapter 7, example 2.
#
# This example illustrates using texture coordinates on
# a Face Set.
#

import sys

from pivy.coin import *
from pivy.sogui import *

# set this variable to 0 if you want to use the other method
IV_STRICT = 1

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    root = SoSeparator()
    
    # Choose a texture
    brick = SoTexture2()
    root.addChild(brick)
    brick.filename = "brick.1.rgb"

    if IV_STRICT:
        # This is the preferred code for Inventor 2.1 

        # Using the new SoVertexProperty node is more efficient
        myVertexProperty = SoVertexProperty()

        # Define the square's spatial coordinates
        myVertexProperty.vertex.set1Value(0, SbVec3f(-3, -3, 0))
        myVertexProperty.vertex.set1Value(1, SbVec3f( 3, -3, 0))
        myVertexProperty.vertex.set1Value(2, SbVec3f( 3,  3, 0))
        myVertexProperty.vertex.set1Value(3, SbVec3f(-3,  3, 0))

        # Define the square's normal
        myVertexProperty.normal.set1Value(0, SbVec3f(0, 0, 1))

        # Define the square's texture coordinates
        myVertexProperty.texCoord.set1Value(0, SbVec2f(0, 0))
        myVertexProperty.texCoord.set1Value(1, SbVec2f(1, 0))
        myVertexProperty.texCoord.set1Value(2, SbVec2f(1, 1))
        myVertexProperty.texCoord.set1Value(3, SbVec2f(0, 1))

        # SoTextureCoordinateBinding node is now obsolete--in Inventor 2.1,
        # texture coordinates will always be generated if none are 
        # provided.
        #
        # tBind = SoTextureCoordinateBinding()
        # root.addChild(tBind)
        # tBind.value(SoTextureCoordinateBinding.PER_VERTEX)
        #
        # Define normal binding
        myVertexProperty.normalBinding = SoNormalBinding.OVERALL

        # Define a FaceSet
        myFaceSet = SoFaceSet()
        root.addChild(myFaceSet)
        myFaceSet.numVertices.set1Value(0, 4)

        myFaceSet.vertexProperty.setValue(myVertexProperty)

    else:
        # Define the square's spatial coordinates
        coord = SoCoordinate3()
        root.addChild(coord)
        coord.point.set1Value(0, SbVec3f(-3, -3, 0))
        coord.point.set1Value(1, SbVec3f( 3, -3, 0))
        coord.point.set1Value(2, SbVec3f( 3,  3, 0))
        coord.point.set1Value(3, SbVec3f(-3,  3, 0))

        # Define the square's normal
        normal = SoNormal()
        root.addChild(normal)
        normal.vector.set1Value(0, SbVec3f(0, 0, 1))

        # Define the square's texture coordinates
        texCoord = SoTextureCoordinate2()
        root.addChild(texCoord)
        texCoord.point.set1Value(0, SbVec2f(0, 0))
        texCoord.point.set1Value(1, SbVec2f(1, 0))
        texCoord.point.set1Value(2, SbVec2f(1, 1))
        texCoord.point.set1Value(3, SbVec2f(0, 1))

        # Define normal binding
        nBind = SoNormalBinding()
        root.addChild(nBind)
        nBind.value = SoNormalBinding.OVERALL

        # SoTextureCoordinateBinding node is now obsolete--in Inventor 2.1,
        # texture coordinates will always be generated if none are 
        # provided.
        #
        # tBind = SoTextureCoordinateBinding()
        # root.addChild(tBind)
        # tBind.value.setValue(SoTextureCoordinateBinding.PER_VERTEX)
        #

        # Define a FaceSet
        myFaceSet = SoFaceSet()
        root.addChild(myFaceSet)
        myFaceSet.numVertices.set1Value(0, 4)

    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("Texture Coordinates")

    # In Inventor 2.1, if the machine does not have hardware texture
    # mapping, we must override the default drawStyle to display textures.
    # myViewer.setDrawStyle(SoGuiViewer.STILL, SoGuiViewer.VIEW_AS_IS)

    myViewer.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
