#!/usr/bin/env python
# -*- coding: utf-8 -*-
# **************************************************************************
#   Copyright (c) 2018 WandererFan <wandererfan@gmail.com>                *
#                                                                         *
#   This file is part of the FreeCAD CAx development system.              *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU Lesser General Public License (LGPL)    *
#   as published by the Free Software Foundation; either version 2 of     *
#   the License, or (at your option) any later version.                   *
#   for detail see the LICENCE text file.                                 *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful,            *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU Library General Public License for more details.                  *
#                                                                         *
#   You should have received a copy of the GNU Library General Public     *
#   License along with FreeCAD; if not, write to the Free Software        *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#   USA                                                                   *
# **************************************************************************

# Name:    moveView macro
# About:   move Views from Drawing Page to TechDraw Page in the current doc
# Usage:   select 1 Drawing Page and 1 TechDraw Page, run moveViews macro
# Outcome: Content of Drawing Page will be inserted into TechDraw Page as
#          DrawViewSymbol (ie an SVG symbol)

import FreeCAD
import FreeCADGui

svgHead = (
    "<svg\n"
    + ' xmlns="http://www.w3.org/2000/svg" version="1.1"\n'
    + '    xmlns:freecad="http://www.freecad.org/wiki/index.php?title=Svg_Namespace">\n'
)
svgTail = "\n</svg>"


def moveViews():
    s = FreeCADGui.Selection.getSelection()

    if len(s) != 2:
        print("Please select 1 Drawing Page and 1 TechDraw Page")
        return

    print("First object in selection is a: ", s[0].TypeId)
    print("Second object in selection is a: ", s[1].TypeId)
    if s[0].isDerivedFrom("Drawing::FeaturePage") and s[1].isDerivedFrom(
        "TechDraw::DrawPage"
    ):
        dPage = s[0]
        tPage = s[1]
    elif s[0].isDerivedFrom("TechDraw::DrawPage") and s[1].isDerivedFrom(
        "Drawing::FeaturePage"
    ):
        tPage = s[0]
        dPage = s[1]
    else:
        print("Please select 1 Drawing Page and 1 TechDraw Page")
        return

    for o in dPage.OutList:
        newName = "DraftView" + str(i).zfill(3)
        print("moving " + o.Name + " to " + newName)
        svg = svgHead + o.ViewResult + svgTail
        no = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewSymbol", newName)
        no.Symbol = svg
        tPage.addView(no)

    print("moveViews moved " + str(len(dPage.OutList)) + " views")


if __name__ == "__main__":
    moveViews()
