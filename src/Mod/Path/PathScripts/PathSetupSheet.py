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

import FreeCAD
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathUtil as PathUtil
import PySide

from PathScripts.PathGeom import PathGeom


__title__ = "Setup Sheet for a Job."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "A container for all default values and job specific configuration values."

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule()
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

def translate(context, text, disambig=None):
    return PySide.QtCore.QCoreApplication.translate(context, text, disambig)

class Template:
    HorizRapid = 'HorizRapid'
    VertRapid = 'VertRapid'
    SafeHeightOffset = 'SafeHeightOffset'
    SafeHeightExpression = 'SafeHeightExpression'
    ClearanceHeightOffset = 'ClearanceHeightOffset'
    ClearanceHeightExpression = 'ClearanceHeightExpression'
    StartDepthExpression = 'StartDepthExpression'
    FinalDepthExpression = 'FinalDepthExpression'
    StepDownExpression = 'StepDownExpression'

    All = [HorizRapid, VertRapid, SafeHeightOffset, SafeHeightExpression, ClearanceHeightOffset, ClearanceHeightExpression, StartDepthExpression, FinalDepthExpression, StepDownExpression]


def _traverseTemplateAttributes(attrs, codec):
    coded = {}
    for key,value in PathUtil.keyValueIter(attrs):
        if type(value) == dict:
            PathLog.debug("%s is a dict" % key)
            coded[key] = _traverseTemplateAttributes(value, codec)
        elif type(value) == list:
            PathLog.debug("%s is a list" % key)
            coded[key] = [_traverseTemplateAttributes(attr, codec) for attr in value]
        elif PathUtil.isString(value):
            PathLog.debug("%s is a string" % key)
            coded[key] = codec(value)
        else:
            PathLog.debug("%s is %s" % (key, type(value)))
            coded[key] = value
    return coded

class SetupSheet:
    '''Property container object used by a Job to hold global reference values. '''

    TemplateReference = '${SetupSheet}'

    DefaultSafeHeightOffset      = '3 mm'
    DefaultClearanceHeightOffset = '5 mm'
    DefaultSafeHeightExpression      = "StartDepth+${SetupSheet}.SafeHeightOffset"
    DefaultClearanceHeightExpression = "StartDepth+${SetupSheet}.ClearanceHeightOffset"

    DefaultStartDepthExpression = 'OpStartDepth'
    DefaultFinalDepthExpression = 'OpFinalDepth'
    DefaultStepDownExpression   = 'OpToolDiameter'

    def __init__(self, obj):
        self.obj = obj
        obj.addProperty('App::PropertySpeed', 'VertRapid',  'ToolController', translate('PathSetupSheet', 'Default speed for horizontal rapid moves.'))
        obj.addProperty('App::PropertySpeed', 'HorizRapid', 'ToolController', translate('PathSetupSheet', 'Default speed for vertical rapid moves.'))

        obj.addProperty('App::PropertyLength', 'SafeHeightOffset',          'OperationHeights', translate('PathSetupSheet', 'The usage of this field depends on SafeHeightExpression - by default its value is added to StartDepth and used for SafeHeight of an operation.'))
        obj.addProperty('App::PropertyString', 'SafeHeightExpression',      'OperationHeights', translate('PathSetupSheet', 'Expression set for the SafeHeight of new operations.'))
        obj.addProperty('App::PropertyLength', 'ClearanceHeightOffset',     'OperationHeights', translate('PathSetupSheet', 'The usage of this field depends on ClearanceHeightExpression - by default is value is added to StartDepth and used for ClearanceHeight of an operation.'))
        obj.addProperty('App::PropertyString', 'ClearanceHeightExpression', 'OperationHeights', translate('PathSetupSheet', 'Expression set for the ClearanceHeight of new operations.'))

        obj.addProperty('App::PropertyString', 'StartDepthExpression', 'OperationDepths', translate('PathSetupSheet', 'Expression used for StartDepth of new operations.'))
        obj.addProperty('App::PropertyString', 'FinalDepthExpression', 'OperationDepths', translate('PathSetupSheet', 'Expression used for FinalDepth of new operations.'))
        obj.addProperty('App::PropertyString', 'StepDownExpression',   'OperationDepths', translate('PathSetupSheet', 'Expression used for StepDown of new operations.'))

        obj.SafeHeightOffset          = self.decodeAttributeString(self.DefaultSafeHeightOffset)
        obj.ClearanceHeightOffset     = self.decodeAttributeString(self.DefaultClearanceHeightOffset)
        obj.SafeHeightExpression      = self.decodeAttributeString(self.DefaultSafeHeightExpression)
        obj.ClearanceHeightExpression = self.decodeAttributeString(self.DefaultClearanceHeightExpression)

        obj.StartDepthExpression = self.decodeAttributeString(self.DefaultStartDepthExpression)
        obj.FinalDepthExpression = self.decodeAttributeString(self.DefaultFinalDepthExpression)
        obj.StepDownExpression   = self.decodeAttributeString(self.DefaultStepDownExpression)

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        for obj in FreeCAD.ActiveDocument.Objects:
            if hasattr(obj, 'Proxy') and obj.Proxy == self:
                self.obj = obj
                break
        return None

    def hasDefaultToolRapids(self):
        return PathGeom.isRoughly(self.obj.VertRapid.Value, 0) and PathGeom.isRoughly(self.obj.HorizRapid.Value, 0)

    def hasDefaultOperationHeights(self):
        if self.obj.SafeHeightOffset.UserString != FreeCAD.Units.Quantity(self.DefaultSafeHeightOffset).UserString:
            return False
        if self.obj.ClearanceHeightOffset.UserString != FreeCAD.Units.Quantity(self.DefaultClearanceHeightOffset).UserString:
            return False
        if self.obj.SafeHeightExpression != self.decodeAttributeString(self.DefaultSafeHeightExpression):
            return False
        if self.obj.ClearanceHeightExpression != self.decodeAttributeString(self.DefaultClearanceHeightExpression):
            return False
        return True

    def hasDefaultOperationDepths(self):
        if self.obj.StartDepthExpression != self.DefaultStartDepthExpression:
            return False
        if self.obj.FinalDepthExpression != self.DefaultFinalDepthExpression:
            return False
        if self.obj.StepDownExpression != self.DefaultStepDownExpression:
            return False
        return True

    def setFromTemplate(self, attrs):
        '''setFromTemplate(attrs) ... sets the default values from the given dictionary.'''
        for name in Template.All:
            if attrs.get(name) is not None:
                setattr(self.obj, name, attrs[name])

    def templateAttributes(self, includeRapids=True, includeHeights=True, includeDepths=True):
        '''templateAttributes(includeRapids, includeHeights, includeDepths) ... answers a dictionary with the default values.'''
        attrs = {}
        if includeRapids:
            attrs[Template.VertRapid]  = self.obj.VertRapid.UserString
            attrs[Template.HorizRapid] = self.obj.HorizRapid.UserString
        if includeHeights:
            attrs[Template.SafeHeightOffset]          = self.obj.SafeHeightOffset.UserString
            attrs[Template.SafeHeightExpression]      = self.obj.SafeHeightExpression
            attrs[Template.ClearanceHeightOffset]     = self.obj.ClearanceHeightOffset.UserString
            attrs[Template.ClearanceHeightExpression] = self.obj.ClearanceHeightExpression
        if includeDepths:
            attrs[Template.StartDepthExpression] = self.obj.StartDepthExpression
            attrs[Template.FinalDepthExpression] = self.obj.FinalDepthExpression
            attrs[Template.StepDownExpression]   = self.obj.StepDownExpression
        return attrs

    def expressionReference(self):
        '''expressionReference() ... returns the string to be used in expressions'''
        # Using the Name here and not the Label (both would be valid) because the Name 'fails early'.
        #
        # If there is a Name/Label conflict and an expression is bound to the Name we'll get an error
        # on creation (Property not found). Not good, but at least there's some indication that
        # something's afoul.
        #
        # If the expression is based on the Label everything works out nicely - until the document is
        # saved and loaded from disk. The Labels change in order to avoid the Name/Label conflict
        # but the expression stays the same. If the user's lucky the expression is broken because the
        # conflicting object doesn't have the properties reference by the expressions. If the user is
        # not so lucky those properties also exist in the other object, there is no indication that
        # anything is wrong but the expressions will substitute the values from the wrong object.
        #
        # I prefer the question: "why do I get this error when I create ..." over "my cnc machine just
        # rammed it's tool head into the table ..." or even "I saved my file and now it's corrupt..."
        #
        # https://forum.freecadweb.org/viewtopic.php?f=10&t=24839
        # https://forum.freecadweb.org/viewtopic.php?f=10&t=24845
        return self.obj.Name

    def encodeAttributeString(self, attr):
        '''encodeAttributeString(attr) ... return the encoded string of a template attribute.'''
        return PathUtil.toUnicode(attr.replace(self.expressionReference(), self.TemplateReference))
    def decodeAttributeString(self, attr):
        '''decodeAttributeString(attr) ... return the decoded string of a template attribute.'''
        return PathUtil.toUnicode(attr.replace(self.TemplateReference, self.expressionReference()))

    def encodeTemplateAttributes(self, attrs):
        '''encodeTemplateAttributes(attrs) ... return a dictionary with all values encoded.'''
        return _traverseTemplateAttributes(attrs, self.encodeAttributeString)

    def decodeTemplateAttributes(self, attrs):
        '''decodeTemplateAttributes(attrs) ... expand template attributes to reference the receiver where applicable.'''
        return _traverseTemplateAttributes(attrs, self.decodeAttributeString)


def Create(name='SetupSheet'):
    obj = FreeCAD.ActiveDocument.addObject('App::FeaturePython', name)
    proxy = SetupSheet(obj)
    return obj
