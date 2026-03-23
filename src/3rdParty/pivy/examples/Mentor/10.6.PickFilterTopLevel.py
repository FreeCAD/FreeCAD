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
# chapter 10, example 6.
#
# This example demonstrates the use of the pick filter
# callback to implement a top level selection policy.
# That is, always select the top most group beneath the
# selection node,  rather than selecting the actual
# shape that was picked.
#

from __future__ import print_function
import sys

from pivy.coin import *
from pivy.sogui import *

# Pick the topmost node beneath the selection node
def pickFilterCB(void, pick):
    # See which child of selection got picked
    p = pick.getPath()
    
    for i in range(p.getLength() - 1):
        n = p.getNode(i)
        if n.isOfType(SoSelection.getClassTypeId()):
            break

    # Copy 2 nodes from the path:
    # selection and the picked child
    return p.copy(i, 2)

def main():
    # Initialization
    mainWindow = SoGui.init(sys.argv[0])
    
    # Open the data file
    input = SoInput()
    datafile = "parkbench.iv"
    if not input.openFile(datafile):
        print("Cannot open %s for reading." % (datafile), file=sys.stderr)
        sys.exit(1)

    # Read the input file
    sep = SoSeparator()
    sep.addChild(SoDB.readAll(input))
   
    # Create two selection roots - one will use the pick filter.
    topLevelSel = SoSelection()
    topLevelSel.addChild(sep)
    topLevelSel.setPickFilterCallback(pickFilterCB)

    defaultSel = SoSelection()
    defaultSel.addChild(sep)

    # Create two viewers, one to show the pick filter for top level
    # selection, the other to show default selection.
    viewer1 = SoGuiExaminerViewer(mainWindow)
    viewer1.setSceneGraph(topLevelSel)
    boxhra1 = SoBoxHighlightRenderAction()
    viewer1.setGLRenderAction(boxhra1)
    viewer1.redrawOnSelectionChange(topLevelSel)
    viewer1.setTitle("Top Level Selection")

    viewer2 = SoGuiExaminerViewer()
    viewer2.setSceneGraph(defaultSel)
    boxhra2 = SoBoxHighlightRenderAction()
    viewer2.setGLRenderAction(boxhra2)
    viewer2.redrawOnSelectionChange(defaultSel)
    viewer2.setTitle("Default Selection")

    viewer1.show()
    viewer2.show()
   
    SoGui.show(mainWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
