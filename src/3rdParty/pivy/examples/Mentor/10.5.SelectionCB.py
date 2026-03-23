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
# This is an example from The Inventor Mentor,
# chapter 10, example 5.
#
# The scene graph has a sphere and a text 3D object. 
# A selection node is placed at the top of the scene graph. 
# When an object is selected, a selection callback is called
# to change the material color of that object.
#

import sys

from pivy.coin import *
from pivy.sogui import *

# global data
textMaterial, sphereMaterial = [None]*2
reddish = (1.0, 0.2, 0.2)  # Color when selected
white   = (0.8, 0.8, 0.8)  # Color when not selected

# This routine is called when an object gets selected. 
# We determine which object was selected, and change 
# that objects material color.
def mySelectionCB(void, selectionPath):
    if selectionPath.getTail().isOfType(SoText3.getClassTypeId()):
        textMaterial.diffuseColor.setValue(reddish)
    elif selectionPath.getTail().isOfType(SoSphere.getClassTypeId()):
        sphereMaterial.diffuseColor.setValue(reddish)

# This routine is called whenever an object gets deselected. 
# We determine which object was deselected, and reset 
# that objects material color.
def myDeselectionCB(void, deselectionPath):
    if deselectionPath.getTail().isOfType(SoText3.getClassTypeId()):
        textMaterial.diffuseColor = white
    elif deselectionPath.getTail().isOfType(SoSphere.getClassTypeId()):
        sphereMaterial.diffuseColor = white

def main():
    global textMaterial, sphereMaterial
    
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    # Create and set up the selection node
    selectionRoot = SoSelection()
    selectionRoot.policy = SoSelection.SINGLE
    selectionRoot.addSelectionCallback(mySelectionCB)
    selectionRoot.addDeselectionCallback(myDeselectionCB)

    # Create the scene graph
    root = SoSeparator()
    selectionRoot.addChild(root)

    myCamera = SoPerspectiveCamera()
    root.addChild(myCamera)
    root.addChild(SoDirectionalLight())

    # Add a sphere node
    sphereRoot = SoSeparator()
    sphereTransform = SoTransform()
    sphereTransform.translation = (17., 17., 0.)
    sphereTransform.scaleFactor = (8., 8., 8.)
    sphereRoot.addChild(sphereTransform)

    sphereMaterial = SoMaterial()
    sphereMaterial.diffuseColor = (.8, .8, .8)
    sphereRoot.addChild(sphereMaterial)
    sphereRoot.addChild(SoSphere())
    root.addChild(sphereRoot)

    # Add a text node
    textRoot = SoSeparator()
    textTransform = SoTransform()
    textTransform.translation = (0., -1., 0.)
    textRoot.addChild(textTransform)

    textMaterial = SoMaterial()
    textMaterial.diffuseColor = (.8, .8, .8)
    textRoot.addChild(textMaterial)
    textPickStyle = SoPickStyle()
    textPickStyle.style = SoPickStyle.BOUNDING_BOX
    textRoot.addChild(textPickStyle)
    myText = SoText3()
    myText.string = "rhubarb"
    textRoot.addChild(myText)
    root.addChild(textRoot)

    myRenderArea = SoGuiRenderArea(myWindow)
    myRenderArea.setSceneGraph(selectionRoot)
    myRenderArea.setTitle("My Selection Callback")
    myRenderArea.show()

    # Make the camera see the whole scene
    myViewport = myRenderArea.getViewportRegion()
    myCamera.viewAll(root, myViewport, 2.0)

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
