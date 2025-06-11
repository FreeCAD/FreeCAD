# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 Daniel Wood <s.d.wood.82@gmail.com>                *
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

import FreeCAD
import Path
import Path.TurningOp.TurnBase as TurnBase

from PySide import QtCore

import liblathe.op.partoff as LiblatheOp

__title__ = "CAM Turning Parting Operation"
__author__ = "dubstar-04 (Daniel Wood)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class implementation for turning profiling operations."


def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectTurnPart(TurnBase.ObjectOp):
    """Proxy class for turning Part operations."""

    def opGenerateGCode(self, obj, turnTool):
        """
        Generate GCode for the op
        """
        partOp = LiblatheOp.PartoffOP()
        partOp.setParams(self.getProps(obj))

        FcBB = self.stockPlane.BoundBox
        partOp.add_stock_from_limits(FcBB.XMin, FcBB.XMax, FcBB.ZMin, FcBB.ZMax)

        partOp.addPartSegments(self.partOutline)
        partOp.add_tool(turnTool)

        pathCode = partOp.getGCode()

        for command in pathCode:
            pathCommand = Path.Command(command.get_movement(), command.getParams())
            self.commandlist.append(pathCommand)


def SetupProperties():
    """SetupProperties() ... Returns a list of properties to be set for the operation."""
    setup = []
    setup.append("StepOver")
    setup.append("FinishPasses")
    setup.append("AllowGrooving")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a TurnPart operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectTurnPart(obj, name, parentJob)
    return obj
