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
"""Provides the object code for all annotation type objects.

This is used by many objects that show dimensions and text created on screen
through Coin (pivy).
- DimensionBase
- LinearDimension
- AngularDimension
- Label
- Text
"""
## @package draft_annotation
# \ingroup draftobjects
# \brief Provides the object code for all annotation type objects.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

from draftutils.messages import _wrn
from draftutils.translate import _tr


class DraftAnnotation(object):
    """The Draft Annotation Base object.

    This class is not used directly, but inherited by all Draft annotation
    objects.

    LinearDimension through DimensionBase
    AngularDimension through DimensionBase
    Label
    Text
    """

    def __init__(self, obj, typ="Annotation"):
        self.Type = typ
        obj.Proxy = self

    def onDocumentRestored(self, obj):
        """Execute code when the document is restored.

        Check if new properties are present after the object is restored
        in order to migrate older objects.
        """
        if hasattr(obj, "ViewObject") and obj.ViewObject:
            vobj = obj.ViewObject
            self.add_missing_properties_0v19(obj, vobj)

    def add_missing_properties_0v19(self, obj, vobj):
        """Provide missing annotation properties, if they don't exist."""
        vproperties = vobj.PropertiesList

        if 'ScaleMultiplier' not in vproperties:
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
            vobj.ScaleMultiplier = 1.00

            _info = "added view property 'ScaleMultiplier'"
            _wrn("v0.19, " + obj.Label + ", " + _tr(_info))

        if 'AnnotationStyle' not in vproperties:
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
            for key in obj.Document.Meta.keys():
                if key.startswith("Draft_Style_"):
                    styles.append(key[12:])

            vobj.AnnotationStyle = [""] + styles

            _info = "added view property 'AnnotationStyle'"
            _wrn("v0.19, " + obj.Label + ", " + _tr(_info))

    def __getstate__(self):
        """Return a tuple of objects to save or None.

        Save the Type.
        """
        return self.Type

    def __setstate__(self, state):
        """Set the internal properties from the restored state.

        Restore the Type of the object.
        """
        if state:
            if isinstance(state, dict) and ("Type" in state):
                # During the migration of the classes
                # the 'DraftText' type was changed to 'Text' type
                if state["Type"] == "DraftText":
                    state["Type"] = "Text"
                    _info = "migrated 'DraftText' type to 'Text'"
                    _wrn("v0.19, " + _tr(_info))
                self.Type = state["Type"]
            else:
                if state == "DraftText":
                    state = "Text"
                    _info = "migrated 'DraftText' type to 'Text'"
                    _wrn("v0.19, " + _tr(_info))
                self.Type = state

    def execute(self, obj):
        """Execute when the object is created or recomputed.

        Does nothing.
        """
        return

    def onChanged(self, obj, prop):
        """Execute when a property is changed.

        Does nothing.
        """
        return

## @}
