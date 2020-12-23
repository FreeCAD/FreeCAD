#!/usr/bin/env python
# -*- coding: utf-8 -*-

# move Views from Drawing Page to TechDraw Page in the current document
# usage: select 1 Drawing Page and 1 TechDraw Page, run moveViews macro
# outcome: Content of Drawing Page will be inserted into TechDraw Page as DrawViewSymbol 
#          (ie an SVG symbol)

import FreeCAD
import FreeCADGui
import Part
import Drawing
import TechDraw

svgHead = "<svg\n" + " xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"\n" + "    xmlns:freecad=\"http://www.freecadweb.org/wiki/index.php?title=Svg_Namespace\">\n"
svgTail = "\n</svg>"

def moveViews():
    s = FreeCADGui.Selection.getSelection()

    if len(s) != 2:
        print ("Please select 1 Drawing Page and 1 TechDraw Page")
        return

    print ("First object in selection is a: ", s[0].TypeId)
    print ("Second object in selection is a: ", s[1].TypeId)
    if s[0].isDerivedFrom("Drawing::FeaturePage")  and \
       s[1].isDerivedFrom("TechDraw::DrawPage"):
       dPage = s[0]
       tPage = s[1]
    elif s[0].isDerivedFrom("TechDraw::DrawPage") and \
        s[1].isDerivedFrom("Drawing::FeaturePage"):
        tPage = s[0]
        dPage = s[1]
    else:
        print ("Please select 1 Drawing Page and 1 TechDraw Page")
        return

    i = 1
    for o in dPage.OutList:
        newName = "DraftView" + str(i).zfill(3)
        print ("moving " + o.Name + " to " + newName)
        svg = svgHead + o.ViewResult + svgTail
        no = FreeCAD.ActiveDocument.addObject('TechDraw::DrawViewSymbol',newName)
        no.Symbol = svg
        tPage.addView(no)
        i += 1

    print ("moveViews moved " + str(i-1) + " views")

if __name__ == '__main__':
    moveViews()
