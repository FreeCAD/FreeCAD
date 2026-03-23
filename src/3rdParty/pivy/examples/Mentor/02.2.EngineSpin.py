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
# chapter 2, example 2.
#
# Use an engine to make the cone spin.
#

import sys

from pivy.coin import *
from pivy.sogui import *

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    root = SoSeparator()
    myCamera = SoPerspectiveCamera()
    root.addChild(myCamera)
    root.addChild(SoDirectionalLight())
    
    # This transformation is modified to rotate the cone
    myRotXYZ = SoRotationXYZ()
    root.addChild(myRotXYZ)

    myMaterial = SoMaterial()
    myMaterial.diffuseColor = (1.0, 0.0, 0.0)   # Red
    root.addChild(myMaterial)
    root.addChild(SoCone())

    # An engine rotates the object. The output of myCounter 
    # is the time in seconds since the program started.
    # Connect this output to the angle field of myRotXYZ
    myRotXYZ.axis = SoRotationXYZ.X     # rotate about X axis
    myCounter = SoElapsedTime()
    myRotXYZ.angle.connectFrom(myCounter.timeOut)

    myRenderArea = SoGuiRenderArea(myWindow)
    myCamera.viewAll(root, myRenderArea.getViewportRegion())
    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("Engine Spin")
    myRenderArea.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
