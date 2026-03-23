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
# This is an example from The Inventor Mentor,
# chapter 17, example 3.
#
# This example draws the same scene as Example 17.2, 
# but using a GLX window.
#

from __future__ import print_function
import sys, time

from OpenGL.GL import *

from pivy.coin import *
from pivy.sogui import *

WINWIDTH  = 400 
WINHEIGHT = 400 

floorObj = []

# Build a Inventor scene with two objects and some light
def buildScene(root):
    # Some light
    root.addChild(SoLightModel())
    root.addChild(SoDirectionalLight())

    # A red cube translated to the left and down
    myTrans = SoTransform()
    myTrans.translation = (-2.0, -2.0, 0.0)
    root.addChild(myTrans)

    myMtl = SoMaterial()
    myMtl.diffuseColor = (1.0, 0.0, 0.0)
    root.addChild(myMtl)
   
    root.addChild(SoCube())

    # A blue sphere translated right
    myTrans = SoTransform()
    myTrans.translation = (4.0, 0.0, 0.0)
    root.addChild(myTrans)

    myMtl = SoMaterial()
    myMtl.diffuseColor = (0.0, 0.0, 1.0)
    root.addChild(myMtl)
   
    root.addChild(SoSphere())


# Build the floor that will be rendered using OpenGL.
def buildFloor():
    global floorObj
    a = 0

    for i in range(9):
        for j in range(9):
            floorObj.append([-5.0 + j*1.25, 0.0, -5.0 + i*1.25])
            a+=1

# Callback used by GLX window
def waitForNotify(Display, e, arg):
    return e.type == MapNotify and e.xmap.window == arg

# Create and initialize GLX window.
def openWindow():
    attributeList = (GLX_RGBA,
                     GLX_RED_SIZE, 1,
                     GLX_GREEN_SIZE, 1,
                     GLX_BLUE_SIZE, 1,
                     GLX_DEPTH_SIZE, 1,
                     GLX_DOUBLEBUFFER,  
                     None)

    # Open the X display 
    display = XOpenDisplay(0)

    # Initialize the GLX visual and context
    vi = glXChooseVisual(display, DefaultScreen(display), attributeList)
    cx = glXCreateContext(display, vi, 0, GL_TRUE)

    # Create the X color map
    cmap = XCreateColormap(display, RootWindow(display, vi.screen), vi.visual, AllocNone)

    # Create and map the X window
    swa.colormap = cmap
    swa.border_pixel = 0
    swa.event_mask = StructureNotifyMask
    window = XCreateWindow(display, RootWindow(display, vi.screen), 100, 100, WINWIDTH,            
                           WINHEIGHT, 0, vi.depth, InputOutput, vi.visual, 
                           (CWBorderPixel | CWColormap | CWEventMask), swa)
    XMapWindow(display, window)
    XIfEvent(display, event, waitForNotify, window)

    # Attach the GLX context to the window
    glXMakeCurrent(display, window, cx)

    return (display, window)


# Draw the lines that make up the floor, using OpenGL
def drawFloor():
    global floorObj

    glBegin(GL_LINES)
    for i in range(4):
        glVertex3fv(floorObj[i*18])
        glVertex3fv(floorObj[(i*18)+8])
        glVertex3fv(floorObj[(i*18)+17])
        glVertex3fv(floorObj[(i*18)+9])

    i+=1
    glVertex3fv(floorObj[i*18])
    glVertex3fv(floorObj[(i*18)+8])
    glEnd()

    glBegin(GL_LINES)
    for i in range(4):
        glVertex3fv(floorObj[i*2])
        glVertex3fv(floorObj[(i*2)+72])
        glVertex3fv(floorObj[(i*2)+73])
        glVertex3fv(floorObj[(i*2)+1])

    i+=1
    glVertex3fv(floorObj[i*2])
    glVertex3fv(floorObj[(i*2)+72])
    glEnd()

def main():
    # Initialize Inventor
    # SoDB.init() invoked automatically upon coin module import

    # Build a simple scene graph
    root = SoSeparator()
    buildScene(root)

    # Build the floor geometry
    buildFloor()

    # Create and initialize window
    display, window = openWindow()
    glEnable(GL_DEPTH_TEST)
    glClearColor(0.8, 0.8, 0.8, 1.0)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    # Set up the camera using OpenGL.
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    gluPerspective(90.0, 1.0, 2.0, 12.0)

    glMatrixMode(GL_MODELVIEW)
    glLoadIdentity()
    glTranslatef(0.0, 0.0, -5.0)

    # Render the floor using OpenGL
    glPushMatrix()
    glTranslatef(0.0, -3.0, 0.0)
    glColor3f(0.7, 0.0, 0.0)
    glLineWidth(2.0)
    glDisable(GL_LIGHTING)
    drawFloor()
    glEnable(GL_LIGHTING)
    glPopMatrix()

    # Render the scene
    myViewport(WINWIDTH, WINHEIGHT)
    myRenderAction = SoGLRenderAction(myViewport)
    
    myRenderAction.apply(root)
    glXSwapBuffers(display, window) 
   
    # With inventor 2.1, it's necessary to reset the lazy element
    # any time GL calls are made outside of inventor.  In this example,
    # between the first and second rendering, the inventor state must
    # have both diffuse color and light model reset, since these are
    # modified by the GLX rendering code.  For more information about
    # the lazy element, see the publication,
    # "Open Inventor 2.1 Porting and Performance Tips"

    # To reset the lazy element, first we obtain the state
    # from the action, then obtain the lazy element from the state, 
    # and finally apply a reset to that lazy element.

    state = myRenderAction.getState()
    lazyElt = SoLazyElement_getInstance(state)
    lazyElt.reset(state, (SoLazyElement.DIFFUSE_MASK)|(SoLazyElement.LIGHT_MODEL_MASK))

    time.sleep(5)
   
    # Rerender the floor using OpenGL again:
    glClearColor(0.8, 0.8, 0.8, 1.0)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    
    glPushMatrix()
    glTranslatef(0.0, -3.0, 0.0)
    glColor3f(0.0, 0.7, 0.0)
    glLineWidth(2.0)
    glDisable(GL_LIGHTING)
    drawFloor()
    glEnable(GL_LIGHTING)
    glPopMatrix()
   
    # Redraw the rest of the scene: 
    myRenderAction.apply(root)
    glXSwapBuffers(display, window)

    time.sleep(10) 
    return 0

if __name__ == "__main__":
    print("This example is not functional as it is GLX dependent!")
    sys.exit(1)
    main()
