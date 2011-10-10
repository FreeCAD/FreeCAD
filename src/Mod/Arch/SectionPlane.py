import FreeCAD,FreeCADGui,Part,Component
from FreeCAD import Vector
from PyQt4 import QtCore
from pivy import coin

class CommandSectionPlane:
    "the Arch SectionPlane command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_SectionPlane',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_SectionPlane","Section Plane"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_SectionPlane","Adds a section plane object to the document")}

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Section Plane")
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Section")
        SectionPlane(obj)
        ViewProviderSectionPlane(obj.ViewObject)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

class SectionPlane:
    "A section plane object"
    def __init__(self,obj):
        obj.Proxy = self
        self.Type = "Visual Component"
        
    def execute(self,obj):
        pl = obj.Placement
        l = obj.ViewObject.DisplaySize
        p = Part.makePlane(l,l,Vector(l/2,-l/2,0),Vector(0,0,-1))
        obj.Shape = p
        obj.Placement = pl

class ViewProviderSectionPlane(Component.ViewProviderComponent):
    "A View Provider for Section Planes"
    def __init__(self,vobj):
        vobj.addProperty("App::PropertyLength","DisplaySize","Base",
                        "The display size of the section plane")
        vobj.DisplaySize = 1
        vobj.Transparency = 85
        vobj.LineWidth = 1
        vobj.LineColor = (0.0,0.0,0.4,1.0)
        vobj.Proxy = self
        self.Object = vobj.Object

    def getIcon(self):
        return """
            /* XPM */
            static char * Arch_SectionPlane_xpm[] = {
            "16 16 3 1",
            " 	c None",
            ".	c #000000",
            "+	c #FFFFFF",
            "     ......     ",
            "   ..+++.++..   ",
            "  .+++++..+++.  ",
            " .++++++...+++. ",
            " .++.+++....++. ",
            ".++.+.++.....++.",
            ".+.+++.+......+.",
            ".+.+++.+........",
            ".+.+++.+........",
            ".+.....+......+.",
            ".+.+++.+.....++.",
            " .++++++....++. ",
            " .++++++...+++. ",
            "  .+++++..+++.  ",
            "   ..+++.++..   ",
            "     ......     "};
            """

    def claimChildren(self):
        return []

    def attach(self,vobj):
        self.Object = vobj.Object
        # adding arrows
        rn = vobj.RootNode
        self.col = coin.SoBaseColor()
        self.setColor()
        ds = coin.SoDrawStyle()
        ds.style = coin.SoDrawStyle.LINES
        self.lcoords = coin.SoCoordinate3()
        ls = coin.SoLineSet()
        ls.numVertices.setValues([2,2,2,2])
        self.mcoords = coin.SoCoordinate3()
        mk = coin.SoMarkerSet()
        mk.markerIndex = coin.SoMarkerSet.TRIANGLE_FILLED_5_5
        pt = coin.SoAnnotation()
        pt.addChild(self.col)
        pt.addChild(ds)
        pt.addChild(self.lcoords)
        pt.addChild(ls)
        pt.addChild(self.mcoords)
        pt.addChild(mk)
        rn.addChild(pt)
        self.setVerts()

    def setColor(self):
        print self.Object.ViewObject.LineColor
        self.col.rgb.setValue(self.Object.ViewObject.LineColor[0],
                              self.Object.ViewObject.LineColor[1],
                              self.Object.ViewObject.LineColor[2])

    def setVerts(self):
        hd = self.Object.ViewObject.DisplaySize/2
        verts = []
        verts.extend([[-hd,-hd,0],[-hd,-hd,-hd/3]])
        verts.extend([[hd,-hd,0],[hd,-hd,-hd/3]])
        verts.extend([[hd,hd,0],[hd,hd,-hd/3]])
        verts.extend([[-hd,hd,0],[-hd,hd,-hd/3]])
        self.lcoords.point.setValues(0,8,verts)
        self.mcoords.point.setValues(0,4,[verts[1],verts[3],verts[5],verts[7]])

    def updateData(self,obj,prop):
        if prop in ["Shape","Placement"]:
            self.setVerts()
        return

    def onChanged(self,vobj,prop):
        if prop == "LineColor":
            self.setColor()
        elif prop == "DisplaySize":
            vobj.Object.Proxy.execute(vobj.Object)
        return

FreeCADGui.addCommand('Arch_SectionPlane',CommandSectionPlane())
