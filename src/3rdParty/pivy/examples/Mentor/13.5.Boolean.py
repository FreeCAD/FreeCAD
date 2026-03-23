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
# chapter 13, example 6.
#
# Boolean engine.  Derived from example 13.5.
# The smaller duck stays still while the bigger duck moves,
# and starts moving as soon as the bigger duck stops.
#

from __future__ import print_function
import sys

from pivy.coin import *
from pivy.sogui import *

# This routine is called for every mouse button event.
def myMousePressCB(gate, eventCB):
    event = eventCB.getEvent()

    # Check for mouse button being pressed
    if SoMouseButtonEvent.isButtonPressEvent(event, SoMouseButtonEvent.ANY):

        # Toggle the gate that controls the duck motion
        if gate.enable.getValue():
            gate.enable = FALSE
        else:
            gate.enable = TRUE

        eventCB.setHandled()


def main():
    # Print out usage message
    print("Only one duck can move at a time.")
    print("Click the left mouse button to toggle between the two ducks.")

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
    pondTranslation = SoTranslation()
    pondTranslation.translation = (0., -6.725, 0.)
    pond.addChild(pondTranslation)
    # water
    waterMaterial = SoMaterial()
    waterMaterial.diffuseColor = (0., 0.3, 0.8)
    pond.addChild(waterMaterial)
    waterCylinder = SoCylinder()
    waterCylinder.radius = 4.0
    waterCylinder.height = 0.5
    pond.addChild(waterCylinder)
    # rock
    rockMaterial = SoMaterial()
    rockMaterial.diffuseColor = (0.8, 0.23, 0.03)
    pond.addChild(rockMaterial)
    rockSphere = SoSphere()
    rockSphere.radius = 0.9
    pond.addChild(rockSphere)

    # Read the duck object from a file and add to the group
    myInput = SoInput()
    if not myInput.openFile("duck.iv"):
        sys.exit(1)
    duckObject = SoDB.readAll(myInput)
    if duckObject == None:
        sys.exit(1)

#############################################################
# CODE FOR The Inventor Mentor STARTS HERE  

    # Bigger duck group
    bigDuck = SoSeparator()
    root.addChild(bigDuck)
    bigDuckRotXYZ = SoRotationXYZ()
    bigDuck.addChild(bigDuckRotXYZ)
    bigInitialTransform = SoTransform()
    bigInitialTransform.translation = (0., 0., 3.5)
    bigInitialTransform.scaleFactor = (6., 6., 6.)
    bigDuck.addChild(bigInitialTransform)
    bigDuck.addChild(duckObject)

    # Smaller duck group
    smallDuck = SoSeparator()
    root.addChild(smallDuck)
    smallDuckRotXYZ = SoRotationXYZ()
    smallDuck.addChild(smallDuckRotXYZ)
    smallInitialTransform = SoTransform()
    smallInitialTransform.translation = (0., -2.24, 1.5)
    smallInitialTransform.scaleFactor = (4., 4., 4.)
    smallDuck.addChild(smallInitialTransform)
    smallDuck.addChild(duckObject)

    # Use a gate engine to start/stop the rotation of 
    # the bigger duck.
    bigDuckGate = SoGate(SoMFFloat.getClassTypeId())
    bigDuckTime = SoElapsedTime()
    bigDuckGate.input.connectFrom(bigDuckTime.timeOut) 
    bigDuckRotXYZ.axis = SoRotationXYZ.Y  # Y axis
    bigDuckRotXYZ.angle.connectFrom(bigDuckGate.output)

    # Each mouse button press will enable/disable the gate 
    # controlling the bigger duck.
    myEventCB = SoEventCallback()
    myEventCB.addEventCallback(SoMouseButtonEvent.getClassTypeId(),
                               myMousePressCB, bigDuckGate)
    root.addChild(myEventCB)

    # Use a Boolean engine to make the rotation of the smaller
    # duck depend on the bigger duck.  The smaller duck moves
    # only when the bigger duck is still.
    myBoolean = SoBoolOperation()
    myBoolean.a.connectFrom(bigDuckGate.enable)
    myBoolean.operation = SoBoolOperation.NOT_A

    smallDuckGate = SoGate(SoMFFloat.getClassTypeId())
    smallDuckTime = SoElapsedTime()
    smallDuckGate.input.connectFrom(smallDuckTime.timeOut) 
    smallDuckGate.enable.connectFrom(myBoolean.output) 
    smallDuckRotXYZ.axis = SoRotationXYZ.Y  # Y axis
    smallDuckRotXYZ.angle.connectFrom(smallDuckGate.output)

# CODE FOR The Inventor Mentor ENDS HERE
#############################################################

    myRenderArea = SoGuiRenderArea(myWindow)
    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("Duck and Duckling")
    myRenderArea.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
