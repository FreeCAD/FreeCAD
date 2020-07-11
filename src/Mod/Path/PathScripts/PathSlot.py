# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 Russell Johnson (russ4262) <russ4262@gmail.com>    *
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

from __future__ import print_function

__title__ = "Path Slot Operation"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of Slot operation."
__contributors__ = ""

# Standard
# Third-party
# FreeCAD
import PathScripts.PathOp as PathOp
import PathScripts.path_operations.slot as SlotOp


PathOpBase = PathOp.PathOpBase
LazyLoader = PathOpBase.LazyLoader
# Add pointers to existing imports in PathOpBase
FreeCAD     = PathOpBase.FreeCAD
Path        = PathOpBase.Path
PathLog     = PathOpBase.PathLog
PathUtils   = PathOpBase.PathUtils
QtCore      = PathOpBase.QtCore
# LazyLoader loaded modules
if FreeCAD.GuiUp:
    FreeCADGui = LazyLoader('FreeCADGui', globals(), 'FreeCADGui')

# Add pointers to existing functions in OpFeatures
translate = PathOpBase.translate  # Qt translation handling

# Set logging level
PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule()


class ObjectSlot(PathOp.ObjectOp):
    '''Proxy object for Slot operation.
    Methods with "op" prefix are sub-calls from parent methods
    by same name without prefix in PathOp module's base class.'''

    # Methods for initializing the operation.
    # This group are all called in the __init__() contstructor
    # of the base operation class, in PathOpBase module.
    def opInit(self, obj):
        '''opInit(obj) ...
        This is the subclass extension to the base __init__() constructor.
        '''
        PathLog.debug('opInit()')
        # Instantiate the operation (strategy) object
        self.pathOperation = SlotOp.SlotOperation(obj,
                                                  parent=self,
                                                  initialize=True)

    def opFeatures(self, obj):
        '''opFeatures(obj) ...
        Return all standard features.
        '''
        PathLog.debug('opFeatures()')
        features = 0
        # Retrieve textual feature list from operation class
        featureList = SlotOp.SlotOperation.getFeatures()
        for f in featureList:
            features = features | getattr(PathOp, f)
        return features

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj, job) ...
        Initialize defaults
        '''
        # need to overwrite the default depth calculations for facing
        PathLog.debug('opSetDefaultValues()')
        d = None
        if job:
            if job.Stock:
                d = PathUtils.guessDepths(job.Stock.Shape, None)
                PathLog.debug("job.Stock exists")
            else:
                PathLog.debug("job.Stock NOT exist")
        else:
            PathLog.debug("job NOT exist")

        if d is not None:
            obj.OpFinalDepth.Value = d.final_depth
            obj.OpStartDepth.Value = d.start_depth
        else:
            obj.OpFinalDepth.Value = -10
            obj.OpStartDepth.Value = 10

        PathLog.debug(('Default OpFinalDepth: '
                       '{} mm'.format(obj.OpFinalDepth.Value)))
        PathLog.debug(('Defualt OpStartDepth: '
                       '{} mm'.format(obj.OpStartDepth.Value)))

        # Set default values and editor modes for properties
        self.pathOperation.finishPropertySetup(job, obj)

    # Maintenance and support methods
    def opOnChanged(self, obj, prop):
        # PathLog.info('opOnChanged() prop: {}'.format(prop))
        self.pathOperation.onChanged(prop)

    def opUpdateDepths(self, obj):
        PathLog.debug('opUpdateDepths()')
        # if hasattr(obj, 'Base') and obj.Base:
        #    obj.OpFinalDepth = self.pathOperation._ZMin
        obj.OpFinalDepth = self.pathOperation._ZMin

    # Document restoration calls this method
    def opOnDocumentRestored(self, obj):
        '''opOnDocumentRestored(obj) ...
        Implement if an op needs special handling on document restore.
        '''
        PathLog.debug('opOnDocumentRestored()')
        self.pathOperation = SlotOp.SlotOperation(obj,
                                                  parent=self,
                                                  initialize=True)

    # Primary method to contain actual operation actions
    def opExecute(self, obj):
        '''opExecute(obj) ... process Slot operation'''
        PathLog.track()

        CMDS = list()
        self.depthParams = PathUtils.depth_params(
            obj.ClearanceHeight.Value, obj.SafeHeight.Value,
            obj.StartDepth.Value, obj.StepDown.Value, 0.0,
            obj.FinalDepth.Value
        )

        # Manage temporary debug objects
        self.isDebug = False \
            if PathLog.getLevel(PathLog.thisModule()) != 4 else True
        self.showDebugObjects = obj.ShowDebugObjects
        if not self.isDebug:
            self.showDebugObjects = False
        if self.showDebugObjects:
            self._makeTmpDebugGrp()

        cmds = self.pathOperation.execute(self, obj)
        if cmds:
            CMDS.extend(cmds)

        # Save gcode produced
        self.commandlist.extend(CMDS)

        # Hide the temporary objects
        if self.showDebugObjects:
            if FreeCAD.GuiUp:
                objName = self.tmpDebugGrp.Name
                guiObj = FreeCADGui.ActiveDocument.getObject(objName)
                guiObj.Visibility = False
            self.tmpDebugGrp.purgeTouched()

        return True
# Eclass


def SetupProperties():
    ''' SetupProperties() ...
    Return list of properties required for operation.
    '''
    propList = SlotOp.SlotOperation.getPropertyDefinitions()
    return [tup[1] for tup in propList]


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Slot operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectSlot(obj, name)
    return obj
