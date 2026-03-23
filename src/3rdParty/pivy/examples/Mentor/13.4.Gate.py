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
# chapter 13, example 5.
#
# Gate engine.
# Mouse button presses enable and disable a gate engine.
# The gate engine controls an elapsed time engine that
# controls the motion of the duck.
#

from __future__ import print_function
import sys

from pivy.coin import *
from pivy.sogui import *

#############################################################
# CODE FOR The Inventor Mentor STARTS HERE  (part 2)

# This routine is called for every mouse button event.
def myMousePressCB(userData, eventCB):
    # In Pivy no cast is necessary as it gets autocasted for you.
    gate = userData
    event = eventCB.getEvent()

    # Check for mouse button being pressed
    if SoMouseButtonEvent.isButtonPressEvent(event, SoMouseButtonEvent.ANY):

        # Toggle the gate that controls the duck motion
        if gate.enable.getValue():
            gate.enable = FALSE
        else:
            gate.enable = TRUE

        eventCB.setHandled()

# CODE FOR The Inventor Mentor ENDS HERE
#############################################################


def main():
    # Print out usage message
    print("Click the left mouse button to enable/disable the duck motion")

    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])  
    if myWindow == None: sys.exit(1)     

    root = SoSeparator()

    # Add a camera and light
    myCamera = SoPerspectiveCamera()
    myCamera.position = (0., -4., 8.0)
    myCamera.heightAngle = M_PI/2.5
    myCamera.nearDistance = 1.0
    myCamera.farDistance = 15.0
    root.addChild(myCamera)
    root.addChild(SoDirectionalLight())

    # Rotate scene slightly to get better view
    globalRotXYZ = SoRotationXYZ()
    globalRotXYZ.axis = SoRotationXYZ.X
    globalRotXYZ.angle = M_PI/9
    root.addChild(globalRotXYZ)

    # Pond group
    pond = SoSeparator()
    root.addChild(pond)
    cylMaterial = SoMaterial()
    cylMaterial.diffuseColor = (0., 0.3, 0.8)
    pond.addChild(cylMaterial)
    cylTranslation = SoTranslation()
    cylTranslation.translation = (0., -6.725, 0.)
    pond.addChild(cylTranslation)
    myCylinder = SoCylinder()
    myCylinder.radius = 4.0
    myCylinder.height = 0.5
    pond.addChild(myCylinder)

#############################################################
# CODE FOR The Inventor Mentor STARTS HERE  (part 1)

    # Duck group
    duck = SoSeparator()
    root.addChild(duck)

    # Read the duck object from a file and add to the group
    myInput = SoInput()
    if not myInput.openFile("duck.iv"):
        sys.exit(1)
    duckObject = SoDB.readAll(myInput)
    if duckObject == None:
        sys.exit(1)

    # Set up the duck transformations
    duckRotXYZ = SoRotationXYZ()
    duck.addChild(duckRotXYZ)
    initialTransform = SoTransform()
    initialTransform.translation = (0., 0., 3.)
    initialTransform.scaleFactor = (6., 6., 6.)
    duck.addChild(initialTransform)

    duck.addChild(duckObject)

    # Update the rotation value if the gate is enabled.
    myGate = SoGate(SoMFFloat.getClassTypeId())
    myCounter = SoElapsedTime()
    myGate.input.connectFrom(myCounter.timeOut) 
    duckRotXYZ.axis = SoRotationXYZ.Y  # rotate about Y axis
    duckRotXYZ.angle.connectFrom(myGate.output)

    # Add an event callback to catch mouse button presses.
    # Each button press will enable or disable the duck motion.
    myEventCB = SoEventCallback()
    myEventCB.addEventCallback(SoMouseButtonEvent.getClassTypeId(),
                               myMousePressCB, myGate)
    root.addChild(myEventCB)

# CODE FOR The Inventor Mentor ENDS HERE
#############################################################

    myRenderArea = SoGuiRenderArea(myWindow)
    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("Duck Pond")
    myRenderArea.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
