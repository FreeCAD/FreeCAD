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
from draftutils.messages import _wrn
from draftutils.translate import translate


class DraftAnnotation(object):
    """The Draft Annotation Base object.

    This class is not used directly, but inherited by all Draft annotation
    objects.

    LinearDimension through DimensionBase
    AngularDimension through DimensionBase
    Label
    Text
    """

    def onDocumentRestored(self, obj):
        """Execute code when the document is restored.

        Check if new properties are present after the object is restored
        in order to migrate older objects.
        """
        if not hasattr(obj, "ViewObject"):
            return
        vobj = obj.ViewObject
        if not vobj:
            return
        if hasattr(vobj, "ScaleMultiplier") and hasattr(vobj, "AnnotationStyle"):
            return
        self.add_missing_properties_0v19(obj, vobj)

    def add_missing_properties_0v19(self, obj, vobj):
        """Provide missing annotation properties."""
        multiplier = None
        if not hasattr(vobj, "ScaleMultiplier"):
            multiplier = 1.00
            _wrn("v0.19, " + obj.Label + ", "
                 + translate("draft", "added view property 'ScaleMultiplier'"))
        if not hasattr(vobj, "AnnotationStyle"):
            _wrn("v0.19, " + obj.Label + ", "
                 + translate("draft", "added view property 'AnnotationStyle'"))
        vobj.Proxy.set_annotation_properties(vobj, vobj.PropertiesList)
        if multiplier is not None:
            vobj.ScaleMultiplier = multiplier

    def dumps(self):

        return

    def loads(self,state):

        return


## @}
