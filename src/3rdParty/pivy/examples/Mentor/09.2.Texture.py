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
# chapter 9, example 2.
#
# Using the offscreen renderer to generate a texture map.
# Generate simple scene and grab the image to use as
# a texture map.
#

from __future__ import print_function
import sys

from pivy.coin import *
from pivy.sogui import *

def generateTextureMap(root, texture, textureWidth, textureHeight):
    myViewport = SbViewportRegion(textureWidth, textureHeight)

    # Render the scene
    myRenderer = SoOffscreenRenderer(myViewport)
    myRenderer.setBackgroundColor(SbColor(0.3, 0.3, 0.3))
    if not myRenderer.render(root):
        del myRenderer
        return FALSE

    # Generate the texture
    texture.image.setValue(SbVec2s(textureWidth, textureHeight),
                           SoOffscreenRenderer.RGB, myRenderer.getBuffer())

    del myRenderer
    return TRUE

def main():
    # Initialize Inventor and Qt
    appWindow = SoGui.init(sys.argv[0])
    if appWindow == None:
        sys.exit(1)

    # Make a scene from reading in a file
    texRoot = SoSeparator()
    input = SoInput()

    input.openFile("jumpyMan.iv")
    result = SoDB.readAll(input)

    myCamera = SoPerspectiveCamera()
    rot = SoRotationXYZ()
    rot.axis = SoRotationXYZ.X
    rot.angle = M_PI_2
    myCamera.position = (-0.2, -0.2, 2.0)
    myCamera.scaleHeight(0.4)
    texRoot.addChild(myCamera)
    texRoot.addChild(SoDirectionalLight())
    texRoot.addChild(rot)
    texRoot.addChild(result)

    # Generate the texture map
    texture = SoTexture2()
    if generateTextureMap(texRoot, texture, 64, 64):
        print("Successfully generated texture map")
    else:
        print("Could not generate texture map")

    # Make a scene with a cube and apply the texture to it
    root = SoSeparator()
    root.addChild(texture)
    root.addChild(SoCube())

    # Initialize an Examiner Viewer
    viewer = SoGuiExaminerViewer(appWindow)
    viewer.setSceneGraph(root)
    viewer.setTitle("Offscreen Rendered Texture")

    # In Inventor 2.1, if the machine does not have hardware texture
    # mapping, we must override the default drawStyle to display textures.
    # viewer.setDrawStyle(SoGuiViewer.STILL, SoGuiViewer.VIEW_AS_IS)

    viewer.show()

    SoGui.show(appWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
