# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

__title__ = "Base class for all operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base class and properties implementation for all Path operations."

# Standard
import time
# Third-party
# FreeCAD
import Path
import PathScripts.PathGeom as PathGeom
import PathScripts.path_features.features as OpFeatures

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
FreeCAD = LazyLoader('FreeCAD', globals(), 'FreeCAD')
Part = LazyLoader('Part', globals(), 'Part')

# Add pointers to existing imports in OpFeatures
QtCore = OpFeatures.QtCore
PathLog = OpFeatures.PathLog
PathUtil = OpFeatures.PathUtil
PathUtils = OpFeatures.PathUtils

# Add pointers to existing functions in OpFeatures
QT_TRANSLATE = OpFeatures.QT_TRANSLATE
translate = OpFeatures.translate

# Set logging level using PathLog
PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule()


# Register strategy oriented operations here
strategy_operations = ['Profile', 'Slot', 'Surface', 'Waterline']


class ObjectOpBase(object):
    '''
    Base class for proxy objects of all Path operations.

    Use this class as a base class for new operations. It provides properties
    and some functionality for the standard properties each operation supports.
    By OR'ing features from the feature list an operation can select which ones
    of the standard features it requires and/or supports.

    The currently supported features are:
        FeatureTool          ... Use of a ToolController
        FeatureDepths        ... Depths, for start, final
        FeatureHeights       ... Heights, safe and clearance
        FeatureStartPoint    ... Supports setting a start point
        FeatureFinishDepth   ... Operation supports a finish depth
        FeatureStepDown      ... Support for step down
        FeatureNoFinalDepth  ... Disable support for final depth modifications
        FeatureBaseVertexes  ... Base geometry support for vertexes
        FeatureBaseEdges     ... Base geometry support for edges
        FeatureBaseFaces     ... Base geometry support for faces
        FeatureBasePanels    ... Base geometry support for Arch.Panels
        FeatureLocations     ... Base location support
        FeatureCoolant       ... Support for operation coolant
    These features are managed by the FeatureManager class within
    the features/features.py module

    The base class handles all base API and forwards calls to subclasses with
    an op prefix. For instance, an op is not expected to overwrite onChanged(),
    but implement the function opOnChanged().
    If a base class overwrites a base API function it should call the super's
    implementation - otherwise the base functionality might be broken.
    '''

    # Methods to initialize operation base class
    def __init__(self, obj, name):
        PathLog.track()
        PathLog.debug('PathOpBase name: {}'.format(name))
        self.strategy = name

        # Attribute members being set later
        self.commandlist = None
        self.model = None
        self.stock = None
        self.tool = None
        self.radius = None
        self.horizFeed = None
        self.horizRapid = None
        self.vertFeed = None
        self.vertRapid = None

        # Universal axial feed rates
        self.axialFeed = 0.0
        self.axialRapid = 0.0
        # Axis-specific feed rates
        self.aAxisFeed = 0.0
        self.aAxisRapid = 0.0
        self.bAxisFeed = 0.0
        self.bAxisRapid = 0.0
        self.cAxisFeed = 0.0
        self.cAxisRapid = 0.0

        # Additional instantiation of isntance attributes
        self._initAttributes(obj)

        # Add standard properties for all operations
        self._addStandardProperties(obj)

        # Initialize properties associated with requested features
        self.features = self.opFeatures(obj)
        self.featureManager = self._initFeatures(obj, init=True)

        # Call operation's subclass version of class constructor
        self.opInit(obj)  # replaces initOperation(obj) for naming scheme
        self.initOperation(obj)  # DIMINISHED for nameing consistency

        # Apply property defaults unless otherwise flagged
        if (not hasattr(obj, 'DoNotSetDefaultValues') or
                not obj.DoNotSetDefaultValues):
            job = PathUtils.addToJob(obj)
            if job:
                # self.job = job
                self._setDefaultValues(job, obj)
                job.SetupSheet.Proxy.setOperationProperties(obj, name)
                obj.recompute()
                obj.Proxy = self

        # Make final call to subclass during constuctor
        # now that default values are set
        self._initEnd(obj)

        # Last, set visibility (editor modes)
        self._setStaticEditorModes(obj)

    def _initAttributes(self, obj):
        '''_initAttributes(obj) ...
        Sets attributes necessary for new class instance,
        as well as on document restore action.'''
        self.job = None
        self.addNewProps = None
        self.editorModes = list()
        self.propertiesReady = False
        self.tmpDebugGrp = None
        self.pathOperation = None  # Will be reference to oper. class
        # Debugging attributes
        self.isDebug = False
        self.showDebugObjects = False

        # Call subclass version of this method
        self.opInitAttributes(obj)

    def _initEnd(self, obj):
        '''_initEnd(self, obj) ...
        This base method is executed at the end of the `__init__()`
        constructor in order to execute instructions after base properties
        are instantiated an their default values set.
        Do not overwrite. Implement `opInitEnd()` instead.
        '''
        # Call subclass version
        self.opInitEnd(obj)

    def _addStandardProperties(self, obj):
        obj.addProperty("App::PropertyBool",
                        "Active",
                        "Path",
                        QT_TRANSLATE("PathOp",
                                     ("Make False, to prevent operation from"
                                      " generating code")))
        obj.addProperty("App::PropertyString",
                        "Comment",
                        "Path",
                        QT_TRANSLATE("PathOp",
                                     "An optional comment for this Operation"))
        obj.addProperty("App::PropertyString",
                        "UserLabel",
                        "Path",
                        QT_TRANSLATE("PathOp",
                                     "User Assigned Label"))

        obj.addProperty("App::PropertyString",
                        "CycleTime",
                        "Path",
                        QT_TRANSLATE("PathOp",
                                     "Operations Cycle Time Estimation"))
        obj.setEditorMode('CycleTime', 1)  # read-only

        obj.addProperty("App::PropertyEnumeration",
                        "Operation",
                        "Path",
                        QT_TRANSLATE("PathOp",
                                     "An optional comment for this Operation"))
        if self.strategy in strategy_operations:
            obj.Operation = ['Profile', 'Slot', 'Surface',
                             'Waterline']
            # obj.Operation = ['Adaptive', 'Custom', 'Deburr', 'Drilling',
            #                'Engrave', 'Helix', 'MillFace', 'Pocket',
            #                'PocketShape', 'Profile', 'Slot', 'Surface',
            #                'Waterline']
        else:
            obj.Operation = ['None']
        obj.setEditorMode('Operation', 2)

        self._addDebugProperty(obj)

    def _setDefaultValues(self, job, obj):
        '''_setDefaultValues(job, obj) ... base implementation.
        Do not overwrite, overwrite opSetDefaultValues() instead.
        '''
        PathLog.debug('_setDefaultValues()')
        features = self.features  # self.opFeatures(obj)

        obj.Active = True
        if self.strategy in strategy_operations:
            obj.Operation = self.strategy
        else:
            obj.Operation = 'None'
        obj.ShowDebugObjects = False

        if features and self.featureManager:
            self.featureManager.setFeatureDefaultValues(job, obj)

        # Call operation subclass method extension
        self.opSetDefaultValues(obj, job)

    def _initFeatures(self, obj, init=False):
        if not self.features:
            msg = translate('PathOpBase', 'No features to manage.')
            PathLog.debug(msg)
            return False

        featureManager = OpFeatures.FeatureManager(OpFeatures,
                                                   obj, self.features)
        if init:
            featureManager.applyRequestedFeatures()

        return featureManager

    def _addDebugProperty(self, obj):
        obj.addProperty("App::PropertyBool",
                        "ShowDebugObjects",
                        "Debug",
                        QT_TRANSLATE("App::Property",
                                     ("Show the temporary path construction"
                                      " objects when module is in"
                                      " DEBUG mode.")))
        obj.setEditorMode('ShowDebugObjects', 2)
        obj.ShowDebugObjects = False

    def _setStaticEditorModes(self, obj):
        '''_setStaticEditorModes(self, obj, features) ...
        Set editor modes at initialization and document restoration
        for all properties requiring a static editor mode.
        Editor modes are not preserved during document store/restore.
        '''
        features = self.features

        for op in ['OpStartDepth', 'OpFinalDepth',
                   'OpToolDiameter', 'CycleTime']:
            if hasattr(obj, op):
                obj.setEditorMode(op, 1)  # read-only

        if OpFeatures.FeatureDepths & features:
            if OpFeatures.FeatureNoFinalDepth & features:
                obj.setEditorMode('OpFinalDepth', 2)

        self.opSetStaticEditorModes(obj)

    def applyExpression(self, obj, prop, expr):
        '''applyExpression(obj, prop, expr) ...
        Set expression expr on obj.prop if expr is set.
        '''
        if expr:
            obj.setExpression(prop, expr)
            return True
        return False

    # Maintenance and support methods
    def onChanged(self, obj, prop):
        '''onChanged(obj, prop) ...
        Base implementation of the FC notification framework.
        Do not overwrite, overwrite opOnChanged() instead.
        '''
        # PathLog.debug('obj.State:\n{}'.format(obj.State))
        if ('Restore' not in obj.State and
                prop in ['Base', 'StartDepth', 'FinalDepth']):
            self.updateDepths(obj, True)

        if hasattr(self, 'propertiesReady') and self.propertiesReady:
            self.opOnChanged(obj, prop)

    def updateDepths(self, obj, ignoreErrors=False):
        '''updateDepths(obj, ignoreErrors=False) ...
        Base implementation calculating depths depending on base geometry.
        Should not be overwritten.
        '''

        def faceZmin(bb, fbb):
            if fbb.ZMax == fbb.ZMin and fbb.ZMax == bb.ZMax:  # top face
                return fbb.ZMin
            elif fbb.ZMax > fbb.ZMin and fbb.ZMax == bb.ZMax:
                # vertical face, full cut
                return fbb.ZMin
            elif fbb.ZMax > fbb.ZMin and fbb.ZMin > bb.ZMin:
                # internal vertical wall
                return fbb.ZMin
            elif fbb.ZMax == fbb.ZMin and fbb.ZMax > bb.ZMin:  # face/shelf
                return fbb.ZMin
            return bb.ZMin

        if not self._setBaseAndStock(obj, ignoreErrors):
            return False

        stockBB = self.stock.Shape.BoundBox
        zmin = stockBB.ZMin
        zmax = stockBB.ZMax

        obj.OpStockZMin = zmin
        obj.OpStockZMax = zmax

        if hasattr(obj, 'Base') and obj.Base:
            for base, sublist in obj.Base:
                bb = base.Shape.BoundBox
                zmax = max(zmax, bb.ZMax)
                for sub in sublist:
                    try:
                        fbb = base.Shape.getElement(sub).BoundBox
                        zmin = max(zmin, faceZmin(bb, fbb))
                        zmax = max(zmax, fbb.ZMax)
                    except Part.OCCError as e:
                        PathLog.error(e)

        else:
            # clearing with stock boundaries
            job = PathUtils.findParentJob(obj)
            zmax = stockBB.ZMax
            zmin = job.Proxy.modelBoundBox(job).ZMax

        if OpFeatures.FeatureDepths & self.features:
            # first set update final depth, it's value is not negotiable
            if not PathGeom.isRoughly(obj.OpFinalDepth.Value, zmin):
                obj.OpFinalDepth = zmin
            zmin = obj.OpFinalDepth.Value

            def minZmax(z):
                if (hasattr(obj, 'StepDown') and not
                        PathGeom.isRoughly(obj.StepDown.Value, 0)):
                    return z + obj.StepDown.Value
                else:
                    return z + 1

            # ensure zmax is higher than zmin
            if (zmax - 0.0001) <= zmin:
                zmax = minZmax(zmin)

            # update start depth if requested and required
            if not PathGeom.isRoughly(obj.OpStartDepth.Value, zmax):
                obj.OpStartDepth = zmax
        else:
            # every obj has a StartDepth
            if obj.StartDepth.Value != zmax:
                obj.StartDepth = zmax

        self.opUpdateDepths(obj)

    def _setBaseAndStock(self, obj, ignoreErrors=False):
        job = PathUtils.findParentJob(obj)
        if not job:
            if not ignoreErrors:
                msg = translate("Path", "No parent job found for operation.")
                PathLog.error(msg)
            return False
        if not job.Model.Group:
            if not ignoreErrors:
                msg = translate("Path",
                                "Parent job %s doesn't have a base object") % \
                                job.Label
                PathLog.error(msg)
            return False
        self.job = job
        self.model = job.Model.Group
        self.stock = job.Stock
        return True

    def addBaseProperty(self, obj):
        obj.addProperty("App::PropertyLinkSubListGlobal",
                        "Base", "Path",
                        QT_TRANSLATE("PathOp",
                                     ("The base geometry"
                                      " for this operation")))

    # This method called upon document restoration
    def onDocumentRestored(self, obj):
        # Re-instantiate some attributes
        self._initAttributes(obj)
        self.features = self.opFeatures(obj)
        self.job = PathUtils.findParentJob(obj)

        # Address feature-related needs on restoration,
        #   such as missing properties or values for backward compatibility.
        self.featureManager = self._initFeatures(obj, init=False)
        self.featureManager.onDocumentRestored(self.job, obj, self.features)

        if not hasattr(obj, 'ShowDebugObjects'):
            self._addDebugProperty(obj)

        # Call operation's subclass method for document restoration
        self.opOnDocumentRestored(obj)

        # Set visibility (editor modes)
        self._setStaticEditorModes(obj)

    # Main execution method for the base operation class
    @PathUtils.waiting_effects
    def execute(self, obj):
        '''execute(obj) ... base implementation - do not overwrite!
        Verifies that the operation is assigned to a job and that the job
        also has a valid Base.
        It also sets the following instance variables that can and should be
        safely be used by implementation of opExecute():
            self.commandlist  ... a list to collect all operation's commands
            self.model        ... List of base objects of the Job itself
            self.stock        ... Stock object for the Job itself
            self.tool         ... the actual tool being used
            self.radius       ... the main radius of the tool being used
            Linear feed rates
            self.vertFeed     ... vertical feed rate of assigned tool
            self.vertRapid    ... vertical rapid rate of assigned tool
            self.horizFeed    ... horizontal feed rate of assigned tool
            self.horizRapid   ... horizontal rapid rate of assigned tool
            Rotational feed rates
            self.aAxisFeed    ... A axis feed rate of assigned tool
            self.aAxisRapid   ... A axis rapid rate of assigned tool
            self.bAxisFeed    ... B axis feed rate of assigned tool
            self.bAxisRapid   ... B axis rapid rate of assigned tool
            self.cAxisFeed    ... C axis feed rate of assigned tool
            self.cAxisRapid   ... C axis rapid rate of assigned tool

        Once everything is validated and above variables are set
        the implementation calls opExecute(obj) - which is expected to add
        the generated commands to self.commandlist
        Finally the base implementation adds a rapid move to clearance height
        and assigns the receiver's Path property from the command list.
        '''
        PathLog.track()
        features = self.opFeatures(obj)

        if not obj.Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            return

        if not self._setBaseAndStock(obj):
            return

        if OpFeatures.FeatureCoolant & features:
            if not hasattr(obj, 'CoolantMode'):
                msg = translate("Path",
                                ("No coolant property found."
                                 " Please recreate operation."))
                PathLog.error(msg)

        if OpFeatures.FeatureTool & features:
            tc = obj.ToolController
            if tc is None or tc.ToolNumber == 0:
                msg = translate("Path",
                                ("No Tool Controller is selected."
                                 " We need a tool to build a Path."))
                PathLog.error(msg)
                return
            else:
                self.vertFeed = tc.VertFeed.Value
                self.horizFeed = tc.HorizFeed.Value
                self.vertRapid = tc.VertRapid.Value
                self.horizRapid = tc.HorizRapid.Value

                # These axial feed rates should be set within the operation's code
                # Universal axial feed rates
                self.axialFeed = 0.0
                self.axialRapid = 0.0
                # Axis-specific feed rates
                self.aAxisFeed = 0.0
                self.aAxisRapid = 0.0
                self.bAxisFeed = 0.0
                self.bAxisRapid = 0.0
                self.cAxisFeed = 0.0
                self.cAxisRapid = 0.0

                tool = tc.Proxy.getTool(tc)
                if not tool or float(tool.Diameter) == 0:
                    msg = translate("Path",
                                    ("No Tool found or diameter is zero."
                                     " We need a tool to build a Path."))
                    PathLog.error(msg)
                    return
                self.radius = float(tool.Diameter) / 2.0
                self.tool = tool
                obj.OpToolDiameter = tool.Diameter

        self.updateDepths(obj)
        """
        now that all op values are set make sure the user properties
        get updated accordingly, in case they still have an expression
        referencing any op values
        """
        obj.recompute()

        self.commandlist = []
        self.commandlist.append(Path.Command("(%s)" % obj.Label))
        if obj.Comment:
            self.commandlist.append(Path.Command("(%s)" % obj.Comment))

        result = self.opExecute(obj)

        if OpFeatures.FeatureHeights & features:
            # Let's finish by rapid to clearance...just for safety
            cmdDict = {"Z": obj.ClearanceHeight.Value}
            self.commandlist.append(Path.Command("G0", cmdDict))

        path = Path.Path(self.commandlist)
        obj.Path = path
        obj.CycleTime = self.getCycleTimeEstimate(obj)
        self.job.Proxy.getCycleTime()
        return result

    # Accessory methods for the base operation class
    def getJob(self, obj):
        '''getJob(obj) ... return the job this operation is part of.'''
        if not hasattr(self, 'job') or self.job is None:
            if not self._setBaseAndStock(obj):
                return None
        return self.job

    def getCycleTimeEstimate(self, obj):

        tc = obj.ToolController

        if tc is None or tc.ToolNumber == 0:
            PathLog.error(translate("Path", "No Tool Controller selected."))
            return translate('Path', 'Tool Error')

        hFeedrate = tc.HorizFeed.Value
        vFeedrate = tc.VertFeed.Value
        hRapidrate = tc.HorizRapid.Value
        vRapidrate = tc.VertRapid.Value

        if hFeedrate == 0 or vFeedrate == 0:
            msg = translate("Path",
                            ("Tool Controller feedrates required"
                             " to calculate the cycle time."))
            PathLog.warning(msg)
            return translate('Path', 'Feedrate Error')

        if hRapidrate == 0 or vRapidrate == 0:
            msg = translate("Path",
                            ("Add Tool Controller Rapid Speeds on the "
                             "SetupSheet for more accurate cycle times."))
            PathLog.warning(msg)

        # Get the cycle time in seconds
        seconds = obj.Path.getCycleTime(hFeedrate, vFeedrate,
                                        hRapidrate, vRapidrate)

        if not seconds:
            return translate('Path', 'Cycletime Error')

        # Convert the cycle time to a HH:MM:SS format
        cycleTime = time.strftime("%H:%M:%S", time.gmtime(seconds))

        return cycleTime

    def addBase(self, obj, base, sub):
        PathLog.track(obj, base, sub)
        base = PathUtil.getPublicObject(base)

        if self._setBaseAndStock(obj):
            for model in self.job.Model.Group:
                if base == self.job.Proxy.baseObject(self.job, model):
                    base = model
                    break

            baselist = obj.Base
            if baselist is None:
                baselist = []

            for p, el in baselist:
                if p == base and sub in el:
                    msg = translate("Path",
                                    ("Base object"
                                     " %s.%s "
                                     "already in the list") %
                                    (base.Label, sub))
                    PathLog.notice(msg + "\n")
                    return

            if not self.opRejectAddBase(obj, base, sub):
                baselist.append((base, sub))
                obj.Base = baselist
            else:
                msg = translate("Path",
                                "Base object %s.%s rejected by operation") % \
                                (base.Label, sub)
                PathLog.notice(msg + "\n")

    def _makeTmpDebugGrp(self):
        '''_makeTmpDebugGrp() ... Called to create and manage a single
        temporary Group object for debugging purposes.  The Group object
        is a container for debugging objects created in during execution
        of some operations.'''
        FCAD = FreeCAD.ActiveDocument
        for grpNm in ['tmpDebugGrp', 'tmpDebugGrp001']:
            if hasattr(FCAD, grpNm):
                for go in FCAD.getObject(grpNm).Group:
                    # Clear subgroup objects
                    if hasattr(go, 'Group'):
                        for sgo in go.Group:
                            FCAD.removeObject(sgo.Name)
                    FCAD.removeObject(go.Name)
                FCAD.removeObject(grpNm)
        self.tmpDebugGrp = FCAD.addObject('App::DocumentObjectGroup',
                                          'tmpDebugGrp')

    def _addDebugObject(self, objShape, objName, force=False):
        if self.showDebugObjects or force:
            do = FreeCAD.ActiveDocument.addObject('Part::Feature',
                                                  'tmp_' + objName)
            do.Shape = objShape
            do.purgeTouched()
        if self.showDebugObjects:
            self.tmpDebugGrp.addObject(do)

    # PathWB 1.1 strategy-type, data-driven property management methods
    def createOperationProperties(self, obj, definitions,
                                  enumerations, warn=False):
        '''createOperationProperties(obj) ...
        Create operation properties for the current operation object based on
        a list of tuples - each formated as (prop type, name, group, tooltip).

        This method can be called upon document restoration to verify
        all properties exist. Missing or new properties will be added,
        and their default value assigned.'''
        newProps = list()

        # Create (add) properties to the object received
        for (prtyp, nm, grp, tt) in definitions:
            if not hasattr(obj, nm):
                obj.addProperty(prtyp, nm, grp, tt)
                newProps.append(nm)

        newPropCnt = len(newProps)
        if newPropCnt == 0:
            self.propertiesReady = True
            return False

        # Set enumeration lists for enumeration properties
        for n in enumerations:
            if n in newProps:
                setattr(obj, n, enumerations[n])

        if (warn and not
                (newPropCnt == 1 and newProps[0] == 'ShowDebugObjects')):
            newPropMsg = translate('PathOp', 'New property added to')
            newPropMsg += ' "{}": {}'.format(obj.Label, newProps) + '. '
            newPropMsg += translate('PathOp', 'Check default value(s).')
            PathLog.warning(newPropMsg)

        return newProps

    def setDefaultValuesForProperties(self, obj, propList, defaultValues):
        '''setDefaultValuesForProperties(obj, propList, defaultValues) ...
        Set the default values for the all properties keyed
        in the `defaultValues` dictionary.
        '''
        # Set defaults per obj attribute policy
        if (not hasattr(obj, 'DoNotSetDefaultValues') or
                not obj.DoNotSetDefaultValues):
            for prop in propList:
                setattr(obj, prop, defaultValues[prop])
        self.propertiesReady = True

    def updateEnumerations(self, obj, propEnums):
        '''updateEnumerations(obj, propEnums) ...
        Method updates property enumerations based on active conditions
        in the operation.

        In this method, existing property values must be stored, and then
        restored after the assignment of updated enumerations - initial values
        are not automatically maintained through an enumeration list update.
        Often called by change event, or forced after certain property
        values are modified.

        Only needs to be called in base class if one or more base
        properties require dynamic enumeration lists.
        '''
        PathLog.debug('updateEnumerations()')
        origVals = dict()

        # Save existing values and then update enumerations
        #   for enumeration property
        for key in propEnums:
            # save present value
            origVals[key] = getattr(obj, key)  # obj.getPropertyByName(key)
            # apply new enumerations
            setattr(obj, key, propEnums[key])
            # restore value if available
            if origVals[key] in propEnums[key]:
                setattr(obj, key, origVals[key])
            else:
                # use first in list if original not available
                setattr(obj, key, propEnums[key][0])
# Eclass


"""
Possible PathWB 2.0 Workflow
NOTE: GUI workflow will differ some from that of the commandline
GUI workflow
- Initialize base properties for Base Geometry [BG]
- Analyze Base Geometry [BG]:
    - Feature types (face, edge, vertex) will influence strategies
    - Feature counts will limit strategies
        Examples:
            no BG = profile contour, surface model, waterline model
            one feature = profile, millface, drilling, slot, surface, etc...
            two features = (same as one feature)
            > three features = eliminates slot operation
    - Features creating a closed/open loop will influence strategies
    - Feature orientation will affect the strategies
- Retrieve available strategies based on BG analysis
- Present combobox of available strategies
- Update UI pages based on strategy selection
    - Get features, properties, enumerations, defaults
    - Set features, proeprties, enumerations, defaults
    - Present Depths, Heights, Avoid, and Operation UI panels
"""
