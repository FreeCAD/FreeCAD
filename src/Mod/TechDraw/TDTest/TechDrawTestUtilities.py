import FreeCAD
import os


def createPageWithSVGTemplate(doc=None):
    """Returns a page with an SVGTemplate added on the ActiveDocument"""
    path = os.path.dirname(os.path.abspath(__file__))
    templateFileSpec = path + "/TestTemplate.svg"

    if not doc:
        doc = FreeCAD.ActiveDocument

    page = doc.addObject("TechDraw::DrawPage", "Page")
    doc.addObject("TechDraw::DrawSVGTemplate", "Template")
    doc.Template.Template = templateFileSpec
    doc.Page.Template = doc.Template
    return page
