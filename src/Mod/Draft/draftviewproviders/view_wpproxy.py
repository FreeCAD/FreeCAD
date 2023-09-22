# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides the viewprovider code for the WorkingPlaneProxy object."""
## @package view_wpproxy
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for the WorkingPlaneProxy object.

## \addtogroup draftviewproviders
# @{
import pivy.coin as coin
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui


class ViewProviderWorkingPlaneProxy:
    """A View Provider for working plane proxies"""

    def __init__(self,vobj):
        # ViewData: 0,1,2: position; 3,4,5,6: rotation; 7: near dist; 8: far dist, 9:aspect ratio;
        # 10: focal dist; 11: height (ortho) or height angle (persp); 12: ortho (0) or persp (1)

        _tip = "The display length of this section plane"
        vobj.addProperty("App::PropertyLength", "DisplaySize",
                         "Draft", QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "The size of the arrows of this section plane"
        vobj.addProperty("App::PropertyLength", "ArrowSize",
                         "Draft", QT_TRANSLATE_NOOP("App::Property", _tip))

        vobj.addProperty("App::PropertyPercent","Transparency","Base","")

        vobj.addProperty("App::PropertyFloat","LineWidth","Base","")

        vobj.addProperty("App::PropertyColor","LineColor","Base","")

        vobj.addProperty("App::PropertyFloatList","ViewData","Base","")

        vobj.addProperty("App::PropertyBool","RestoreView","Base","")

        vobj.addProperty("App::PropertyMap","VisibilityMap","Base","")

        vobj.addProperty("App::PropertyBool","RestoreState","Base","")

        vobj.DisplaySize = 100
        vobj.ArrowSize = 5
        vobj.Transparency = 70
        vobj.LineWidth = 1

        param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        c = param.GetUnsigned("ColorHelpers", 674321151)
        vobj.LineColor = c & 0xFFFFFF00

        vobj.Proxy = self
        vobj.RestoreView = True
        vobj.RestoreState = True

    def getIcon(self):
        return ":/icons/Draft_SelectPlane.svg"

    def claimChildren(self):
        return []

    def doubleClicked(self,vobj):
        Gui.runCommand("Draft_SelectPlane")
        return True

    def setupContextMenu(self,vobj,menu):
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"),"Write camera position",menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),self.writeCamera)
        menu.addAction(action1)
        action2 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"),"Write objects state",menu)
        QtCore.QObject.connect(action2,QtCore.SIGNAL("triggered()"),self.writeState)
        menu.addAction(action2)

    def writeCamera(self):
        if hasattr(self,"Object"):
            n = Gui.ActiveDocument.ActiveView.getCameraNode()
            App.Console.PrintMessage(QT_TRANSLATE_NOOP("Draft","Writing camera position")+"\n")
            cdata = list(n.position.getValue().getValue())
            cdata.extend(list(n.orientation.getValue().getValue()))
            cdata.append(n.nearDistance.getValue())
            cdata.append(n.farDistance.getValue())
            cdata.append(n.aspectRatio.getValue())
            cdata.append(n.focalDistance.getValue())
            if isinstance(n,coin.SoOrthographicCamera):
                cdata.append(n.height.getValue())
                cdata.append(0.0) # orthograhic camera
            elif isinstance(n,coin.SoPerspectiveCamera):
                cdata.append(n.heightAngle.getValue())
                cdata.append(1.0) # perspective camera
            self.Object.ViewObject.ViewData = cdata

    def writeState(self):
        if hasattr(self,"Object"):
            App.Console.PrintMessage(QT_TRANSLATE_NOOP("Draft","Writing objects shown/hidden state")+"\n")
            vis = {}
            for o in App.ActiveDocument.Objects:
                if o.ViewObject:
                    vis[o.Name] = str(o.ViewObject.Visibility)
            self.Object.ViewObject.VisibilityMap = vis

    def attach(self,vobj):
        self.clip = None
        self.mat1 = coin.SoMaterial()
        self.mat2 = coin.SoMaterial()
        self.fcoords = coin.SoCoordinate3()
        fs = coin.SoIndexedFaceSet()
        fs.coordIndex.setValues(0,7,[0,1,2,-1,0,2,3])
        self.drawstyle = coin.SoDrawStyle()
        self.drawstyle.style = coin.SoDrawStyle.LINES
        self.lcoords = coin.SoCoordinate3()
        import PartGui # Required for "SoBrepEdgeSet" (because a WorkingPlaneProxy is not a Part::FeaturePython object).
        ls = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        ls.coordIndex.setValues(0,28,[0,1,-1,2,3,4,5,-1,6,7,-1,8,9,10,11,-1,12,13,-1,14,15,16,17,-1,18,19,20,21])
        sep = coin.SoSeparator()
        psep = coin.SoSeparator()
        fsep = coin.SoSeparator()
        fsep.addChild(self.mat2)
        fsep.addChild(self.fcoords)
        fsep.addChild(fs)
        psep.addChild(self.mat1)
        psep.addChild(self.drawstyle)
        psep.addChild(self.lcoords)
        psep.addChild(ls)
        sep.addChild(fsep)
        sep.addChild(psep)
        vobj.addDisplayMode(sep,"Default")
        self.onChanged(vobj,"DisplaySize")
        self.onChanged(vobj,"LineColor")
        self.onChanged(vobj,"Transparency")
        self.Object = vobj.Object

    def getDisplayModes(self,vobj):
        return ["Default"]

    def getDefaultDisplayMode(self):
        return "Default"

    def setDisplayMode(self,mode):
        return mode

    def updateData(self,obj,prop):
        if prop in ["Placement"]:
            self.onChanged(obj.ViewObject,"DisplaySize")
        return

    def onChanged(self,vobj,prop):
        if prop == "LineColor":
            l = vobj.LineColor
            self.mat1.diffuseColor.setValue([l[0],l[1],l[2]])
            self.mat2.diffuseColor.setValue([l[0],l[1],l[2]])

        elif prop == "Transparency":
            if hasattr(vobj,"Transparency"):
                self.mat2.transparency.setValue(vobj.Transparency/100.0)

        elif prop in ["DisplaySize","ArrowSize"]:
            if hasattr(vobj,"DisplaySize"):
                l = vobj.DisplaySize.Value/2
            else:
                l = 1
            verts = []
            fverts = []
            l1 = 0.1
            if hasattr(vobj,"ArrowSize"):
                l1 = vobj.ArrowSize.Value if vobj.ArrowSize.Value > 0 else 0.1
            l2 = l1/3
            pl = App.Placement(vobj.Object.Placement)
            fverts.append(pl.multVec(App.Vector(-l,-l,0)))
            fverts.append(pl.multVec(App.Vector(l,-l,0)))
            fverts.append(pl.multVec(App.Vector(l,l,0)))
            fverts.append(pl.multVec(App.Vector(-l,l,0)))

            verts.append(pl.multVec(App.Vector(0,0,0)))
            verts.append(pl.multVec(App.Vector(l-l1,0,0)))
            verts.append(pl.multVec(App.Vector(l-l1,l2,0)))
            verts.append(pl.multVec(App.Vector(l,0,0)))
            verts.append(pl.multVec(App.Vector(l-l1,-l2,0)))
            verts.append(pl.multVec(App.Vector(l-l1,l2,0)))

            verts.append(pl.multVec(App.Vector(0,0,0)))
            verts.append(pl.multVec(App.Vector(0,l-l1,0)))
            verts.append(pl.multVec(App.Vector(-l2,l-l1,0)))
            verts.append(pl.multVec(App.Vector(0,l,0)))
            verts.append(pl.multVec(App.Vector(l2,l-l1,0)))
            verts.append(pl.multVec(App.Vector(-l2,l-l1,0)))

            verts.append(pl.multVec(App.Vector(0,0,0)))
            verts.append(pl.multVec(App.Vector(0,0,l-l1)))
            verts.append(pl.multVec(App.Vector(-l2,0,l-l1)))
            verts.append(pl.multVec(App.Vector(0,0,l)))
            verts.append(pl.multVec(App.Vector(l2,0,l-l1)))
            verts.append(pl.multVec(App.Vector(-l2,0,l-l1)))
            verts.append(pl.multVec(App.Vector(0,-l2,l-l1)))
            verts.append(pl.multVec(App.Vector(0,0,l)))
            verts.append(pl.multVec(App.Vector(0,l2,l-l1)))
            verts.append(pl.multVec(App.Vector(0,-l2,l-l1)))

            self.lcoords.point.setValues(verts)
            self.fcoords.point.setValues(fverts)

        elif prop == "LineWidth":
            self.drawstyle.lineWidth = vobj.LineWidth
        return

    def dumps(self):
        return None

    def loads(self,state):
        return None

## @}
