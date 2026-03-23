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
# chapter 14, example 3.
#
# This example illustrates the creation of motion hierarchies
# using nodekits by creating a model of a balance-style scale.
#
# It adds an SoEventCallback to the "callback" list in the 
#     nodekit called 'support.'
# The callback will have the following response to events:
# Pressing right arrow key == lower the right pan
# Pressing left arrow key  == lower the left pan
# The pans are lowered by animating three rotations in the 
#     motion hierarchy.
# Use an SoText2Kit to print instructions to the user as part
#     of the scene.
#

import sys

from pivy.coin import *
from pivy.sogui import *

# Callback Function for Animating the Balance Scale.
# -- used to make the balance tip back and forth
# -- Note: this routine is only called in response to KeyPress
#    events since the call 'setEventInterest(KeyPressMask)' is
#    made on the SoEventCallback node that uses it.
# -- The routine checks if the key pressed was left arrow (which
#    is XK_Left in X-windows talk), or right arrow (which is
#    XK_Right)
# -- The balance is made to tip by rotating the beam part of the
#    scale (to tip it) and then compensating (making the strings
#    vertical again) by rotating the string parts in the opposite
#    direction.
def tipTheBalance(support, # The nodekit representing 'support', the
                  # fulcrum of the balance. Passed in during
                  # main routine, below. 
                  eventCB):

    ev = eventCB.getEvent()

    # Which Key was pressed?
    # If Right or Left Arrow key, then continue...
    if SoKeyboardEvent.isKeyPressEvent(ev, SoKeyboardEvent.RIGHT_ARROW) or \
       SoKeyboardEvent.isKeyPressEvent(ev, SoKeyboardEvent.LEFT_ARROW):
        startRot, beamIncrement, stringIncrement = SbRotation(), SbRotation(), SbRotation()
        
        # These three parts are extracted based on knowledge of the
        # motion hierarchy (see the diagram in the main routine.
        beam1   = support.getPart("childList[0]",TRUE)
        string1 = beam1.getPart("childList[0]",TRUE)
        string2 = beam1.getPart("childList[1]",TRUE)

        # Set angular increments to be .1 Radians about the Z-Axis
        # The strings rotate opposite the beam, and the two types
        # of key press produce opposite effects.
        if SoKeyboardEvent.isKeyPressEvent(ev, SoKeyboardEvent.RIGHT_ARROW):
            beamIncrement.setValue(SbVec3f(0, 0, 1), -.1)
            stringIncrement.setValue(SbVec3f(0, 0, 1), .1)
        else:
            beamIncrement.setValue(SbVec3f(0, 0, 1), .1)
            stringIncrement.setValue(SbVec3f(0, 0, 1), -.1)

        # Use SO_GET_PART to find the transform for each of the 
        # rotating parts and modify their rotations.

        xf = beam1.getPart("transform", TRUE)
        startRot = xf.rotation.getValue()
        startRot *= beamIncrement
        xf.rotation = startRot

        xf = string1.getPart("transform", TRUE)
        startRot = xf.rotation.getValue()
        startRot *= stringIncrement
        xf.rotation = startRot

        xf = string2.getPart("transform", TRUE)
        startRot = xf.rotation.getValue()
        startRot *= stringIncrement     
        xf.rotation = startRot

        eventCB.setHandled()

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])  
    if myWindow == None: sys.exit(1)     

    myScene = SoSceneKit()

    myScene.setPart("lightList[0]", SoLightKit())
    myScene.setPart("cameraList[0]", SoCameraKit())
    myScene.setCameraNumber(0)

    # Create the Balance Scale -- put each part in the 
    # childList of its parent, to build up this hierarchy:
    #
    #                    myScene
    #                       |
    #                     support
    #                       |
    #                     beam
    #                       |
    #                   --------
    #                   |       |
    #                string1  string2
    #                   |       |
    #                tray1     tray2

    support = SoShapeKit()
    support.setPart("shape", SoCone())
    support.set("shape { height 3 bottomRadius .3 }")
    myScene.setPart("childList[0]", support)

    beam = SoShapeKit()
    beam.setPart("shape", SoCube())
    beam.set("shape { width 3 height .2 depth .2 }")
    beam.set("transform { translation 0 1.5 0 } ")
    support.setPart("childList[0]", beam)

    string1 = SoShapeKit()
    string1.setPart("shape", SoCylinder())
    string1.set("shape { radius .05 height 2}")
    string1.set("transform { translation -1.5 -1 0 }")
    string1.set("transform { center 0 1 0 }")
    beam.setPart("childList[0]", string1)

    string2 = SoShapeKit()
    string2.setPart("shape", SoCylinder())
    string2.set("shape { radius .05 height 2}")
    string2.set("transform { translation 1.5 -1 0 } ")
    string2.set("transform { center 0 1 0 } ")
    beam.setPart("childList[1]", string2)

    tray1 = SoShapeKit()
    tray1.setPart("shape", SoCylinder())
    tray1.set("shape { radius .75 height .1 }")
    tray1.set("transform { translation 0 -1 0 } ")
    string1.setPart("childList[0]", tray1)

    tray2 = SoShapeKit()
    tray2.setPart("shape", SoCylinder())
    tray2.set("shape { radius .75 height .1 }")
    tray2.set("transform { translation 0 -1 0 } ")
    string2.setPart("childList[0]", tray2)

    # Add EventCallback so Balance Responds to Events
    myCallbackNode = SoEventCallback()
    myCallbackNode.addEventCallback(SoKeyboardEvent.getClassTypeId(),
                                    tipTheBalance, support)
    support.setPart("callbackList[0]", myCallbackNode)

    # Add Instructions as Text in the Scene...
    myText = SoShapeKit()
    myText.setPart("shape", SoText2())
    myText.set("shape { string \"Press Left or Right Arrow Key\" }")
    myText.set("shape { justification CENTER }")
    myText.set("font { name \"Helvetica-Bold\" }")
    myText.set("font { size 16.0 }")
    myText.set("transform { translation 0 -2 0 }")
    myScene.setPart("childList[1]", myText)

    myRenderArea = SoGuiRenderArea(myWindow)

    # Get camera from scene and tell it to viewAll...
    myCamera = myScene.getPart("cameraList[0].camera", TRUE)
    myCamera.viewAll(myScene, myRenderArea.getViewportRegion())

    myRenderArea.setSceneGraph(myScene)
    myRenderArea.setTitle("Balance Scale Made of Nodekits")
    myRenderArea.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
