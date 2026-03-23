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
# chapter 6, example 1.
#
# This example renders a globe and uses 2D text to label the
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
    myFont.size = 24.0
    root.addChild(myFont)

    # Add the globe, a sphere with a texture map.
    # Put it within a separator.
    sphereSep = SoSeparator()
    myTexture2 = SoTexture2()
    sphereComplexity = SoComplexity()
    sphereComplexity.value = 0.55
    root.addChild(sphereSep)
    sphereSep.addChild(myTexture2)
    sphereSep.addChild(sphereComplexity)
    sphereSep.addChild(SoSphere())
    myTexture2.filename = "globe.rgb"

    # Add Text2 for AFRICA, translated to proper location.
    africaSep = SoSeparator()
    africaTranslate = SoTranslation()
    africaText = SoText2()
    africaTranslate.translation = (.25,.0,1.25)
    africaText.string = "AFRICA"
    root.addChild(africaSep)
    africaSep.addChild(africaTranslate)
    africaSep.addChild(africaText)

    # Add Text2 for ASIA, translated to proper location.
    asiaSep = SoSeparator()
    asiaTranslate = SoTranslation()
    asiaText = SoText2()
    asiaTranslate.translation = (.8,.8,0)
    asiaText.string = "ASIA"
    root.addChild(asiaSep)
    asiaSep.addChild(asiaTranslate)
    asiaSep.addChild(asiaText)

    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("2D Text")

    # In Inventor 2.1, if the machine does not have hardware texture
    # mapping, we must override the default drawStyle to display textures.
    myViewer.setDrawStyle(SoGuiViewer.STILL, SoGuiViewer.VIEW_AS_IS)

    myViewer.show()
    myViewer.viewAll()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
