# ***************************************************************************
# *   Copyright (c) 2020 Carlo Pavan <carlopav@gmail.com>                   *
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

import FreeCAD as App
import draftutils.utils as utils
from draftutils.messages import _msg

if App.GuiUp:
    import FreeCADGui as Gui

param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")


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
        self.set_graphics_properties(vobj, properties)

    def set_annotation_properties(self, vobj, properties):
        """Set annotation properties only if they don't already exist."""
        if "ScaleMultiplier" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "General scaling factor that affects "
                                     "the annotation consistently\n"
                                     "because it scales the text, "
                                     "and the line decorations, if any,\n"
                                     "in the same proportion.")
            vobj.addProperty("App::PropertyFloat",
                             "ScaleMultiplier",
                             "Annotation",
                             _tip)
            annotation_scale = param.GetFloat("DraftAnnotationScale", 1.0)
            if annotation_scale != 0:
                vobj.ScaleMultiplier = 1 / annotation_scale

        if "AnnotationStyle" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Annotation style to apply "
                                     "to this object.\n"
                                     "When using a saved style "
                                     "some of the view properties "
                                     "will become read-only;\n"
                                     "they will only be editable by changing "
                                     "the style through "
                                     "the 'Annotation style editor' tool.")
            vobj.addProperty("App::PropertyEnumeration",
                             "AnnotationStyle",
                             "Annotation",
                             _tip)
            styles = []
            for key in vobj.Object.Document.Meta.keys():
                if key.startswith("Draft_Style_"):
                    styles.append(key[12:])

            vobj.AnnotationStyle = [""] + styles

    def set_graphics_properties(self, vobj, properties):
        """Set graphics properties only if they don't already exist."""
        if "LineWidth" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Line width")
            vobj.addProperty("App::PropertyFloat",
                             "LineWidth",
                             "Graphics",
                             _tip)

        if "LineColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Line color")
            vobj.addProperty("App::PropertyColor",
                             "LineColor",
                             "Graphics",
                             _tip)

    def __getstate__(self):
        """Return a tuple of objects to save or None."""
        return None

    def __setstate__(self, state):
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
        modes = []
        return modes

    def setDisplayMode(self, mode):
        """Return the saved display mode."""
        return mode

    def onChanged(self, vobj, prop):
        """Execute when a view property is changed."""
        properties = vobj.PropertiesList
        meta = vobj.Object.Document.Meta

        if prop == "AnnotationStyle" and "AnnotationStyle" in properties:
            if not vobj.AnnotationStyle or vobj.AnnotationStyle == " ":
                # unset style
                _msg(16 * "-")
                _msg("Unset style")
                for visprop in utils.ANNOTATION_STYLE.keys():
                    if visprop in properties:
                        # make property writable
                        vobj.setPropertyStatus(visprop, '-ReadOnly')
            else:
                # set style
                styles = {}
                for key, value in meta.items():
                    if key.startswith("Draft_Style_"):
                        styles[key[12:]] = json.loads(value)

                if vobj.AnnotationStyle in styles:
                    _msg(16 * "-")
                    _msg("Style: {}".format(vobj.AnnotationStyle))
                    style = styles[vobj.AnnotationStyle]
                    for visprop in style.keys():
                        if visprop in properties:
                            try:
                                getattr(vobj, visprop).setValue(style[visprop])
                                _msg("setValue: "
                                     "'{}', '{}'".format(visprop,
                                                         style[visprop]))
                            except AttributeError:
                                setattr(vobj, visprop, style[visprop])
                                _msg("setattr: "
                                     "'{}', '{}'".format(visprop,
                                                         style[visprop]))
                            # make property read-only
                            vobj.setPropertyStatus(visprop, 'ReadOnly')

    def execute(self, vobj):
        """Execute when the object is created or recomputed."""
        return

    def setEdit(self, vobj, mode=0):
        """Execute the code when entering the specific edit mode.

        Currently only mode 0 works.
        """
        if mode == 0:
            Gui.runCommand("Draft_Edit")
            return True
        return False

    def unsetEdit(self, vobj, mode=0):
        """Execute the code when leaving the specific edit mode.

        Currently only mode 0 works.
        It runs the finish method of the currently active command, and close
        the task panel if any.
        """
        if App.activeDraftCommand:
            App.activeDraftCommand.finish()
        Gui.Control.closeDialog()
        return False

    def getIcon(self):
        """Return the path to the icon used by the view provider."""
        return ":/icons/Draft_Text.svg"

    def claimChildren(self):
        """Return objects that will be placed under it in the tree view.

        Editor: perhaps this is not useful???
        """
        objs = []
        if hasattr(self.Object, "Base"):
            objs.append(self.Object.Base)
        if hasattr(self.Object, "Objects"):
            objs.extend(self.Object.Objects)
        if hasattr(self.Object, "Components"):
            objs.extend(self.Object.Components)
        if hasattr(self.Object, "Group"):
            objs.extend(self.Object.Group)

        return objs

## @}
