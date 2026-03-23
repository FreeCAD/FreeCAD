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
# chapter 17, example 2.
#
# Example of combining Inventor and OpenGL rendering.
# Create an Inventor render area and draw a red cube 
# and a blue sphere.  Render the floor with OpenGL 
# through a Callback node.
#

import sys

from OpenGL.GL import *

from pivy.coin import *
from pivy.sogui import *

floorObj = []

# Build a scene with two objects and some light
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

# Callback routine to render the floor using OpenGL
def myCallbackRoutine(void, action):
    # only render the floor during GLRender actions:
    if not action.isOfType(SoGLRenderAction.getClassTypeId()): return
   
    glPushMatrix()
    glTranslatef(0.0, -3.0, 0.0)
    glColor3f(0.0, 0.7, 0.0)
    glLineWidth(2)
    glDisable(GL_LIGHTING)  # so we don't have to set normals
    drawFloor()
    glEnable(GL_LIGHTING)   
    glLineWidth(1)
    glPopMatrix()
   
    # With Inventor 2.1, it's necessary to reset SoGLLazyElement after
    # making calls (such as glColor3f()) that affect material state.
    # In this case, the diffuse color and light model are being modified,
    # so the logical-or of DIFFUSE_MASK and LIGHT_MODEL_MASK is passed 
    # to SoGLLazyElement::reset().  
    # Additional information can be found in the publication
    # "Open Inventor 2.1 Porting and Performance Tips"
  
    # state = action.getState()
    # lazyElt = SoLazyElement.getInstance(state)
    # lazyElt.reset(state, (SoLazyElement.DIFFUSE_MASK)|(SoLazyElement.LIGHT_MODEL_MASK))

def main():
    # Initialize Inventor utilities
    myWindow = SoGui.init("Example 17.1")

    buildFloor()

    # Build a simple scene graph, including a camera and
    # a SoCallback node for performing some GL rendering.
    root = SoSeparator()

    myCamera = SoPerspectiveCamera()
    myCamera.position = (0.0, 0.0, 5.0)
    myCamera.heightAngle = M_PI/2.0  # 90 degrees
    myCamera.nearDistance = 2.0
    myCamera.farDistance = 12.0
    root.addChild(myCamera)

    myCallback = SoCallback()
    myCallback.setCallback(myCallbackRoutine)
    root.addChild(myCallback)

    buildScene(root)
   
    # Initialize an Inventor Qt RenderArea and draw the scene.
    myRenderArea = SoGuiRenderArea(myWindow)
    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("OpenGL Callback")
    myRenderArea.setBackgroundColor(SbColor(.8, .8, .8))
    myRenderArea.show()
    drawFloor()
    
    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
