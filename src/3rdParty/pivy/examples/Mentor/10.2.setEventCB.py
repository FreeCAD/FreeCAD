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
#  This is an example from The Inventor Mentor
#  chapter 10, example 2.
#
#  This demonstrates using SoGuiRenderArea::setEventCallback().
#  which causes events to be sent directly to the application
#  without being sent into the scene graph.
#  
# Clicking the left mouse button and dragging will draw 
#       points in the xy plane beneath the mouse cursor.
# Clicking middle mouse and holding causes the point set 
#       to rotate about the Y axis. 
# Clicking right mouse clears all points drawn so far out 
#       of the point set.
#

from __future__ import print_function
import sys

from pivy.coin import *
from pivy.gui.soqt import *

# PySide module has to be imported as last one if used in the same namespace
from pivy.qt.QtCore import *
from pivy.qt.QtGui import *

# Timer sensor 
# Rotate 90 degrees every second, update 30 times a second
myTicker = None
UPDATE_RATE    = 1.0/30.0
ROTATION_ANGLE = M_PI/60.0

def myProjectPoint(myRenderArea, mousex, mousey, use_coin=False):
    # Take the x,y position of mouse, and normalize to [0,1].
    # X windows have 0,0 at the upper left,
    # Inventor expects 0,0 to be the lower left.

    size = myRenderArea.getSize()
    if not use_coin:
        mousey = size[1] - mousey

    ratio = float(size[0]) / size[1] - 1
    ratiox = (ratio > 0) * ratio
    ratioy = (ratio < 0) * ratio
    x = (float(mousex) + (float(mousex) - 0.5 * float(size[0])) * ratiox) / size[0]
    y = (float(mousey) + (float(mousey) - 0.5 * float(size[1])) * ratioy) / size[1]
   
    # Get the camera and view volume
    root = myRenderArea.getSceneGraph()
    myCamera = root.getChild(0)
    myViewVolume = myCamera.getViewVolume()
   
    # Project the mouse point to a line
    p0, p1 = myViewVolume.projectPointToLine(SbVec2f(x,y))

    # Midpoint of the line intersects a plane thru the origin
    intersection = (p0 + p1) * 0.5

    return intersection

def myAddPoint(myRenderArea, point):
    root = myRenderArea.getSceneGraph()
    coord = root.getChild(2)
    myPointSet = root.getChild(3)
   
    coord.point.set1Value(coord.point.getNum(), point)
    myPointSet.numPoints = coord.point.getNum()

def myClearPoints(myRenderArea):
    root = myRenderArea.getSceneGraph()
    coord = root.getChild(2)
    myPointSet = root.getChild(3)
   
    # Delete all values starting from 0
    coord.point.deleteValues(0) 
    myPointSet.numPoints = 0

def tickerCallback(myCamera, sensor):
    mtx = SbMatrix()

    # Adjust the position
    pos = myCamera.position.getValue()
    rot = SbRotation(SbVec3f(0,1,0), ROTATION_ANGLE)
    mtx.setRotate(rot)
    pos = mtx.multVecMatrix(pos)
    myCamera.position = pos

    # Adjust the orientation
    myCamera.orientation.setValue(myCamera.orientation.getValue() * rot)
    
###############################################################
# CODE FOR The Inventor Mentor STARTS HERE  (part 1)

def myAppEventHandler(myRenderArea, anyevent):
    handled = TRUE

    if anyevent.type() == QEvent.MouseButtonPress:
        if anyevent.button() == QMouseEvent.LeftButton:
            vec = myProjectPoint(myRenderArea, anyevent.x(), anyevent.y())
            myAddPoint(myRenderArea, vec)
        elif anyevent.button() == QMouseEvent.MidButton:
            myTicker.schedule()  # start spinning the camera
        elif anyevent.button() == QMouseEvent.RightButton:
            myClearPoints(myRenderArea)  # clear the point set

    elif anyevent.type() == QEvent.MouseButtonRelease:
        if anyevent.button() == QMouseEvent.MidButton:
            myTicker.unschedule()  # stop spinning the camera

    elif anyevent.type() == QEvent.MouseMove:
        if anyevent.state() == QMouseEvent.LeftButton:
            vec = myProjectPoint(myRenderArea, anyevent.x(), anyevent.y())
            myAddPoint(myRenderArea, vec)

    else:
        handled = FALSE

    return handled

def myAppEventHandlerQt4(myRenderArea, anyevent):
    handled = TRUE

    if anyevent.type() == QEvent.MouseButtonPress:
        if anyevent.button() == Qt.LeftButton:
            vec = myProjectPoint(myRenderArea, anyevent.x(), anyevent.y())
            myAddPoint(myRenderArea, vec)
        elif anyevent.button() == Qt.MidButton:
            myTicker.schedule()  # start spinning the camera
        elif anyevent.button() == Qt.RightButton:
            myClearPoints(myRenderArea)  # clear the point set

    elif anyevent.type() == QEvent.MouseButtonRelease:
        if anyevent.button() == Qt.MidButton:
            myTicker.unschedule()  # stop spinning the camera

    elif anyevent.type() == QEvent.MouseMove:
        if anyevent.buttons() == Qt.LeftButton:
            vec = myProjectPoint(myRenderArea, anyevent.x(), anyevent.y())
            myAddPoint(myRenderArea, vec)

    else:
        handled = FALSE

    return handled


# CODE FOR The Inventor Mentor ENDS HERE
###############################################################


###############################################################
# CALLBACK WORKAROUND STARTS HERE

DRAW = FALSE
def myAppEventHandlerCoin(myRenderArea, anyevent):
    global DRAW
    handled = TRUE
    event = anyevent.getEvent()
    myRenderArea.draw = False
    if isinstance(event, SoMouseButtonEvent):
        if (event.getState() == SoMouseButtonEvent.DOWN):
            if event.getButton() == event.BUTTON1:
                pos = event.getPosition()
                vec = myProjectPoint(myRenderArea, pos[0], pos[1], use_coin=True)
                myAddPoint(myRenderArea, vec)
                DRAW=TRUE
            elif event.getButton() == event.BUTTON3:
                myTicker.schedule()  # start spinning the camera
            elif event.getButton() == event.BUTTON2:
                myClearPoints(myRenderArea)  # clear the point set

        elif (event.getState() == SoMouseButtonEvent.UP):
            if event.getButton() == event.BUTTON1:
                DRAW = False
            if event.getButton() == event.BUTTON3:
                myTicker.unschedule()  # stop spinning the camera

    elif isinstance(event, SoLocation2Event):
        if DRAW:
            pos = event.getPosition()
            vec = myProjectPoint(myRenderArea, pos[0], pos[1], use_coin=True)
            myAddPoint(myRenderArea, vec)

    else:
        handled = FALSE

    return handled

# CALLBACK WORKAROUND ENDS HERE 
###############################################################

def main():
    global myTicker
    
    # Print out usage instructions
    print("Mouse buttons:")
    print("\tLeft (with mouse motion): adds points")
    print("\tMiddle: rotates points about the Y axis")
    print("\tRight: deletes all the points")

    # Initialize Inventor and Qt
    appWindow = SoQt.init(sys.argv[0])
    if appWindow == None:
        sys.exit(1)

    # Create and set up the root node
    root = SoSeparator()

    # Add a camera
    myCamera = SoPerspectiveCamera()
    root.addChild(myCamera)                 # child 0
   
    # Use the base color light model so we don't need to 
    # specify normals
    myLightModel = SoLightModel()
    myLightModel.model = SoLightModel.BASE_COLOR
    root.addChild(myLightModel)             # child 1
   
    # Set up the camera view volume
    myCamera.position = (0, 0, 4)
    myCamera.nearDistance = 1.0
    myCamera.farDistance = 7.0
    myCamera.heightAngle = M_PI/3.0
   
    # Add a coordinate and point set
    myCoord = SoCoordinate3()
    myPointSet = SoPointSet()
    root.addChild(myCoord)                  # child 2
    root.addChild(myPointSet)               # child 3

    # Timer sensor to tick off time while middle mouse is down
    myTicker = SoTimerSensor(tickerCallback, myCamera)
    myTicker.setInterval(UPDATE_RATE)

    # Create a render area for viewing the scene
    myRenderArea = SoQtRenderArea(appWindow)
    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("My Event Handler")


###############################################################
# as a workaround use a SoEventCallback:
    if len(sys.argv) > 1 and sys.argv[1] == "coin":
        myEventCallback = SoEventCallback()
        root.addChild(myEventCallback)
        myEventCallback.addEventCallback(SoEvent.getClassTypeId(), myAppEventHandlerCoin, myRenderArea)
# end of workaround
###############################################################

###############################################################
# TODO: this does not work with pyside
# it will run once * is solved:
# https://bugreports.qt.io/browse/PYSIDE-31?jql=project%20%3D%20PYSIDE%20AND%20resolution%20%3D%20Unresolved%20ORDER%20BY%20assignee%20ASC%2C%20priority%20DESC
# 
# CODE FOR The Inventor Mentor STARTS HERE  (part 2)
#
#    # Have render area send events to us instead of the scene 
#    # graph.  We pass the render area as user data.
    else:
        if (SoQt.getVersionToolkitString().startswith('4') or
            SoQt.getVersionToolkitString().startswith('5')):
           myRenderArea.setEventCallback(myAppEventHandlerQt4, myRenderArea)
        else:
           myRenderArea.setEventCallback(myAppEventHandler, myRenderArea)
#
# CODE FOR The Inventor Mentor ENDS HERE
###############################################################

    # Show our application window, and loop forever...
    myRenderArea.show()
    SoQt.show(appWindow)
    SoQt.mainLoop()

if __name__ == "__main__":
    main()
