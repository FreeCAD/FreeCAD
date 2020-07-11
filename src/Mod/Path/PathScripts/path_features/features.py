# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 russ4262 <russ4262@gmail.com>                      *
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

__title__ = "Base class for all features."
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = ("Base class and properties implementation"
           " for all standard Path features.")

# Standard
# Third-party
from PySide import QtCore
# FreeCAD
import PathScripts.PathLog as PathLog
import PathScripts.PathUtil as PathUtil
import PathScripts.PathUtils as PathUtils


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule()


QT_TRANSLATE = QtCore.QT_TRANSLATE_NOOP


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


def applyExpression(obj, prop, expr):
    '''applyExpression(obj, prop, expr) ...
    Set expression expr on obj.prop if expr is set.
    '''
    if expr:
        obj.setExpression(prop, expr)
        return True
    return False


# Available features using binary bit assignments in HEX format
FeatureTool         = 0x0001     # ToolController
FeatureDepths       = 0x0002     # FinalDepth, StartDepth
FeatureHeights      = 0x0004     # ClearanceHeight, SafeHeight
FeatureStartPoint   = 0x0008     # StartPoint
FeatureFinishDepth  = 0x0010     # FinishDepth
FeatureStepDown     = 0x0020     # StepDown
FeatureNoFinalDepth = 0x0040     # edit or not edit FinalDepth
FeatureBaseVertexes = 0x0080     # Base
FeatureBaseEdges    = 0x0100     # Base
FeatureBaseFaces    = 0x0200     # Base
FeatureBasePanels   = 0x0400     # Base
FeatureLocations    = 0x0800     # Locations
FeatureCoolant      = 0x1000     # Coolant

FeatureBaseGeometry = FeatureBaseVertexes | FeatureBaseFaces \
    | FeatureBaseEdges | FeatureBasePanels
DefaultFeatures = FeatureTool | FeatureDepths | FeatureHeights \
    | FeatureStartPoint | FeatureBaseGeometry | FeatureFinishDepth \
    | FeatureCoolant


# This dictionary should mirror the feature constants above
FEATURES = {
    'FeatureTool': FeatureTool,  # ToolController
    'FeatureDepths': FeatureDepths,  # FinalDepth, StartDepth
    'FeatureHeights': FeatureHeights,  # ClearanceHeight, SafeHeight
    'FeatureStartPoint': FeatureStartPoint,  # StartPoint
    'FeatureFinishDepth': FeatureFinishDepth,  # FinishDepth
    'FeatureStepDown': FeatureStepDown,  # StepDown
    'FeatureNoFinalDepth': FeatureNoFinalDepth,  # edit or not edit FinalDepth
    'FeatureBaseVertexes': FeatureBaseVertexes,  # Base
    'FeatureBaseEdges': FeatureBaseEdges,  # Base
    'FeatureBaseFaces': FeatureBaseFaces,  # Base
    'FeatureBasePanels': FeatureBasePanels,  # Base
    'FeatureLocations': FeatureLocations,  # Locations
    'FeatureCoolant': FeatureCoolant,  # Coolant
    'FeatureBaseGeometry' : FeatureBaseGeometry
}


class FeatureManager:
    '''
    Managing class for all standard features for all Path operations.

    This class manages most non-GUI activity related to standard features
    available as requested for all regular Path operations.

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

    To add a new feature:
        - Add a constant for the new feature to the list near
            the top of this module. Its name must start with 'Feature'.
        - Add an entry by the same name into the FEATURES dictionary.
            Set the value to point to the constant you just created.
        - Create an '_add_[[Your-Feature-Constant-Name]]' method
            in the 'Add Feature Methods' section of methods in this
            FeatureManager class.  The method should contain instructions
            to be executed if the feature is requested by an operation.
        - Most '_add-' methods include the creation of one or more properties.
        - If the property requires a unique default value, place that
            instruction set in the 'setFeatureDefaultValues()' method further
            below, within a corresponding `if :` clause, as demonstrated.
        - If the new feature has GUI elements, they should be addressed
            within the gui_features.py module.
    '''

    # Methods to initialize operation base class
    def __init__(self, thisModule, obj, features):
        PathLog.track()
        self.module = thisModule
        self.obj = obj
        self.features = features
        self.activeFeatures = list()
        self.propDefs = list()
        self.editorModes = list()

    def applyRequestedFeatures(self):
        PathLog.debug('applyRequestedFeatures()')
        features = self.features

        # Pre-process special feature instuctions
        self._preProcessFeatures()

        for feat, featBit in FEATURES.items():
            # featVal = getattr(self.module, feat)
            # if featVal & features:
            if featBit & features:
                self.activeFeatures.append(feat)
                featureMethod = getattr(self, '_add_' + feat)
                featureMethod()  # call the add feature method

        msg = translate('features', 'Added requested features:')
        msg += '\n{}'.format(self.activeFeatures)
        PathLog.debug(' - ' + msg)

    def _preProcessFeatures(self):
        if not (FeatureDepths & self.features):
            # StartDepth has become necessary for expressions
            # on other properties
            useNote = QT_TRANSLATE("features",
                                   ("Starting Depth internal use only"
                                    " for derived values"))
            self.obj.addProperty("App::PropertyDistance",
                                 "StartDepth", "Path", useNote)
            self.obj.setEditorMode('StartDepth', 1)  # read-only

        # Add OpStockZMin and OpStockZMax properties as read-only
        self._addOpValues(['stockz'])

    # Add Feature Methods
    def _add_DefaultFeatures(self):
        addList = ['FeatureTool', 'FeatureDepths', 'FeatureHeights',
                   'FeatureStartPoint', 'FeatureBaseGeometry',
                   'FeatureFinishDepth', 'FeatureCoolant']
        for feat in addList:
            self.activeFeatures.append(feat)
            featureMethod = getattr(self, '_add_' + feat)
            featureMethod()  # call the '_add_' feature method

    def _add_FeatureBaseEdges(self):
        # Base
        pass

    def _add_FeatureBaseFaces(self):
        # Base
        pass

    def _add_FeatureBaseGeometry(self, restore=False):
        obj = self.obj
        addNew = True

        def addBaseProperty(obj):
            useNote = QT_TRANSLATE("features",
                                   ("The base geometry for this operation"))
            self.obj.addProperty("App::PropertyLinkSubListGlobal",
                                 "Base", "Path", useNote)

        # Combination of FeatureBaseVertexes, FeatureBaseFaces,
        #   FeatureBaseEdges, and FeatureBasePanels
        if restore:
            if hasattr(obj, 'Base'):
                baseType = obj.getTypeIdOfProperty('Base')
                if baseType == 'App::PropertyLinkSubList':
                    msg = translate('features',
                                    ('Replacing link property with global link'
                                     ' (%s).' % obj.State))
                    PathLog.info(msg)
                    base = obj.Base
                    obj.removeProperty('Base')
                    addBaseProperty(obj)
                    obj.Base = base
                    obj.touch()
                    obj.Document.recompute()
                    addNew = False

        if addNew:
            addBaseProperty(obj)

    def _add_FeatureBasePanels(self):
        # Base
        pass

    def _add_FeatureBaseVertexes(self):
        # Base
        pass

    def _add_FeatureCoolant(self):
        # Coolant
        useNote = QT_TRANSLATE("features",
                               ("Coolant mode for this operation"))
        self.obj.addProperty("App::PropertyString",
                             "CoolantMode", "Path", useNote)

    def _add_FeatureCycleTime(self):
        # Cycle time
        useNote = QT_TRANSLATE("features",
                               ("Operations Cycle Time Estimation"))
        self.obj.addProperty("App::PropertyString",
                             "CycleTime", "Path", useNote)

        self.obj.setEditorMode('CycleTime', 1)  # read-only

    def _add_FeatureDepths(self):
        # FinalDepth, StartDepth
        useNote = QT_TRANSLATE("features",
                               ("Starting Depth of Tool:"
                                " first cut depth in Z"))
        self.obj.addProperty("App::PropertyDistance",
                             "StartDepth", "Path", useNote)

        useNote = QT_TRANSLATE("features",
                               ("Final Depth of Tool- lowest value in Z"))
        self.obj.addProperty("App::PropertyDistance",
                             "FinalDepth", "Path", useNote)

        # Add OpStartDepth and OpFinalDepth properties as read-only
        self._addOpValues(['start', 'final'])

    def _add_FeatureFinishDepth(self):
        # FinishDepth
        useNote = QT_TRANSLATE("features",
                               ("Maximum material removed on final pass."))
        self.obj.addProperty("App::PropertyDistance",
                             "FinishDepth", "Depth", useNote)

    def _add_FeatureHeights(self):
        # ClearanceHeight, SafeHeight
        useNote = QT_TRANSLATE("features",
                               ("The height needed to clear clamps"
                                " and obstructions"))
        self.obj.addProperty("App::PropertyDistance",
                             "ClearanceHeight", "Depth", useNote)

        useNote = QT_TRANSLATE("features",
                               ("Rapid Safety Height between locations."))
        self.obj.addProperty("App::PropertyDistance",
                             "SafeHeight", "Depth", useNote)

    def _add_FeatureLocations(self):
        # Locations
        useNote = QT_TRANSLATE("features",
                               "Base locations for this operation")
        self.obj.addProperty("App::PropertyVectorList",
                             "Locations", "Path", useNote)

    def _add_FeatureNoFinalDepth(self):
        # Not edit FinalDepth
        if FeatureDepths & self.features:
            self.obj.setEditorMode('FinalDepth', 2)  # hide

    def _add_FeatureStartPoint(self):
        # StartPoint
        useNote = QT_TRANSLATE("features",
                               ("The start point of this path"))
        self.obj.addProperty("App::PropertyVectorDistance",
                             "StartPoint", "Start Point", useNote)

        useNote = QT_TRANSLATE("features",
                               ("Make True, if specifying a Start Point"))
        self.obj.addProperty("App::PropertyBool",
                             "UseStartPoint", "Start Point", useNote)

    def _add_FeatureStepDown(self):
        # StepDown
        useNote = QT_TRANSLATE("features",
                               ("Incremental Step Down of Tool"))
        self.obj.addProperty("App::PropertyDistance",
                             "StepDown", "Depth", useNote)

    def _add_FeatureTool(self):
        # ToolController
        useNote = QT_TRANSLATE("features",
                               ("The tool controller that will be used"
                                " to calculate the path"))
        self.obj.addProperty("App::PropertyLink",
                             "ToolController", "Path", useNote)

        # Add OpToolDiameter property as read-only
        self._addOpValues(['tooldia'])

    # Support methods
    def _addOpValues(self, valueList):
        if 'start' in valueList:
            useNote = QT_TRANSLATE("features",
                                   "Holds the calculated value"
                                   " for the StartDepth")
            self.obj.addProperty("App::PropertyDistance",
                                 "OpStartDepth", "Op Values", useNote)
            self.obj.setEditorMode('OpStartDepth', 1)  # read-only

        if 'final' in valueList:
            useNote = QT_TRANSLATE("features",
                                   "Holds the calculated value"
                                   " for the FinalDepth")
            self.obj.addProperty("App::PropertyDistance",
                                 "OpFinalDepth", "Op Values", useNote)
            self.obj.setEditorMode('OpFinalDepth', 1)  # read-only

        if 'tooldia' in valueList:
            useNote = QT_TRANSLATE("features",
                                   ("Holds the diameter of the tool"))
            self.obj.addProperty("App::PropertyDistance",
                                 "OpToolDiameter", "Op Values", useNote)
            self.obj.setEditorMode('OpToolDiameter', 1)  # read-only

        if 'stockz' in valueList:
            useNote = QT_TRANSLATE("features",
                                   "Holds the max Z value of Stock")
            self.obj.addProperty("App::PropertyDistance",
                                 "OpStockZMax", "Op Values", useNote)
            self.obj.setEditorMode('OpStockZMax', 1)  # read-only
            useNote = QT_TRANSLATE("features",
                                   "Holds the min Z value of Stock")
            self.obj.addProperty("App::PropertyDistance",
                                 "OpStockZMin", "Op Values", useNote)
            self.obj.setEditorMode('OpStockZMin', 1)  # read-only

    def setFeatureDefaultValues(self, job, obj):
        '''setFeatureDefaultValues(job, obj) ...
        Sets default values for requested features.

        Caller: PathOpBase._setDefaultValues().
        '''
        self.job = job
        self.obj = obj
        features = self.features  # self.opFeatures(obj)

        if FeatureTool & features:
            if 1 < len(job.Operations.Group):
                op = job.Operations.Group[-2]
                obj.ToolController = PathUtil.toolControllerForOp(op)
            else:
                obj.ToolController = PathUtils.findToolController(obj)
                # return None
            obj.OpToolDiameter = obj.ToolController.Tool.Diameter

        if FeatureCoolant & features:
            obj.CoolantMode = job.SetupSheet.CoolantMode

        if FeatureDepths & features:
            if applyExpression(obj, 'StartDepth',
                                    job.SetupSheet.StartDepthExpression):
                obj.OpStartDepth = 1.0
            else:
                obj.StartDepth = 1.0
            if applyExpression(obj, 'FinalDepth',
                                    job.SetupSheet.FinalDepthExpression):
                obj.OpFinalDepth = 0.0
            else:
                obj.FinalDepth = 0.0
        else:
            obj.StartDepth = 1.0

        if FeatureStepDown & features or 'FeatureStepDown' in self.activeFeatures:
            PathLog.warning(' -FeatureStepDown default')
            if not applyExpression(obj, 'StepDown',
                                   job.SetupSheet.StepDownExpression):
                PathLog.warning(" -obj.StepDown = '1 mm'")
                obj.StepDown = '1 mm'

        if FeatureHeights & features:
            safeExpr = job.SetupSheet.SafeHeightExpression
            if safeExpr:
                if not applyExpression(obj, 'SafeHeight', safeExpr):
                    obj.SafeHeight = '3 mm'
            clearExpr = job.SetupSheet.ClearanceHeightExpression
            if clearExpr:
                if not applyExpression(obj, 'ClearanceHeight', clearExpr):
                    obj.ClearanceHeight = '5 mm'

        if FeatureStartPoint & features:
            obj.UseStartPoint = False

    def onDocumentRestored(self, job, obj, features):
        '''onDocumentRestored(job, obj, features) ...
        Executes feature-related instructions upon document restoration,
        such as verifying that required properties exist.

        Caller: PathOpBase.onDocumentRestored().
        '''
        # Re-instantiate some attributes
        self.job = job
        self.obj = obj
        self.features = features

        # Update older files to new Base global link
        self._add_FeatureBaseGeometry(restore=True)

        if FeatureTool & features and not hasattr(obj, 'OpToolDiameter'):
            self._addOpValues(['tooldia'])

        if FeatureCoolant & features and not hasattr(obj, 'CoolantMode'):
            self._add_FeatureCoolant

        if FeatureDepths & features and not hasattr(obj, 'OpStartDepth'):
            self._addOpValues(['start', 'final'])
            if FeatureNoFinalDepth & features:
                obj.setEditorMode('OpFinalDepth', 2)

        if not hasattr(obj, 'OpStockZMax'):
            self._addOpValues(['stockz'])

        if not hasattr(obj, 'CycleTime'):
            self._add_FeatureCycleTime

# Eclass
