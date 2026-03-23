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
# chapter 16, example 4.
#
# This example builds a render area and Material Editor within 
# a window supplied by the application. It uses a Motif form 
# widget to lay both components inside the same window.  
# It attaches the editor to the material of an object.
#

from __future__ import print_function
import sys

from pivy.coin import *
from pivy.sogui import *

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
   
    # Build the form to hold both components
    myForm = QtCreateWidget("Form", xmFormWidgetClass, myWindow, None, 0)
   
    # Build the render area and Material Editor
    myRenderArea = SoGuiRenderArea(myForm)
    myRenderArea.setSize(SbVec2s(200, 200))
    myEditor = SoGuiMaterialEditor(myForm)
   
    # Layout the components within the form
    args = []
    QtSetArg(args[0], XmNtopAttachment,    XmATTACH_FORM)
    QtSetArg(args[1], XmNbottomAttachment, XmATTACH_FORM)
    QtSetArg(args[2], XmNleftAttachment,   XmATTACH_FORM) 
    QtSetArg(args[3], XmNrightAttachment,  XmATTACH_POSITION)
    QtSetArg(args[4], XmNrightPosition,    40)
    QtSetValues(myRenderArea.getWidget(), args, 5)
    QtSetArg(args[2], XmNrightAttachment,  XmATTACH_FORM) 
    QtSetArg(args[3], XmNleftAttachment,   XmATTACH_POSITION)
    QtSetArg(args[4], XmNleftPosition,     41) 
    QtSetValues(myEditor.getWidget(), args, 5)
    
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
   
    # Make the scene graph visible
    myRenderArea.setSceneGraph(root)
   
    # Attach the material editor to the material in the scene
    myEditor.attach(myMaterial)
   
    # Show the main window
    myRenderArea.show()
    myEditor.show()
    SoGui.show(myForm)    # this calls QtManageChild
    SoGui.show(myWindow)  # this calls QtRealizeWidget
   
    # Loop forever
    SoGui.mainLoop()

if __name__ == "__main__":
    print("This example is not functional as it is GUI toolkit dependent!")
    sys.exit(1)
    main()
