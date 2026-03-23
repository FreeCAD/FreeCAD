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
# chapter 10, example 7.
#
# This example demonstrates the use of the pick filter
# callback to pick through manipulators.
#
# The scene graph has several objects. Clicking the left
# mouse on an object selects it and adds a manipulator to
# it. Clicking again deselects it and removes the manipulator.
# In this case, the pick filter is needed to deselect the
# object rather than select the manipulator.
#

import sys

from pivy.coin import *
from pivy.sogui import *

# Returns path to xform left of the input path tail.
# Inserts the xform if none found. In this example,
# assume that the xform is always the node preceding
# the selected shape.
def findXform(p):
    # Copy the input path up to tail's parent.
    returnPath = p.copy(0, p.getLength() - 1)

    # Get the parent of the selected shape
    g = p.getNodeFromTail(1)
    tailNodeIndex = p.getIndexFromTail(0)

    # Check if there is already a transform node
    if tailNodeIndex > 0:
        n = g.getChild(tailNodeIndex - 1)
        if n.isOfType(SoTransform.getClassTypeId()):
            # Append to returnPath and return it.
            returnPath.append(n)
            return returnPath

    # Otherwise, add a transform node.
    xf = SoTransform()
    g.insertChild(xf, tailNodeIndex) # right before the tail
    # Append to returnPath and return it.
    returnPath.append(xf)
    return returnPath

# Returns the manip affecting this path. In this example,
# the manip is always preceding the selected shape.
def findManip(p):
    # Copy the input path up to tail's parent.
    returnPath = p.copy(0, p.getLength() - 1)

    # Get the index of the last node in the path.
    tailNodeIndex = p.getIndexFromTail(0)

    # Append the left sibling of the tail to the returnPath
    returnPath.append(tailNodeIndex - 1)
    return returnPath

# Add a manipulator to the transform affecting this path
# The first parameter, userData, is not used.
def selCB(void, path):
    if path.getLength() < 2: return
    
    # Find the transform affecting this object
    xfPath = findXform(path)
    
    # Replace the transform with a manipulator
    manip = SoHandleBoxManip()
    manip.replaceNode(xfPath)

# Remove the manipulator affecting this path.
# The first parameter, userData, is not used.
def deselCB(void, path):
    if path.getLength() < 2: return

    # Find the manipulator affecting this object
    manipPath = findManip(path)

    # Replace the manipulator with a transform 
    manip = manipPath.getTail()
    manip.replaceManip(manipPath, SoTransform())

##############################################################
# CODE FOR The Inventor Mentor STARTS HERE  (part 1)

def pickFilterCB(void, pick):
    filteredPath = None
    
    # See if the picked object is a manipulator. 
    # If so, change the path so it points to the object the manip
    # is attached to.
    p = pick.getPath()
    n = p.getTail()
    if n.isOfType(SoTransformManip.getClassTypeId()):
        # Manip picked! We know the manip is attached
        # to its next sibling. Set up and return that path.
        manipIndex = p.getIndex(p.getLength() - 1)
        filteredPath = p.copy(0, p.getLength() - 1)
        filteredPath.append(manipIndex + 1) # get next sibling
    else:
        filteredPath = p

    return filteredPath

# CODE FOR The Inventor Mentor ENDS HERE  
##############################################################

# Create a sample scene graph
def myText(str, i, color):
    sep  = SoSeparator()
    col  = SoBaseColor()
    xf   = SoTransform()
    text = SoText3()
   
    col.rgb = color
    xf.translation = (6.0 * i, 0.0, 0.0)
    text.string = str
    text.parts = SoText3.FRONT | SoText3.SIDES
    text.justification = SoText3.CENTER
    sep.addChild(col)
    sep.addChild(xf)
    sep.addChild(text)
   
    return sep

def buildScene():
    scene = SoSeparator()
    font  = SoFont()
   
    font.size = 10
    scene.addChild(font)
    scene.addChild(myText("O",  0, SbColor(0, 0, 1)))
    scene.addChild(myText("p",  1, SbColor(0, 1, 0)))
    scene.addChild(myText("e",  2, SbColor(0, 1, 1)))
    scene.addChild(myText("n",  3, SbColor(1, 0, 0)))
    # Open Inventor is two words!
    scene.addChild(myText("I",  5, SbColor(1, 0, 1)))
    scene.addChild(myText("n",  6, SbColor(1, 1, 0)))
    scene.addChild(myText("v",  7, SbColor(1, 1, 1)))
    scene.addChild(myText("e",  8, SbColor(0, 0, 1)))
    scene.addChild(myText("n",  9, SbColor(0, 1, 0)))
    scene.addChild(myText("t", 10, SbColor(0, 1, 1)))
    scene.addChild(myText("o", 11, SbColor(1, 0, 0)))
    scene.addChild(myText("r", 12, SbColor(1, 0, 1)))
   
    return scene

def main():
    # Initialization
    mainWindow = SoGui.init(sys.argv[0])
    
    # Create a scene graph. Use the toggle selection policy.
    sel = SoSelection()
    sel.policy = SoSelection.TOGGLE
    sel.addChild(buildScene())

    # Create a viewer
    viewer = SoGuiExaminerViewer(mainWindow)
    viewer.setSceneGraph(sel)
    viewer.setTitle("Select Through Manips")
    viewer.show()

    # Selection callbacks
    sel.addSelectionCallback(selCB)
    sel.addDeselectionCallback(deselCB)
    sel.setPickFilterCallback(pickFilterCB)
    
    SoGui.show(mainWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
