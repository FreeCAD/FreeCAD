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

__title__ = "Template class for all standard operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Template class for all standard Path operations."
__contributors__ = "russ4262 (Russell Johnson)"

# Standard
# Third-party
# FreeCAD
import PathScripts.PathOpBase as PathOpBase

"""
This setLevel() method call on PathLog sets the current logging level
for this module: INFO.
To change to debug logging, change 'INFO' to 'DEBUG'.
Placement of `PathLog.debug('Debug message here...')` calls within
the code will be not be printed to the Report View or Console, unless
you set the logging level to DEBUG as stated above.
See the PathLog.py module for more information about its usage
and the various log levels available.
"""
PathLog = PathOpBase.PathLog
PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule()


"""
The `translate()` function marks the text passed to it to be translated
for users of other languages.
Messages destined for the user should be wrapped within a `translate()`
function call.  The `context` argument should be the module name, and
the text will be your message, both as strings.
An example using a PathLog error level to show an error message like:
`
if error:
    PathLog.error(translate('PathPocket', 'Tool is larger than pocket.'))
`
"""
# Qt translation handling through PathOpBase.translate() function
translate = PathOpBase.translate


# Pointers to source Feature declarations found in OpFeatures module
# These are required here for backward compatibility
# --- Delete these pointers upon implementation of this template ---
OpFeatures = PathOpBase.OpFeatures

FeatureTool = OpFeatures.FeatureTool
FeatureDepths = OpFeatures.FeatureDepths
FeatureHeights = OpFeatures.FeatureHeights
FeatureStartPoint = OpFeatures.FeatureStartPoint
FeatureFinishDepth = OpFeatures.FeatureFinishDepth
FeatureStepDown = OpFeatures.FeatureStepDown
FeatureNoFinalDepth = OpFeatures.FeatureNoFinalDepth
FeatureBaseVertexes = OpFeatures.FeatureBaseVertexes
FeatureBaseEdges = OpFeatures.FeatureBaseEdges
FeatureBaseFaces = OpFeatures.FeatureBaseFaces
FeatureBasePanels = OpFeatures.FeatureBasePanels
FeatureLocations = OpFeatures.FeatureLocations
FeatureCoolant = OpFeatures.FeatureCoolant

FeatureBaseGeometry = OpFeatures.FeatureBaseGeometry
DefaultFeatures = OpFeatures.DefaultFeatures


class ObjectOp(PathOpBase.ObjectOpBase):
    '''Template class for all standard operation classes.

    This class only contains template, or placeholder, methods
    to be used in standard operations. Some of these methods
    must be overwritten in new operations. The remainder are
    only necessary as needed.

    The naming scheme is consistent; all operation subclass methods
    begin with the lowercase "op" prefix.  There should be a base
    method without the "op" prefix found in the base class located
    in the PathOpBase module.  Each base class method calls the
    "op" subclass version found here.
    '''

    def __getstate__(self):
        '''
        __getstat__(self) ... called when receiver is saved.
        Can safely be overwritten by subclasses.
        '''
        return None

    def __setstate__(self, state):
        '''
        __getstat__(self) ... called when receiver is restored.
        Likely called farther upstream, before the
        `opOnDocumentRestored(obj)` method included below.
        Can safely be overwritten by subclasses.
        '''
        return None

    # Methods for initializing the operation.
    # This group are all called in the __init__() contstructor
    # of the base operation class, in PathOpBase module.
    def initOperation(self, obj):
        '''
        initOperation(obj) ...
        Previously implemented to create additional properties as
        an extension to the base __init__() constructor.
        DIMINISHED: Please use `opInit()` instead in order to maintain
        a consistent nameing convention, using the "op" prefix.
        '''
        PathLog.debug('initOperation()')

    def opInit(self, obj):
        '''
        opInit(obj) ...
        Subclass extension of `__init__()` constructor in base class.
        Implement to create additional properties.
        Replacement for original `initOperation()`.

        Caller: __init__() of base class (after attributes and features).

        Should be overwritten by subclasses.
        '''
        PathLog.debug('opInit()')

    def opInitAttributes(self, obj):
        '''
        opInitAttributes(obj) ... Set class attributes that must be set
        at new class instantiation and also upon document restoration.
        This method is called early in the base `__init__()` constructor,
        as well as early in the `onDocumentRestored()` method.

        Caller: __init__() of base class (near beginning).

        Should be overwritten by sub-class.
        '''
        PathLog.debug('opInitAttributes()')

    def opFeatures(self, obj):
        '''
        opFeatures(obj) ...
        Returns the OR'ed list of features used and supported by the operation.
        The default implementation returns: FeatureTool, FeatureDepths,
        FeatureHeights, FeatureStartPoint, FeatureBaseGeometry,
        FeatureFinishDepth, and FeatureCoolant.

        Callers:
            __init__() of base class (with `initAttributes()`).
            onDocumentRestored() of base class (at beginning).


        Should be overwritten by subclasses.
        '''
        PathLog.debug('opFeatures()')
        return DefaultFeatures

    def opInitEnd(self, obj):
        '''
        opInitEnd(obj) ...
        Subclass extension of `__init__()` constructor in base class.
        This is the final call within the base `__init__()` constructor.

        Caller: __init__() of base class (penultimate call)

        Can safely be overwritten by subclasses.
        '''
        PathLog.debug('opInitEnd()')

    def opSetDefaultValues(self, obj, job):
        '''
        opSetDefaultValues(obj, job) ...
        Overwrite to set initial default values.
        Called after the receiver has been fully created with all properties.

        Caller: __init__() of base class (prior to `opInitEnd()`).

        Can safely be overwritten by subclasses.
        '''
        PathLog.debug('opSetDefaultValues()')

    def opSetStaticEditorModes(self, obj):
        '''opSetStaticEditorModes(self, obj, features) ...
        Set editor modes at initialization and document restoration
        for all properties requiring a static editor mode.

        Editor modes are not preserved during document store/restore.

        This method is automatically called at the end of both,
        the constructor and `onDocumentRestored()` methods in the
        base class.

        Callers:
            __init__() of base class (last call).
            onDocumentRestored() of base class (at end).

        Can safely be overwritten.
        '''
        PathLog.debug('opSetStaticEditorModes()')

    # Maintenance and support methods
    def opOnChanged(self, obj, prop):
        '''
        opOnChanged(obj, prop) ... Overwrite to process property changes.
        This is a callback function that is invoked each time a property of the
        receiver is assigned a value. Note that the FC framework does not
        distinguish between assigning a different value and assigning the same
        value again.
        Can safely be overwritten by subclasses.
        '''
        PathLog.debug('opOnChanged()')

    def opUpdateDepths(self, obj):
        '''
        opUpdateDepths(obj) ...
        Overwrite to implement special depths calculation.
        This method is called early in the primary `execute()` method
        of the base operation class, in the PathOpBase module.
        Can safely be overwritten by subclass.
        '''
        PathLog.debug('opUpdateDepths()')

    def opRejectAddBase(self, obj, base, sub):
        '''
        opRejectAddBase(base, sub) ...
        If op returns True the addition of the feature is prevented.
        Pertains to Base Geometry feature.
        Should be overwritten by subclasses.
        '''
        PathLog.debug('opRejectAddBase()')
        return False

    # Document restoration calls this method
    def opOnDocumentRestored(self, obj):
        '''
        opOnDocumentRestored(obj) ... Implement if an operation needs
        special handling, like migrating the data model, or setting
        initial editor modes (visibility) upon document restoration.
        Should be overwritten by subclasses.
        '''
        PathLog.debug('opOnDocumentRestored()')

    # Primary method to contain actual operation actions
    def opExecute(self, obj):
        '''
        opExecute(obj) ...
        Called whenever the receiver needs to be recalculated.
        See documentation of `execute()` for a list of
        base functionality provided.
        Should be overwritten by subclasses.
        '''
        PathLog.debug('opExecute()')
# Eclass
