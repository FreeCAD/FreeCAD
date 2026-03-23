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
# chapter 13, example 3.
#
# Elapsed time engine.
# The output from an elapsed time engine is used to control
# the translation of the object.  The resulting effect is
# that the figure slides across the scene.
#

import sys

from pivy.coin import *
from pivy.sogui import *

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])  
    if myWindow == None: sys.exit(1)     

    root = SoSeparator()

    # Add a camera and light
    myCamera = SoPerspectiveCamera()
    myCamera.position = (-2.0, -2.0, 5.0)
    myCamera.heightAngle = M_PI/2.5
    myCamera.nearDistance = 2.0
    myCamera.farDistance = 7.0
    root.addChild(myCamera)
    root.addChild(SoDirectionalLight())

    # Set up transformations
    slideTranslation = SoTranslation()
    root.addChild(slideTranslation)
    initialTransform = SoTransform()
    initialTransform.translation = (-5., 0., 0.)
    initialTransform.scaleFactor = (10., 10., 10.)
    initialTransform.rotation.setValue(SbVec3f(1,0,0), M_PI/2.)
    root.addChild(initialTransform)

    # Read the figure object from a file and add to the scene
    myInput = SoInput()
    if not myInput.openFile("jumpyMan.iv"):
        sys.exit (1)
    figureObject = SoDB.readAll(myInput)
    if figureObject == None:
        sys.exit(1)
    root.addChild(figureObject)

    # Make the X translation value change over time.
    myCounter = SoElapsedTime()
    slideDistance = SoComposeVec3f()
    slideDistance.x.connectFrom(myCounter.timeOut)
    slideTranslation.translation.connectFrom(slideDistance.vector)

    myRenderArea = SoGuiRenderArea(myWindow)
    myRegion = SbViewportRegion(myRenderArea.getSize()) 
    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("Sliding Man")
    myRenderArea.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
