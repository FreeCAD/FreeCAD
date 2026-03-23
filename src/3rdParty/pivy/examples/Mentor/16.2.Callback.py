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
# chapter 16, example 2.
#
# This example builds a render area in a window supplied by
# the application and a Material Editor in its own window.
# It uses callbacks for the component to report new values.
#

from __future__ import print_function
import sys

from pivy.coin import *
from pivy.sogui import *

#  This is called by the Material Editor when a value changes
def myMaterialEditorCB(userData, newMtl):
    # In Pivy no cast is necessary as it gets autocasted for you.
    myMtl = userData

    # Copy all the fields from the new material
    myMtl.copyFieldValues(newMtl)

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])  
    if myWindow == None: sys.exit(1)     

    # Build the render area in the applications main window
    myRenderArea = SoGuiRenderArea(myWindow)
    myRenderArea.setSize(SbVec2s(200, 200))
   
    # Build the Material Editor in its own window
    try:
        myEditor = SoGuiMaterialEditor()
    except:
        print("The SoGuiMaterialEditor node has not been implemented in the " + \
              "SoGui bindings of Coin!")
        sys.exit(1)
   
    # Create a scene graph
    root = SoSeparator()
    myCamera = SoPerspectiveCamera()
    myMaterial = SoMaterial()
   
    myCamera.position = (0.212482, -0.881014, 2.5)
    myCamera.heightAngle = M_PI/4
    root.addChild(myCamera)
    root.addChild(SoDirectionalLight())
    root.addChild(myMaterial)

    # Read the geometry from a file and add to the scene
    myInput = SoInput()
    if not myInput.openFile("dogDish.iv"):
        sys.exit(1)
    geomObject = SoDB.readAll(myInput)
    if geomObject == None:
        sys.exit(1)
    root.addChild(geomObject)

    # Add a callback for when the material changes
    myEditor.addMaterialChangedCallback(myMaterialEditorCB, myMaterial) 

    # Set the scene graph
    myRenderArea.setSceneGraph(root)

    # Show the main window and the Material Editor
    myRenderArea.setTitle("Editor Callback")
    myRenderArea.show()
    SoGui.show(myWindow)
    myEditor.show()

    # Loop forever
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
