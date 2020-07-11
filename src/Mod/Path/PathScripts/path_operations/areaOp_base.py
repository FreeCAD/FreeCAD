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

__title__ = "Base class for PathArea based operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base class and properties for Path.Area based operations."
__contributors__ = ""

# Standard
# Third-party
# FreeCAD
import PathScripts.PathOp as PathOp
import PathScripts.PathGeom as PathGeom

PathOpBase = PathOp.PathOpBase
OpFeatures = PathOpBase.OpFeatures
LazyLoader  = PathOpBase.LazyLoader
# Add pointers to existing imports in PathOpBase
FreeCAD     = PathOpBase.FreeCAD
Path        = PathOpBase.Path
PathLog     = PathOpBase.PathLog
PathUtils   = PathOpBase.PathUtils
QtCore      = PathOpBase.QtCore
# LazyLoader loaded modules
Draft       = LazyLoader('Draft', globals(), 'Draft')
math        = LazyLoader('math', globals(), 'math')
Part        = LazyLoader('Part', globals(), 'Part')
Rotation    = \
    LazyLoader('PathScripts.path_support.rotation',
               globals(), 'PathScripts.path_support.rotation')
if FreeCAD.GuiUp:
    FreeCADGui = LazyLoader('FreeCADGui', globals(), 'FreeCADGui')

# Add pointers to existing functions in OpFeatures
translate = PathOpBase.translate  # Qt translation handling

# Set logging level
PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule()


class ObjectOp(PathOp.ObjectOp):
    '''Base class for all Path.Area based operations.
    Provides standard features including debugging properties AreaParams,
    PathParams and removalshape, all hidden.
    The main reason for existence is to implement the standard interface
    to Path.Area so subclasses only have to provide the shapes, returned
    from areaOpShapes() method in the subclass operations.'''

    # Methods for initializing the operation.
    # This group are all called in the __init__() contstructor
    # of the base operation class, in PathOpBase module.
    def opInit(self, obj):
        '''opInit(obj) ... Contains common instructions
        for execution in __init__() method of operation class and
        calls initAreaOp() in subclass.
        Do not overwrite, overwrite initAreaOp(obj) instead.'''
        PathLog.track()

        self.opInitProperties(obj)

        self.areaOpInit(obj)  # preferred for naming scheme adherence
        self.initAreaOp(obj)  # DIMINISHED for naming scheme

    def opInitAttributes(self, obj):
        '''opInitAttributes(obj) ...
        Set class attributes that must be set at new operation instantiation
        and also upon document restoration.
        '''
        self.initWithRotation = False
        self.areaOpInitAttributes(obj)

    def opFeatures(self, obj):
        '''opFeatures(obj) ...
        Returns the base features supported by all Path.Area based operations.
        The standard feature list is OR'ed with the return value
        of areaOpFeatures().
        Do not overwrite, implement areaOpFeatures(obj) instead.
        '''
        areaOpFeatures = 0
        subFeatures = self.areaOpFeatures(obj)
        if isinstance(areaOpFeatures, list):
            for feat in subFeatures:
                featBit = getattr(PathOp, feat)
                areaOpFeatures = areaOpFeatures | featBit
        else:
            areaOpFeatures = subFeatures

        return PathOp.FeatureTool | PathOp.FeatureDepths \
            | PathOp.FeatureStepDown | PathOp.FeatureHeights \
            | PathOp.FeatureStartPoint | PathOp.FeatureCoolant \
            | areaOpFeatures

    def opInitProperties(self, obj):
        PathLog.debug('opInitProperties()')

        # Debugging
        if not hasattr(obj, 'AreaParams'):
            obj.addProperty("App::PropertyString", "AreaParams", "Path")
            obj.setEditorMode('AreaParams', 2)  # hide
        if not hasattr(obj, 'PathParams'):
            obj.addProperty("App::PropertyString", "PathParams", "Path")
            obj.setEditorMode('PathParams', 2)  # hide
        if not hasattr(obj, 'removalshape'):
            obj.addProperty("Part::PropertyPartShape", "removalshape", "Path")
            obj.setEditorMode('removalshape', 2)  # hide

        # Rotation support
        if not hasattr(obj, 'EnableRotation'):
            obj.addProperty("App::PropertyEnumeration",
                            "EnableRotation",
                            "Rotation",
                            QtCore.QT_TRANSLATE_NOOP("App::Property",
                                                     ("Enable rotation to gain"
                                                      " access to pockets or"
                                                      " areas not normal to"
                                                      " Z axis.")))
            obj.EnableRotation = ['Off', 'A(x)', 'B(y)', 'A & B']

    def opPropertyDefinitions(self):
        '''opPropertyDefinitions() ...
        Returns a list of property definition tuples.
        Each tuple contains property definition information for a single
        property in the form of a tuple: (prototype, name, section, tooltip).
        '''
        propertyDefinitions = []
        # Append property definitions from PathAreaOp subclasses
        if self:
            propertyDefinitions.extend(self.areaOpPropertyDefinitions())
        return propertyDefinitions

    def opPropertyEnumerations(self):
        # Enumeration lists for App::PropertyEnumeration properties
        enums = {}
        if self:
            areaOPE = self.areaOpPropertyEnumerations()
            for key, val in areaOPE.items():
                enums[key] = val
        return enums

    def opPropertyDefaults(self, obj, job):
        '''opPropertyDefaults(obj, job) ...
        Base implementation, do not overwrite.
        Do not overwrite, overwrite areaOpPropertyDefaults(obj, job) instead.
        '''
        PathLog.debug('opPropertyDefaults()')
        features = self.opFeatures(obj)
        defaults = {'EnableRotation': 'Off',
                    'AreaParams': '',
                    'PathParams': ''}

        # Initial setting for EnableRotation is taken
        #    from Job settings/SetupSheet
        # User may override on per-operation basis as needed.
        if hasattr(job.SetupSheet, 'SetupEnableRotation'):
            defaults['EnableRotation'] = job.SetupSheet.SetupEnableRotation

        if PathOp.FeatureDepths & features:
            '''The base implementation sets the depths and heights based on the
            _opShapeForDepths() return value'''
            try:
                shape = self._opShapeForDepths(obj, job)
            except Exception as ee:
                PathLog.error(ee)
                shape = None

            # Set initial start and final depths
            if shape is None:
                PathLog.debug("shape is None")
                startDepth = 1.0
                finalDepth = 0.0
            else:
                bb = job.Stock.Shape.BoundBox
                startDepth = bb.ZMax
                finalDepth = bb.ZMin

            # Adjust start and final depths if rotation is enabled
            if defaults['EnableRotation'] != 'Off':
                self.initWithRotation = True
                self.stockBB = self.job.Stock.Shape.BoundBox
                # Calculate rotational distances/radii
                #    return is list with tuples:
                #    [(xRotRad, yRotRad, zRotRad), (clrOfst, safOfset)]
                opHeights = Rotation.opDetermineRotationRadii(self, obj)
                (xRotRad, yRotRad, zRotRad) = opHeights[0]
                PathLog.debug("opHeights[0]: " + str(opHeights[0]))
                PathLog.debug("opHeights[1]: " + str(opHeights[1]))

                if defaults['EnableRotation'] == 'A(x)':
                    startDepth = xRotRad
                if defaults['EnableRotation'] == 'B(y)':
                    startDepth = yRotRad
                else:
                    startDepth = max(xRotRad, yRotRad)
                finalDepth = -1 * startDepth

            # defaults['StartDepth'] = startDepth
            # defaults['FinalDepth'] = finalDepth
            defaults['OpStartDepth'] = startDepth
            defaults['OpFinalDepth'] = finalDepth

        # Retreive operation-specific defaults and merge them into defaults
        aOPD = self.areaOpPropertyDefaults(obj, job)
        for key, val in aOPD.items():
            defaults[key] = val

        return defaults

    def opSetStaticEditorModes(self, obj):
        '''opSetStaticEditorModes(self, obj, features) ...
        Set editor modes at initialization and document restoration
        for all properties requiring a static editor mode.

        Editor modes are not preserved during document store/restore.

        This method is automatically called at the end of both,
        the constructor and `onDocumentRestored()` methods in the
        base class.
        '''
        PathLog.debug('opSetStaticEditorModes()')
        modesDict = {
            'removalshape': 2,
            'AreaParams': 2,
            'PathParams': 2
        }
        for prop, mode in modesDict.items():
            obj.setEditorMode(prop, mode)

    def opSetEditorModes(self, obj):
        '''opSetEditorModes(self, obj) ...
        Apply editor modes returned by `self.opSetEditorModes(obj)`.
        '''
        pass

    # Maintenance and support methods
    def opOnChanged(self, obj, prop):
        '''opOnChanged(obj, prop) ...
        Base implementation of the notification framework - do not overwrite.
        The base implementation takes a stab at determining Heights and Depths
        if the operations's Base changes.
        Do not overwrite, overwrite areaOpOnChanged(obj, prop) instead.
        '''
        # PathLog.track(obj.Label, prop)
        if prop in ['AreaParams', 'PathParams', 'removalshape']:
            obj.setEditorMode(prop, 2)

        if prop == 'Base' and len(obj.Base) == 1:
            (base, sub) = obj.Base[0]
            bb = base.Shape.BoundBox  # parent boundbox
            subobj = base.Shape.getElement(sub[0])
            fbb = subobj.BoundBox  # feature boundbox

            if hasattr(obj, 'Side'):
                if bb.XLength == fbb.XLength and bb.YLength == fbb.YLength:
                    obj.Side = "Outside"
                else:
                    obj.Side = "Inside"

        self.areaOpOnChanged(obj, prop)

    def opOnDocumentRestored(self, obj):
        for prop in ['AreaParams', 'PathParams', 'removalshape']:
            if hasattr(obj, prop):
                obj.setEditorMode(prop, 2)

        self.opInitProperties(obj)
        self.areaOpOnDocumentRestored(obj)

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj) ... base implementation, do not overwrite.
        Overwrite areaOpSetDefaultValues(obj, job) instead.'''
        PathLog.debug("opSetDefaultValues(%s, %s)" % (obj.Label, job.Label))

        # Initial setting for EnableRotation is taken
        #     from Job settings/SetupSheet
        # User may override on per-operation basis as needed.
        if hasattr(job.SetupSheet, 'SetupEnableRotation'):
            obj.EnableRotation = job.SetupSheet.SetupEnableRotation
        else:
            obj.EnableRotation = 'Off'
        PathLog.debug(("opSetDefaultValues(): Enable Rotation:"
                       " {}".format(obj.EnableRotation)))

        if PathOp.FeatureDepths & self.opFeatures(obj):
            try:
                shape = self.areaOpShapeForDepths(obj, job)
            except Exception as ee:
                PathLog.error(ee)
                shape = None

            # Set initial start and final depths
            if shape is None:
                PathLog.debug("shape is None")
                startDepth = 1.0
                finalDepth = 0.0
            else:
                bb = job.Stock.Shape.BoundBox
                startDepth = bb.ZMax
                finalDepth = bb.ZMin

            # Adjust start and final depths if rotation is enabled
            if obj.EnableRotation != 'Off':
                self.initWithRotation = True
                self.stockBB = self.job.Stock.Shape.BoundBox
                # Calculate rotational distances/radii
                # return is list with tuples:
                #    [(xRotRad, yRotRad, zRotRad), (clrOfst, safOfset)]
                opHeights = self.opDetermineRotationRadii(obj)
                (xRotRad, yRotRad, zRotRad) = opHeights[0]
                PathLog.debug("opHeights[0]: " + str(opHeights[0]))
                PathLog.debug("opHeights[1]: " + str(opHeights[1]))

                if obj.EnableRotation == 'A(x)':
                    startDepth = xRotRad
                if obj.EnableRotation == 'B(y)':
                    startDepth = yRotRad
                else:
                    startDepth = max(xRotRad, yRotRad)
                finalDepth = -1 * startDepth

            obj.StartDepth.Value = startDepth
            obj.FinalDepth.Value = finalDepth
            obj.OpStartDepth.Value = startDepth
            obj.OpFinalDepth.Value = finalDepth

            PathLog.debug(("Default OpDepths are Start: {},"
                           " and Final: {}".format(obj.OpStartDepth.Value,
                                                   obj.OpFinalDepth.Value)))
            PathLog.debug(("Default Depths are Start: {},"
                           " and Final: {}".format(startDepth, finalDepth)))

        self.areaOpSetDefaultValues(obj, job)

    def opUpdateDepths(self, obj):
        self.areaOpUpdateDepths(obj)

    # Main execution method with support methods
    #   for PathAreaOp-based operations
    def opExecute(self, obj, getsim=False):
        '''opExecute(obj, getsim=False) ...
        Implementation of Path.Area ops.
        determines the parameters for _buildPathArea().
        Do not overwrite, implement
            areaOpAreaParams(obj, isHole) ...
                op specific area param dictionary
            areaOpPathParams(obj, isHole) ...
                op specific path param dictionary
            areaOpShapes(obj)             ...
                the shape for path area to process
            areaOpUseProjection(obj)      ...
                return true if operation can use projection instead.
        '''
        PathLog.track()

        # Instantiate class variables for operation reference
        self.endVector = None
        self.rotateFlag = False
        self.leadIn = 2.0
        self.cloneNames = []
        self.guiMsgs = []
        self.tempObjectNames = []
        self.stockBB = self.job.Stock.Shape.BoundBox
        # Clear temporary group and recreate for temp job clones
        Rotation.useTempJobClones(self, 'Delete')
        self.rotStartDepth = None

        if obj.EnableRotation == 'Off':
            strDep = obj.StartDepth.Value
            finDep = obj.FinalDepth.Value
        else:
            # Calculate operation heights based upon rotation radii
            opHeights = Rotation.opDetermineRotationRadii(self, obj)
            (self.xRotRad, self.yRotRad, self.zRotRad) = opHeights[0]
            (self.clrOfset, self.safOfst) = opHeights[1]

            # Set clearance and safe heights based upon rotation radii
            if obj.EnableRotation == 'A(x)':
                strDep = self.xRotRad
            elif obj.EnableRotation == 'B(y)':
                strDep = self.yRotRad
            else:
                strDep = max(self.xRotRad, self.yRotRad)
            finDep = -1 * strDep

            self.rotStartDepth = strDep
            obj.ClearanceHeight.Value = strDep + self.clrOfset
            obj.SafeHeight.Value = strDep + self.safOfst

            # Set axial feed rates based upon horizontal feed rates
            safeCircum = 2 * math.pi * (obj.SafeHeight.Value -
                                        obj.FinalDepth.Value)
            if safeCircum != 0 and self.horizFeed != 0:
                self.axialFeed = 360.0 / safeCircum * self.horizFeed
            if safeCircum != 0 and self.horizRapid != 0:
                self.axialRapid = 360.0 / safeCircum * self.horizRapid

            # Create visual axes when debugging.
            if PathLog.getLevel(PathLog.thisModule()) == 4:
                Rotation.visualAxis(self)

        # Initiate depthparams and calculate operation heights
        #   for rotational operation
        #self.depthparams = self._customDepthParams(obj,
        #                                           obj.StartDepth.Value,
        #                                           obj.FinalDepth.Value)
        self.depthparams = self._customDepthParams(obj, strDep, finDep)

        # Set start point
        if (PathOp.FeatureStartPoint & self.opFeatures(obj) and
                obj.UseStartPoint):
            start = obj.StartPoint
        else:
            start = None

        aOS = self.areaOpShapes(obj)

        # Adjust tuples length received from other PathWB
        #   tools/operations beside PathPocketShape
        shapes = []
        for shp in aOS:
            if len(shp) == 2:
                (fc, iH) = shp
                #     fc, iH,   sub,   angle, axis,
                tup = (fc, iH, 'otherOp', 0.0, 'S',
                       #       strtDep,             finDep
                       obj.StartDepth.Value, obj.FinalDepth.Value)
                shapes.append(tup)
            else:
                shapes.append(shp)

        # Sort paths in multi-path operations
        if len(shapes) > 1:
            jobs = list()
            for s in shapes:
                if s[2] == 'OpenEdge':
                    shp = Part.makeCompound(s[0])
                else:
                    shp = s[0]
                jobs.append({
                    'x': shp.BoundBox.XMax,
                    'y': shp.BoundBox.YMax,
                    'shape': s
                })

            jobs = PathUtils.sort_jobs(jobs, ['x', 'y'])

            shapes = [j['shape'] for j in jobs]

        """
        Send the shapes, and related details in the tuple, to be
        processed into paths by Path.Area.  Rotation commands are
        added as needed during the processing.
        """
        sims = []
        numShapes = len(shapes)
        for ns in range(0, numShapes):
            profileEdgesIsOpen = False
            (shape, isHole, sub, angle, axis, strDep, finDep) = shapes[ns]
            if sub == 'OpenEdge':
                profileEdgesIsOpen = True
                if (PathOp.FeatureStartPoint & self.opFeatures(obj) and
                        obj.UseStartPoint):
                    osp = obj.StartPoint
                    cmdDict = {'X': osp.x, 'Y': osp.y, 'F': self.horizRapid}
                    self.commandlist.append(Path.Command('G0', cmdDict))

            if ns < numShapes - 1:
                nextAxis = shapes[ns + 1][4]
            else:
                nextAxis = 'L'

            self.depthparams = self._customDepthParams(obj, strDep, finDep)

            try:
                if profileEdgesIsOpen:
                    (pp, sim) = self._buildProfileOpenEdges(obj, shape, isHole,
                                                            start, getsim)
                else:
                    (pp, sim) = self._buildPathArea(obj, shape, isHole,
                                                    start, getsim)
            except Exception as e:
                FreeCAD.Console.PrintError(str(e) + '\n')
                FreeCAD.Console.PrintError(('Something unexpected happened.'
                                            ' Check project and tool config.'
                                            '\n'))
            else:
                if profileEdgesIsOpen:
                    ppCmds = pp
                else:
                    ppCmds = pp.Commands
                if obj.EnableRotation != 'Off' and self.rotateFlag is True:
                    # Rotate model to index for cut
                    if axis == 'X':
                        axisOfRot = 'A'
                    elif axis == 'Y':
                        axisOfRot = 'B'
                    elif axis == 'Z':
                        axisOfRot = 'C'
                    else:
                        axisOfRot = 'A'
                    # Rotate Model to correct angle
                    cmdDict = {axisOfRot: angle, 'F': self.axialRapid}
                    ppCmds.insert(0, Path.Command('G0', cmdDict))

                    # Raise cutter to safe height
                    cmdDict = {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}
                    ppCmds.insert(0, Path.Command('G0', cmdDict))

                    # Return start index if axis of rotation changes.
                    if numShapes > 1:
                        if ns != numShapes - 1:
                            if axis != nextAxis:
                                cmdDict = {axisOfRot: 0.0,
                                           'F': self.axialRapid}
                                ppCmds.append(Path.Command('G0', cmdDict))
                # Eif

                # Save gcode commands to object command list
                self.commandlist.extend(ppCmds)
                sims.append(sim)
            # Eif

            if self.areaOpRetractTool(obj) and self.endVector is not None:
                self.endVector[2] = obj.ClearanceHeight.Value
                cmdDict = {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}
                self.commandlist.append(Path.Command('G0', cmdDict))

        """
        This next block of code scans previous operations in the Job.
        It is looking for chains of rotational operations using
        the same axis of rotation.  Within groups of operations
        that use the same axis for rotation, a return to initial
        zero point is omitted, allowing for less travel between
        operations working with the same axis of rotation.
        """
        # Minimize rotations between operations of the same axis.
        if self.rotateFlag is True:
            resetAxis = False
            lastJobOp = None
            nextJobOp = None
            opIdx = 0
            JOB = self.job
            jobOps = JOB.Operations.Group
            numJobOps = len(jobOps)

            for joi in range(0, numJobOps):
                jo = jobOps[joi]
                if jo.Name == obj.Name:
                    opIdx = joi
            lastOpIdx = opIdx - 1
            nextOpIdx = opIdx + 1
            if lastOpIdx > -1:
                lastJobOp = jobOps[lastOpIdx]
            if nextOpIdx < numJobOps:
                nextJobOp = jobOps[nextOpIdx]

            if lastJobOp is not None:
                if hasattr(lastJobOp, 'EnableRotation'):
                    PathLog.debug(('Last Op, {}, has `EnableRotation` set to'
                                   ' {}'.format(lastJobOp.Label,
                                                lastJobOp.EnableRotation)))
                    if lastJobOp.EnableRotation != obj.EnableRotation:
                        resetAxis = True
            # If last shape, check next op EnableRotation setting
            if ns == numShapes - 1:
                if nextJobOp is not None:
                    if hasattr(nextJobOp, 'EnableRotation'):
                        msg = ('Next Op, {}, has `EnableRotation` set to'
                               ' {}'.format(nextJobOp.Label,
                                            nextJobOp.EnableRotation))
                        PathLog.debug(msg)
                        if nextJobOp.EnableRotation != obj.EnableRotation:
                            resetAxis = True

            # Raise to safe height if rotation activated
            cmdDict = {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}
            self.commandlist.append(Path.Command('G0', cmdDict))
            # reset rotational axes if necessary
            if resetAxis is True:
                cmdDict = {'A': 0.0, 'F': self.axialRapid}
                self.commandlist.append(Path.Command('G0', cmdDict))
                cmdDict = {'B': 0.0, 'F': self.axialRapid}
                self.commandlist.append(Path.Command('G0', cmdDict))

        """
        This last block is to clean up the temporary clones that were used
        to create rotational operations.
        """
        # Delete temp job clone group and contents
        Rotation.useTempJobClones(self, 'Delete')
        # Process GUI messages to user
        self.guiMessage('title', None, show=True)
        for ton in self.tempObjectNames:  # remove temporary objects by name
            FreeCAD.ActiveDocument.removeObject(ton)
        PathLog.debug("obj.Name: " + str(obj.Name) + "\n\n")

        return sims

    def _opShapeForDepths(self, obj, job):
        '''_opShapeForDepths(obj) ...
        Returns the shape used to make an initial calculation
        for the depths being used.
        The default implementation returns the job's Base.Shape.
        '''
        if job:
            if job.Stock:
                PathLog.debug("job=%s base=%s shape=%s" % (job,
                                                           job.Stock,
                                                           job.Stock.Shape))
                return job.Stock.Shape
            else:
                msg = translate("PathAreaOp",
                                "job %s has no Base.") % job.Label
                PathLog.warning(msg)
        else:
            msg = translate("PathAreaOp",
                            "no job for op %s found.") % obj.Label
            PathLog.warning(msg)
        return None

    def _buildPathArea(self, obj, baseobject, isHole, start, getsim):
        '''_buildPathArea(obj, baseobject, isHole, start, getsim) ...
        Internal function.
        '''
        PathLog.track()
        PathLog.debug("_buildPathArea()")
        area = Path.Area()
        area.setPlane(PathUtils.makeWorkplane(baseobject))
        area.add(baseobject)

        areaParams = self.areaOpAreaParams(obj, isHole)
        if hasattr(obj, 'ExpandProfile') and obj.ExpandProfile != 0:
            areaParams = self.areaOpAreaParamsExpandProfile(obj, isHole)

        heights = [i for i in self.depthparams]
        PathLog.debug('depths: {}'.format(heights))
        area.setParams(**areaParams)
        obj.AreaParams = str(area.getParams())

        PathLog.debug("Area with params: {}".format(area.getParams()))

        try:
            sections = area.makeSections(mode=0,
                                         project=self.areaOpUseProjection(obj),
                                         heights=heights)
        except Exception as ee:
            PathLog.error(str(ee))
            PathLog.error('Failed to create sections.')
            return False

        PathLog.debug("sections = %s" % sections)
        shapelist = [sec.getShape() for sec in sections]
        PathLog.debug("shapelist = %s" % shapelist)

        pathParams = self.areaOpPathParams(obj, isHole)
        pathParams['shapes'] = shapelist
        pathParams['feedrate'] = self.horizFeed
        pathParams['feedrate_v'] = self.vertFeed
        pathParams['verbose'] = True
        pathParams['resume_height'] = obj.SafeHeight.Value
        pathParams['retraction'] = obj.ClearanceHeight.Value
        pathParams['return_end'] = True
        # Note that emitting preambles between moves breaks some dressups
        #   and prevents path optimization on some controllers
        pathParams['preamble'] = False

        if not self.areaOpRetractTool(obj):
            pathParams['threshold'] = 2.001 * self.radius

        if self.endVector is not None:
            pathParams['start'] = self.endVector
        elif (PathOp.FeatureStartPoint & self.opFeatures(obj) and
                obj.UseStartPoint):
            pathParams['start'] = obj.StartPoint

        paramDict = {key: value for key, value in pathParams.items() if key != 'shapes'}
        obj.PathParams = str(paramDict)
        PathLog.debug("Path with params: {}".format(obj.PathParams))

        (pp, end_vector) = Path.fromShapes(**pathParams)
        PathLog.debug('pp: {}, end vector: {}'.format(pp, end_vector))
        self.endVector = end_vector

        simobj = None
        if getsim:
            areaParams['Thicken'] = True
            areaParams['ToolRadius'] = self.radius - self.radius * .005
            area.setParams(**areaParams)
            sec = area.makeSections(mode=0,
                                    project=False,
                                    heights=heights)[-1].getShape()
            simobj = sec.extrude(FreeCAD.Vector(0.0,
                                                0.0,
                                                baseobject.BoundBox.ZMax))

        return pp, simobj

    def _buildProfileOpenEdges(self, obj, edgeList, isHole, start, getsim):
        '''_buildProfileOpenEdges(obj, edgeList, isHole, start, getsim) ...
        Internal function.
        '''
        PathLog.track()
        PathLog.debug("_buildProfileOpenEdges()")

        paths = []
        heights = [i for i in self.depthparams]
        PathLog.debug('depths: {}'.format(heights))
        lstIdx = len(heights) - 1
        for i in range(0, len(heights)):
            for baseShape in edgeList:
                hWire = Part.Wire(Part.__sortEdges__(baseShape.Edges))
                transVect = FreeCAD.Vector(0.0,
                                           0.0,
                                           heights[i] - hWire.BoundBox.ZMin)
                hWire.translate(transVect)

                pathParams = {}
                pathParams['shapes'] = [hWire]
                pathParams['feedrate'] = self.horizFeed
                pathParams['feedrate_v'] = self.vertFeed
                pathParams['verbose'] = True
                pathParams['resume_height'] = obj.SafeHeight.Value
                pathParams['retraction'] = obj.ClearanceHeight.Value
                pathParams['return_end'] = True
                # Note that emitting preambles between moves breaks
                #   some dressups and prevents path optimization
                #   on some controllers
                pathParams['preamble'] = False

                if self.endVector is None:
                    V = hWire.Wires[0].Vertexes
                    lv = len(V) - 1
                    pathParams['start'] = FreeCAD.Vector(V[0].X,
                                                         V[0].Y,
                                                         V[0].Z)
                    if obj.Direction == 'CCW':
                        pathParams['start'] = FreeCAD.Vector(V[lv].X,
                                                             V[lv].Y,
                                                             V[lv].Z)
                else:
                    pathParams['start'] = self.endVector

                paramDict = {key: value for key, value in pathParams.items() if key != 'shapes'}
                obj.PathParams = str(paramDict)
                PathLog.debug("Path with params: {}".format(obj.PathParams))

                (pp, end_vector) = Path.fromShapes(**pathParams)
                paths.extend(pp.Commands)
                if end_vector:
                    msg = 'pp: {}, end vector: {}'.format(pp, end_vector)
                    PathLog.debug(msg)

        if end_vector:
            self.endVector = end_vector

        simobj = None
        if getsim and False:
            areaParams['ToolRadius'] = self.radius - self.radius * .005
            area.setParams(**areaParams)
            sec = area.makeSections(mode=0,
                                    project=False,
                                    heights=heights)[-1].getShape()
            simobj = sec.extrude(FreeCAD.Vector(0.0,
                                                0.0,
                                                baseobject.BoundBox.ZMax))

        return paths, simobj

    # Independent support methods
    def guiMessage(self, title, msg, show=False):
        '''guiMessage(title, msg, show=False) ...
        Handle op related GUI messages to user.'''
        if msg is not None:
            self.guiMsgs.append((title, msg))
        if show is True:
            if len(self.guiMsgs) > 0:
                if FreeCAD.GuiUp:
                    from PySide.QtGui import QMessageBox
                    for entry in self.guiMsgs:
                        (title, msg) = entry
                        QMessageBox.warning(None, title, msg)
                    self.guiMsgs = []
                    return True
                else:
                    for entry in self.guiMsgs:
                        (title, msg) = entry
                        PathLog.warning("{}:: {}".format(title, msg))
                    self.guiMsgs = []
                    return True
        return False

    def _customDepthParams(self, obj, strDep, finDep):
        '''_customDepthParams(obj, strDep, finDep) ...
        Calculate a list of depth parameters based on the provided
        start depth (strDep) and final depth (finDep) arguments.'''
        finish_step = 0.0
        if hasattr(obj, "FinishDepth"):
            finish_step = obj.FinishDepth.Value
        cdp = PathUtils.depth_params(
            clearance_height=obj.ClearanceHeight.Value,
            safe_height=obj.SafeHeight.Value,
            start_depth=strDep,
            step_down=obj.StepDown.Value,
            z_finish_step=finish_step,
            final_depth=finDep,
            user_depths=None)
        return cdp
# Eclass


def SetupProperties():
    # setup = ['EnableRotation']
    setup = [tup[1] for tup in ObjectOp.opPropertyDefinitions(False)]
    return setup
