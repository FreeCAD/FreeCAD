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

    bitmap_x = ("xxxx             xxxx" +
                "xxxxx           xxxxx" +
                "xxxxxx         xxxxxx" +
                "xxxxxxx       xxxxxxx" +
                " xxxxxxx     xxxxxxx " +
                "  xxxxxxx   xxxxxxx  " +
                "   xxxxxxx xxxxxxx   " +
                "    xxxxxxxxxxxxx    " +
                "     xxxxxxxxxxx     " +
                "      xxxxxxxxx      " +
                "       xxxxxxx       " +
                "      xxxxxxxxx      " +
                "     xxxxxxxxxxx     " +
                "    xxxxxxxxxxxxx    " +
                "   xxxxxxx xxxxxxx   " +
                "  xxxxxxx   xxxxxxx  " +
                " xxxxxxx     xxxxxxx " +
                "xxxxxxx       xxxxxxx" +
                "xxxxxx         xxxxxx" +
                "xxxxx           xxxxx" +
                "xxxx             xxxx")

    SoMarkerSet.CUSTOM_BIT_MAP_1 = SoMarkerSet.getNumDefinedMarkers()
    SoMarkerSet.addMarker(SoMarkerSet.CUSTOM_BIT_MAP_1, SbVec2s([21,21]), bitmap_x,
                          False, False)

    bitmap_dot = ("    xxxxxx    " +
                  "  xxxxxxxxxx  " +
                  " xxxxxxxxxxxx " +
                  " xxxxxxxxxxxx " +
                  "xxxxxxxxxxxxxx" +
                  "xxxxxxxxxxxxxx" +
                  "xxxxxxxxxxxxxx" +
                  "xxxxxxxxxxxxxx" +
                  "xxxxxxxxxxxxxx" +
                  "xxxxxxxxxxxxxx" +
                  " xxxxxxxxxxxx " +
                  " xxxxxxxxxxxx " +
                  "  xxxxxxxxxx  " +
                  "    xxxxxx    ")
    
    SoMarkerSet.CUSTOM_BIT_MAP_2 = SoMarkerSet.getNumDefinedMarkers()
    SoMarkerSet.addMarker(SoMarkerSet.CUSTOM_BIT_MAP_2, SbVec2s([14, 14]), bitmap_dot,
                          False, False)

    color = SoMaterial()
    color.diffuseColor = (1., 0., 0.)

    marker1 = SoMarkerSet()
    marker1.markerIndex = SoMarkerSet.CUSTOM_BIT_MAP_1
    data1 = SoCoordinate3()
    data1.point.setValue(0, 0, 0)
    data1.point.setValues(0, 1, [[0., 0., 0.]])

    marker2 = SoMarkerSet()
    marker2.markerIndex = SoMarkerSet.CUSTOM_BIT_MAP_2
    data2 = SoCoordinate3()
    data2.point.setValue(0, 0, 0)
    data2.point.setValues(0, 1, [[1., 0., 0.]])

    myCamera = SoPerspectiveCamera()
    root.addChild(myCamera)
    root.addChild(SoDirectionalLight())
    root.addChild(color)
    root.addChild(data1)
    root.addChild(marker1)
    root.addChild(data2)
    root.addChild(marker2)

    myRenderArea = SoGuiRenderArea(myWindow)

    myCamera.viewAll(root, myRenderArea.getViewportRegion())

    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("Hello Cone")
    myRenderArea.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()