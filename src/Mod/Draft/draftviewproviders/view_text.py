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
"""This module provides the Draft Text view provider classes
"""
## @package text
# \ingroup DRAFT
# \brief This module provides the view provider code for Draft Text.


import FreeCAD as App
import DraftVecUtils, DraftGeomUtils
import math, sys
from pivy import coin
from PySide.QtCore import QT_TRANSLATE_NOOP
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
from draftviewproviders.view_draft_annotation import ViewProviderDraftAnnotation


class ViewProviderText(ViewProviderDraftAnnotation):
    """A View Provider for the Draft Text annotation"""


    def __init__(self,vobj):

        super(ViewProviderText, self).__init__(vobj)

        self.set_properties(vobj)

        self.Object = vobj.Object
        vobj.Proxy = self


    def set_properties(self, vobj):

        vobj.addProperty("App::PropertyLength","FontSize",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "The size of the text"))
        vobj.addProperty("App::PropertyFont","FontName",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "The font of the text"))
        vobj.addProperty("App::PropertyEnumeration","Justification",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "The vertical alignment of the text"))
        vobj.addProperty("App::PropertyColor","TextColor",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "Text color"))
        vobj.addProperty("App::PropertyFloat","LineSpacing",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "Line spacing (relative to font size)"))

        param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        annotation_scale = param.GetFloat("DraftAnnotationScale", 1.0)
        vobj.ScaleMultiplier = 1 / annotation_scale

        vobj.Justification = ["Left","Center","Right"]
        vobj.FontName = utils.get_param("textfont","sans")
        vobj.FontSize = utils.get_param("textheight",1)


    def getIcon(self):
        return ":/icons/Draft_Text.svg"


    def claimChildren(self):
        return []


    def attach(self,vobj):
        '''Setup the scene sub-graph of the view provider'''

        # backwards compatibility
        self.ScaleMultiplier = 1.00

        self.mattext = coin.SoMaterial()
        textdrawstyle = coin.SoDrawStyle()
        textdrawstyle.style = coin.SoDrawStyle.FILLED
        self.trans = coin.SoTransform()
        self.font = coin.SoFont()
        self.text2d = coin.SoAsciiText()
        self.text3d = coin.SoText2()
        self.text2d.string = self.text3d.string = "Label" # need to init with something, otherwise, crash!
        self.text2d.justification = coin.SoAsciiText.LEFT
        self.text3d.justification = coin.SoText2.LEFT
        self.node2d = coin.SoGroup()
        self.node2d.addChild(self.trans)
        self.node2d.addChild(self.mattext)
        self.node2d.addChild(textdrawstyle)
        self.node2d.addChild(self.font)
        self.node2d.addChild(self.text2d)
        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.trans)
        self.node3d.addChild(self.mattext)
        self.node3d.addChild(textdrawstyle)
        self.node3d.addChild(self.font)
        self.node3d.addChild(self.text3d)
        vobj.addDisplayMode(self.node2d,"2D text")
        vobj.addDisplayMode(self.node3d,"3D text")
        self.onChanged(vobj,"TextColor")
        self.onChanged(vobj,"FontSize")
        self.onChanged(vobj,"FontName")
        self.onChanged(vobj,"Justification")
        self.onChanged(vobj,"LineSpacing")

    def getDisplayModes(self,vobj):
        return ["2D text","3D text"]


    def setDisplayMode(self,mode):
        return mode


    def updateData(self,obj,prop):
        if prop == "Text":
            if obj.Text:
                if sys.version_info.major >= 3:
                    self.text2d.string.setValues([l for l in obj.Text if l])
                    self.text3d.string.setValues([l for l in obj.Text if l])
                else:
                    self.text2d.string.setValues([l.encode("utf8") for l in obj.Text if l])
                    self.text3d.string.setValues([l.encode("utf8") for l in obj.Text if l])
        elif prop == "Placement":
            self.trans.translation.setValue(obj.Placement.Base)
            self.trans.rotation.setValue(obj.Placement.Rotation.Q)


    def onChanged(self,vobj,prop):
        if prop == "ScaleMultiplier":
            if "ScaleMultiplier" in vobj.PropertiesList:
                if vobj.ScaleMultiplier:
                    self.ScaleMultiplier = vobj.ScaleMultiplier
            if "FontSize" in vobj.PropertiesList:
                self.font.size = vobj.FontSize.Value * self.ScaleMultiplier
        elif prop == "TextColor":
            if "TextColor" in vobj.PropertiesList:
                l = vobj.TextColor
                self.mattext.diffuseColor.setValue([l[0],l[1],l[2]])
        elif (prop == "FontName"):
            if "FontName" in vobj.PropertiesList:
                self.font.name = vobj.FontName.encode("utf8")
        elif prop  == "FontSize":
            if "FontSize" in vobj.PropertiesList:
                self.font.size = vobj.FontSize.Value * self.ScaleMultiplier
        elif prop == "Justification":
            try:
                if getattr(vobj, "Justification", None) is not None:
                    if vobj.Justification == "Left":
                        self.text2d.justification = coin.SoAsciiText.LEFT
                        self.text3d.justification = coin.SoText2.LEFT
                    elif vobj.Justification == "Right":
                        self.text2d.justification = coin.SoAsciiText.RIGHT
                        self.text3d.justification = coin.SoText2.RIGHT
                    else:
                        self.text2d.justification = coin.SoAsciiText.CENTER
                        self.text3d.justification = coin.SoText2.CENTER
            except AssertionError:
                pass # Race condition - Justification enum has not been set yet
        elif prop == "LineSpacing":
            if "LineSpacing" in vobj.PropertiesList:
                self.text2d.spacing = vobj.LineSpacing
                self.text3d.spacing = vobj.LineSpacing
