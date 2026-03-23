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
# chapter 13, example 4.
#
# Time counter engine.
# The output from an time counter engine is used to control
# horizontal and vertical motion of a figure object.
# The resulting effect is that the figure jumps across
# the screen.
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
    myCamera.position = (-8.0, -7.0, 20.0)
    myCamera.heightAngle = M_PI/2.5
    myCamera.nearDistance = 15.0
    myCamera.farDistance = 25.0
    root.addChild(myCamera)
    root.addChild(SoDirectionalLight())

##############################################################
# CODE FOR The Inventor Mentor STARTS HERE

    # Set up transformations
    jumpTranslation = SoTranslation()
    root.addChild(jumpTranslation)
    initialTransform = SoTransform()
    initialTransform.translation = (-20., 0., 0.)
    initialTransform.scaleFactor = (40., 40., 40.)
    initialTransform.rotation.setValue(SbVec3f(1,0,0), M_PI/2.)
    root.addChild(initialTransform)

    # Read the man object from a file and add to the scene
    myInput = SoInput()
    if not myInput.openFile("jumpyMan.iv"):
        sys.exit(1)
    manObject = SoDB.readAll(myInput)
    if manObject == None:
        sys.exit(1)
    root.addChild(manObject)

    # Create two counters, and connect to X and Y translations.
    # The Y counter is small and high frequency.
    # The X counter is large and low frequency.
    # This results in small jumps across the screen, 
    # left to right, again and again and again and ....
    jumpHeightCounter = SoTimeCounter()
    jumpWidthCounter = SoTimeCounter()
    jump = SoComposeVec3f()

    jumpHeightCounter.max = 4
    jumpHeightCounter.frequency = 1.5
    jumpWidthCounter.max = 40
    jumpWidthCounter.frequency = 0.15

    jump.x.connectFrom(jumpWidthCounter.output)
    jump.y.connectFrom(jumpHeightCounter.output)
    jumpTranslation.translation.connectFrom(jump.vector)

# CODE FOR The Inventor Mentor ENDS HERE
##############################################################

    myRenderArea = SoGuiRenderArea(myWindow)
    myRegion = SbViewportRegion(myRenderArea.getSize()) 
    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("Jumping Man")
    myRenderArea.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
