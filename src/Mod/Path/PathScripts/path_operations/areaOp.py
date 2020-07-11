# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

__title__ = "Template class for PathArea based operations."
__author__ = "sliptonic (Brad Collette), russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base class and properties for Path.Area based operations."
__contributors__ = ""

# Standard
# Third-party
# FreeCAD
import PathScripts.path_operations.areaOp_base as PathAreaOpBase

PathOpBase = PathAreaOpBase.PathOp.PathOpBase
# Add pointers to existing imports in PathOpBase
PathLog     = PathOpBase.PathLog
QtCore      = PathOpBase.QtCore


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
PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule()


"""
The `translate` pointer below points to a
`translate(context, text)` function. The function marks the text within
to be translated for users of other languages.
Messages destined for the user should be wrapped within a `translate()`
function call.  The `context` argument should be the module name, and
the text will be your message, both as strings.
An example using a PathLog error level to show an error message is:
`
if toolDiameter > holeDiameter:
    PathLog.error(translate('PathPocket', 'Tool is larger than pocket.'))
`
"""
# Add pointers to existing functions in PathOpBase
# Qt translation handling
translate = PathOpBase.translate


# Include pointer to source function in base module
# --- Delete this pointer upon implementation of this template ---
SetupProperties = PathAreaOpBase.SetupProperties


class ObjectOp(PathAreaOpBase.ObjectOp):
    '''Template class for operations based on
    usage of `Path.Area`.

    This class only contains template, or placeholder, methods
    to be used in related operations. Some of these methods
    must be overwritten in new operations. The remainder are
    only necessary as needed.

    The naming scheme is consistent; all Path.Area operation subclass
    methods begin with the "areaOp" prefix.  There should be a base
    method without the "area" prefix found in the PathAreaOp base class located
    in the `PathScripts/operations/areaOp_base.py` module.  Each base class
    method calls the "areaOp" subclass version found here.
    '''

    # Template (placeholder) methods to be overwritten in operation sub-classes

    # Methods for initializing the operation.
    # This group are all called in the __init__() contstructor
    # of the base operation class, in PathOpBase module.
    def initAreaOp(self, obj):
        '''initAreaOp(obj) ... Overwrite if the receiver class needs
        initialisation. This is method is a subcall (extension) of
        the operation class's __init__() method.
        DIMINISHED: Use areaOpInit() below to maintian naming scheme.'''
        pass

    def areaOpInit(self, obj):
        '''areaOpInit(obj) ... Overwrite if the receiver class needs
        initialisation. This is method is a subcall (extension) of
        the operation class's __init__() method.'''
        pass

    def areaOpInitAttributes(self, obj):
        '''areaOpInitAttributes(obj) ... Overwrite if the receiver class needs
        initialisation of class attributes that will also be required
        upon document restoration. This is method is a subcall (extension) of
        the operation class's __init__() method.
        Can safely be overwritten by subclasses.'''
        pass

    def areaOpFeatures(self, obj):
        '''areaOpFeatures(obj) ...
        Overwrite to add operation-specific features.
        Can safely be overwritten by subclasses.'''
        return 0

    def areaOpPropertyDefinitions(self):
        '''opPropertyDefinitions() ...
        Returns a list of property definition tuples.
        Each tuple contains property definition information for a single
        property in the form of a tuple: (prototype, name, section, tooltip).
        '''
        return []

    def areaOpPropertyEnumerations(self):
        '''areaOpPropertyEnumerations(obj, job) ...
        Returns dictionary of property enumerations.'''
        PathLog.debug('areaOpPropertyEnumerations()')
        # Enumeration lists for App::PropertyEnumeration properties
        return {}

    def areaOpPropertyDefaults(self, obj, job):
        '''areaOpPropertyDefaults(obj, job) ...
        Returns list of property default values as a dictionary.'''
        PathLog.debug('areaOpPropertyDefaults()')
        return {}

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ...
        Overwrite to set initial values of operation-specific properties.
        Should be overwritten by subclasses.'''
        pass

    def areaOpSetStaticEditorModes(self, obj):
        '''areaOpSetStaticEditorModes(self, obj) ...
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
        PathLog.debug('areaOpSetStaticEditorModes()')

    # Maintenance and support methods
    def areaOpSetDynamicEditorModes(self, obj):
        '''areaOpSetDynamicEditorModes(self, obj) ...
        Set editor modes dynamically for all properties whose editor
        mode or visibility might change due to certain changes
        in property settings.
        '''
        PathLog.debug('areaOpSetDynamicEditorModes()')

    def areaOpOnChanged(self, obj, prop):
        '''areaOpOnChanged(obj, porp) ...
        Overwrite to process operation-specific changes to the operation.
        Can safely be overwritten by subclasses.'''
        pass

    def areaOpUpdateDepths(self, obj):
        '''areaOpUpdateDepths(obj) ...
        Can safely be overwritten by subclasses.'''
        pass

    def areaOpRetractTool(self, obj):
        '''areaOpRetractTool(obj) ...
        Return False to keep the tool at current level between shapes.
        Default is True.
        Should be overwritten by subclasses if desired return is False.'''
        return True

    def areaOpAreaParams(self, obj, isHole):
        '''areaOpAreaParams(obj, isHole) ...
        Return operation-specific area parameters in a dictionary.
        Note that the resulting parameters are stored in
        the property AreaParams, which is hidden in the GUI by default.
        Must be overwritten by subclasses.'''
        pass

    def areaOpPathParams(self, obj, isHole):
        '''areaOpPathParams(obj, isHole) ...
        Return operation-specific path parameters in a dictionary.
        Note that the resulting parameters are stored in
        the property PathParams, which is hidden in the GUI by default.
        Must be overwritten by subclasses.'''
        pass

    def areaOpUseProjection(self, obj):
        '''areaOpUseProcjection(obj) ...
        Return True if the operation can use procjection, defaults to False.
        Can safely be overwritten by subclasses.'''
        return False

    def areaOpShapeForDepths(self, obj, job):
        '''areaOpShapeForDepths(obj) ...
        Returns the shape used to make an initial calculation for the depths
        being used. The default implementation returns the job's Base.Shape
        Can safely be overwritten if included calculation is not sufficient.
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

    # Document restoration calls this method
    def areaOpOnDocumentRestored(self, obj):
        '''areaOpOnDocumentRestored(obj) ...
        Should be overwritten to fully restore receiver.'''
        pass

    # Main focus: Return shapes to PathAreaOp
    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ...
        Return all shapes to be processed by Path.Area for this operation.
        Must be overwritten by subclasses.'''
        pass
# Eclass
