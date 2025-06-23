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

__title__ = "FreeCAD visualization registry"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package post_visualization
#  \ingroup FEM
#  \brief A registry to collect visualizations for use in menus

# Note: This file is imported from FreeCAD App files. Do not import any FreeCADGui
#       directly to support cmd line use.

import copy
from dataclasses import dataclass

from PySide import QtCore

import FreeCAD


# Registry to handle visualization commands
# #########################################

_registry = {}


@dataclass
class _Extraction:

    name: str
    icon: str
    dimension: str
    extracttype: str
    module: str
    factory: str


@dataclass
class _Visualization:

    name: str
    icon: str
    module: str
    factory: str
    extractions: list[_Extraction]


# Register a visualization by type, icon and factory function
def register_visualization(visualization_type, icon, module, factory):
    if visualization_type in _registry:
        raise ValueError("Visualization type already registered")

    _registry[visualization_type] = _Visualization(visualization_type, icon, module, factory, [])


def register_extractor(
    visualization_type, extraction_type, icon, dimension, etype, module, factory
):

    if not visualization_type in _registry:
        raise ValueError("visualization not registered yet")

    extraction = _Extraction(extraction_type, icon, dimension, etype, module, factory)
    _registry[visualization_type].extractions.append(extraction)


def get_registered_visualizations():
    return copy.deepcopy(_registry)


def _to_command_name(name):
    return "FEM_PostVisualization" + name


class _VisualizationGroupCommand:

    def GetCommands(self):
        visus = _registry.keys()
        cmds = [_to_command_name(v) for v in visus]
        return cmds

    def GetDefaultCommand(self):
        return 0

    def GetResources(self):
        return {
            "MenuText": QtCore.QT_TRANSLATE_NOOP("FEM", "Data Visualizations"),
            "ToolTip": QtCore.QT_TRANSLATE_NOOP(
                "FEM", "Different visualizations to show post processing data in"
            ),
        }

    def IsActive(self):
        if not FreeCAD.ActiveDocument:
            return False

        import FemGui

        return bool(FemGui.getActiveAnalysis())


class _VisualizationCommand:

    def __init__(self, visualization_type):
        self._visualization_type = visualization_type

    def GetResources(self):

        cmd = _to_command_name(self._visualization_type)
        vis = _registry[self._visualization_type]
        tooltip = f"Create a {self._visualization_type} post processing data visualization"

        return {
            "Pixmap": vis.icon,
            "MenuText": QtCore.QT_TRANSLATE_NOOP(cmd, "Create {}".format(self._visualization_type)),
            "Accel": "",
            "ToolTip": QtCore.QT_TRANSLATE_NOOP(cmd, tooltip),
            "CmdType": "AlterDoc",
        }

    def IsActive(self):
        # active analysis available
        if not FreeCAD.ActiveDocument:
            return False

        import FemGui

        return bool(FemGui.getActiveAnalysis())

    def Activated(self):
        import FreeCADGui

        vis = _registry[self._visualization_type]
        FreeCAD.ActiveDocument.openTransaction(f"Create {vis.name}")

        FreeCADGui.addModule(vis.module)
        FreeCADGui.addModule("FemGui")

        FreeCADGui.doCommand(f"obj = {vis.module}.{vis.factory}(FreeCAD.ActiveDocument)")
        FreeCADGui.doCommand(f"FemGui.getActiveAnalysis().addObject(obj)")

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.doCommand("FreeCADGui.ActiveDocument.setEdit(obj)")


def setup_commands(toplevel_name):
    # creates all visualization commands and registers them. The
    # toplevel group command will have the name provided to this function.

    import FreeCADGui

    # first all visualization and extraction commands
    for vis in _registry:
        FreeCADGui.addCommand(_to_command_name(vis), _VisualizationCommand(vis))

    # build the group command!
    FreeCADGui.addCommand("FEM_PostVisualization", _VisualizationGroupCommand())
