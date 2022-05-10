# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides the viewprovider code for the Text object."""
## @package view_text
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for the Text object.

## \addtogroup draftviewproviders
# @{
import pivy.coin as coin
import sys
from PySide.QtCore import QT_TRANSLATE_NOOP

import draftutils.utils as utils

from draftviewproviders.view_draft_annotation \
    import ViewProviderDraftAnnotation


class ViewProviderText(ViewProviderDraftAnnotation):
    """Viewprovider for the Draft Text annotation."""

    def __init__(self, vobj):
        super(ViewProviderText, self).__init__(vobj)

        self.set_properties(vobj)
        self.Object = vobj.Object
        vobj.Proxy = self

    def set_properties(self, vobj):
        """Set the properties only if they don't already exist."""
        super(ViewProviderText, self).set_properties(vobj)
        properties = vobj.PropertiesList

        if "FontSize" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The size of the text")
            vobj.addProperty("App::PropertyLength",
                             "FontSize",
                             "Text",
                             _tip)
            vobj.FontSize = utils.get_param("textheight", 1)

        if "FontName" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The font of the text")
            vobj.addProperty("App::PropertyFont",
                             "FontName",
                             "Text",
                             _tip)
            vobj.FontName = utils.get_param("textfont", "sans")

        if "Justification" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The vertical alignment of the text")
            vobj.addProperty("App::PropertyEnumeration",
                             "Justification",
                             "Text",
                             _tip)
            vobj.Justification = ["Left", "Center", "Right"]

        if "TextColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Text color")
            vobj.addProperty("App::PropertyColor",
                             "TextColor",
                             "Text",
                             _tip)

        if "LineSpacing" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Line spacing (relative to font size)")
            vobj.addProperty("App::PropertyFloat",
                             "LineSpacing",
                             "Text",
                             _tip)

    def getIcon(self):
        """Return the path to the icon used by the view provider."""
        return ":/icons/Draft_Text.svg"

    def claimChildren(self):
        """Return objects that will be placed under it in the tree view."""
        return []

    def attach(self, vobj):
        """Set up the scene sub-graph of the view provider."""
        # Main attributes of the Coin scenegraph
        self.mattext = coin.SoMaterial()
        self.trans = coin.SoTransform()
        self.font = coin.SoFont()
        self.text2d = coin.SoText2()  # Faces the camera always
        self.text3d = coin.SoAsciiText()  # Can be oriented in 3D space

        textdrawstyle = coin.SoDrawStyle()
        textdrawstyle.style = coin.SoDrawStyle.FILLED

        # The text string needs to be initialized to something,
        # otherwise it may crash
        self.text2d.string = self.text3d.string = "Label"
        self.text2d.justification = coin.SoText2.LEFT
        self.text3d.justification = coin.SoAsciiText.LEFT

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

        vobj.addDisplayMode(self.node2d, "2D text")
        vobj.addDisplayMode(self.node3d, "3D text")
        self.onChanged(vobj, "TextColor")
        self.onChanged(vobj, "FontSize")
        self.onChanged(vobj, "FontName")
        self.onChanged(vobj, "Justification")
        self.onChanged(vobj, "LineSpacing")
        self.onChanged(vobj, "ScaleMultiplier")
        self.Object = vobj.Object

    def getDisplayModes(self, vobj):
        """Return the display modes that this viewprovider supports."""
        return ["2D text", "3D text"]

    def setDisplayMode(self, mode):
        """Return the saved display mode."""
        return mode

    def updateData(self, obj, prop):
        """Execute when a property from the Proxy class is changed."""
        if prop == "Text" and obj.Text:
            self.text2d.string.setValue("")
            self.text3d.string.setValue("")
            _list = [l for l in obj.Text if l]
            self.text2d.string.setValues(_list)
            self.text3d.string.setValues(_list)

        elif prop == "Placement":
            self.trans.translation.setValue(obj.Placement.Base)
            self.trans.rotation.setValue(obj.Placement.Rotation.Q)

    def onChanged(self, vobj, prop):
        """Execute when a view property is changed."""
        super(ViewProviderText, self).onChanged(vobj, prop)

        properties = vobj.PropertiesList

        if (prop == "ScaleMultiplier" and "ScaleMultiplier" in properties
                and vobj.ScaleMultiplier):
            if "FontSize" in properties:
                self.font.size = vobj.FontSize.Value * vobj.ScaleMultiplier

        elif prop == "TextColor" and "TextColor" in properties:
            col = vobj.TextColor
            self.mattext.diffuseColor.setValue([col[0], col[1], col[2]])

        elif prop == "FontName" and "FontName" in properties:
            self.font.name = vobj.FontName.encode("utf8")

        elif (prop == "FontSize" and "FontSize" in properties
              and "ScaleMultiplier" in properties and vobj.ScaleMultiplier):
            # In v0.19 this code causes an `AttributeError` exception
            # during loading of the document as `ScaleMultiplier`
            # apparently isn't set immediately when the document loads.
            # So the if condition tests for the existence
            # of `ScaleMultiplier` before even changing the font size.
            # A try-except block may be used as well to catch and ignore
            # this error.
            self.font.size = vobj.FontSize.Value * vobj.ScaleMultiplier

        elif prop == "Justification" and "Justification" in properties:
            # This code was surrounded by a try-except block in order to catch
            # and ignore an `AssertionError` exception. This was the result
            # of a race condition that would result in the `Justification`
            # property not being set at the time that this code is invoked.
            # However, in v0.19 this problem doesn't seem to exist any more
            # so we removed the try-except block.
            if vobj.Justification == "Left":
                self.text2d.justification = coin.SoText2.LEFT
                self.text3d.justification = coin.SoAsciiText.LEFT
            elif vobj.Justification == "Right":
                self.text2d.justification = coin.SoText2.RIGHT
                self.text3d.justification = coin.SoAsciiText.RIGHT
            else:
                self.text2d.justification = coin.SoText2.CENTER
                self.text3d.justification = coin.SoAsciiText.CENTER

        elif prop == "LineSpacing" and "LineSpacing" in properties:
            self.text2d.spacing = vobj.LineSpacing
            self.text3d.spacing = vobj.LineSpacing

    def setEdit(self,vobj,mode):

        import FreeCADGui
        self.text = ''
        FreeCADGui.draftToolBar.sourceCmd = self
        FreeCADGui.draftToolBar.taskUi()
        FreeCADGui.draftToolBar.textUi()
        FreeCADGui.draftToolBar.textValue.setPlainText("\n".join(vobj.Object.Text))
        FreeCADGui.draftToolBar.textValue.setFocus()

    def unsetEdit(self,vobj=None,mode=0):

        import FreeCADGui
        FreeCADGui.Control.closeDialog()
        return True

    def doubleClicked(self,vobj):

        self.setEdit(vobj,None)

    def createObject(self):

        import FreeCAD
        import FreeCADGui
        if hasattr(self,"Object"):
            txt = self.text
            if not txt:
                self.finish()
                return None
            # If the last element is an empty string "" we remove it
            if not txt[-1]:
                txt.pop()
            t_list = ['"' + l + '"' for l in txt]
            list_as_text = ", ".join(t_list)
            string = '[' + list_as_text + ']'
            FreeCADGui.doCommand("FreeCAD.ActiveDocument."+self.Object.Name+".Text = "+string)
            FreeCAD.ActiveDocument.recompute()
            self.finish()

    def finish(self,args=None):

        import FreeCADGui
        FreeCADGui.draftToolBar.sourceCmd = None
        FreeCADGui.draftToolBar.offUi()


# Alias for compatibility with v0.18 and earlier
ViewProviderDraftText = ViewProviderText

## @}
