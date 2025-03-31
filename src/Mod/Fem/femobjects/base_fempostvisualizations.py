# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM postprocessing data exxtractor base objcts"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package base_fempostextractors
#  \ingroup FEM
#  \brief base objects for data extractors

from vtkmodules.vtkCommonDataModel import vtkTable

from . import base_fempythonobject

# helper functions
# ################

def is_visualization_object(obj):
    if not hasattr(obj, "Proxy"):
        return False

    return hasattr(obj.Proxy, "VisualizationType")

def get_visualization_type(obj):
    # returns the extractor type string, or throws exception if
    # not a extractor
    return obj.Proxy.VisualizationType


# Base class for all visualizations
# Note: Never use directly, always subclass! This class does not create a
#       Visualization variable, hence will not work correctly.
class PostVisualization(base_fempythonobject.BaseFemPythonObject):

    def __init__(self, obj):
        super().__init__(obj)
        self._setup_properties(obj)

    def _setup_properties(self, obj):
        pl = obj.PropertiesList
        for prop in self._get_properties():
            if not prop.name in pl:
                prop.add_to_object(obj)


    def onDocumentRestored(self, obj):
        self._setup_properties(obj)

    def _get_properties(self):
        return []
