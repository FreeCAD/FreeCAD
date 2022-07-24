from __future__ import print_function
import FreeCAD
import os


def createPageWithSVGTemplate(
    templateName: str = "TestTemplate.svg",
):
    """Returns a page with an SVGTemplate added on the ActiveDocument"""
    print("2")
    path = os.path.dirname(os.path.abspath(__file__))
    print("TDTestAnno path: " + path)
    templateFileSpec = path + templateName

    page = FreeCAD.ActiveDocument.addObject("TechDraw::DrawPage", "Page")
    FreeCAD.ActiveDocument.addObject("TechDraw::DrawSVGTemplate", "Template")
    FreeCAD.ActiveDocument.Template.Template = templateFileSpec
    FreeCAD.ActiveDocument.Page.Template = FreeCAD.ActiveDocument.Template
    return page
