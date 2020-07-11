# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2020 Schildkroet                                        *
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

__title__ = "Path Profile Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = ("Path Profile operation based on entire model,"
           " selected faces or selected edges.")
__contributors__ = "Schildkroet"

# Standard
# Third-party
# FreeCAD
import PathScripts.path_operations.areaOp as PathAreaOp
import PathScripts.path_operations.profile as ProfileOp

PathOp = PathAreaOp.PathAreaOpBase.PathOp
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


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule()


class ObjectProfile(PathAreaOp.ObjectOp):
    '''Proxy object for Profile operations based on entire model,
    selected faces, and selected edges.'''

    # Methods for initializing the operation.
    # This group are all called in the __init__() contstructor
    # of the base operation class, in PathOpBase module.
    def areaOpInit(self, obj):
        '''areaOpInit(obj) ... creates all profile specific properties.'''
        PathLog.debug('areaOpInit()')
        self.removalshapes = None  # Declared later
        self.pathOperation = ProfileOp.ProfileOperation(obj,
                                                        parent=self,
                                                        initialize=True)

    def areaOpFeatures(self, obj):
        '''areaOpFeatures(obj) ... returns operation-specific features'''
        PathLog.debug('areaOpFeatures()')
        features = 0
        # Retrieve feature list from strategy object
        featureList = ProfileOp.ProfileOperation.getFeatures()
        for f in featureList:
            features = features | getattr(PathOp, f)
        return features

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ...
        Overwrite to set initial values of operation-specific properties.
        Should be overwritten by subclasses.'''
        PathLog.debug('areaOpSetDefaultValues()')
        # Set default values and editor modes for properties
        self.pathOperation.finishPropertySetup(job, obj)

    # Maintenance and support methods
    def areaOpOnChanged(self, obj, prop):
        '''areaOpOnChanged(obj, prop) ...
        Updates certain property visibilities depending on changed properties.
        '''
        # PathLog.info('areaOpOnChanged() prop: {}'.format(prop))
        self.pathOperation.onChanged(prop)

    def areaOpUpdateDepths(self, obj):
        PathLog.debug('areaOpUpdateDepths()')
        obj.OpStartDepth = obj.OpStockZMax
        obj.OpFinalDepth = obj.OpStockZMin
        if self.pathOperation._ZMin:
            obj.OpFinalDepth = self.pathOperation._ZMin

    def areaOpAreaParams(self, obj, isHole):
        '''areaOpAreaParams(obj, isHole) ...
        Returns dictionary with area parameters.
        Do not overwrite.'''
        params = {}
        params['Fill'] = 0
        params['Coplanar'] = 0
        params['SectionCount'] = -1

        offset = obj.OffsetExtra.Value  # 0.0
        if obj.UseComp:
            offset = self.radius + obj.OffsetExtra.Value
        if obj.Side == 'Inside':
            offset = 0 - offset
        if isHole:
            offset = 0 - offset
        params['Offset'] = offset

        jointype = ['Round', 'Square', 'Miter']
        params['JoinType'] = jointype.index(obj.JoinType)

        if obj.JoinType == 'Miter':
            params['MiterLimit'] = obj.MiterLimit

        return params

    def areaOpAreaParamsExpandProfile(self, obj, isHole):
        '''areaOpPathParamsExpandProfile(obj, isHole) ...
        Return dictionary with area parameters for expaned profile.
        '''
        params = {}
        expProfStepover = float(obj.ExpandProfileStepOver) / 100.0

        params['Fill'] = 1
        params['Coplanar'] = 0
        params['PocketMode'] = 1
        params['SectionCount'] = -1
        params['PocketStepover'] = self.tool.Diameter * expProfStepover
        extraOffset = obj.OffsetExtra.Value
        params['PocketExtraOffset'] = extraOffset
        params['ToolRadius'] = self.radius

        params['PocketMode'] = 2  # set to 'Offset'
        params['JoinType'] = 0  # jointype = ['Round', 'Square', 'Miter']

        return params

    def areaOpPathParams(self, obj, isHole):
        '''areaOpPathParams(obj, isHole) ...
        Returns dictionary with path parameters.
        Do not overwrite.
        '''
        params = {}

        # Reverse the direction for holes
        if isHole:
            direction = "CW" if obj.Direction == "CCW" else "CCW"
        else:
            direction = obj.Direction

        if direction == 'CCW':
            params['orientation'] = 0
        else:
            params['orientation'] = 1

        if not obj.UseComp:
            if direction == 'CCW':
                params['orientation'] = 1
            else:
                params['orientation'] = 0

        return params

    def areaOpUseProjection(self, obj):
        '''areaOpUseProjection(obj) ... returns True'''
        if obj.ExpandProfile.Value == 0.0:
            return True
        return False

    # Document restoration calls this method
    def areaOpOnDocumentRestored(self, obj):
        '''areaOpOnDocumentRestored(self, obj) ...
        Called by opOnDocumentRestored(obj) in parent class.
        Contains operation-specific actions to be completed
        upon document restore.
        '''
        PathLog.debug('areaOpOnDocumentRestored()')
        self.pathOperation = ProfileOp.ProfileOperation(obj,
                                                        parent=self,
                                                        initialize=True)

    # Main method called in opExecute() method of PathAreaOp parent class
    # to return a list of tuples that contain shape and orientation details.
    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ...
        Returns envelope for all base shapes or wires for Arch.Panels.
        '''
        PathLog.track()

        shapes = []
        baseSubsTuples = list()
        allTuples = list()
        edgeFaces = list()
        subCount = 0
        self.offsetExtra = obj.OffsetExtra.Value
        self.expandProfile = None

        # Manage temporary debug objects
        self.isDebug = False
        if PathLog.getLevel(PathLog.thisModule()) == 4:
            self.isDebug = True
        self.showDebugObjects = obj.ShowDebugObjects
        if not self.isDebug:
            self.showDebugObjects = False
        if self.showDebugObjects:
            self._makeTmpDebugGrp()

        shapes = self.pathOperation.execute(self, obj)

        self.removalshapes = shapes
        PathLog.debug("%d shapes" % len(shapes))

        # Hide the temporary objects
        if self.showDebugObjects:
            if FreeCAD.GuiUp:
                objName = self.tmpDebugGrp.Name
                guiObj = FreeCADGui.ActiveDocument.getObject(objName)
                guiObj.Visibility = False
            self.tmpDebugGrp.purgeTouched()

        return shapes
# Eclass


def SetupProperties():
    setup = PathAreaOp.SetupProperties()
    propList = ProfileOp.ProfileOperation.getPropertyDefinitions()
    setup.extend([tup[1] for tup in propList])
    return setup


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Profile based on entire model,
    selected faces, and selected edges.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectProfile(obj, name)
    return obj
