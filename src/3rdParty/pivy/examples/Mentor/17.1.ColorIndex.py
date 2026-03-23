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
# This is an example from The Inventor Mentor
# chapter 17, example 1.
#
# This examples shows how the user can create a custom
# X visual for doing color index rendering with
# an Inventor Viewer. This shows how to create the right
# visual, as well as load the color map with the wanted
# colors.
#

from __future__ import print_function
import sys

from OpenGL.GLUT import *
from OpenGL.GL import *
from OpenGL.GLX import *

from pivy.coin import *
from pivy.sogui import *

# window attribute list to create a color index visual.
# This will create a double buffered color index window
# with the maximum number of bits and a zbuffer.
GLX_DOUBLEBUFFER=5
GLX_BUFFER_SIZE=2
GLX_DEPTH_SIZE=12
attribList = (GLX_DOUBLEBUFFER, 
              GLX_BUFFER_SIZE, 1, 
              GLX_DEPTH_SIZE, 1, 
              None)

# list of colors to load in the color map
colors = ((.2, .2, .2), (.5, 1, .5), (.5, .5, 1))

sceneBuffer = """#Inventor V2.0 ascii

Separator {
   LightModel { model BASE_COLOR }
   ColorIndex { index 1 }
   Coordinate3 { point [ -1 -1 -1, -1 1 -1, 1 1 1, 1 -1 1] }
   FaceSet {}
   ColorIndex { index 2 }
   Coordinate3 { point [ -1 -1 1, -1 1 1, 1 1 -1, 1 -1 -1] }
   FaceSet {}
}"""

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
   
    # read the scene graph in
    input = SoInput()
    input.setBuffer(sceneBuffer)
    scene = SoDB.readAll(input) 
    if not scene:
        print("Couldn't read scene")
        sys.exit(1)

    # create the color index visual
    vis = glXChooseVisual(QtDisplay(myWindow), 
                          XScreenNumberOfScreen(QtScreen(myWindow)),
                          attribList)
    if not vis:
        print("Couldn't create visual")
        sys.exit(1)
   
    # allocate the viewer, set the scene, the visual and
    # load the color map with the wanted colors.
    #
    # Color 0 will be used for the background (default) while
    # color 1 and 2 are used by the objects.
    #
    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setNormalVisual(vis)
    myViewer.setColorMap(0, 3, colors)
    myViewer.setSceneGraph(scene)
    myViewer.setTitle("Color Index Mode")
   
    # Show the viewer and loop forever...
    myViewer.show()
    XtRealizeWidget(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    print("This example is not functional as it is GLX dependent!")
    sys.exit(1)
    main()
