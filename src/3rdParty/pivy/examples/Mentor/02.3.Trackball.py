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
# chapter 2, example 3.
#
# Use the trackball manipulator to edit/rotate a red cone
#

import sys

from pivy.coin import *
from pivy.sogui import *

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    root = SoSeparator()

    myCamera = SoPerspectiveCamera()
    root.addChild(myCamera)             # child 0
    root.addChild(SoDirectionalLight()) # child 1
    root.addChild(SoTrackballManip())   # child 2

    myMaterial = SoMaterial()
    myMaterial.diffuseColor = (1.0, 0.0, 0.0)
    root.addChild(myMaterial)
    root.addChild(SoCone())

    myRenderArea = SoGuiRenderArea(myWindow)
    myCamera.viewAll(root, myRenderArea.getViewportRegion())
    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("Trackball")
    myRenderArea.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
