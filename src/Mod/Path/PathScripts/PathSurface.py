# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

__title__ = "Path Surface Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of 3D Surface operation."
__contributors__ = ""


# OCL must be installed
import FreeCAD
from PySide import QtCore
try:
    import ocl
except ImportError:
    msg = QtCore.QCoreApplication.translate("PathSurface",
        "This operation requires OpenCamLib to be installed.")
    FreeCAD.Console.PrintError(msg + "\n")
    raise ImportError
    # import sys
    # sys.exit(msg)

import PathScripts.PathOp as PathOp
import PathScripts.path_operations.surface as SurfaceOp
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
if FreeCAD.GuiUp:
    FreeCADGui = LazyLoader('FreeCADGui', globals(), 'FreeCADGui')


# Set logging level
PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule()


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectSurface(PathOp.ObjectOp):
    '''Proxy object for Surfacing operation.'''

    def opInit(self, obj):
        '''opInit(obj) ...
        This is the subclass extension to the base __init__() constructor.
        '''
        PathLog.debug('opInit()')
        # Instantiate the operation (strategy) object
        self.pathOperation = SurfaceOp.SurfaceOperation(obj,
                                                  parent=self,
                                                  initialize=True)

    def opFeatures(self, obj):
        '''opFeatures(obj) ...
        Return all standard features.
        '''
        PathLog.debug('opFeatures()')
        features = 0
        # Retrieve textual feature list from operation class
        featureList = SurfaceOp.SurfaceOperation.getFeatures()
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
        self.pathOperation = SurfaceOp.SurfaceOperation(obj,
                                                  parent=self,
                                                  initialize=True)

    # Primary method to contain actual operation actions
    def opExecute(self, obj):
        '''opExecute(obj) ... process Slot operation'''
        PathLog.track()

        CMDS = list()

        # Manage temporary debug objects
        self.isDebug = False \
            if PathLog.getLevel(PathLog.thisModule()) != 4 else True
        self.showDebugObjects = obj.ShowDebugObjects
        if not self.isDebug:
            self.showDebugObjects = False
        if self.showDebugObjects:
            self._makeTmpDebugGrp()

        # ######  MAIN COMMANDS FOR OPERATION ######
        cmds = self.pathOperation.execute(self, obj)
        if cmds:
            CMDS.extend(cmds)

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
    propList = SurfaceOp.SurfaceOperation.getPropertyDefinitions()
    return [tup[1] for tup in propList]


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Surface operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectSurface(obj, name)
    return obj
