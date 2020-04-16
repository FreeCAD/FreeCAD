# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""This module provides the Draft Label view provider classes
"""
## @package label
# \ingroup DRAFT
# \brief This module provides the view provider code for Draft Label.


import FreeCAD as App
import DraftVecUtils, DraftGeomUtils
import math, sys
from pivy import coin
from PySide.QtCore import QT_TRANSLATE_NOOP
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
from draftviewproviders.view_draft_annotation import ViewProviderDraftAnnotation

if App.GuiUp:
    import FreeCADGui as Gui
    

class ViewProviderLabel(ViewProviderDraftAnnotation):
    """A View Provider for the Label annotation object"""

    def __init__(self,vobj):

        super(ViewProviderLabel, self).__init__(vobj)

        self.set_properties(vobj)

        self.Object = vobj.Object
        vobj.Proxy = self

    def set_properties(self, vobj):

        # Text properties

        vobj.addProperty("App::PropertyLength","TextSize",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "The size of the text"))

        vobj.addProperty("App::PropertyFont","TextFont",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "The font of the text"))

        vobj.addProperty("App::PropertyEnumeration","TextAlignment",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "The vertical alignment of the text"))

        vobj.addProperty("App::PropertyColor","TextColor",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "Text color"))

        vobj.addProperty("App::PropertyInteger","MaxChars",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "The maximum number of characters on each line of the text box"))

        # Graphics properties

        vobj.addProperty("App::PropertyLength","ArrowSize",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property",
                         "The size of the arrow"))

        vobj.addProperty("App::PropertyEnumeration","ArrowType",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property",
                         "The type of arrow of this label"))

        vobj.addProperty("App::PropertyEnumeration","Frame",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property",
                         "The type of frame around the text of this object"))

        vobj.addProperty("App::PropertyBool","Line",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property",
                         "Display a leader line or not"))

        vobj.addProperty("App::PropertyFloat","LineWidth",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property",
                         "Line width"))

        vobj.addProperty("App::PropertyColor","LineColor",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property",
                         "Line color"))

        self.Object = vobj.Object
        vobj.TextAlignment = ["Top","Middle","Bottom"]
        vobj.TextAlignment = "Middle"
        vobj.LineWidth = utils.get_param("linewidth",1)
        vobj.TextFont = utils.get_param("textfont")
        vobj.TextSize = utils.get_param("textheight",1)
        vobj.ArrowSize = utils.get_param("arrowsize",1)
        vobj.ArrowType = utils.ARROW_TYPES
        vobj.ArrowType = utils.ARROW_TYPES[utils.get_param("dimsymbol")]
        vobj.Frame = ["None","Rectangle"]
        vobj.Line = True
        param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        annotation_scale = param.GetFloat("DraftAnnotationScale", 1.0)
        vobj.ScaleMultiplier = 1 / annotation_scale


    def getIcon(self):
        return ":/icons/Draft_Label.svg"

    def claimChildren(self):
        return []

    def attach(self,vobj):
        '''Setup the scene sub-graph of the view provider'''
        self.arrow = coin.SoSeparator()
        self.arrowpos = coin.SoTransform()
        self.arrow.addChild(self.arrowpos)
        self.matline = coin.SoMaterial()
        self.drawstyle = coin.SoDrawStyle()
        self.drawstyle.style = coin.SoDrawStyle.LINES
        self.lcoords = coin.SoCoordinate3()
        self.line = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.mattext = coin.SoMaterial()
        textdrawstyle = coin.SoDrawStyle()
        textdrawstyle.style = coin.SoDrawStyle.FILLED
        self.textpos = coin.SoTransform()
        self.font = coin.SoFont()
        self.text2d = coin.SoText2()
        self.text3d = coin.SoAsciiText()
        self.text2d.string = self.text3d.string = "Label" # need to init with something, otherwise, crash!
        self.text2d.justification = coin.SoText2.RIGHT
        self.text3d.justification = coin.SoAsciiText.RIGHT
        self.fcoords = coin.SoCoordinate3()
        self.frame = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.lineswitch = coin.SoSwitch()
        switchnode = coin.SoSeparator()
        switchnode.addChild(self.line)
        switchnode.addChild(self.arrow)
        self.lineswitch.addChild(switchnode)
        self.lineswitch.whichChild = 0
        self.node2d = coin.SoGroup()
        self.node2d.addChild(self.matline)
        self.node2d.addChild(self.arrow)
        self.node2d.addChild(self.drawstyle)
        self.node2d.addChild(self.lcoords)
        self.node2d.addChild(self.lineswitch)
        self.node2d.addChild(self.mattext)
        self.node2d.addChild(textdrawstyle)
        self.node2d.addChild(self.textpos)
        self.node2d.addChild(self.font)
        self.node2d.addChild(self.text2d)
        self.node2d.addChild(self.fcoords)
        self.node2d.addChild(self.frame)
        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.matline)
        self.node3d.addChild(self.arrow)
        self.node3d.addChild(self.drawstyle)
        self.node3d.addChild(self.lcoords)
        self.node3d.addChild(self.lineswitch)
        self.node3d.addChild(self.mattext)
        self.node3d.addChild(textdrawstyle)
        self.node3d.addChild(self.textpos)
        self.node3d.addChild(self.font)
        self.node3d.addChild(self.text3d)
        self.node3d.addChild(self.fcoords)
        self.node3d.addChild(self.frame)
        vobj.addDisplayMode(self.node2d,"2D text")
        vobj.addDisplayMode(self.node3d,"3D text")
        self.onChanged(vobj,"LineColor")
        self.onChanged(vobj,"TextColor")
        self.onChanged(vobj,"ArrowSize")
        self.onChanged(vobj,"Line")

    def getDisplayModes(self,vobj):
        return ["2D text","3D text"]

    def getDefaultDisplayMode(self):
        return "3D text"

    def setDisplayMode(self,mode):
        return mode

    def updateData(self,obj,prop):
        if prop == "Points":
            from pivy import coin
            if len(obj.Points) >= 2:
                self.line.coordIndex.deleteValues(0)
                self.lcoords.point.setValues(obj.Points)
                self.line.coordIndex.setValues(0,len(obj.Points),range(len(obj.Points)))
                self.onChanged(obj.ViewObject,"TextSize")
                self.onChanged(obj.ViewObject,"ArrowType")
            if obj.StraightDistance > 0:
                self.text2d.justification = coin.SoText2.RIGHT
                self.text3d.justification = coin.SoAsciiText.RIGHT
            else:
                self.text2d.justification = coin.SoText2.LEFT
                self.text3d.justification = coin.SoAsciiText.LEFT
        elif prop == "Text":
            if obj.Text:
                if sys.version_info.major >= 3:
                    self.text2d.string.setValues([l for l in obj.Text if l])
                    self.text3d.string.setValues([l for l in obj.Text if l])
                else:
                    self.text2d.string.setValues([l.encode("utf8") for l in obj.Text if l])
                    self.text3d.string.setValues([l.encode("utf8") for l in obj.Text if l])
                self.onChanged(obj.ViewObject,"TextAlignment")

    def getTextSize(self,vobj):
        if vobj.DisplayMode == "3D text":
            text = self.text3d
        else:
            text = self.text2d
        v = Gui.ActiveDocument.ActiveView.getViewer().getSoRenderManager().getViewportRegion()
        b = coin.SoGetBoundingBoxAction(v)
        text.getBoundingBox(b)
        return b.getBoundingBox().getSize().getValue()

    def onChanged(self,vobj,prop):
        if prop == "ScaleMultiplier":
            if not hasattr(vobj,"ScaleMultiplier"):
                return
            if hasattr(vobj,"TextSize") and hasattr(vobj,"TextAlignment"):
                self.update_label(vobj)
            if hasattr(vobj,"ArrowSize"):
                s = vobj.ArrowSize.Value * vobj.ScaleMultiplier
                if s:
                    self.arrowpos.scaleFactor.setValue((s,s,s))
        elif prop == "LineColor":
            if hasattr(vobj,"LineColor"):
                l = vobj.LineColor
                self.matline.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "TextColor":
            if hasattr(vobj,"TextColor"):
                l = vobj.TextColor
                self.mattext.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "LineWidth":
            if hasattr(vobj,"LineWidth"):
                self.drawstyle.lineWidth = vobj.LineWidth
        elif (prop == "TextFont"):
            if hasattr(vobj,"TextFont"):
                self.font.name = vobj.TextFont.encode("utf8")
        elif prop in ["TextSize","TextAlignment"] and hasattr(vobj,"ScaleMultiplier"):
            if hasattr(vobj,"TextSize") and hasattr(vobj,"TextAlignment"):
                self.update_label(vobj)
        elif prop == "Line":
            if hasattr(vobj,"Line"):
                if vobj.Line:
                    self.lineswitch.whichChild = 0
                else:
                    self.lineswitch.whichChild = -1
        elif prop == "ArrowType":
            if hasattr(vobj,"ArrowType"):
                if len(vobj.Object.Points) > 1:
                    if hasattr(self,"symbol"):
                        if self.arrow.findChild(self.symbol) != -1:
                                self.arrow.removeChild(self.symbol)
                    s = utils.ARROW_TYPES.index(vobj.ArrowType)
                    self.symbol = gui_utils.dim_symbol(s)
                    self.arrow.addChild(self.symbol)
                    self.arrowpos.translation.setValue(vobj.Object.Points[-1])
                    v1 = vobj.Object.Points[-2].sub(vobj.Object.Points[-1])
                    if not DraftVecUtils.isNull(v1):
                        v1.normalize()
                        v2 = App.Vector(0,0,1)
                        if round(v2.getAngle(v1),4) in [0,round(math.pi,4)]:
                            v2 = App.Vector(0,1,0)
                        v3 = v1.cross(v2).negative()
                        q = App.Placement(DraftVecUtils.getPlaneRotation(v1,v3,v2)).Rotation.Q
                        self.arrowpos.rotation.setValue((q[0],q[1],q[2],q[3]))
        elif prop == "ArrowSize":
            if hasattr(vobj,"ArrowSize") and hasattr(vobj,"ScaleMultiplier"):
                s = vobj.ArrowSize.Value * vobj.ScaleMultiplier
                if s:
                    self.arrowpos.scaleFactor.setValue((s,s,s))
        elif prop == "Frame":
            if hasattr(vobj,"Frame"):
                self.frame.coordIndex.deleteValues(0)
                if vobj.Frame == "Rectangle":
                    tsize = self.getTextSize(vobj)
                    pts = []
                    base = vobj.Object.Placement.Base.sub(App.Vector(self.textpos.translation.getValue().getValue()))
                    pts.append(base.add(App.Vector(0,tsize[1]*3,0)))
                    pts.append(pts[-1].add(App.Vector(-tsize[0]*6,0,0)))
                    pts.append(pts[-1].add(App.Vector(0,-tsize[1]*6,0)))
                    pts.append(pts[-1].add(App.Vector(tsize[0]*6,0,0)))
                    pts.append(pts[0])
                    self.fcoords.point.setValues(pts)
                    self.frame.coordIndex.setValues(0,len(pts),range(len(pts)))


    def update_label(self, vobj):
        self.font.size = vobj.TextSize.Value * vobj.ScaleMultiplier
        v = App.Vector(1,0,0)
        if vobj.Object.StraightDistance > 0:
            v = v.negative()
        v.multiply(vobj.TextSize/10)
        tsize = self.getTextSize(vobj)
        if (tsize is not None) and (len(vobj.Object.Text) > 1):
            v = v.add(App.Vector(0,(tsize[1]-1)*2,0))
        if vobj.TextAlignment == "Top":
            v = v.add(App.Vector(0,-tsize[1]*2,0))
        elif vobj.TextAlignment == "Middle":
            v = v.add(App.Vector(0,-tsize[1],0))
        v = vobj.Object.Placement.Rotation.multVec(v)
        pos = vobj.Object.Placement.Base.add(v)
        self.textpos.translation.setValue(pos)
        self.textpos.rotation.setValue(vobj.Object.Placement.Rotation.Q)