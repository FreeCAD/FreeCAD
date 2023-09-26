# -*- coding: utf-8 -*-
# ***************************************************************************
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
import Path.Base.Util as PathUtil
import Path.Base.SetupSheetOpPrototype as PathSetupSheetOpPrototype
from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "Setup Sheet for a Job."
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "A container for all default values and job specific configuration values."

_RegisteredOps: dict = {}


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class Template:

    HorizRapid = "HorizRapid"
    VertRapid = "VertRapid"
    CoolantMode = "CoolantMode"
    SafeHeightOffset = "SafeHeightOffset"
    SafeHeightExpression = "SafeHeightExpression"
    ClearanceHeightOffset = "ClearanceHeightOffset"
    ClearanceHeightExpression = "ClearanceHeightExpression"
    StartDepthExpression = "StartDepthExpression"
    FinalDepthExpression = "FinalDepthExpression"
    StepDownExpression = "StepDownExpression"
    Fixtures = "Fixtures"
    OrderOutputBy = "OrderOutputBy"
    SplitOutput = "SplitOutput"

    All = [
        HorizRapid,
        VertRapid,
        CoolantMode,
        SafeHeightOffset,
        SafeHeightExpression,
        ClearanceHeightOffset,
        ClearanceHeightExpression,
        StartDepthExpression,
        FinalDepthExpression,
        StepDownExpression,
    ]


def _traverseTemplateAttributes(attrs, codec):
    Path.Log.debug(attrs)
    coded = {}
    for key, value in attrs.items():
        if type(value) == dict:
            Path.Log.debug("%s is a dict" % key)
            coded[key] = _traverseTemplateAttributes(value, codec)
        elif type(value) == list:
            Path.Log.debug("%s is a list" % key)
            coded[key] = [_traverseTemplateAttributes(attr, codec) for attr in value]
        elif isinstance(value, str):
            Path.Log.debug("%s is a string" % key)
            coded[key] = codec(value)
        else:
            Path.Log.debug("%s is %s" % (key, type(value)))
            coded[key] = value
    return coded


class SetupSheet:
    """Property container object used by a Job to hold global reference values."""

    TemplateReference = "${SetupSheet}"

    DefaultSafeHeightOffset = "3 mm"
    DefaultClearanceHeightOffset = "5 mm"
    DefaultSafeHeightExpression = "OpStockZMax+${SetupSheet}.SafeHeightOffset"
    DefaultClearanceHeightExpression = "OpStockZMax+${SetupSheet}.ClearanceHeightOffset"

    DefaultStartDepthExpression = "OpStartDepth"
    DefaultFinalDepthExpression = "OpFinalDepth"
    DefaultStepDownExpression = "OpToolDiameter"

    DefaultCoolantModes = ["None", "Flood", "Mist"]

    def __init__(self, obj):
        self.obj = obj
        obj.addProperty(
            "App::PropertySpeed",
            "VertRapid",
            "ToolController",
            QT_TRANSLATE_NOOP(
                "App::Property", "Default speed for horizontal rapid moves."
            ),
        )
        obj.addProperty(
            "App::PropertySpeed",
            "HorizRapid",
            "ToolController",
            QT_TRANSLATE_NOOP(
                "App::Property", "Default speed for vertical rapid moves."
            ),
        )
        obj.addProperty(
            "App::PropertyStringList",
            "CoolantModes",
            "CoolantMode",
            QT_TRANSLATE_NOOP("App::Property", "Coolant Modes"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "CoolantMode",
            "CoolantMode",
            QT_TRANSLATE_NOOP("App::Property", "Default coolant mode."),
        )
        obj.addProperty(
            "App::PropertyLength",
            "SafeHeightOffset",
            "OperationHeights",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The usage of this field depends on SafeHeightExpression - by default its value is added to the start depth and used for the safe height of an operation.",
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "SafeHeightExpression",
            "OperationHeights",
            QT_TRANSLATE_NOOP(
                "App::Property", "Expression for the safe height of new operations."
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "ClearanceHeightOffset",
            "OperationHeights",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The usage of this field depends on ClearanceHeightExpression - by default is value is added to the start depth and used for the clearance height of an operation.",
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "ClearanceHeightExpression",
            "OperationHeights",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Expression for the clearance height of new operations.",
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "StartDepthExpression",
            "OperationDepths",
            QT_TRANSLATE_NOOP(
                "App::Property", "Expression used for the start depth of new operations."
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "FinalDepthExpression",
            "OperationDepths",
            QT_TRANSLATE_NOOP(
                "App::Property", "Expression used for the final depth of new operations."
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "StepDownExpression",
            "OperationDepths",
            QT_TRANSLATE_NOOP(
                "App::Property", "Expression used for step down of new operations."
            ),
        )

        obj.SafeHeightOffset = self.decodeAttributeString(self.DefaultSafeHeightOffset)
        obj.ClearanceHeightOffset = self.decodeAttributeString(
            self.DefaultClearanceHeightOffset
        )
        obj.SafeHeightExpression = self.decodeAttributeString(
            self.DefaultSafeHeightExpression
        )
        obj.ClearanceHeightExpression = self.decodeAttributeString(
            self.DefaultClearanceHeightExpression
        )

        obj.StartDepthExpression = self.decodeAttributeString(
            self.DefaultStartDepthExpression
        )
        obj.FinalDepthExpression = self.decodeAttributeString(
            self.DefaultFinalDepthExpression
        )
        obj.StepDownExpression = self.decodeAttributeString(
            self.DefaultStepDownExpression
        )

        obj.CoolantModes = self.DefaultCoolantModes
        obj.CoolantMode = self.DefaultCoolantModes

        obj.Proxy = self

    def dumps(self):
        return None

    def loads(self, state):
        for obj in FreeCAD.ActiveDocument.Objects:
            if hasattr(obj, "Proxy") and obj.Proxy == self:
                self.obj = obj
                break
        return None

    def hasDefaultToolRapids(self):
        return Path.Geom.isRoughly(self.obj.VertRapid.Value, 0) and Path.Geom.isRoughly(
            self.obj.HorizRapid.Value, 0
        )

    def hasDefaultOperationHeights(self):
        if (
            self.obj.SafeHeightOffset.UserString
            != FreeCAD.Units.Quantity(self.DefaultSafeHeightOffset).UserString
        ):
            return False
        if (
            self.obj.ClearanceHeightOffset.UserString
            != FreeCAD.Units.Quantity(self.DefaultClearanceHeightOffset).UserString
        ):
            return False
        if self.obj.SafeHeightExpression != self.decodeAttributeString(
            self.DefaultSafeHeightExpression
        ):
            return False
        if self.obj.ClearanceHeightExpression != self.decodeAttributeString(
            self.DefaultClearanceHeightExpression
        ):
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

    def hasDefaultCoolantMode(self):
        return self.obj.CoolantMode == "None"

    def setFromTemplate(self, attrs):
        """setFromTemplate(attrs) ... sets the default values from the given dictionary."""
        for name in Template.All:
            if attrs.get(name) is not None:
                setattr(self.obj, name, attrs[name])

        for opName, op in _RegisteredOps.items():
            opSetting = attrs.get(opName)
            if opSetting is not None:
                prototype = op.prototype(opName)
                for propName in op.properties():
                    value = opSetting.get(propName)
                    if value is not None:
                        prop = prototype.getProperty(propName)
                        propertyName = OpPropertyName(opName, propName)
                        propertyGroup = OpPropertyGroup(opName)
                        prop.setupProperty(
                            self.obj,
                            propertyName,
                            propertyGroup,
                            prop.valueFromString(value),
                        )

    def templateAttributes(
        self,
        includeRapids=True,
        includeCoolantMode=True,
        includeHeights=True,
        includeDepths=True,
        includeOps=None,
    ):
        """templateAttributes(includeRapids, includeHeights, includeDepths) ... answers a dictionary with the default values."""
        attrs = {}

        if includeRapids:
            attrs[Template.VertRapid] = self.obj.VertRapid.UserString
            attrs[Template.HorizRapid] = self.obj.HorizRapid.UserString

        if includeCoolantMode:
            attrs[Template.CoolantMode] = self.obj.CoolantMode

        if includeHeights:
            attrs[Template.SafeHeightOffset] = self.obj.SafeHeightOffset.UserString
            attrs[Template.SafeHeightExpression] = self.obj.SafeHeightExpression
            attrs[
                Template.ClearanceHeightOffset
            ] = self.obj.ClearanceHeightOffset.UserString
            attrs[
                Template.ClearanceHeightExpression
            ] = self.obj.ClearanceHeightExpression

        if includeDepths:
            attrs[Template.StartDepthExpression] = self.obj.StartDepthExpression
            attrs[Template.FinalDepthExpression] = self.obj.FinalDepthExpression
            attrs[Template.StepDownExpression] = self.obj.StepDownExpression

        if includeOps:
            for opName in includeOps:
                settings = {}
                op = _RegisteredOps[opName]
                for propName in op.properties():
                    prop = OpPropertyName(opName, propName)
                    if hasattr(self.obj, prop):
                        settings[propName] = PathUtil.getPropertyValueString(
                            self.obj, prop
                        )
                attrs[opName] = settings

        return attrs

    def expressionReference(self):
        """expressionReference() ... returns the string to be used in expressions"""
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
        # https://forum.freecad.org/viewtopic.php?f=10&t=24839
        # https://forum.freecad.org/viewtopic.php?f=10&t=24845
        return self.obj.Name

    def encodeAttributeString(self, attr):
        """encodeAttributeString(attr) ... return the encoded string of a template attribute."""
        return str(
            attr.replace(self.expressionReference(), self.TemplateReference)
        )

    def decodeAttributeString(self, attr):
        """decodeAttributeString(attr) ... return the decoded string of a template attribute."""
        return str(
            attr.replace(self.TemplateReference, self.expressionReference())
        )

    def encodeTemplateAttributes(self, attrs):
        """encodeTemplateAttributes(attrs) ... return a dictionary with all values encoded."""
        return _traverseTemplateAttributes(attrs, self.encodeAttributeString)

    def decodeTemplateAttributes(self, attrs):
        """decodeTemplateAttributes(attrs) ... expand template attributes to reference the receiver where applicable."""
        return _traverseTemplateAttributes(attrs, self.decodeAttributeString)

    def operationsWithSettings(self):
        """operationsWithSettings() ... returns a list of operations which currently have some settings defined."""
        ops = []
        for name, value in _RegisteredOps.items():
            for prop in value.registeredPropertyNames(name):
                if hasattr(self.obj, prop):
                    ops.append(name)
                    break
        return list(sorted(ops))

    def setOperationProperties(self, obj, opName):
        Path.Log.track(obj.Label, opName)
        try:
            op = _RegisteredOps[opName]
            for prop in op.properties():
                propName = OpPropertyName(opName, prop)
                if hasattr(self.obj, propName):
                    setattr(obj, prop, getattr(self.obj, propName))
        except Exception:
            Path.Log.info("SetupSheet has no support for {}".format(opName))

    def onDocumentRestored(self, obj):

        if not hasattr(obj, "CoolantModes"):
            obj.addProperty(
                "App::PropertyStringList",
                "CoolantModes",
                "CoolantMode",
                QT_TRANSLATE_NOOP("App::Property", "Coolant Modes"),
            )
            obj.CoolantModes = self.DefaultCoolantModes

        if not hasattr(obj, "CoolantMode"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "CoolantMode",
                "CoolantMode",
                QT_TRANSLATE_NOOP("App::Property", "Default coolant mode."),
            )
            obj.CoolantMode = self.DefaultCoolantModes


def Create(name="SetupSheet"):
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython", name)
    obj.Proxy = SetupSheet(obj)
    return obj


class _RegisteredOp(object):
    def __init__(self, factory, properties):
        self.factory = factory
        self.properties = properties

    def registeredPropertyNames(self, name):
        return [OpPropertyName(name, prop) for prop in self.properties()]

    def prototype(self, name):
        ptt = PathSetupSheetOpPrototype.OpPrototype(name)
        self.factory("OpPrototype.%s" % name, ptt)
        return ptt


def RegisterOperation(name, objFactory, setupProperties):
    global _RegisteredOps
    _RegisteredOps[name] = _RegisteredOp(objFactory, setupProperties)


def OpNamePrefix(name):
    return name.replace("Path", "").replace(" ", "").replace("_", "")


def OpPropertyName(opName, propName):
    return "{}{}".format(OpNamePrefix(opName), propName)


def OpPropertyGroup(opName):
    return "Op {}".format(opName)
