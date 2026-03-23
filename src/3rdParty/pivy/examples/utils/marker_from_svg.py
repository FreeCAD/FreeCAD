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


import sys

from pivy.sogui import *
from pivy import coin, utils

def main():
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    # add a new marker type:
    utils.add_marker_from_svg("test.svg", "CUSTOM_MARKER",  30)

    root = coin.SoSeparator()
    color = coin.SoMaterial()
    color.diffuseColor = (1., 0., 0.)

    marker = coin.SoMarkerSet()
    marker.markerIndex = coin.SoMarkerSet.CUSTOM_MARKER
    data = coin.SoCoordinate3()
    data.point.setValue(0, 0, 0)
    data.point.setValues(0, 1, [[0., 0., 0.]])

    myCamera = coin.SoPerspectiveCamera()
    root.addChild(myCamera)
    root.addChild(coin.SoDirectionalLight())
    root.addChild(color)
    root.addChild(data)
    root.addChild(marker)
    root.addChild(data)
    root.addChild(marker)

    myRenderArea = SoGuiRenderArea(myWindow)

    myCamera.viewAll(root, myRenderArea.getViewportRegion())

    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("Hello Cone")
    myRenderArea.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()