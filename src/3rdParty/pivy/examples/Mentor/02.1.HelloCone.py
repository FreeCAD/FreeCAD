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
# chapter 2, example 1.
#
# Hello Cone example program; draws a red cone in a window.
#

import sys

from pivy.sogui import *
from pivy.coin import *

def main():
    # Initialize Inventor. This returns a main window to use.
    # If unsuccessful, exit.

    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    # Make a scene containing a red cone
    root = SoSeparator()
    myCamera = SoPerspectiveCamera()
    myMaterial = SoMaterial()
    root.addChild(myCamera)
    root.addChild(SoDirectionalLight())
    myMaterial.diffuseColor = (1.0, 0.0, 0.0)   # Red
    root.addChild(myMaterial)
    root.addChild(SoCone())

    # Create a renderArea in which to see our scene graph.
    # The render area will appear within the main window.
    myRenderArea = SoGuiRenderArea(myWindow)

    # Make myCamera see everything.
    myCamera.viewAll(root, myRenderArea.getViewportRegion())

    # Put our scene in myRenderArea, change the title
    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("Hello Cone")
    myRenderArea.show()

    SoGui.show(myWindow)  # Display main window
    SoGui.mainLoop()    # Main Inventor event loop

if __name__ == "__main__":
    main()
