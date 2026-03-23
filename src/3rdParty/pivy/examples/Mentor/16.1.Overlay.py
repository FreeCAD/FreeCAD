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
# chapter 16, example 1.
#
# This example shows how to use the overlay planes with the
# viewer components. By default color 0 is used for the
# overlay planes background color (clear color), so we use
# color 1 for the object. This example also shows how to
# load the overlay color map with the wanted color.
#

from __future__ import print_function
import sys

from pivy.coin import *
from pivy.sogui import *

overlayScene = """
#Inventor V2.0 ascii

Separator {
   OrthographicCamera {
      position 0 0 5
      nearDistance 1.0
      farDistance 10.0
      height 1
   }
   LightModel { model BASE_COLOR }
   ColorIndex { index 1 }
   Coordinate3 { point [ -1 -1 0, -1 1 0, 1 1 0, 1 -1 0] }
   FaceSet {}
}"""

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])  
    if myWindow == None: sys.exit(1)     

    # read the scene graph in
    input = SoInput()
    input.setBuffer(overlayScene)
    scene = SoDB.readAll(input)
    if scene == None:
        print("Couldn't read scene")
        sys.exit(1)

    # Allocate the viewer, set the overlay scene and
    # load the overlay color map with the wanted color.
    color = SbColor(.5, 1, .5)
    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(SoCone())
    myViewer.setOverlaySceneGraph(scene)
    myViewer.setOverlayColorMap(1, 1, color)
    myViewer.setTitle("Overlay Plane")
   
    # Show the viewer and loop forever
    myViewer.show()
    # QtRealizeWidget(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
