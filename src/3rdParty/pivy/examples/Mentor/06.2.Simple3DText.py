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
# chapter 6, example 2.
#
# This example renders a globe and uses 3D text to label the
# continents Africa and Asia.
#

import sys

from pivy.coin import *
from pivy.sogui import *

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    root = SoGroup()

    # Choose a font
    myFont = SoFont()
    myFont.name = "Times-Roman"
    myFont.size = .2
    root.addChild(myFont)

    # We'll color the front of the text white, and the sides 
    # dark grey. So use a materialBinding of PER_PART and
    # two diffuseColor values in the material node.
    myMaterial = SoMaterial()
    myBinding = SoMaterialBinding()
    myMaterial.diffuseColor.set1Value(0, SbColor(1,1,1))
    myMaterial.diffuseColor.set1Value(1, SbColor(.1,.1,.1))
    myBinding.value = SoMaterialBinding.PER_PART
    root.addChild(myMaterial)
    root.addChild(myBinding)

    # Create the globe
    sphereSep = SoSeparator()
    myTexture2 = SoTexture2()
    sphereComplexity = SoComplexity()
    sphereComplexity.value = 0.55
    root.addChild(sphereSep)
    sphereSep.addChild(myTexture2)
    sphereSep.addChild(sphereComplexity)
    sphereSep.addChild(SoSphere())
    myTexture2.filename = "globe.rgb"

    # Add Text3 for AFRICA, transformed to proper location.
    africaSep = SoSeparator()
    africaTransform = SoTransform()
    africaText = SoText3()
    africaTransform.rotation.setValue(SbVec3f(0,1,0), .4)
    africaTransform.translation = (.25, .0, 1.25)
    africaText.parts = SoText3.ALL
    africaText.string = "AFRICA"
    root.addChild(africaSep)
    africaSep.addChild(africaTransform)
    africaSep.addChild(africaText)

    # Add Text3 for ASIA, transformed to proper location.
    asiaSep = SoSeparator()
    asiaTransform = SoTransform()
    asiaText = SoText3()
    asiaTransform.rotation.setValue(SbVec3f(0,1,0), 1.5)
    asiaTransform.translation = (.8, .6, .5)
    asiaText.parts = SoText3.ALL
    asiaText.string = "ASIA"
    root.addChild(asiaSep)
    asiaSep.addChild(asiaTransform)
    asiaSep.addChild(asiaText)

    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("3D Text")

    # In Inventor 2.1, if the machine does not have hardware texture
    # mapping, we must override the default drawStyle to display textures.
    myViewer.setDrawStyle(SoGuiViewer.STILL, SoGuiViewer.VIEW_AS_IS)

    myViewer.show()
    myViewer.viewAll()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
