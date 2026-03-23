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
# chapter 10, example 8.
#
# This example demonstrates the use of the pick filter
# callback to always select nodekits. This makes it especially
# easy to edit attributes of objects because the nodekit takes
# care of the part creation details.
#

from __future__ import print_function
import math, sys

from pivy.coin import *
from pivy.sogui import *

class UserData:
    sel = None
    editor = None
    ignore = None

##############################################################
## CODE FOR The Inventor Mentor STARTS HERE  (part 1)

# Truncate the pick path so a nodekit is selected
def pickFilterCB(void, pick):
    # See which child of selection got picked
    p = pick.getPath()

    for i in range(p.getLength() - 1, -1, -1):
        n = p.getNode(i)
        if n.isOfType(SoShapeKit.getClassTypeId()):
            break

    # Copy the path down to the nodekit
    return p.copy(0, i+1)

# CODE FOR The Inventor Mentor ENDS HERE
##############################################################


# Create a sample scene graph
def buildScene():
    g = SoGroup()
    
    # Place a dozen shapes in circular formation
    for i in range(12):
        k = SoShapeKit()
        k.setPart("shape", SoCube())
        xf = k.getPart("transform", TRUE)
        xf.translation = (8*math.sin(i*M_PI/6), 8*math.cos(i*M_PI/6), 0.0)
        g.addChild(k)

    return g

# Update the material editor to reflect the selected object
def selectCB(userData, path):
    kit = path.getTail()
    kitMtl = kit.getPart("material", TRUE)

    # ud = userData
    userData.ignore = TRUE
    userData.editor.setMaterial(kitMtl)
    userData.ignore = FALSE

# This is called when the user chooses a new material
# in the material editor. This updates the material
# part of each selected node kit.
def mtlChangeCB(userData, mtl):
    # Our material change callback is invoked when the
    # user changes the material, and when we change it
    # through a call to SoGuiMaterialEditor.setMaterial.
    # In this latter case, we ignore the callback invocation
    # ud = userData

    if userData.ignore:
        return

    sel = userData.sel
    
    # Our pick filter guarantees the path tail will
    # be a shape kit.
    for i in range(sel.getNumSelected()):
        p = sel.getPath(i)
        kit = p.getTail()
        kitMtl = kit.getPart("material", TRUE)
        kitMtl.copyFieldValues(mtl)

def main():
    # Initialization
    mainWindow = SoGui.init(sys.argv[0])
    
    # Create our scene graph.
    sel = SoSelection()
    sel.addChild(buildScene())

    # Create a viewer with a render action that displays highlights
    viewer = SoGuiExaminerViewer(mainWindow)
    viewer.setSceneGraph(sel)
    boxhra = SoBoxHighlightRenderAction()
    viewer.setGLRenderAction(boxhra)
    viewer.redrawOnSelectionChange(sel)
    viewer.setTitle("Select Node Kits")
    viewer.show()

    # Create a material editor
    try:
        ed = SoGuiMaterialEditor()
    except:
        print("The SoGuiMaterialEditor node has not been implemented in the " + \
              "SoGui bindings of Coin!")
        sys.exit(1)
    ed.show()

    # User data for our callbacks
    userData = UserData()
    userData.sel = sel
    userData.editor = ed
    userData.ignore = FALSE
   
    # Selection and material change callbacks
    ed.addMaterialChangedCallback(mtlChangeCB, userData)
    sel.setPickFilterCallback(pickFilterCB)
    sel.addSelectionCallback(selectCB, userData)
   
    SoGui.show(mainWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
