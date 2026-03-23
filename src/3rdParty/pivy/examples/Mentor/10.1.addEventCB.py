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
# This is an example from The Inventor Mentor
# chapter 10, example 1.
#
# The scene graph has 4 objects which may be
# selected by picking with the left mouse button
# (use shift key to extend the selection to more
# than one object).
# 
# Hitting the up arrow key will increase the size of
# each selected object; hitting down arrow will decrease
# the size of each selected object.
#

from __future__ import print_function
import sys

from pivy.coin import *
from pivy.sogui import *

# Global data
cubeTransform, sphereTransform, coneTransform, cylTransform = [None] * 4

# Scale each object in the selection list
def myScaleSelection(selection, sf):
    global cubeTransform, sphereTransform, coneTransform, cylTransform

    # Scale each object in the selection list
    for i in range(selection.getNumSelected()):
        selectedPath = selection.getPath(i)
        xform = None

        # Look for the shape node, starting from the tail of the
        # path.  Once we know the type of shape, we know which
        # transform to modify
        for j in range(selectedPath.getLength()):
            if xform != None: break
            n = selectedPath.getNodeFromTail(j)

            if n.isOfType(SoCube.getClassTypeId()):
                xform = cubeTransform
            elif n.isOfType(SoCone.getClassTypeId()):
                xform = coneTransform
            elif n.isOfType(SoSphere.getClassTypeId()):
                xform = sphereTransform
            elif n.isOfType(SoCylinder.getClassTypeId()):
                xform = cylTransform

        # Apply the scale
        xform.scaleFactor = xform.scaleFactor.getValue() * sf

###############################################################
# CODE FOR The Inventor Mentor STARTS HERE  (part 2)

# If the event is down arrow, then scale down every object 
# in the selection list if the event is up arrow, scale up.
# The userData = selection is the selectionRoot from main().
def myKeyPressCB(selection, eventCB):
    event = eventCB.getEvent()
    # check for the Up and Down arrow keys being pressed
    if SoKeyboardEvent_isKeyPressEvent(event, SoKeyboardEvent.UP_ARROW):
        myScaleSelection(selection, 1.1)
        eventCB.setHandled()
    elif SoKeyboardEvent_isKeyPressEvent(event, SoKeyboardEvent.DOWN_ARROW):
        myScaleSelection(selection, 1.0/1.1)
        eventCB.setHandled()

# CODE FOR The Inventor Mentor ENDS HERE
###############################################################

def main():
    global cubeTransform, sphereTransform, coneTransform, cylTransform
    # Print out usage message
    print("Left mouse button        - selects object")
    print("<shift>Left mouse button - selects multiple objects")
    print("Up and Down arrows       - scale selected objects")

    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None:
        sys.exit(1)

    # Create and set up the selection node
    selectionRoot = SoSelection()
    selectionRoot.policy = SoSelection.SHIFT
   
    # Add a camera and some light
    myCamera = SoPerspectiveCamera()
    selectionRoot.addChild(myCamera)
    selectionRoot.addChild(SoDirectionalLight())

###############################################################
# CODE FOR The Inventor Mentor STARTS HERE  (part 1)

    # An event callback node so we can receive key press events
    myEventCB = SoEventCallback()
    myEventCB.addEventCallback(SoKeyboardEvent.getClassTypeId(), 
                               myKeyPressCB, selectionRoot)
    selectionRoot.addChild(myEventCB)

# CODE FOR The Inventor Mentor ENDS HERE
###############################################################

    # Add some geometry to the scene

    # a red cube
    cubeRoot = SoSeparator()
    cubeMaterial = SoMaterial()
    cubeTransform = SoTransform()
    cubeRoot.addChild(cubeTransform)
    cubeRoot.addChild(cubeMaterial)
    cubeRoot.addChild(SoCube())
    cubeTransform.translation = (-2, 2, 0)
    cubeMaterial.diffuseColor = (.8, 0, 0)
    selectionRoot.addChild(cubeRoot)

    # a blue sphere
    sphereRoot = SoSeparator()
    sphereMaterial = SoMaterial()
    sphereTransform = SoTransform()
    sphereRoot.addChild(sphereTransform)
    sphereRoot.addChild(sphereMaterial)
    sphereRoot.addChild(SoSphere())
    sphereTransform.translation = (2, 2, 0)
    sphereMaterial.diffuseColor = (0, 0, .8)
    selectionRoot.addChild(sphereRoot)

    # a green cone
    coneRoot = SoSeparator()
    coneMaterial = SoMaterial()
    coneTransform = SoTransform()
    coneRoot.addChild(coneTransform)
    coneRoot.addChild(coneMaterial)
    coneRoot.addChild(SoCone())
    coneTransform.translation = (2, -2, 0)
    coneMaterial.diffuseColor = (0, .8, 0)
    selectionRoot.addChild(coneRoot)

    # a magenta cylinder
    cylRoot = SoSeparator()
    cylMaterial = SoMaterial()
    cylTransform = SoTransform()
    cylRoot.addChild(cylTransform)
    cylRoot.addChild(cylMaterial)
    cylRoot.addChild(SoCylinder())
    cylTransform.translation = (-2, -2, 0)
    cylMaterial.diffuseColor = (.8, 0, .8)
    selectionRoot.addChild(cylRoot)

    # Create a render area for viewing the scene
    myRenderArea = SoGuiRenderArea(myWindow)
    myRenderArea.setSceneGraph(selectionRoot)
    
    # need to make a reference like this otherwise SoBoxHighlightRenderAction() gets
    # dereferenced after the myRenderArea.setGLRenderAction() call, resulting in its
    # destructor to be called.
    # i.e.: myRenderArea.setGLRenderAction(SoBoxHighlightRenderAction()) would result
    # in a segfault!
    # in my opinion this should _not_ happen, but it does! :(
    #
    # FIXME: investigate why this is so...
    # myRenderArea.setGLRenderAction(SoBoxHighlightRenderAction())
    boxhra = SoBoxHighlightRenderAction()
    myRenderArea.setGLRenderAction(boxhra)
    
    myRenderArea.redrawOnSelectionChange(selectionRoot)
    myRenderArea.setTitle("Adding Event Callbacks")

    # Make the camera see the whole scene
    viewportRegion = myRenderArea.getViewportRegion()
    myCamera.viewAll(selectionRoot, viewportRegion, 2.0)

    # FIXME: soqt maybe has problems here!
    selectionRoot.addSelectionCallback(myWindow.scheduleRedraw)

    # Show our application window, and loop forever...
    myRenderArea.show()
    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
