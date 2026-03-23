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
# chapter 4, example 1.
#
# Camera example.  
# A blinker node is used to switch between three 
# different views of the same scene. The cameras are 
# switched once per second.
#

import sys

from pivy.coin import *
from pivy.sogui import *

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    root = SoSeparator()

    # Create a blinker node and put it in the scene. A blinker
    # switches between its children at timed intervals.
    myBlinker = SoBlinker()
    root.addChild(myBlinker)

    # Create three cameras. Their positions will be set later.
    # This is because the viewAll method depends on the size
    # of the render area, which has not been created yet.
    orthoViewAll = SoOrthographicCamera()
    perspViewAll = SoPerspectiveCamera()
    perspOffCenter = SoPerspectiveCamera()
    myBlinker.addChild(orthoViewAll)
    myBlinker.addChild(perspViewAll)
    myBlinker.addChild(perspOffCenter)

    # Create a light
    root.addChild(SoDirectionalLight())

    # Read the object from a file and add to the scene
    myInput = SoInput()
    if not myInput.openFile("parkbench.iv"):
        sys.exit(1)

    fileContents = SoDB.readAll(myInput)
    if fileContents == None:
        sys.exit(1)

    myMaterial = SoMaterial()
    myMaterial.diffuseColor = (0.8, 0.23, 0.03) 
    root.addChild(myMaterial)
    root.addChild(fileContents)

    myRenderArea = SoGuiRenderArea(myWindow)

    # Establish camera positions. 
    # First do a viewAll on all three cameras.  
    # Then modify the position of the off-center camera.
    myRegion = SbViewportRegion(myRenderArea.getSize())
    orthoViewAll.viewAll(root, myRegion)
    perspViewAll.viewAll(root, myRegion)
    perspOffCenter.viewAll(root, myRegion)
    initialPos = perspOffCenter.position.getValue()
    x,y,z = initialPos.getValue()
    perspOffCenter.position = (x+x/2., y+y/2., z+z/4.)

    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("Cameras")
    myRenderArea.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
