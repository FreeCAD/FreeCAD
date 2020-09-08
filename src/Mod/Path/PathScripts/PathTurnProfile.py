# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Daniel Wood <s.d.wood.82@gmail.com>                *
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
import PathScripts.PathTurnBase as PathTurnBase
import PathScripts.PathLog as PathLog
from PySide import QtCore

import LibLathe.LLProfileOP as LLP
from LibLathe.LLPoint import Point
from LibLathe.LLSegment import Segment

__title__ = "Path Turn Profile Operation"
__author__ = "dubstar-04 (Daniel Wood)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class implementation for turning profiling operations."

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectTurnProfile(PathTurnBase.ObjectOp):
    '''Proxy class for turning profile operations.'''

    def generate_gcode(self, obj):
        '''
        Generate GCode for the op
        '''
        profileOP = LLP.ProfileOP()
        profileOP.set_params(self.getProps(obj))
        profileOP.add_stock(self.stock.Shape.BoundBox)
        profileOP.add_part(self.model[0].Shape.BoundBox)
        profileOP.add_part_edges(self.part_outline)
        PathCode = profileOP.get_gcode()

        for pathlist in PathCode:
            #print('pathlist', pathlist)
            for command in pathlist:
                #print('command:', command.get_movement(), command.get_params())
                pathCommand = Path.Command(command.get_movement(), command.get_params())            
                self.commandlist.append(pathCommand)

    def opSetDefaultValues(self, obj, job):
        obj.OpStartDepth = job.Stock.Shape.BoundBox.ZMax
        obj.OpFinalDepth = job.Stock.Shape.BoundBox.ZMin

    def opUpdateDepths(self, obj):
        obj.OpStartDepth = obj.OpStockZMax
        obj.OpFinalDepth = obj.OpStockZMin
        print('opSetDefaultValues:', obj.OpStartDepth.Value, obj.OpFinalDepth.Value)      

def SetupProperties():
    setup = []
    setup.append("Direction")
    setup.append("StepOver")
    setup.append("MinDia")
    setup.append("MaxDia")
    setup.append("StartOffset")
    setup.append("EndOffset")
    setup.append("AllowGrooving")
    setup.append("AllowFacing")
    return setup

def Create(name, obj=None):
    '''Create(name) ... Creates and returns a TurnProfile operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectTurnProfile(obj, name)
    return obj
