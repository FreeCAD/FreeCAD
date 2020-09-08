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
import PathScripts.PathUtils as PathUtils
import PathScripts.PathTurnBase as PathTurnBase
from PySide import QtCore

import LibLathe.LLFaceOP as LLF
from LibLathe.LLPoint import Point
from LibLathe.LLSegment import Segment

__title__ = "Path Turn Facing Operation"
__author__ = "dubstar-04 (Daniel Wood)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class implementation for turning facing operations."

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ObjectTurnFace(PathTurnBase.ObjectOp):
    '''Proxy class for turning facing operations.'''

    def opFeatures(self, obj):
        '''opFeatures(obj) ... returns the OR'ed list of features used and supported by the operation.'''
        return PathTurnBase.PathOp.FeatureDiameters | PathTurnBase.PathOp.FeatureTool | PathTurnBase.PathOp.FeatureDepths | PathTurnBase.PathOp.FeatureNoFinalDepth | PathTurnBase.PathOp.FeatureCoolant 
    
    def generate_gcode(self, obj):
        '''
        Generate GCode for the op
        '''
        #self.clear_path() 
        facingOP = LLF.FaceOP()
        facingOP.set_params(self.getProps(obj))
        facingOP.add_stock(self.stock.Shape.BoundBox)
        facingOP.add_part(self.model[0].Shape.BoundBox)
        facingOP.add_part_edges(self.part_outline)
        PathCode = facingOP.get_gcode()

        for pathlist in PathCode:
            #print('pathlist', pathlist)
            for command in pathlist:
                #print('command:', command.get_movement(), command.get_params())
                pathCommand = Path.Command(command.get_movement(), command.get_params())            
                self.commandlist.append(pathCommand)

    def opSetDefaultValues(self, obj, job):
        obj.OpStartDepth = obj.OpStockZMax

    def opUpdateDepths(self, obj):
        obj.OpStartDepth = obj.OpStockZMax

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
    '''Create(name) ... Creates and returns a TurnFace operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectTurnFace(obj, name)
    return obj
