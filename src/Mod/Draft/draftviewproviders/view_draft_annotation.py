# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2020 Carlo Pavan <carlopav@gmail.com>                   *
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
"""Provides the viewprovider code for all annotation type objects.

This is inherited and used by many viewproviders that show dimensions
and text created on screen through Coin (pivy).
- DimensionBase
- LinearDimension
- AngularDimension
- Label
- Text

Its edit mode launches the `Draft_Edit` command.
"""
## @package view_draft_annotation
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for all annotation type objects.

## \addtogroup draftviewproviders
# @{
import json
from PySide.QtCore import QT_TRANSLATE_NOOP

import PySide.QtCore as QtCore
import PySide.QtGui as QtGui

import FreeCAD as App
import FreeCADGui as Gui
from draftutils import params
from draftutils import utils
from draftutils.translate import translate


class ViewProviderDraftAnnotation(object):
    """The base class for Draft Annotation viewproviders.

    This class is intended to be inherited by more specialized viewproviders,
    for example, `Text`, `LinearDimension`, `RadialDimension`, and `Label`.
    """

    def __init__(self, vobj):
        self.set_properties(vobj)
        self.Object = vobj.Object
        vobj.Proxy = self

    def set_properties(self, vobj):
        """Set the properties only if they don't already exist."""
        properties = vobj.PropertiesList
        self.set_annotation_properties(vobj, properties)
        self.set_text_properties(vobj, properties)
        self.set_units_properties(vobj, properties)
        self.set_graphics_properties(vobj, properties)

    def set_annotation_properties(self, vobj, properties):
        """Set annotation properties only if they don't already exist."""
        if "ScaleMultiplier" not in properties:
            _tip = QT_TRANSLATE_NOOP(
                "App::Property",
                "General scaling factor that affects "
                "the annotation consistently\n"
                "because it scales the text, "
                "and the line decorations, if any,\n"
                "in the same proportion.",
            )
            vobj.addProperty(
                "App::PropertyFloat", "ScaleMultiplier", "Annotation", _tip, locked=True
            )
            vobj.ScaleMultiplier = params.get_param("DefaultAnnoScaleMultiplier")

        if "AnnotationStyle" not in properties:
            _tip = QT_TRANSLATE_NOOP(
                "App::Property",
                "Annotation style to apply "
                "to this object.\n"
                "When using a saved style "
                "some of the view properties "
                "will become read-only;\n"
                "they will only be editable by changing "
                "the style through "
                "the 'Annotation style editor' tool.",
            )
            vobj.addProperty(
                "App::PropertyEnumeration", "AnnotationStyle", "Annotation", _tip, locked=True
            )
            styles = []
            for key in vobj.Object.Document.Meta.keys():
                if key.startswith("Draft_Style_"):
                    styles.append(key[12:])

            vobj.AnnotationStyle = [""] + styles

    def set_text_properties(self, vobj, properties):
        """Set text properties only if they don't already exist."""
        if "FontName" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Font name")
            vobj.addProperty("App::PropertyFont", "FontName", "Text", _tip, locked=True)
            vobj.FontName = params.get_param("textfont")

        if "FontSize" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Font size")
            vobj.addProperty("App::PropertyLength", "FontSize", "Text", _tip, locked=True)
            vobj.FontSize = params.get_param("textheight")

        if "TextColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Text color")
            vobj.addProperty("App::PropertyColor", "TextColor", "Text", _tip, locked=True)
            vobj.TextColor = params.get_param("DefaultTextColor") | 0x000000FF

    def set_units_properties(self, vobj, properties):
        return

    def set_graphics_properties(self, vobj, properties):
        """Set graphics properties only if they don't already exist."""
        if vobj.Object.Proxy.Type == "Text":
            return

        if "ArrowSizeStart" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Arrow size")
            vobj.addProperty("App::PropertyLength", "ArrowSizeStart", "Graphics", _tip, locked=True)
            vobj.ArrowSizeStart = params.get_param("arrowsizestart")

        if "ArrowTypeStart" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Arrow type")
            vobj.addProperty(
                "App::PropertyEnumeration", "ArrowTypeStart", "Graphics", _tip, locked=True
            )
            vobj.ArrowTypeStart = utils.ARROW_TYPES
            vobj.ArrowTypeStart = "None"

        if vobj.Object.Proxy.Type != "Label":

            if "ArrowSizeEnd" not in properties:
                _tip = QT_TRANSLATE_NOOP("App::Property", "Arrow size")
                vobj.addProperty(
                    "App::PropertyLength", "ArrowSizeEnd", "Graphics", _tip, locked=True
                )
                vobj.ArrowSizeEnd = params.get_param("arrowsizeend")

            if "ArrowTypeEnd" not in properties:
                _tip = QT_TRANSLATE_NOOP("App::Property", "Arrow type")
                vobj.addProperty(
                    "App::PropertyEnumeration", "ArrowTypeEnd", "Graphics", _tip, locked=True
                )
                vobj.ArrowTypeEnd = utils.ARROW_TYPES
                vobj.ArrowTypeEnd = "None"

        if "LineWidth" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Line width")
            vobj.addProperty("App::PropertyFloat", "LineWidth", "Graphics", _tip, locked=True)
            vobj.LineWidth = params.get_param("DefaultAnnoLineWidth")

        if "LineColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Line color")
            vobj.addProperty("App::PropertyColor", "LineColor", "Graphics", _tip, locked=True)
            vobj.LineColor = params.get_param("DefaultAnnoLineColor") | 0x000000FF

    def dumps(self):
        """Return a tuple of objects to save or None."""
        return None

    def loads(self, state):
        """Set the internal properties from the restored state."""
        return None

    def attach(self, vobj):
        """Set up the scene sub-graph of the view provider."""
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        """Execute when a property from the Proxy class is changed."""
        return

    def getDisplayModes(self, vobj):
        """Return the display modes that this viewprovider supports."""
        return ["World", "Screen"]

    def getDefaultDisplayMode(self):
        """Return the default display mode."""
        return ["World", "Screen"][params.get_param("DefaultAnnoDisplayMode")]

    def setDisplayMode(self, mode):
        """Return the saved display mode."""
        return mode

    def onChanged(self, vobj, prop):
        """Execute when a view property is changed."""
        properties = vobj.PropertiesList
        meta = vobj.Object.Document.Meta

        if prop == "AnnotationStyle" and "AnnotationStyle" in properties:
            if not vobj.AnnotationStyle or vobj.AnnotationStyle == "":
                # unset style
                for visprop in utils.get_default_annotation_style().keys():
                    if visprop in properties:
                        # make property writable
                        vobj.setPropertyStatus(visprop, "-ReadOnly")
            else:
                # set style
                for key, value in meta.items():
                    if key.startswith("Draft_Style_") and key[12:] == vobj.AnnotationStyle:
                        style = utils.repair_annotation_style(json.loads(value))
                        for visprop in style.keys():
                            if visprop in properties:
                                # make property read-only
                                vobj.setPropertyStatus(visprop, "ReadOnly")
                                value = style[visprop]
                                try:
                                    if vobj.getTypeIdOfProperty(visprop) == "App::PropertyColor":
                                        value = value | 0x000000FF
                                    setattr(vobj, visprop, value)
                                except TypeError:
                                    pass
                        break

    def execute(self, vobj):
        """Execute when the object is created or recomputed."""
        return

    def setEdit(self, vobj, mode):
        """Execute the code when entering the specific edit mode.

        See view_base.py.
        """
        if mode != 0:
            return None

        if utils.get_type(vobj.Object) == "AngularDimension":
            return False  # Required, else edit mode is entered.

        if not "Draft_Edit" in Gui.listCommands():
            self.wb_before_edit = Gui.activeWorkbench()
            Gui.activateWorkbench("DraftWorkbench")
        Gui.runCommand("Draft_Edit")
        return True

    def unsetEdit(self, vobj, mode):
        """Execute the code when leaving the specific edit mode.

        See view_base.py.
        """
        if mode != 0:
            return None

        if hasattr(App, "activeDraftCommand") and App.activeDraftCommand:
            App.activeDraftCommand.finish()
        Gui.Control.closeDialog()
        if hasattr(self, "wb_before_edit"):
            Gui.activateWorkbench(self.wb_before_edit.name())
            delattr(self, "wb_before_edit")
        return True

    def doubleClicked(self, vobj):
        self.edit()

    def setupContextMenu(self, vobj, menu):
        if utils.get_type(self.Object) in ("AngularDimension", "Label"):
            return

        action_edit = QtGui.QAction(translate("draft", "Edit"), menu)
        QtCore.QObject.connect(action_edit, QtCore.SIGNAL("triggered()"), self.edit)
        menu.addAction(action_edit)

    def edit(self):
        Gui.ActiveDocument.setEdit(self.Object, 0)

    def getIcon(self):
        """Return the path to the icon used by the view provider."""
        return ":/icons/Draft_Text.svg"


## @}
