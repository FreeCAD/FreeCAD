# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui

import draftutils.utils as utils
from draftutils.translate import translate

from draftviewproviders.view_draft_annotation \
    import ViewProviderDraftAnnotation


class ViewProviderText(ViewProviderDraftAnnotation):
    """Viewprovider for the Draft Text annotation."""

    def set_text_properties(self, vobj, properties):
        """Set text properties only if they don't already exist."""
        super().set_text_properties(vobj, properties)

        if "Justification" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Horizontal alignment")
            vobj.addProperty("App::PropertyEnumeration",
                             "Justification",
                             "Text",
                             _tip)
            vobj.Justification = ["Left", "Center", "Right"]

        if "LineSpacing" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Line spacing (relative to font size)")
            vobj.addProperty("App::PropertyFloat",
                             "LineSpacing",
                             "Text",
                             _tip)
            vobj.LineSpacing = 1.0

    def getIcon(self):
        """Return the path to the icon used by the view provider."""
        return ":/icons/Draft_Text.svg"

    def attach(self, vobj):
        """Set up the scene sub-graph of the view provider."""
        self.Object = vobj.Object

        # Main attributes of the Coin scenegraph
        self.trans = coin.SoTransform()
        self.mattext = coin.SoMaterial()
        self.font = coin.SoFont()
        self.text_wld = coin.SoAsciiText() # World orientation. Can be oriented in 3D space.
        self.text_scr = coin.SoText2()     # Screen orientation. Always faces the camera.

        textdrawstyle = coin.SoDrawStyle()
        textdrawstyle.style = coin.SoDrawStyle.FILLED

        # The text string needs to be initialized to something,
        # otherwise it may crash
        self.text_wld.string = self.text_scr.string = "Label"
        self.text_wld.justification = coin.SoAsciiText.LEFT
        self.text_scr.justification = coin.SoText2.LEFT

        self.node_wld = coin.SoGroup()
        self.node_wld.addChild(self.trans)
        self.node_wld.addChild(self.mattext)
        self.node_wld.addChild(textdrawstyle)
        self.node_wld.addChild(self.font)
        self.node_wld.addChild(self.text_wld)

        self.node_scr = coin.SoGroup()
        self.node_scr.addChild(self.trans)
        self.node_scr.addChild(self.mattext)
        self.node_scr.addChild(textdrawstyle)
        self.node_scr.addChild(self.font)
        self.node_scr.addChild(self.text_scr)

        vobj.addDisplayMode(self.node_wld, "World")
        vobj.addDisplayMode(self.node_scr, "Screen")
        self.onChanged(vobj, "TextColor")
        self.onChanged(vobj, "FontSize")
        self.onChanged(vobj, "FontName")
        self.onChanged(vobj, "Justification")
        self.onChanged(vobj, "LineSpacing")
        self.onChanged(vobj, "ScaleMultiplier")

    def updateData(self, obj, prop):
        """Execute when a property from the Proxy class is changed."""
        if prop == "Text" and obj.Text:
            self.text_wld.string.setValue("")
            self.text_scr.string.setValue("")
            _list = [l for l in obj.Text if l]
            self.text_wld.string.setValues(_list)
            self.text_scr.string.setValues(_list)

        elif prop == "Placement":
            self.trans.translation.setValue(obj.Placement.Base)
            self.trans.rotation.setValue(obj.Placement.Rotation.Q)

    def onChanged(self, vobj, prop):
        """Execute when a view property is changed."""
        super().onChanged(vobj, prop)

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
                self.text_wld.justification = coin.SoAsciiText.LEFT
                self.text_scr.justification = coin.SoText2.LEFT
            elif vobj.Justification == "Right":
                self.text_wld.justification = coin.SoAsciiText.RIGHT
                self.text_scr.justification = coin.SoText2.RIGHT
            else:
                self.text_wld.justification = coin.SoAsciiText.CENTER
                self.text_scr.justification = coin.SoText2.CENTER

        elif prop == "LineSpacing" and "LineSpacing" in properties:
            self.text_wld.spacing = vobj.LineSpacing
            self.text_scr.spacing = vobj.LineSpacing

    def edit(self):
        if not hasattr(Gui, "draftToolBar"):
            import DraftGui
        self.text = ""
        Gui.draftToolBar.sourceCmd = self
        Gui.draftToolBar.taskUi(title=translate("draft", "Text"), icon="Draft_Text")
        Gui.draftToolBar.textUi()
        Gui.draftToolBar.continueCmd.hide()
        Gui.draftToolBar.textValue.setPlainText("\n".join(self.Object.Text))
        Gui.draftToolBar.textValue.setFocus()

    def createObject(self):
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
            Gui.doCommand("FreeCAD.ActiveDocument." + self.Object.Name + ".Text = " + string)
            App.ActiveDocument.recompute()
            self.finish()

    def finish(self, cont=False):
        Gui.draftToolBar.sourceCmd = None
        Gui.draftToolBar.offUi()


# Alias for compatibility with v0.18 and earlier
ViewProviderDraftText = ViewProviderText

## @}
