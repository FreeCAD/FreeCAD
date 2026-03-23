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
# chapter 7, example 3.
#
# This example illustrates using texture functions to
# generate texture coordinates on a sphere.
# It draws three texture mapped spheres, each with a 
# different repeat frequency as defined by the fields of the 
# SoTextureCoordinatePlane node.
#

import sys

from pivy.coin import *
from pivy.sogui import *

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    root = SoSeparator()

    # Choose a texture.
    faceTexture = SoTexture2()
    root.addChild(faceTexture)
    faceTexture.filename = "sillyFace.rgb"

    # Make the diffuse color pure white
    myMaterial = SoMaterial()
    myMaterial.diffuseColor = (1,1,1)
    root.addChild(myMaterial)

    # This texture2Transform centers the texture about (0,0,0) 
    myTexXf = SoTexture2Transform()
    myTexXf.translation = (.5,.5)
    root.addChild(myTexXf)

    # Define a texture coordinate plane node.  This one will 
    # repeat with a frequency of two times per unit length.
    # Add a sphere for it to affect.
    texPlane1 = SoTextureCoordinatePlane()
    texPlane1.directionS = (2,0,0)
    texPlane1.directionT = (0,2,0)
    root.addChild(texPlane1)
    root.addChild(SoSphere())

    # A translation node for spacing the three spheres.
    myTranslation = SoTranslation()
    myTranslation.translation = (2.5,0,0)

    # Create a second sphere with a repeat frequency of 1.
    texPlane2 = SoTextureCoordinatePlane()
    texPlane2.directionS = (1,0,0)
    texPlane2.directionT = (0,1,0)
    root.addChild(myTranslation)
    root.addChild(texPlane2)
    root.addChild(SoSphere())

    # The third sphere has a repeat frequency of .5
    texPlane3 = SoTextureCoordinatePlane()
    texPlane3.directionS = (.5,0,0)
    texPlane3.directionT = (0,.5,0)
    root.addChild(myTranslation)
    root.addChild(texPlane3)
    root.addChild(SoSphere())

    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("Texture Coordinate Plane")

    # In Inventor 2.1, if the machine does not have hardware texture
    # mapping, we must override the default drawStyle to display textures.
    # myViewer.setDrawStyle(SoGuiViewer.STILL, SoGuiViewer.VIEW_AS_IS)
    
    myViewer.show()
    myViewer.viewAll()
    
    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
