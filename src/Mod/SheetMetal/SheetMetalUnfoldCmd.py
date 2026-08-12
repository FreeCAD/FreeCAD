########################################################################
#
#  SheetMetalUnfoldCmd.py
#
#  Copyright 2014, 2018 Ulrich Brammer <ulrich@Pauline>
#  Copyright 2023 Ondsel Inc.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
########################################################################

import os
import sys

import FreeCAD
import Part

import SheetMetalKfactor
import SheetMetalTools
import SheetMetalUnfolder
from engineering_mode import engineering_mode_enabled

translate = FreeCAD.Qt.translate
SMLogger = SheetMetalTools.SMLogger

if sys.version_info.major == 3 and sys.version_info.minor < 10:
    NewUnfolderAvailable = False
    FreeCAD.Console.PrintWarning(
        translate("SheetMetal",
            "Python version is too old for the new unfolder\n"
            "Reverting to the old one\n"
            )
        )
elif SheetMetalTools.smIsNetworkxAvailable():
    import SheetMetalNewUnfolder
    import networkx as nx

    if not hasattr(nx, "Graph"):
        NewUnfolderAvailable = False
    else:
        from SheetMetalNewUnfolder import BendAllowanceCalculator

        NewUnfolderAvailable = True
else:
    NewUnfolderAvailable = False
    FreeCAD.Console.PrintWarning(
        translate("SheetMetal",
            "Networkx dependency is missing and required for the new Unfolder\n"
            "Try uninstalling SheetMetal, refresh Addon Manager's cache, and reinstall\n"
            )
        )

# IMPORTANT: please remember to change the element map version in case
# of any changes in modeling logic.
smElementMapVersion = "sm1."

# List of properties to be saved as defaults.
smUnfoldDefaultVars = [
    "KFactorStandard",
    "GenerateSketch",
    "SeparateSketchLayers",
]
smUnfoldNonSavedDefaultVars = [
    "UnfoldTransparency",
    "SketchColor",
    "InternalColor",
    "BendLineColor",
    "BendLabelColor",
    "ExportType",
]

GENSKETCHCOLOR = "#000080"
OUTLINESKETCHCOLOR = "#c00000"
BENDLINESKETCHCOLOR = "#ff5733"
BENDLABELCOLOR = "#33ff33"
KFACTOR = 0.40


###################################################################################################
# Helper functions
###################################################################################################

def smUnfoldExportSketches(obj, useDialog=True):
    if len(obj.UnfoldSketches) == 0:
        return
    sketches = []
    if len(obj.UnfoldSketches) == 1:
        sketchNames = [obj.UnfoldSketches[0]]
    else:
        sketchNames = obj.UnfoldSketches
    for name in sketchNames:
        sketch = obj.Document.getObject(name)
        if sketch is None:
            return
        sketches.append(sketch)
    exptype = obj.Proxy.ExportType
    expname = obj.Label.removesuffix("_Unfold")
    filename = f"{FreeCAD.ActiveDocument.Name}-{expname}.{exptype}"
    SheetMetalTools.smGuiExportSketch(sketches, exptype, filename, useDialog)


###################################################################################################
# Object class
###################################################################################################

class SMUnfold:
    """Class object for the unfold command."""

    def __init__(self, obj, selobj, sel_elements):
        """Add wall or Wall with radius bend."""
        selobj, sel_elements = SheetMetalTools.smUpdateLinks(obj, selobj, sel_elements)
        SheetMetalTools.smAddProperty(obj,
            "App::PropertyLinkSub",
            "baseObject",
            translate("App::Property", "Base Object"),
            (selobj, sel_elements),
        )
        self.addVerifyProperties(obj)
        SheetMetalTools.taskRestoreDefaults(obj, smUnfoldDefaultVars)
        # Setup transient properties.
        self.SketchColor = GENSKETCHCOLOR
        self.InternalColor = OUTLINESKETCHCOLOR
        self.BendLineColor = BENDLINESKETCHCOLOR
        self.BendLabelColor = BENDLABELCOLOR
        self.UnfoldTransparency = 0
        self.ExportType = "dxf"
        self.visibleSketches = []
        SheetMetalTools.taskRestoreDefaults(self, smUnfoldNonSavedDefaultVars)
        obj.Proxy = self
        self.UnfoldSketches = []

    def addVerifyProperties(self, obj):
        SheetMetalTools.smAddProperty(obj,
            "App::PropertyFloatConstraint",
            "KFactor",
            translate("SheetMetal", "Manual K-Factor value"),
            (0.4, 0.0, 2.0, 0.01),
        )
        SheetMetalTools.smAddEnumProperty(obj,
            "KFactorStandard",
            translate("SheetMetal", "K-Factor standard"),
            ["ansi", "din"],
            "ansi",
        )
        SheetMetalTools.smAddProperty(obj,
            "App::PropertyString",
            "MaterialSheet",
            translate("SheetMetal", "Material definition sheet"),
            "_manual",
            readOnly=True,
        )
        SheetMetalTools.smAddBoolProperty(obj,
            "ManualRecompute",
            translate("SheetMetal", "If set, object recomputation will be done on demand only"),
            False,
        )
        SheetMetalTools.smAddBoolProperty(obj,
            "GenerateSketch",
            translate("SheetMetal", "Generate unfold sketch"),
            False,
        )
        SheetMetalTools.smAddBoolProperty(obj,
            "SeparateSketchLayers",
            translate(
                "SheetMetal",
                "Generate separated unfold sketches for outline, inner lines and bend lines",
            ),
            False,
        )
        SheetMetalTools.smAddProperty(obj,
            "App::PropertyStringList",
            "UnfoldSketches",
            translate("SheetMetal", "Generated sketches"),
            None,
            "Hidden",
            attribs=8,  # Output only - no recompute if changed
        )
        SheetMetalTools.smAddBoolProperty(obj,
            "ShowBendAngles",
            translate("SheetMetal", "Show bend angles on the unfold sketch"),
            True,
        )
        SheetMetalTools.smAddLengthProperty(obj,
                "FontSize",
                translate("App::Property", "Font size for bend angle labels"),
                2.0)
        # SheetMetalTools.smAddProperty(
        #     obj,
        #     "App::PropertyBool",
        #     "DetachFromBody",
        #     translate
        #     ( "SheetMetal", "Make unfolded shape independent of the object's body"),
        #     False
        # )

    def getElementMapVersion(self, _fp, ver, _prop, restored):
        if not restored:
            return smElementMapVersion + ver
        return None

    def onChanged(self, obj, prop):
        if prop == "Visibility":
            isVisible = obj.Visibility
            visibleSketches = obj.Proxy.visibleSketches if isVisible else []
            for sketchName in obj.UnfoldSketches:
                sketch = obj.Document.getObject(sketchName)
                if sketch is not None:
                    if isVisible and sketchName in visibleSketches:
                        sketch.Visibility = True
                    elif not isVisible:
                        if sketch.Visibility:
                            visibleSketches.append(sketchName)
                            sketch.Visibility = False
            if not isVisible:
                obj.Proxy.visibleSketches = visibleSketches

    def newUnfolder(self, obj, baseObject, baseFace):
        """Use new unfolder system."""
        FreeCAD.Console.PrintMessage("Using V2 unfolding system\n")
        if obj.MaterialSheet in ["_manual", "_none"]:
            bac = BendAllowanceCalculator.from_single_value(obj.KFactor, obj.KFactorStandard)
        else:
            print("Using MDS:", obj.MaterialSheet)
            sheet = FreeCAD.ActiveDocument.getObject(obj.MaterialSheet)
            if sheet is None:
                sheet = FreeCAD.ActiveDocument.getObjectsByLabel(obj.MaterialSheet)[0]
            bac = BendAllowanceCalculator.from_spreadsheet(sheet)
        sel_face, unfolded_shape, bend_lines, root_normal, bend_infodata = SheetMetalNewUnfolder.getUnfold(
            bac, baseObject, baseFace
        )

        sketches = []
        if obj.GenerateSketch and unfolded_shape is not None:
            if not obj.ShowBendAngles:
                bend_infodata = []
            sketches = SheetMetalNewUnfolder.getUnfoldSketches(
                obj.Label,
                sel_face,
                unfolded_shape, 
                bend_lines,
                root_normal, 
                obj.UnfoldSketches,
                obj.SeparateSketchLayers,
                obj.Proxy.SketchColor,
                obj.Proxy.BendLineColor,
                obj.Proxy.InternalColor,
                bend_infodata=bend_infodata,
                bend_label_color=obj.Proxy.BendLabelColor,
                bend_label_size=obj.FontSize,
            )
        return unfolded_shape, sketches

    def oldUnfolder(self, obj, baseObject, baseFace):
        """Use old unfolder system."""
        FreeCAD.Console.PrintMessage("Using V1 unfolding system\n")
        kFactorTable = {1: obj.KFactor}
        if obj.MaterialSheet != "_manual" and obj.MaterialSheet != "_none":
            lookupTable = SheetMetalKfactor.KFactorLookupTable(obj.MaterialSheet)
            kFactorTable = lookupTable.k_factor_lookup

        shape, foldComp, norm, _thename, _err_cd, _fSel, _obN = SheetMetalUnfolder.getUnfold(
                kFactorTable, baseObject, baseFace, obj.KFactorStandard)

        sketches = []
        if obj.GenerateSketch and shape is not None:
            sketches = SheetMetalUnfolder.getUnfoldSketches(
                obj.Label,
                shape,
                foldComp.Edges,
                norm,
                obj.UnfoldSketches,
                obj.SeparateSketchLayers,
                obj.Proxy.SketchColor,
                bendSketchColor=obj.Proxy.BendLineColor,
                internalSketchColor=obj.Proxy.InternalColor,
            )
        return shape, sketches

    def execute(self, fp):
        """Print a short message when doing a recomputation.

        Note:
            This method is mandatory.

        """
        self.addVerifyProperties(fp)
        baseObj, baseFace = SheetMetalTools.smGetSubElementName(fp.baseObject[1][0])
        if baseObj is None:
            baseObj = fp.baseObject[0]
        if not NewUnfolderAvailable or SheetMetalTools.use_old_unfolder():
            shape, sketches = self.oldUnfolder(fp, baseObj, baseFace)
        else:
            shape, sketches = self.newUnfolder(fp, baseObj, baseFace)

        fp.Shape = shape
        parent = SheetMetalTools.smGetParentBody(fp)
        sketchList = []
        for sketch in sketches:
            if sketch is not None:
                sketchList.append(sketch.Name)
                if parent is not None and SheetMetalTools.smGetParentBody(sketch) is None:
                    parent.addObject(sketch)

        # Remove non-used sketches.
        for prop in fp.UnfoldSketches:
            if not prop in sketchList:
                item = fp.Document.getObject(prop)
                if item is not None:
                    fp.Document.removeObject(item.Name)

        fp.UnfoldSketches = sketchList
        SheetMetalTools.smRemoveFromRecompute(fp)


###################################################################################################
# Gui code
###################################################################################################

if SheetMetalTools.isGuiLoaded():

    from PySide import QtGui, QtCore

    Gui = FreeCAD.Gui

    mds_help_url = "https://github.com/shaise/FreeCAD_SheetMetal#material-definition-sheet"
    last_selected_mds = "none"


    ###############################################################################################
    # View Provider
    ###############################################################################################

    class SMUnfoldViewProvider(SheetMetalTools.SMViewProvider):
        """Part / Part WB style ViewProvider."""

        def getIcon(self):
            return os.path.join(SheetMetalTools.icons_path, "SheetMetal_Unfold.svg")

        def claimChildren(self):
            objs = []
            for itemName in self.Object.UnfoldSketches:
                item = self.Object.Document.getObject(itemName)
                if item is not None:
                    objs.append(item)
            return objs

        def getTaskPanel(self, obj):
            return SMUnfoldTaskPanel(obj)


    ###############################################################################################
    # Task Panel
    ###############################################################################################

    class SMUnfoldTaskPanel:
        """Task Panel for the unfold function."""

        def __init__(self, obj):
            QtCore.QDir.addSearchPath("Icons", SheetMetalTools.icons_path)
            self.obj = obj
            self.form = SheetMetalTools.taskLoadUI("UnfoldOptions.ui")

            # Make sure all properties are added.
            obj.Proxy.addVerifyProperties(obj)

            self.setupUi(obj)

        def _boolToState(self, bool):
            return QtCore.Qt.Checked if bool else QtCore.Qt.Unchecked

        def _isManualKSelected(self):
            return self.form.availableMds.currentIndex() == (self.form.availableMds.count() - 1)

        def _isNoMdsSelected(self):
            return self.form.availableMds.currentIndex() == 0

        def _updateSelectedMds(self):
            count = self.form.availableMds.count()
            currentIndex = self.form.availableMds.currentIndex()
            if currentIndex == 0:
                newsheet = "_none"
            elif currentIndex == count - 1:
                newsheet = "_manual"
            else:
                newsheet = self.form.availableMds.currentText()
            if newsheet != self.obj.MaterialSheet:
                self.obj.MaterialSheet = newsheet
                self.recomputeObject()

        def _getLastSelectedMdsIndex(self):
            materialSheet = self.obj.MaterialSheet
            if materialSheet == "_none":
                return 0
            elif materialSheet == "_manual":
                return self.form.availableMds.count() - 1
            for i in range(self.form.availableMds.count()):
                if self.form.availableMds.itemText(i) == materialSheet:
                    return i
            return -1

        def checkKFactorValid(self):
            if self.obj.MaterialSheet == "_none":
                msg = translate("Logger",
                                "Unfold operation needs to know K-factor value(s) to be used.")
                SMLogger.warning(msg)
                msg += translate(
                    "QMessageBox",
                    "<ol>\n"
                    "<li>Either select 'Manual K-factor'</li>\n"
                    "<li>Or use a <a href='{}'>Material Definition Sheet</a></li>\n"
                    "</ol>",
                ).format(mds_help_url)
                SheetMetalTools.smWarnDialog(msg)
                return False
            return True

        def setupUi(self, obj):
            self.updateKFactor(True)
            if obj.Proxy.ExportType == "dxf":
                self.form.dxfExport.setChecked(True)
            else:
                self.form.svgExport.setChecked(True)
            self.SketchColor = GENSKETCHCOLOR
            self.InternalColor = OUTLINESKETCHCOLOR
            self.BendLineColor = BENDLINESKETCHCOLOR
            self.BendLabelColor = BENDLABELCOLOR
            self.populateMdsList()
            SheetMetalTools.taskConnectSelectionSingle(self.form.pushFace, self.form.txtFace, obj,
                                                       "baseObject", ["Face"])
            SheetMetalTools.taskConnectColor(obj.Proxy, self.form.genColor, "SketchColor")
            SheetMetalTools.taskConnectColor(obj.Proxy, self.form.bendColor, "BendLineColor")
            SheetMetalTools.taskConnectColor(obj.Proxy, self.form.internalColor, "InternalColor")
            SheetMetalTools.taskConnectColor(obj.Proxy, self.form.bendLabelColor, "BendLabelColor")
            SheetMetalTools.taskConnectCheck(obj, self.form.chkSketch, "GenerateSketch",
                                             self.chkSketchChange)
            SheetMetalTools.taskConnectCheck(obj, self.form.chkSeparate, "SeparateSketchLayers",
                                             self.chkSketchChange)
            SheetMetalTools.taskConnectCheck(obj, self.form.chkAngleLabels, "ShowBendAngles",
                                             self.chkSketchChange)
            SheetMetalTools.taskConnectCheck(obj, self.form.chkManualUpdate, "ManualRecompute",
                                             self.chkManualChanged)
            SheetMetalTools.taskConnectCheck(obj, self.form.chkManualUpdate, "ManualRecompute",
                                             self.chkManualChanged)
            SheetMetalTools.taskConnectSpin(obj, self.form.floatKFactor, "KFactor")
            SheetMetalTools.taskConnectSpin(obj.Proxy, self.form.transSpin, "UnfoldTransparency",
                                            bindFunction=False)
            SheetMetalTools.taskConnectSpin(obj, self.form.fontSize, "FontSize")
            self.form.pushUnfold.clicked.connect(self.unfoldPressed)
            self.form.pushExport.clicked.connect(self.doExport)
            self.form.availableMds.currentIndexChanged.connect(self.availableMdsChacnge)
            self.form.dxfExport.toggled.connect(self.exportTypeChanged)
            self.form.kfactorAnsi.toggled.connect(self.kfactorStdChanged)

            self.availableMdsChacnge()
            self.chkSketchChange()
            # self.form.update()

        def updateKFactor(self, updateCheck):
            if self.obj.KFactorStandard == "ansi":
                if updateCheck:
                    self.form.kfactorAnsi.setChecked(True)
                self.form.floatKFactor.setProperty("value", self.obj.KFactor)
                self.form.floatKFactor.setProperty("maximum", 1.0)
            else:
                if updateCheck:
                    self.form.kfactorDin.setChecked(True)
                self.form.floatKFactor.setProperty("maximum", 2.0)
                self.form.floatKFactor.setProperty("value", self.obj.KFactor)

        def kfactorStdChanged(self):
            if self.form.kfactorAnsi.isChecked():
                self.obj.KFactorStandard = "ansi"
                self.obj.KFactor /= 2.0
            else:
                self.obj.KFactorStandard = "din"
                self.obj.KFactor *= 2.0
            self.updateKFactor(False)

        def recomputeObject(self, closeTask=False):
            SheetMetalTools.smForceRecompute = True
            if closeTask:
                SheetMetalTools.taskAccept(self)
            else:
                FreeCAD.ActiveDocument.recompute()
            SheetMetalTools.smForceRecompute = False
            # if len(self.obj.UnfoldSketches) > 0:
            #     FreeCAD.ActiveDocument.recompute()

        def accept(self):
            if not self.checkKFactorValid():
                return False
            self.recomputeObject(True)
            self.obj.ViewObject.Transparency = self.obj.Proxy.UnfoldTransparency
            SheetMetalTools.taskSaveDefaults(self.obj, smUnfoldDefaultVars)
            SheetMetalTools.taskSaveDefaults(self.obj.Proxy, smUnfoldNonSavedDefaultVars)
            # self._updateSelectedMds()
            # kFactorTable = self.getKFactorTable()
            return None

        def reject(self):
            FreeCAD.ActiveDocument.abortTransaction()
            Gui.Control.closeDialog()
            FreeCAD.ActiveDocument.recompute()

        def doExport(self):
            smUnfoldExportSketches(self.obj)

        def populateMdsList(self):
            sheetnames = SheetMetalKfactor.getSpreadSheetNames()
            self.form.availableMds.clear()

            self.form.availableMds.addItem(translate("SheetMetal", "Please select"))
            for mds in sheetnames:
                if mds.Label.startswith("material_"):
                    self.form.availableMds.addItem(mds.Label)
            self.form.availableMds.addItem(translate("SheetMetal", "Manual K-Factor"))

            selMdsIndex = self._getLastSelectedMdsIndex()
            if selMdsIndex > 0:
                self.form.availableMds.setCurrentIndex(selMdsIndex)
            elif len(sheetnames) == 1:
                self.form.availableMds.setCurrentIndex(1)
            elif engineering_mode_enabled() or len(sheetnames) > 1:
                self.form.availableMds.setCurrentIndex(0)
            else:
                self.form.availableMds.setCurrentIndex(1)

        def chkSketchChange(self, _value=None):
            genSketch = self.form.chkSketch.isChecked()
            self.form.chkSeparate.setEnabled(genSketch)
            self.form.genColor.setEnabled(genSketch)
            splitSketch = genSketch and self.form.chkSeparate.isChecked()
            self.form.bendColor.setEnabled(splitSketch)
            self.form.internalColor.setEnabled(splitSketch)
            unfoldUpdated = not self.obj in SheetMetalTools.smObjectsToRecompute
            exportEnabled = genSketch and len(self.obj.UnfoldSketches) > 0 and unfoldUpdated
            self.form.groupExport.setEnabled(exportEnabled)

        def exportTypeChanged(self):
            self.obj.Proxy.ExportType = "dxf" if self.form.dxfExport.isChecked() else "svg"

        def chkManualChanged(self, value):
            self.form.pushUnfold.setEnabled(value)

        def unfoldPressed(self):
            if not self.checkKFactorValid():
                return False
            self.recomputeObject()
            self.chkSketchChange()
            return None

        def availableMdsChacnge(self):
            self.form.groupManualFactor.setEnabled(self._isManualKSelected())
            self._updateSelectedMds()
            #self.form.kFactSpin.setEnabled(isManualK)


    ###############################################################################################
    # Commands
    ###############################################################################################

    class SMUnfoldCommandClass:
        """Unfold object."""

        def GetResources(self):
            __dir__ = os.path.dirname(__file__)
            iconPath = os.path.join(__dir__, "Resources", "icons")
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(iconPath, "SheetMetal_Unfold.svg"),
                    "MenuText": translate("SheetMetal", "Unfold"),
                    "Accel": "U",
                    "ToolTip": translate(
                        "SheetMetal",
                        "Flatten folded sheet metal object.\n"
                        "1. Select flat face on sheetmetal shape.\n"
                        "2. Change parameters from task Panel to create "
                        "unfold Shape & Flatten drawing.",
                        ),
                    }

        def Activated(self):
            sel = Gui.Selection.getSelectionEx()[0]
            selobj = sel.Object
            selparent = SheetMetalTools.smGetParentBody(selobj)
            name = "Unfold" if selparent is None else f"{selparent.Name}_Unfold"
            label = "Unfold" if selparent is None else f"{selparent.Label}_Unfold"
            newObj, activeBody = SheetMetalTools.smCreateNewObject(selobj, name, False)
            if newObj is None:
                return
            newObj.Label = label
            SMUnfold(newObj, selobj, sel.SubElementNames)
            SMUnfoldViewProvider(newObj.ViewObject)
            SheetMetalTools.smAddNewObject(selobj, newObj, activeBody, SMUnfoldTaskPanel)

        def IsActive(self):
            if (len(Gui.Selection.getSelection()) != 1
                    or len(Gui.Selection.getSelectionEx()[0].SubElementNames) != 1
            ):
                return False
            selFace = Gui.Selection.getSelectionEx()[0].SubObjects[0]
            return isinstance(selFace.Surface, Part.Plane)


    class SMRecomputeUnfoldsCommandClass:
        """Recompute all unfold objects marked for manual recompute."""

        def GetResources(self):
            __dir__ = os.path.dirname(__file__)
            iconPath = os.path.join(__dir__, "Resources", "icons")
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(iconPath, "SheetMetal_UnfoldUpdate.svg"),
                    "MenuText": translate("SheetMetal", "Unfold Update"),
                    "Accel": "UU",
                    "ToolTip": translate(
                        "SheetMetal",
                        "Update all unfold objects.\n"
                        ),
                    }

        def Activated(self):
            SheetMetalTools.smForceRecompute = True
            for obj in list(SheetMetalTools.smObjectsToRecompute):
                obj.touch()
            FreeCAD.ActiveDocument.recompute()
            SheetMetalTools.smForceRecompute = False

        def IsActive(self):
            return len(SheetMetalTools.smObjectsToRecompute) > 0


    class SMUnfoldUnattendedCommandClass:
        """Unfold object."""

        def GetResources(self):
            __dir__ = os.path.dirname(__file__)
            iconPath = os.path.join(__dir__, "Resources", "icons")
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(iconPath, "SheetMetal_UnfoldUnattended.svg"),
                    "MenuText": translate("SheetMetal", "Unattended Unfold"),
                    "Accel": "U",
                    "ToolTip": translate(
                        "SheetMetal",
                        "Flatten folded sheet metal object with default options\n"
                        "1. Select flat face on sheetmetal shape.\n"
                        "2. Click this command to unfold the object with last used parameters.",
                        ),
                    }

        def Activated(self):
            sel = Gui.Selection.getSelectionEx()[0]
            selobj = sel.Object
            selparent = SheetMetalTools.smGetParentBody(selobj)
            name = "Unfold" if selparent is None else f"{selparent.Name}_Unfold"
            label = "Unfold" if selparent is None else f"{selparent.Label}_Unfold"
            newObj, activeBody = SheetMetalTools.smCreateNewObject(selobj, name, False)
            if newObj is None:
                return
            newObj.Label = label
            SMUnfold(newObj, selobj, sel.SubElementNames)
            SMUnfoldViewProvider(newObj.ViewObject)
            SheetMetalTools.smAddNewObject(selobj, newObj, activeBody)
            selobj.Visibility = True
            return

        def IsActive(self):
            if (len(Gui.Selection.getSelection()) != 1
                    or len(Gui.Selection.getSelectionEx()[0].SubElementNames) != 1
            ):
                return False
            selFace = Gui.Selection.getSelectionEx()[0].SubObjects[0]
            return isinstance(selFace.Surface, Part.Plane)


    Gui.addCommand("SheetMetal_UnattendedUnfold", SMUnfoldUnattendedCommandClass())
    Gui.addCommand("SheetMetal_Unfold", SMUnfoldCommandClass())
    Gui.addCommand("SheetMetal_UnfoldUpdate", SMRecomputeUnfoldsCommandClass())
