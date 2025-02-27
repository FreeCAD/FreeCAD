# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

"""This module contains FreeCAD commands for the BIM workbench"""


import sys
import os
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


import importlib
import inspect


tests = [
    "testAll",
    "testIFC4",
    "testHierarchy",
    "testSites",
    "testBuildings",
    "testStoreys",
    "testUndefined",
    "testSolid",
    "testQuantities",
    "testCommonPsets",
    "testPsets",
    "testMaterials",
    "testStandards",
    "testExtrusions",
    "testStandardCases",
    "testTinyLines",
    "testRectangleProfileDef",
]


class BIM_Preflight:
    def GetResources(self):
        return {
            "Pixmap": "BIM_Preflight",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Preflight", "Preflight checks..."),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Preflight",
                "Checks several characteristics of this model before exporting to IFC",
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        FreeCADGui.BIMPreflightDone = False
        FreeCADGui.Control.showDialog(BIM_Preflight_TaskPanel())


class BIM_Preflight_TaskPanel:

    def __init__(self):
        from PySide import QtCore, QtGui

        self.results = {}  # to store the result message
        self.culprits = {}  # to store objects to highlight
        self.rform = None  # to store the results dialog
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogPreflight.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/BIM_Preflight.svg"))
        for test in tests:
            getattr(self.form, test).setIcon(QtGui.QIcon(":/icons/button_right.svg"))
            getattr(self.form, test).setToolTip(
                translate("BIM", "Press to perform the test")
            )
            if hasattr(self, test):
                getattr(self.form, test).clicked.connect(getattr(self, test))
            self.results[test] = None
            self.culprits[test] = None

        # setup custom tests
        self.customTests = {}
        customModulePath = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "Preflight")
        if os.path.exists(customModulePath):
            customModules = [
                m[:-3] for m in os.listdir(customModulePath) if m.endswith(".py")
            ]
            if customModules:
                sys.path.append(customModulePath)
                for customModule in customModules:
                    mod = importlib.import_module(customModule)
                    if not "Preflight" in mod.__file__:
                        # prevent from using other modules with same name
                        FreeCAD.Console.PrintLog(
                            "Preflight: loaded wrong module - skipping: "
                            + customModule
                            + " "
                            + str(mod)
                            + "\n"
                        )
                        continue
                    FreeCAD.Console.PrintLog(
                        "Preflight: found custom module: "
                        + customModule
                        + " "
                        + str(mod)
                        + "\n"
                    )
                    functions = [
                        o[0]
                        for o in inspect.getmembers(mod)
                        if inspect.isfunction(o[1])
                    ]
                    if functions:
                        box = QtGui.QGroupBox(customModule)
                        lay = QtGui.QGridLayout(box)
                        self.form.layout().addWidget(box)
                        for funcname in functions:
                            FreeCAD.Console.PrintLog(
                                "Preflight: found custom test: " + funcname + "\n"
                            )
                            func = getattr(mod, funcname)
                            descr = func.__doc__
                            if not descr:
                                descr = "Undefined"
                            lab = QtGui.QLabel(descr)
                            lab.setWordWrap(True)
                            but = QtGui.QPushButton()
                            butname = "Custom_" + customModule + "_" + funcname
                            but.setObjectName(butname)
                            setattr(self.form, butname, but)
                            self.reset(butname)
                            row = lay.rowCount()
                            lay.addWidget(lab, row, 0)
                            lay.addWidget(but, row, 1)
                            but.clicked.connect(lambda: self.testCustom(butname))
                            self.customTests[butname] = func

    def getStandardButtons(self):
        from PySide import QtCore, QtGui

        return QtGui.QDialogButtonBox.Close

    def reject(self):
        from PySide import QtCore, QtGui

        QtGui.QApplication.restoreOverrideCursor()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def passed(self, test):
        "sets the button as passed"

        from PySide import QtCore, QtGui

        getattr(self.form, test).setIcon(QtGui.QIcon(":/icons/button_valid.svg"))
        getattr(self.form, test).setText(translate("BIM", "Passed"))
        getattr(self.form, test).setToolTip(
            translate("BIM", "This test has succeeded.")
        )

    def failed(self, test):
        "sets the button as failed"

        from PySide import QtCore, QtGui

        getattr(self.form, test).setIcon(QtGui.QIcon(":/icons/process-stop.svg"))
        getattr(self.form, test).setText("Failed")
        getattr(self.form, test).setToolTip(
            translate("BIM", "This test has failed. Press the button to know more")
        )

    def reset(self, test):
        "reset the button"

        from PySide import QtCore, QtGui

        getattr(self.form, test).setIcon(QtGui.QIcon(":/icons/button_right.svg"))
        getattr(self.form, test).setText(translate("BIM", "Test"))
        getattr(self.form, test).setToolTip(
            translate("BIM", "Press to perform the test")
        )

    def show(self, test):
        "shows test results"

        if (test in self.results) and self.results[test]:
            if (test in self.culprits) and self.culprits[test]:
                FreeCADGui.Selection.clearSelection()
                for c in self.culprits[test]:
                    FreeCADGui.Selection.addSelection(c)
            if not self.rform:
                self.rform = FreeCADGui.PySideUic.loadUi(":/ui/dialogPreflightResults.ui")
                # center the dialog over FreeCAD window
                mw = FreeCADGui.getMainWindow()
                self.rform.move(
                    mw.frameGeometry().topLeft()
                    + mw.rect().center()
                    - self.rform.rect().center()
                )
                self.rform.buttonReport.clicked.connect(self.toReport)
                self.rform.buttonOK.clicked.connect(self.closeReport)
            self.rform.textBrowser.setText(self.results[test])
            label = test.replace("test", "label")
            self.rform.label.setText(getattr(self.form, label).text())
            self.rform.test = test
            self.rform.show()

    def toReport(self):
        "copies the resulting text to the report view"

        if self.rform and hasattr(self.rform, "test") and self.rform.test:
            if self.results[self.rform.test]:
                FreeCAD.Console.PrintMessage(self.results[self.rform.test] + "\n")

    def closeReport(self):
        if self.rform:
            self.rform.test = None
            self.rform.hide()

    def getObjects(self):
        "selects target objects"

        import Draft
        import Arch

        objs = []
        if self.form.getAll.isChecked():
            objs = FreeCAD.ActiveDocument.Objects
        elif self.form.getVisible.isChecked():
            objs = [
                o
                for o in FreeCAD.ActiveDocument.Objects
                if o.ViewObject.Visibility == True
            ]
        else:
            objs = FreeCADGui.Selection.getSelection()
        # clean objects list of unwanted types
        objs = Draft.get_group_contents(objs, walls=True, addgroups=True)
        objs = [obj for obj in objs if not obj.isDerivedFrom("Part::Part2DObject")]
        objs = [obj for obj in objs if not obj.isDerivedFrom("App::Annotation")]
        objs = [
            obj
            for obj in objs
            if (
                hasattr(obj, "Shape")
                and obj.Shape
                and not (obj.Shape.Edges and (not obj.Shape.Faces))
            )
        ]
        objs = Arch.pruneIncluded(objs)
        objs = [
            obj for obj in objs if not obj.isDerivedFrom("App::DocumentObjectGroup")
        ]
        objs = [
            obj
            for obj in objs
            if Draft.getType(obj)
            not in ["DraftText", "Material", "MaterialContainer", "WorkingPlaneProxy"]
        ]
        return objs

    def getToolTip(self, test):
        "gets the toolTip text from the ui file"

        import re

        label = test.replace("test", "label")
        tooltip = getattr(self.form, label).toolTip()
        tooltip = tooltip.replace("</p>", "</p>\n\n")
        tooltip = re.sub(r"<.*?>", "", tooltip)  # strip html tags
        return tooltip

    def testAll(self):
        "runs all tests"

        from PySide import QtCore, QtGui
        from draftutils import todo

        for test in tests:
            if test != "testAll":
                QtGui.QApplication.processEvents()
                self.reset(test)
                if hasattr(self, test):
                    todo.ToDo.delay(getattr(self, test), None)
        for customTest in self.customTests.keys():
            todo.ToDo.delay(self.testCustom, customTest)
        FreeCADGui.BIMPreflightDone = True

    def testIFC4(self):
        "tests for IFC4 support"

        test = "testIFC4"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = None
            msg = None
            try:
                import ifcopenshell
            except ImportError:
                msg = (
                    translate(
                        "BIM",
                        "ifcopenshell is not installed on your system or not available to FreeCAD. This library is responsible for IFC support in FreeCAD, and therefore IFC support is currently disabled. Check %1 to obtain more information.",
                    ).replace("%1", "https://www.freecadweb.org/wiki/Extra_python_modules#IfcOpenShell")
                    + " "
                )
                self.failed(test)
            else:
                if hasattr(
                    ifcopenshell, "schema_identifier"
                ) and ifcopenshell.schema_identifier.startswith("IFC4"):
                    self.passed(test)
                elif hasattr(ifcopenshell, "version"):
                    try:
                        from packaging import version
                        if "-" in ifcopenshell.version:
                            # Prebuild version have a version like 'v0.7.0-<GIT_COMMIT_ID>,
                            # trying to remove the commit id.
                            cur_version = version.parse(ifcopenshell.version.split('-')[0])
                        else:
                            cur_version = version.parse(ifcopenshell.version)
                        min_version = version.parse("0.6")
                        if cur_version >= min_version:
                            self.passed(test)
                        else:
                            msg = self.getToolTip(test)
                            msg = (
                                translate(
                                    "BIM",
                                    "The version of Ifcopenshell installed on your system could not be parsed",
                                )
                                + " "
                            )
                            self.failed(test)
                    except Exception as e:
                        self.failed(test)
                else:
                    msg = self.getToolTip(test)
                    msg += (
                        translate(
                            "BIM",
                            "The version of Ifcopenshell installed on your system will produce files with this schema version:",
                        )
                        + "\n\n"
                    )
                    if hasattr(ifcopenshell, "schema_identifier"):
                        msg += ifcopenshell.schema_identifier + "\n\n"
                    else:
                        msg += "Unable to retrieve schemas information from ifcopenshell\n\n"
                    self.failed(test)
            self.results[test] = msg

    def testHierarchy(self):
        "tests for project hierarchy support"

        import Draft
        from PySide import QtCore, QtGui

        test = "testHierarchy"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None
            sites = False
            buildings = False
            storeys = False
            for obj in self.getObjects():
                if (
                    (Draft.getType(obj) == "Site")
                    or (hasattr(obj, "IfcRole") and (obj.IfcRole == "Site"))
                    or (hasattr(obj, "IfcType") and (obj.IfcType == "Site"))
                ):
                    sites = True
                elif (
                    (Draft.getType(obj) == "Building")
                    or (hasattr(obj, "IfcRole") and (obj.IfcRole == "Building"))
                    or (hasattr(obj, "IfcType") and (obj.IfcType == "Building"))
                ):
                    buildings = True
                elif (
                    hasattr(obj, "IfcRole") and (obj.IfcRole == "Building Storey")
                ) or (hasattr(obj, "IfcType") and (obj.IfcType == "Building Storey")):
                    storeys = True
            if (not sites) or (not buildings) or (not storeys):
                msg = self.getToolTip(test)
                msg += (
                    translate(
                        "BIM", "The following types were not found in the project:"
                    )
                    + "\n"
                )
                if not sites:
                    msg += "\nSite"
                if not buildings:
                    msg += "\nBuilding"
                if not storeys:
                    msg += "\nBuilding Storey"
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testSites(self):
        "tests for Sites support"

        import Draft
        from PySide import QtCore, QtGui

        test = "testSites"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None
            for obj in self.getObjects():
                if (
                    (Draft.getType(obj) == "Building")
                    or (hasattr(obj, "IfcRole") and (obj.IfcRole == "Building"))
                    or (hasattr(obj, "IfcType") and (obj.IfcType == "Building"))
                ):
                    ok = False
                    for parent in obj.InList:
                        if (
                            (Draft.getType(parent) == "Site")
                            or (
                                hasattr(parent, "IfcRole")
                                and (parent.IfcRole == "Site")
                            )
                            or (
                                hasattr(parent, "IfcType")
                                and (parent.IfcType == "Site")
                            )
                        ):
                            if hasattr(parent, "Group") and parent.Group:
                                if obj in parent.Group:
                                    ok = True
                                    break
                    if not ok:
                        self.culprits[test].append(obj)
                        if not msg:
                            msg = self.getToolTip(test)
                            msg += (
                                translate(
                                    "BIM",
                                    "The following Building objects have been found to not be included in any Site. You can resolve the situation by creating a Site object, if none is present in your model, and drag and drop the Building objects into it in the tree view:",
                                )
                                + "\n\n"
                            )
                        msg += obj.Label + "\n"
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testBuildings(self):
        "tests for Buildings support"

        from PySide import QtCore, QtGui

        test = "testBuildings"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None
            for obj in self.getObjects():
                if (hasattr(obj, "IfcRole") and (obj.IfcRole == "Building Storey")) or (
                    hasattr(obj, "IfcType") and (obj.IfcType == "Building Storey")
                ):
                    ok = False
                    for parent in obj.InList:
                        if (
                            hasattr(parent, "IfcRole")
                            and (parent.IfcRole == "Building")
                        ) or (
                            hasattr(parent, "IfcType")
                            and (parent.IfcType == "Building")
                        ):
                            if hasattr(parent, "Group") and parent.Group:
                                if obj in parent.Group:
                                    ok = True
                                    break
                    if not ok:
                        self.culprits[test].append(obj)
                        if not msg:
                            msg = self.getToolTip(test)
                            msg += (
                                translate(
                                    "BIM",
                                    'The following Building Storey (BuildingParts with their IFC role set as "Building Storey") objects have been found to not be included in any Building. You can resolve the situation by creating a Building object, if none is present in your model, and drag and drop the Building Storey objects into it in the tree view:',
                                )
                                + "\n\n"
                            )
                        msg += obj.Label + "\n"
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testStoreys(self):
        "tests for Building Storey support"

        from PySide import QtCore, QtGui

        test = "testStoreys"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None
            for obj in self.getObjects():
                if (
                    hasattr(obj, "IfcRole")
                    and (not obj.IfcRole in ["Building", "Building Storey", "Site"])
                ) or (
                    hasattr(obj, "IfcType")
                    and (not obj.IfcType in ["Building", "Building Storey", "Site"])
                ):
                    ok = False
                    ancestors = obj.InListRecursive
                    # append extra objects not in InList
                    if hasattr(obj,"Host") and not obj.Host in ancestors:
                        ancestors.append(obj.Host)
                    if hasattr(obj,"Hosts"):
                        for h in obj.Hosts:
                            if not h in ancestors:
                                ancestors.append(h)
                    for parent in ancestors:
                        # just check if any of the ancestors is a Building Storey for now. Don't check any further...
                        if (
                            hasattr(parent, "IfcRole")
                            and (parent.IfcRole in ["Building Storey", "Building"])
                        ) or (
                            hasattr(parent, "IfcType")
                            and (parent.IfcType in ["Building Storey", "Building"])
                        ):
                            ok = True
                            break
                    if not ok:
                        self.culprits[test].append(obj)
                        if not msg:
                            msg = self.getToolTip(test)
                            msg += (
                                translate(
                                    "BIM",
                                    'The following BIM objects have been found to not be included in any Building Storey (BuildingParts with their IFC role set as "Building Storey"). You can resolve the situation by creating a Building Storey object, if none is present in your model, and drag and drop these objects into it in the tree view:',
                                )
                                + "\n\n"
                            )
                        msg += obj.Label + "\n"
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testUndefined(self):
        "tests for undefined BIM objects"

        from PySide import QtCore, QtGui

        test = "testUndefined"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            undefined = []
            notbim = []
            msg = None

            for obj in self.getObjects():
                if hasattr(obj, "IfcType"):
                    if obj.IfcType == "Undefined":
                        self.culprits[test].append(obj)
                        undefined.append(obj)
                elif hasattr(obj, "IfcRole"):
                    if obj.IfcRole == "Undefined":
                        self.culprits[test].append(obj)
                        undefined.append(obj)
                else:
                    self.culprits[test].append(obj)
                    notbim.append(obj)
            if undefined or notbim:
                msg = self.getToolTip(test)
                if undefined:
                    msg += (
                        translate(
                            "BIM",
                            'The following BIM objects have the "Undefined" type:',
                        )
                        + "\n\n"
                    )
                    for o in undefined:
                        msg += o.Label + "\n"
                if notbim:
                    msg += (
                        translate("BIM", "The following objects are not BIM objects:")
                        + "\n\n"
                    )
                    for o in notbim:
                        msg += o.Label + "\n"
                        msg += translate(
                            "BIM",
                            "You can turn these objects into BIM objects by using the Modify -> Add Component tool.",
                        )
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testSolid(self):
        "tests for invalid/non-solid BIM objects"

        from PySide import QtCore, QtGui

        test = "testSolid"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None

            for obj in self.getObjects():
                if obj.isDerivedFrom("Part::Feature"):
                    if (not obj.Shape.isNull()) and (
                        (not obj.Shape.isValid()) or (not obj.Shape.Solids)
                    ):
                        self.culprits[test].append(obj)
            if self.culprits[test]:
                msg = self.getToolTip(test)
                msg += (
                    translate(
                        "BIM",
                        "The following BIM objects have an invalid or non-solid geometry:",
                    )
                    + "\n\n"
                )
                for o in self.culprits[test]:
                    msg += o.Label + "\n"
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testQuantities(self):
        "tests for explicit quantities export"

        from PySide import QtCore, QtGui

        test = "testQuantities"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None

            for obj in self.getObjects():
                if hasattr(obj, "IfcAttributes") and (
                    Draft.getType(obj) != "BuildingPart"
                ):
                    for prop in ["Length", "Width", "Height"]:
                        if prop in obj.PropertiesList:
                            if (not "Export" + prop in obj.IfcAttributes) or (
                                obj.IfcAttributes["Export" + prop] == "False"
                            ):
                                self.culprits[test].append(obj)
                                break
            if self.culprits[test]:
                msg = self.getToolTip(test)
                msg += (
                    translate(
                        "BIM",
                        "The objects below have Length, Width or Height properties, but these properties won't be explicitly exported to IFC. This is not necessarily an issue, unless you specifically want these quantities to be exported:",
                    )
                    + "\n\n"
                )
                for o in self.culprits[test]:
                    msg += o.Label + "\n"
                msg += "\n" + translate(
                    "BIM",
                    "To enable exporting of these quantities, use the IFC quantities manager tool located under menu Manage -> Manage IFC Quantities...",
                )
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testCommonPsets(self):
        "tests for common property sets"

        from PySide import QtCore, QtGui
        import csv

        test = "testCommonPsets"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None
            psets = []
            psetspath = os.path.join(
                FreeCAD.getResourceDir(),
                "Mod",
                "Arch",
                "Presets",
                "pset_definitions.csv",
            )
            if os.path.exists(psetspath):
                with open(psetspath, "r") as csvfile:
                    reader = csv.reader(csvfile, delimiter=";")
                    for row in reader:
                        if "Common" in row[0]:
                            psets.append(row[0][5:-6])
            psets = [
                "".join(map(lambda x: x if x.islower() else " " + x, p)) for p in psets
            ]
            psets = [pset.strip() for pset in psets]
            # print(psets)

            for obj in self.getObjects():
                ok = True
                if hasattr(obj, "IfcProperties") and isinstance(
                    obj.IfcProperties, dict
                ):
                    r = None
                    if hasattr(obj, "IfcType"):
                        r = obj.IfcType
                    if hasattr(obj, "IfcRole"):
                        r = obj.IfcRole
                    if r and (r in psets):
                        ok = False
                        if "Pset_" + r.replace(" ", "") + "Common" in ",".join(
                            obj.IfcProperties.values()
                        ):
                            ok = True
                if not ok:
                    self.culprits[test].append(obj)

            if self.culprits[test]:
                msg = self.getToolTip(test)
                msg += (
                    translate(
                        "BIM",
                        "The objects below have a defined IFC type but do not have the associated common property set:",
                    )
                    + "\n\n"
                )
                for o in self.culprits[test]:
                    msg += o.Label + "\n"
                msg += "\n" + translate(
                    "BIM",
                    "To add common property sets to these objects, use the IFC properties manager tool located under menu Manage -> Manage IFC Properties...",
                )
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testPsets(self):
        "tests for property sets integrity"

        from PySide import QtCore, QtGui
        import csv

        test = "testPsets"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None
            psets = {}
            psetspath = os.path.join(
                FreeCAD.getResourceDir(),
                "Mod",
                "Arch",
                "Presets",
                "pset_definitions.csv",
            )
            if os.path.exists(psetspath):
                with open(psetspath, "r") as csvfile:
                    reader = csv.reader(csvfile, delimiter=";")
                    for row in reader:
                        if "Common" in row[0]:
                            psets[row[0]] = row[1:]

            for obj in self.getObjects():
                ok = True
                if hasattr(obj, "IfcProperties") and isinstance(
                    obj.IfcProperties, dict
                ):
                    r = None
                    if hasattr(obj, "IfcType"):
                        r = obj.IfcType
                    elif hasattr(obj, "IfcRole"):
                        r = obj.IfcRole
                    if r and (r != "Undefined"):
                        found = None
                        for pset in psets.keys():
                            for val in obj.IfcProperties.values():
                                if pset in val:
                                    found = pset
                                    break
                        if found:
                            for i in range(int(len(psets[found]) / 2)):
                                p = psets[found][i * 2]
                                t = psets[found][i * 2 + 1]
                                # print("testing for ",p,t,found," in ",obj.IfcProperties)
                                if p in obj.IfcProperties:
                                    if (not found in obj.IfcProperties[p]) or (
                                        not t in obj.IfcProperties[p]
                                    ):
                                        ok = False
                                else:
                                    ok = False
                if not ok:
                    self.culprits[test].append(obj)

            if self.culprits[test]:
                msg = self.getToolTip(test)
                msg += (
                    translate(
                        "BIM",
                        "The objects below have a common property set but that property set doesn't contain all the needed properties:",
                    )
                    + "\n\n"
                )
                for o in self.culprits[test]:
                    msg += o.Label + "\n"
                msg += (
                    "\n"
                    + translate(
                        "BIM",
                        "Verify which properties a certain property set must contain on %1",
                    ).replace("%1", "https://standards.buildingsmart.org/IFC/DEV/IFC4_2/FINAL/HTML/annex/annex-b/alphabeticalorder_psets.htm")
                    + "\n\n"
                )
                msg += translate(
                    "BIM",
                    "To fix the property sets of these objects, use the IFC properties manager tool located under menu Manage -> Manage IFC Properties...",
                )
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testMaterials(self):
        "tests for materials in BIM objects"

        from PySide import QtCore, QtGui

        test = "testMaterials"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None
            for obj in self.getObjects():
                if "Material" in obj.PropertiesList:
                    if not obj.Material:
                        self.culprits[test].append(obj)
            if self.culprits[test]:
                msg = self.getToolTip(test)
                msg += (
                    translate(
                        "BIM", "The following BIM objects have no material attributed:"
                    )
                    + "\n\n"
                )
                for o in self.culprits[test]:
                    msg += o.Label + "\n"
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testStandards(self):
        "tests for standards in BIM objects"

        from PySide import QtCore, QtGui

        test = "testStandards"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None
            for obj in self.getObjects():
                if "StandardCode" in obj.PropertiesList:
                    if not obj.StandardCode:
                        self.culprits[test].append(obj)
                if "Material" in obj.PropertiesList:
                    if obj.Material:
                        if "StandardCode" in obj.Material.PropertiesList:
                            if not obj.Material.StandardCode:
                                self.culprits[test].append(obj.Material)
            if self.culprits[test]:
                msg = self.getToolTip(test)
                msg += (
                    translate(
                        "BIM",
                        "The following BIM objects have no defined standard code:",
                    )
                    + "\n\n"
                )
                for o in self.culprits[test]:
                    msg += o.Label + "\n"
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testExtrusions(self):
        "tests is all objects are extrusions"

        from PySide import QtCore, QtGui
        import Draft

        test = "testExtrusions"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None
            for obj in self.getObjects():
                if hasattr(obj, "Proxy"):
                    if (
                        hasattr(obj, "IfcAttributes")
                        and ("FlagForceBrep" in obj.IfcAttributes.keys())
                        and (obj.IfcAttributes["FlagForceBrep"] == "True")
                    ):
                        self.culprits[test].append(obj)
                    elif hasattr(
                        obj.Proxy, "getExtrusionData"
                    ) and not obj.Proxy.getExtrusionData(obj):
                        self.culprits[test].append(obj)
                    elif Draft.getType(obj) == "BuildingPart":
                        pass
                elif obj.isDerivedFrom("Part::Extrusion"):
                    pass
                elif obj.isDerivedFrom("App::DocumentObjectGroup"):
                    pass
                elif obj.isDerivedFrom("App::MaterialObject"):
                    pass
                else:
                    self.culprits[test].append(obj)
            if self.culprits[test]:
                msg = self.getToolTip(test)
                msg += (
                    translate("BIM", "The following BIM objects are not extrusions:")
                    + "\n\n"
                )
                for o in self.culprits[test]:
                    msg += o.Label + "\n"
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testStandardCases(self):
        "tests for structs and wall standard cases"

        import Draft
        from PySide import QtCore, QtGui

        test = "testStandardCases"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None
            for obj in self.getObjects():
                if Draft.getType(obj) == "Wall":
                    if obj.Base and (len(obj.Base.Shape.Edges) != 1):
                        self.culprits[test].append(obj)
                elif Draft.getType(obj) == "Structure":
                    if obj.Base and (
                        (len(obj.Base.Shape.Wires) != 1)
                        or (not obj.Base.Shape.Wires[0].isClosed())
                    ):
                        self.culprits[test].append(obj)
            if self.culprits[test]:
                msg = self.getToolTip(test)
                msg += (
                    translate(
                        "BIM", "The following BIM objects are not standard cases:"
                    )
                    + "\n\n"
                )
                for o in self.culprits[test]:
                    msg += o.Label + "\n"
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testTinyLines(self):
        "tests for objects with tiny lines (< 0.8mm)"

        from PySide import QtCore, QtGui

        test = "testTinyLines"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = []
            msg = None
            minl = 0.79376  # min 1/32"
            edges = []
            objs = []
            for obj in self.getObjects():
                if obj.isDerivedFrom("Part::Feature"):
                    if obj.Shape:
                        for e in obj.Shape.Edges:
                            if e.Length <= minl:
                                edges.append(e)
                                if not obj in objs:
                                    objs.append(obj)
            if edges:
                import Part
                result = FreeCAD.ActiveDocument.addObject(
                    "Part::Feature", "TinyLinesResult"
                )
                result.Shape = Part.makeCompound(edges)
                result.ViewObject.LineWidth = 5
                self.culprits[test] = [result]
                msg = self.getToolTip(test)
                msg += (
                    translate(
                        "BIM",
                        "The objects below have lines smaller than 1/32 inch or 0.79 mm, which is the smallest line size that Revit accepts. These objects will be discarded when imported into Revit:",
                    )
                    + "\n\n"
                )
                for obj in objs:
                    msg += obj.Label + "\n"
                msg += (
                    "\n"
                    + translate(
                        "BIM",
                        'An additional object, called "TinyLinesResult" has been added to this model, and selected. It contains all the tiny lines found, so you can inspect them and fix the needed objects. Be sure to delete the TinyLinesResult object when you are done!',
                    )
                    + "\n\n"
                )
                msg += translate(
                    "BIM",
                    "Tip: The results are best viewed in Wireframe mode (menu Views -> Draw Style -> Wireframe)",
                )
            if msg:
                self.failed(test)
            else:
                self.passed(test)
            self.results[test] = msg
            QtGui.QApplication.restoreOverrideCursor()

    def testRectangleProfileDef(self):
        "tests for RectangleProfileDef disable"

        test = "testRectangleProfileDef"
        if getattr(self.form, test).text() == "Failed":
            self.show(test)
        else:
            self.reset(test)
            self.results[test] = None
            self.culprits[test] = None
            msg = None
            if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool(
                "DisableIfcRectangleProfileDef", False
            ):
                self.passed(test)
            else:
                msg = self.getToolTip(test)
                self.failed(test)
            self.results[test] = msg

    def testCustom(self, test):
        "performs a custom test"

        if test in self.customTests:
            if getattr(self.form, test).text() == "Failed":
                self.show(test)
            else:
                self.reset(test)
                self.results[test] = None
                result = self.customTests[test]()
                if result == True:
                    self.passed(test)
                else:
                    self.failed(test)
                    self.results[test] = result


FreeCADGui.addCommand("BIM_Preflight", BIM_Preflight())
