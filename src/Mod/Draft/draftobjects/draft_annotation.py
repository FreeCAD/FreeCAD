# ***************************************************************************
# *   (c) 2020 Carlo Pavan                                                  *
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

"""This module provides the object code for Draft Annotation.
"""
## @package annotation
# \ingroup DRAFT
# \brief This module provides the object code for Draft Annotation.

import FreeCAD as App
from PySide.QtCore import QT_TRANSLATE_NOOP
from draftutils import gui_utils

class DraftAnnotation:
    """The Draft Annotation Base object
    This class is not used directly, but inherited by all annotation
    objects.
    """
    def __init__(self, obj, tp="Unknown"):
        if obj:
            obj.Proxy = self
        self.Type = tp

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state

    def execute(self,obj):
        pass

    def onChanged(self, obj, prop):
        pass

class AnnotationStylesContainer:
    """The Annotation Container"""

    def __init__(self, obj):

        self.Type = "AnnotationContainer"
        obj.Proxy = self

    def execute(self, obj):

        g = obj.Group
        g.sort(key=lambda o: o.Label)
        obj.Group = g

    def __getstate__(self):

        if hasattr(self, "Type"):
            return self.Type

    def __setstate__(self, state):

        if state:
            self.Type = state