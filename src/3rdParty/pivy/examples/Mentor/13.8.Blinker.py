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
# chapter 13, example 9.
#
# Blinker node.
# Use a blinker node to flash a neon ad sign on and off
#

import sys

from pivy.coin import *
from pivy.sogui import *

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])  
    if myWindow == None: sys.exit(1)     

    # Set up camera and light
    root = SoSeparator()
    myCamera = SoPerspectiveCamera()
    root.addChild(myCamera)
    root.addChild(SoDirectionalLight())

    # Read in the parts of the sign from a file
    myInput = SoInput()
    if not myInput.openFile("eatAtJosies.iv"):
        sys.exit(1)
    fileContents = SoDB.readAll(myInput)
    if fileContents == None:
        sys.exit(1)

    eatAt = SoNode.getByName("EatAt")
    josie = SoNode.getByName("Josies")
    frame = SoNode.getByName("Frame")

#############################################################
# CODE FOR The Inventor Mentor STARTS HERE

    # Add the non-blinking part of the sign to the root
    root.addChild(eatAt)
   
    # Add the fast-blinking part to a blinker node
    fastBlinker = SoBlinker()
    root.addChild(fastBlinker)
    fastBlinker.speed = 2  # blinks 2 times a second
    fastBlinker.addChild(josie)

    # Add the slow-blinking part to another blinker node
    slowBlinker = SoBlinker()
    root.addChild(slowBlinker)
    slowBlinker.speed = 0.5  # 2 secs per cycle 1 on, 1 off
    slowBlinker.addChild(frame)

# CODE FOR The Inventor Mentor ENDS HERE
#############################################################

    # Set up and display render area 
    myRenderArea = SoGuiRenderArea(myWindow)
    myRegion = SbViewportRegion(myRenderArea.getSize()) 
    myCamera.viewAll(root, myRegion)

    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("Neon")
    myRenderArea.show()
    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
