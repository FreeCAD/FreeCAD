# ######################################################################
#
#  SheetMetalCmd.py
#
#  Copyright 2026 Thomas D (@pierreporte)
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
# ######################################################################

import math

from PySide import QtCore, QtGui

import FreeCAD
import Part

import SheetMetalTools
from SheetMetalCmd import smBend, smGetFace, smRestrict, sheet_thk

# IMPORTANT: please remember to change the element map version in case
# of any changes in modeling logic.
smElementMapVersion = "sm1."

# List of properties to be saved as defaults.
smAddHemDefaultVars = ["HemType", ("width", "defaultWidth"),
                        ("radius", "defaultRadius"), "AutoMiter", ("kfactor", "defaultKFactor")]

translate = FreeCAD.Qt.translate


# Finds the root of f() in the given interval
def bisection_method(f, min_interval, max_interval, eps):
    a = min_interval
    b = max_interval

    fa = f(a)
    fb = f(b)

    if fa * fb > 0:
        raise ValueError("Teardrop hem has unexpected incorrect geometry")

    c = 0.5 * (a + b)
    prev_c = c + 2 * eps  # ensure the loop starts

    while abs(c - prev_c) >= eps:
        prev_c = c

        fc = f(c)

        if fa * fc < 0:
            b = c
            fb = fc
        else:
            a = c
            fa = fc

        c = 0.5 * (a + b)

    return c


def generateOpenHem(thickness, width, includeBend, opening):
    if opening < 0:
        raise ValueError("Opening must be positive")
    
    bendRadius = 0.5*opening
    bendAngle = 180
    legLength = width

    if includeBend:
        if width > 0.5*opening + thickness:
            legLength -= (bendRadius + thickness)
        else:
            raise ValueError("Width must be greater than the bend width (equal to bend radius + thickness)")
    else:
        if width <= 0:
            raise ValueError("Width must be strictly positive")
        
    return (legLength, bendAngle, bendRadius)


def generateRolledHem(thickness, radius, rollAngle=None):
    maxRollAngle = 270.0 + math.degrees(math.asin(radius/(radius+thickness)))
    legLength = 0.0
    bendRadius = radius

    if rollAngle is None:
        bendAngle = maxRollAngle
        return (legLength, bendAngle, bendRadius)
    elif 0.0 < rollAngle <= maxRollAngle:
        bendAngle = rollAngle
        return (legLength, bendAngle, bendRadius)
    elif rollAngle <= 0.0:
        raise ValueError("Roll angle must be strictly positive")
    elif rollAngle > maxRollAngle:
        raise ValueError("Roll angle must not exceed physical maximum ({}°)".format(maxRollAngle))

def generateTeardropHem(thickness, radius, width, includeBend, opening=0.0):
    Lp = width
    R = radius
    t = thickness
    Lbend = R + t
    H = opening

    if not includeBend:
        Lp += Lbend

    if H < 0:
        raise ValueError("Opening must be positive")

    if width >= 2*Lbend:
        legLength = 0.0
        bendRadius = R
        bendAngle = 0.0

        if width == 2*Lbend: # Degenerate teardrop hem
            bendAngle = 270.0
            if H < R:
                legLength = R - H
            else:
                raise ValueError("Opening must be smaller than bend radius")
        else: # Regular teadrop hem
            equation = lambda L: L - Lp + Lbend + t*math.sin(2*math.atan(R/L))
            precision = 1.0e-9
            L = bisection_method(equation, Lp-Lbend-t, Lp-Lbend, precision)

            if H == 0.0:
                theta = math.atan(R/L)
                bendAngle = 180.0 + 2*math.degrees(theta)
                legLength = L
            elif H == 2*bendRadius:
                legLength, _, bendRadius = generateOpenHem(t, Lp-Lbend, opening)
            else:
                theta = math.atan((L-math.sqrt(L**2-2.0*R*H+H**2))/H)
                bendAngle = 180.0 + math.degrees(2*theta)
                legLength = H*(math.cos(2*theta)-1)/math.sin(2*theta)+L
        
        return (legLength, bendAngle, bendRadius)
    else:
        raise ValueError("Width must be greater or equal than the bend width (equal to bend radius + thickness)")


class SMHem:
    def __init__(self, obj, selobj, sel_items, refAngOffset=None, checkRefFace=False):
        """Add Hem on an edge."""
        self.addVerifyProperties(obj, refAngOffset, checkRefFace)

        _tip_ = translate("App::Property", "Base Object")
        obj.addProperty("App::PropertyLinkSub", "baseObject", "Parameters", _tip_
        ).baseObject = (selobj, sel_items)
        obj.Proxy = self
        SheetMetalTools.taskRestoreDefaults(obj, smAddHemDefaultVars)

    def addVerifyProperties(self, obj, refAngOffset=None, checkRefFace=False):
        SheetMetalTools.smAddEnumProperty(obj,
                "HemType",
                translate("App::Property", "Hem Type"),
                ["Flat", "Open", "Teardrop", "Rolled"])
        SheetMetalTools.smAddLengthProperty(obj,
                "radius",
                translate("App::Property", "Bend Radius"),
                1.0)
        SheetMetalTools.smAddBoolProperty(obj,
                "IncludeBend",
                translate("App::Property", "Include Bend"),
                True)
        SheetMetalTools.smAddLengthProperty(obj,
                "width",
                translate("App::Property", "Width of Hem"),
                10.0)
        SheetMetalTools.smAddAngleProperty(obj,
                "RollAngle",
                translate("App::Property", "Roll angle"),
                225.0)
        SheetMetalTools.smAddBoolProperty(obj,
                "opened",
                translate("App::Property", "Opened hem"),
                False)
        SheetMetalTools.smAddLengthProperty(obj,
                "opening",
                translate("App::Property", "Opening of Hem"),
                1.0)
        SheetMetalTools.smAddDistanceProperty(obj,
                "gap1",
                translate("App::Property", "Gap from Left Side"),
                0.0)
        SheetMetalTools.smAddDistanceProperty(obj,
                "gap2",
                translate("App::Property", "Gap from Right Side"),
                0.0)
        SheetMetalTools.smAddBoolProperty(obj,
                "invert",
                translate("App::Property", "Invert Bend Direction"),
                False)
        SheetMetalTools.smAddEnumProperty(obj,
                "BendType",
                translate("App::Property", "Bend Type"),
                ["Material Outside", "Material Inside", "Thickness Outside"])
        SheetMetalTools.smAddLengthProperty(obj,
                "reliefw",
                translate("App::Property", "Relief Width"),
                0.8,
                "ParametersRelief")
        SheetMetalTools.smAddLengthProperty(obj,
                "reliefd",
                translate("App::Property", "Relief Depth"),
                1.0,
                "ParametersRelief")
        SheetMetalTools.smAddBoolProperty(obj,
                "UseReliefFactor",
                translate("App::Property", "Use Relief Factor"),
                False,
                "ParametersRelief")
        SheetMetalTools.smAddEnumProperty(obj,
                "reliefType",
                translate("App::Property", "Relief Type"),
                ["Rectangle", "Round"],
                None,
                "ParametersRelief")
        SheetMetalTools.smAddFloatProperty(obj,
                "ReliefFactor",
                translate("App::Property", "Relief Factor"),
                0.7,
                "ParametersRelief")
        SheetMetalTools.smAddAngleProperty(obj,
                "miterangle1",
                translate("App::Property", "Bend Miter Angle from Left Side"),
                0.0,
                "ParametersMiterangle")
        SheetMetalTools.smAddAngleProperty(obj,
                "miterangle2",
                translate("App::Property", "Bend Miter Angle from Right Side"),
                0.0,
                "ParametersMiterangle")
        SheetMetalTools.smAddLengthProperty(obj,
                "minGap",
                translate("App::Property", "Auto Miter Minimum Gap"),
                0.2,
                "ParametersEx")
        SheetMetalTools.smAddLengthProperty(obj,
                "maxExtendDist",
                translate("App::Property", "Auto Miter maximum Extend Distance"),
                5.0,
                "ParametersEx")
        SheetMetalTools.smAddLengthProperty(obj,
                "minReliefGap",
                translate("App::Property", "Minimum Gap to Relief Cut"),
                1.0,
                "ParametersEx")
        SheetMetalTools.smAddBoolProperty(obj,
                "AutoMiter",
                translate("App::Property", "Enable Auto Miter"),
                True,
                "ParametersEx")
        SheetMetalTools.smAddBoolProperty(obj,
                "unfold",
                translate("App::Property", "Shows Unfold View of Current Bend"),
                False,
                "ParametersEx")
        SheetMetalTools.smAddProperty(obj,
                "App::PropertyFloatConstraint",
                "kfactor",
                translate("App::Property",
                          "Location of Neutral Line. Caution: Using ANSI standards, not DIN."),
                (0.5, 0.0, 1.0, 0.01),
                "ParametersEx")
        
    def setEditorMode(self, fp, prop, mode):
        if (hasattr(fp, prop)):
            fp.setEditorMode(prop, mode)

    def onChanged(self, fp, prop):
        hidden = 2
        visible = 0
        if prop == "HemType":
            if fp.HemType == "Flat":
                self.setEditorMode(fp, "opened", hidden)
                self.setEditorMode(fp, "opening", hidden)
                self.setEditorMode(fp, "radius", hidden)
                self.setEditorMode(fp, "RollAngle", hidden)
                self.setEditorMode(fp, "width", visible)
                self.setEditorMode(fp, "IncludeBend", visible)
            elif fp.HemType == "Open":
                self.setEditorMode(fp, "opened", hidden)
                self.setEditorMode(fp, "radius", hidden)
                self.setEditorMode(fp, "RollAngle", hidden)
                self.setEditorMode(fp, "opening", visible)
                self.setEditorMode(fp, "width", visible)
                self.setEditorMode(fp, "IncludeBend", visible)
            elif fp.HemType == "Teardrop":
                self.setEditorMode(fp, "opened", visible)
                if fp.opened:
                    self.setEditorMode(fp, "opening", visible)
                else:
                    self.setEditorMode(fp, "opening", hidden)
                self.setEditorMode(fp, "radius", visible)
                self.setEditorMode(fp, "RollAngle", hidden)
                self.setEditorMode(fp, "width", visible)
                self.setEditorMode(fp, "IncludeBend", visible)
            elif fp.HemType == "Rolled":
                self.setEditorMode(fp, "opened", visible)
                if fp.opened:
                    self.setEditorMode(fp, "RollAngle", visible)
                else:
                    self.setEditorMode(fp, "RollAngle", hidden)
                self.setEditorMode(fp, "radius", visible)
                self.setEditorMode(fp, "opening", hidden)
                self.setEditorMode(fp, "width", hidden)
                self.setEditorMode(fp, "IncludeBend", hidden)
        elif prop == "opened":
            if fp.HemType == "Teardrop":
                if fp.opened:
                    self.setEditorMode(fp, "opening", visible)
                else:
                    self.setEditorMode(fp, "opening", hidden)
            elif fp.HemType == "Rolled":
                if fp.opened:
                    self.setEditorMode(fp, "RollAngle", visible)
                else:
                    self.setEditorMode(fp, "RollAngle", hidden)

    def getElementMapVersion(self, _fp, ver, _prop, restored):
        if not restored:
            return smElementMapVersion + ver
        return None

    def execute(self, fp):
        """Print a short message when doing a recomputation.

        Note:
            This method is mandatory.

        """
        self.addVerifyProperties(fp)

        bendAList = [fp.width.Value]
        LegLengthList = [fp.width.Value]
        bendR = fp.radius.Value
        allowedAutoMiter = fp.AutoMiter

        # Restrict some params.
        fp.miterangle1.Value = smRestrict(fp.miterangle1.Value, -80.0, 80.0)
        fp.miterangle2.Value = smRestrict(fp.miterangle2.Value, -80.0, 80.0)

        # Pass selected object shape.
        Main_Object = fp.baseObject[0].Shape.copy()
        face = fp.baseObject[1]
        thk, thkDir = sheet_thk(Main_Object, face[0])

        # Gap value needed for first bend set only.
        gap1_list = [0.0 for n in LegLengthList]
        gap2_list = [0.0 for n in LegLengthList]
        gap1_list[0] = fp.gap1.Value
        gap2_list[0] = fp.gap2.Value
        # print(gap1_list, gap2_list)

        # Compute geometrical parameters for selected hem type
        for i, _ in enumerate(LegLengthList):
            values = ()
            if fp.HemType == "Flat":
                values = generateOpenHem(thk, fp.width.Value, fp.IncludeBend, 0.0)
            elif fp.HemType == "Open":
                values = generateOpenHem(thk, fp.width.Value, fp.IncludeBend, fp.opening.Value)
            elif fp.HemType == "Teardrop":
                if fp.opened:
                    values = generateTeardropHem(thk, bendR, fp.width.Value, fp.IncludeBend, fp.opening.Value)
                else:
                    values = generateTeardropHem(thk, bendR, fp.width.Value, fp.IncludeBend)
            elif fp.HemType == "Rolled":
                allowedAutoMiter = False
                if fp.opened:
                    values = generateRolledHem(thk, bendR, fp.RollAngle.Value)
                else:
                    values = generateRolledHem(thk, bendR)

            LegLengthList[i], bendAList[i], bendR = values

        for i, LegLength in enumerate(LegLengthList):
            s, f = smBend(
                thk,
                bendR=bendR,
                bendA=bendAList[i],
                miterA1=fp.miterangle1.Value,
                miterA2=fp.miterangle2.Value,
                BendType=fp.BendType,
                flipped=fp.invert,
                unfold=fp.unfold,
                extLen=LegLength,
                reliefType=fp.reliefType,
                gap1=gap1_list[i],
                gap2=gap2_list[i],
                reliefW=fp.reliefw.Value,
                reliefD=fp.reliefd.Value,
                minReliefgap=fp.minReliefGap.Value,
                kfactor=fp.kfactor,
                ReliefFactor=fp.ReliefFactor,
                UseReliefFactor=fp.UseReliefFactor,
                automiter=allowedAutoMiter,
                selFaceNames=face,
                MainObject=Main_Object,
                mingap=fp.minGap.Value,
                maxExtendGap=fp.maxExtendDist.Value)

            faces = smGetFace(f, s)
            face = faces
            Main_Object = s

        fp.Shape = s


if SheetMetalTools.isGuiLoaded():
    import os

    Gui = FreeCAD.Gui
    icons_path = SheetMetalTools.icons_path
    smEpsilon = SheetMetalTools.smEpsilon


    class SMViewProviderTree(SheetMetalTools.SMViewProvider):
        """Part WB style ViewProvider."""

        def getIcon(self):
            return os.path.join(icons_path, "SheetMetal_AddHem.svg")

        def getTaskPanel(self, obj):
            return SMHemTaskPanel(obj)


    class SMViewProviderFlat(SMViewProviderTree):
        """Part Design WB style ViewProvider.

        Note:
            Backward compatibility only.

        """
    class SMHemTaskPanel:
        """A TaskPanel for the SheetMetal."""

        def __init__(self, obj):
            QtCore.QDir.addSearchPath("Icons", icons_path)
            self.obj = obj
            self.form = SheetMetalTools.taskLoadUI("HemParameters.ui")
            # Make sure all properties are added.
            obj.Proxy.addVerifyProperties(obj)
            # Variable to track which property should be filled when in
            # selection mode. And used to rename the form field (of face
            # reference) only when necessary.
            self.activeRefGeom = None
            # Store tab information for hiding/showing
            self.miterTabIndex = None
            self.miterTabTitle = None
            self.hemTypeChanged()  # Update form for the current hem type.
            self.updateForm()

            # Hem parameters connects  - General.
            self.selParams = SheetMetalTools.taskConnectSelection(
                self.form.AddRemove, self.form.tree, self.obj, ["Edge"], self.form.pushClearSel)
            SheetMetalTools.taskConnectEnum(obj, self.form.HemType, "HemType", self.hemTypeChanged)
            SheetMetalTools.taskConnectSpin(obj, self.form.SpinWidth, "width")
            self.form.buttRevHem.clicked.connect(self.revHem)  # Button click action.
            SheetMetalTools.taskConnectCheck(obj, self.form.checkIncludeBend, "IncludeBend", 
                                             self.includeBendChanged)
            SheetMetalTools.taskConnectSpin(obj, self.form.SpinRadius, "radius")
            SheetMetalTools.taskConnectCheck(obj, self.form.checkOpen, "opened", self.openChanged)
            SheetMetalTools.taskConnectSpin(obj, self.form.SpinAngle, "RollAngle")
            SheetMetalTools.taskConnectSpin(obj, self.form.SpinOpening, "opening")
            SheetMetalTools.taskConnectEnum(obj, self.form.positionType, "BendType", self.bendTypeChanged)
            SheetMetalTools.taskConnectCheck(obj, self.form.UnfoldCheckbox, "unfold")

            # Hem parameters connects  - Offsets.
            SheetMetalTools.taskConnectSpin(obj, self.form.gap1, "gap1")
            SheetMetalTools.taskConnectSpin(obj, self.form.gap2, "gap2")
            self.form.reliefTypeButtonGroup.buttonToggled.connect(self.reliefTypeUpdated)
            SheetMetalTools.taskConnectSpin(obj, self.form.reliefWidth, "reliefw")
            SheetMetalTools.taskConnectSpin(obj, self.form.reliefDepth, "reliefd")

            # Hem parameters connects  - Miter.
            SheetMetalTools.taskConnectCheck(obj, self.form.autoMiterCheckbox, "AutoMiter", self.autoMiterChanged)
            SheetMetalTools.taskConnectSpin(obj, self.form.minGap, "minGap")
            SheetMetalTools.taskConnectSpin(obj, self.form.maxExDist, "maxExtendDist")
            SheetMetalTools.taskConnectSpin(obj, self.form.miterAngle1, "miterangle1")
            SheetMetalTools.taskConnectSpin(obj, self.form.miterAngle2, "miterangle2")

        def revHem(self):  # Button to flip the hem side.
            self.obj.invert = not self.obj.invert
            self.updateForm()

        def isAllowedAlterSelection(self):
            return True

        def isAllowedAlterView(self):
            return True

        def bendTypeChanged(self, value):
            pass

        def reliefTypeUpdated(self):
            self.obj.reliefType = (
                "Rectangle" if self.form.reliefRectangle.isChecked() else "Round")
            self.obj.Document.recompute()

        # `value` parameter just to easily use this function as
        # a callback in form connections.
        def updateForm(self, value=None):
            SheetMetalTools.taskPopulateSelectionList(self.form.tree, self.obj.baseObject)

            # Advanced parameters update.
            if self.obj.reliefType == "Rectangle":
                self.form.reliefRectangle.setChecked(True)
            else:
                self.form.reliefRound.setChecked(True)

            # Button flip the hem - updates check.
            self.form.buttRevHem.setChecked(self.obj.invert)

            # Updates the angle in model.
            self.obj.recompute()
            self.obj.Document.recompute()

        def autoMiterChanged(self, isAutoMiter):
            self.form.groupAutoMiter.setEnabled(isAutoMiter)
            self.form.groupManualMiter.setEnabled(not isAutoMiter)

        def openChanged(self, isOpen):
            type = self.obj.HemType
            if type == "Teardrop":
                self.form.openingGroup.setVisible(isOpen)
            elif type == "Rolled":
                self.form.angleGroup.setVisible(isOpen)

        def includeBendChanged(self, includeBend):
            pass

        def hemTypeChanged(self, index = -1):
            type = self.obj.HemType
            self.form.horizLineBendAng.setVisible(not (type == "Flat"))
            
            # Handle Miter tab visibility
            if type == "Rolled":
                # Hide the Miter tab
                if self.miterTabIndex is None:
                    self.miterTabIndex = self.form.tabWidget.indexOf(self.form.tabMiter)
                    if self.miterTabIndex >= 0:
                        self.miterTabTitle = self.form.tabWidget.tabText(self.miterTabIndex)
                        self.form.tabWidget.removeTab(self.miterTabIndex)
            else:
                # Show the Miter tab
                if self.miterTabIndex is not None and self.miterTabTitle is not None:
                    self.form.tabWidget.insertTab(self.miterTabIndex, self.form.tabMiter, self.miterTabTitle)
                    self.miterTabIndex = None
                    self.miterTabTitle = None
            
            self.form.checkOpen.setVisible(type in ["Teardrop", "Rolled"])
            self.form.openingGroup.setVisible(type == "Open" or (type == "Teardrop" and self.obj.opened))
            self.form.angleGroup.setVisible(type == "Rolled" and self.obj.opened)
            self.form.radiusGroup.setVisible(type in ["Teardrop", "Rolled"])
            self.form.checkIncludeBend.setVisible(type != "Rolled")
            self.form.widthGroup.setVisible(type != "Rolled")

        def accept(self):
            SheetMetalTools.taskAccept(self)
            SheetMetalTools.taskSaveDefaults(self.obj, smAddHemDefaultVars)
            return True

        def reject(self):
            SheetMetalTools.taskReject(self)


    class AddHemCommandClass:
        """Add Hem command."""

        def GetResources(self):
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(icons_path, "SheetMetal_AddHem.svg"),
                    "MenuText": FreeCAD.Qt.translate("SheetMetal", "Make Hem"),
                    "Accel": "Z",
                    "ToolTip": FreeCAD.Qt.translate(
                        "SheetMetal",
                        "Create hems on edges.\n"
                        "1. Select edges to create bends with walls.\n"
                        "2. Use Property editor to modify other parameters",
                        ),
                    }

        def Activated(self):
            doc = FreeCAD.ActiveDocument
            view = Gui.ActiveDocument.ActiveView
            activeBody = None

            # Get the sheet metal object.
            try:
                for obj in Gui.Selection.getSelectionEx():
                    if not "Plane" in obj.Object.TypeId:
                        for subElem in obj.SubElementNames:
                            if type(obj.Object.Shape.getElement(subElem)) == Part.Edge:
                                sel = obj
                                break
                selobj = sel.Object
            except:
                raise Exception("At least one edge must be selected to create a hem.")

            selSubNames = list(sel.SubElementNames)
            selSubObjs = sel.SubObjects

            # Remove faces for wall creation reference because only
            # edges should be used as reference to create hems.
            for subObjName in selSubNames:
                if type(selobj.Shape.getElement(subObjName)) == Part.Face:
                    selSubNames.remove(subObjName)
                    if len(selSubNames) < 1:
                        raise Exception("At least one edge must be selected to create a hem.")

            # Get only one selected face to use for reference to angle
            # and offset.
            faceCount = 0
            refAngOffset = None
            checkRefFace = False
            for obj in Gui.Selection.getSelectionEx():
                for subObj in obj.SubObjects:
                    if type(subObj) == Part.Face and not "Plane" in obj.Object.TypeId:
                        faceCount += 1
                        if faceCount == 1:
                            for subObjName in obj.SubElementNames:
                                if obj.Object.Shape.getElement(subObjName).isEqual(subObj):
                                    refAngOffset = [obj.Object, subObjName]
                                    checkRefFace = True
                        else:
                            print("If more than one face is selected, "
                                  "only the first is used for "
                                  "reference to angle and offset.")
                if "Plane" in obj.Object.TypeId and faceCount == 0:
                    if obj.Object.TypeId == "App::Plane":
                        refAngOffset = [obj.Object, ""]
                    else:
                        refAngOffset = [obj.Object, obj.SubElementNames[0]]
                    checkRefFace = True

            viewConf = SheetMetalTools.GetViewConfig(selobj)
            if hasattr(view, "getActiveObject"):
                activeBody = view.getActiveObject("pdbody")
            if not SheetMetalTools.smIsOperationLegal(activeBody, selobj):
                return
            doc.openTransaction("Hem")
            if activeBody is None or not SheetMetalTools.smIsPartDesign(selobj):
                newObj = doc.addObject("Part::FeaturePython", "Hem")
                SMHem(newObj, selobj, selSubNames, refAngOffset, checkRefFace)
                SMViewProviderTree(newObj.ViewObject)
            else:
                # FreeCAD.Console.PrintLog("found active body: " + activeBody.Name)
                newObj = doc.addObject("PartDesign::FeaturePython", "Hem")
                SMHem(newObj, selobj, selSubNames, refAngOffset, checkRefFace)
                SMViewProviderFlat(newObj.ViewObject)
                activeBody.addObject(newObj)
            SheetMetalTools.SetViewConfig(newObj, viewConf)
            Gui.Selection.clearSelection()
            if SheetMetalTools.is_autolink_enabled():
                root = SheetMetalTools.getOriginalBendObject(newObj)
                if root:
                    if hasattr(root, "Radius"):
                        newObj.setExpression("radius", root.Label + ".Radius")
                    elif hasattr(root, "radius"):
                        newObj.setExpression("radius", root.Label + ".radius")
            newObj.baseObject[0].ViewObject.Visibility = False
            doc.recompute()
            dialog = SMHemTaskPanel(newObj)
            SheetMetalTools.updateTaskTitleIcon(dialog)
            Gui.Control.showDialog(dialog)
            return

        def IsActive(self):
            selobj = Gui.Selection.getSelectionEx()
            objSM = None
            # In this iteration, we will find which selected object is
            # the sheet metal part, 'cause the user can select a face
            # for reference (this object will not be the sheet metal
            # part).
            for obj in selobj:
                for subObj in obj.SubObjects:
                    if type(subObj) == Part.Edge:
                        objSM = obj

            # Test if any selected subObject in the sheet metal
            # isn't edge.
            geomTest = []
            for subObj in objSM.SubObjects:
                if type(subObj) == Part.Edge:
                    geomTest.append(True)
                else:
                    geomTest.append(False)

            if not False in geomTest and geomTest is not None:
                return True
            return None


    Gui.addCommand("SheetMetal_AddHem", AddHemCommandClass())
