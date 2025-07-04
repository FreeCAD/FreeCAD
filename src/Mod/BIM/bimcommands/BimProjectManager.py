# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""This module contains FreeCAD commands for the BIM workbench"""

import math
import os

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_ProjectManager:

    def GetResources(self):

        return {
            "Pixmap": "BIM_ProjectManager",
            "MenuText": QT_TRANSLATE_NOOP("BIM_ProjectManager", "Setup Project…"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_ProjectManager", "Creates or manages a BIM project"
            ),
        }

    def Activated(self):

        import FreeCADGui
        import ArchBuildingPart
        from PySide import QtGui

        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogProjectManager.ui")
        self.project = None
        self.site = None
        self.building = None

        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.form.move(
            mw.frameGeometry().topLeft()
            + mw.rect().center()
            - self.form.rect().center()
        )

        # set things up
        self.form.buildingUse.addItems(ArchBuildingPart.BuildingTypes)
        self.form.setWindowIcon(QtGui.QIcon(":/icons/BIM_ProjectManager.svg"))
        self.form.buttonAdd.clicked.connect(self.addGroup)
        self.form.buttonDel.clicked.connect(self.delGroup)
        self.form.buttonSave.clicked.connect(self.savePreset)
        self.form.presets.currentIndexChanged.connect(self.getPreset)
        self.form.buttonOK.clicked.connect(self.accept)
        self.form.buttonCancel.clicked.connect(self.reject)
        self.fillPresets()

        # Detect existing objects
        sel = FreeCADGui.Selection.getSelection()
        doc = FreeCAD.ActiveDocument
        if doc:
            if len(sel) == 1:
                if hasattr(sel[0], "Proxy") and hasattr(sel[0].Proxy, "ifcfile"):
                    # case 1: a project is selected
                    self.project = sel[0]
                    self.form.groupNewProject.setEnabled(False)
            if hasattr(doc, "Proxy"):
                # case 2: the actuve document is a project
                if hasattr(doc.Proxy, "ifcfile"):
                    self.project = doc
                    self.form.groupNewProject.setEnabled(False)
            if self.project:
                from nativeifc import ifc_tools
                sites = ifc_tools.get_children(self.project, ifctype="IfcSite")
                sites = list(filter(None, [ifc_tools.get_object(s) for s in sites]))
                self.form.projectName.setText(self.project.Label)
            else:
                sites = [o for o in doc.Objects if getattr(o, "IfcType", "") == "Site"]
            if sites:
                self.site = sites[0]
                self.form.siteName.setText(self.site.Label)
                if hasattr(self.site,"Address"):
                    self.form.siteAddress.setText(self.site.Address)
                elif hasattr(self.site,"SiteAddress"):
                    self.form.siteAddress.setText(self.site.SiteAddress)
                if hasattr(self.site,"Longitude"):
                    self.form.siteLongitude.setValue(self.site.Longitude)
                elif hasattr(self.site,"RefLongitude"):
                    self.form.siteLongitude.setValue(self.site.RefLongitude)
                if hasattr(self.site,"Latitude"):
                    self.form.siteLatitude.setValue(self.site.Latitude)
                elif hasattr(self.site,"RefLatitude"):
                    self.form.siteLatitude.setValue(self.site.RefLatitude)
                if hasattr(self.site,"Elevation"):
                    self.form.siteElevation.setText(self.site.Elevation.UserString)
                elif hasattr(self.site,"RefElevation"):
                    self.form.siteElevation.setText(self.site.RefElevation.UserString)
                if hasattr(self.site, "Declination"):
                    self.form.siteElevation.setText(str(self.site.Declination))
            buildings = []
            if self.site and self.project:
                from nativeifc import ifc_tools
                buildings = ifc_tools.get_children(self.site, ifctype="IfcBuilding")
                buildings = list(filter(None, [ifc_tools.get_object(b) for b in buildings]))
            if not buildings:
                buildings = [o for o in doc.Objects if getattr(o, "IfcType", "") == "Building"]
            if buildings:
                from nativeifc import ifc_tools
                self.building = buildings[0]
                self.form.buildingName.setText(self.building.Label)
                levels = ifc_tools.get_children(self.building, ifctype="IfcBuildingStorey")
                if levels:
                    self.form.countLevels.setValue(len(levels))

        # show dialog
        self.form.show()

    def reject(self):

        self.form.hide()
        return True

    def accept(self):

        import Arch
        import Draft
        import FreeCADGui
        import Part
        from draftutils import params

        vaxes = []
        haxes = []
        outline = None
        outtext = None
        human = None
        grp = None

        if self.form.groupNewProject.isChecked():
            self.project = None
            self.site = None
            self.building = None

        # reading form values
        buildingWidth = FreeCAD.Units.Quantity(self.form.buildingWidth.text()).Value
        buildingLength = FreeCAD.Units.Quantity(self.form.buildingLength.text()).Value
        distVAxes = FreeCAD.Units.Quantity(self.form.distVAxes.text()).Value
        distHAxes = FreeCAD.Units.Quantity(self.form.distHAxes.text()).Value
        levelHeight = FreeCAD.Units.Quantity(self.form.levelHeight.text()).Value
        color = self.form.lineColor.property("color").getRgbF()[:3]

        # Document creation
        doc = FreeCAD.ActiveDocument
        if self.form.groupNewProject.isChecked():
            if self.form.radioNative1.isChecked() or \
            self.form.radioNative3.isChecked() or \
            (self.form.radioNative2.isChecked() and doc is None):
                doc = FreeCAD.newDocument()
                if self.form.projectName.text():
                    doc.Label = self.form.projectName.text()
        if doc:
            FreeCAD.setActiveDocument(doc.Name)

        # Project creation
        if self.form.groupNewProject.isChecked():
            from nativeifc import ifc_tools
            if self.form.radioNative2.isChecked():
                self.project = ifc_tools.create_document_object(doc, silent=True)
                if self.form.projectName.text():
                    self.project.Label = self.form.projectName.text()
            elif self.form.radioNative3.isChecked():
                self.project = ifc_tools.convert_document(doc, silent=True)

        # human
        human = None
        if self.form.addHumanFigure.isChecked():
            humanshape = Part.Shape()
            humanshape.importBrep(":/geometry/HumanFigure.brep")
            human = FreeCAD.ActiveDocument.addObject("Part::Feature", "Human")
            human.Shape = humanshape
            human.Placement.move(FreeCAD.Vector(500, 500, 0))

        # Site creation or edition
        outline = None
        if self.form.groupSite.isChecked():
            if not self.site:
                self.site = Arch.makeSite()
                if self.project:
                    from nativeifc import ifc_tools
                    self.site = ifc_tools.aggregate(self.site, self.project)
            self.site.Label = self.form.siteName.text()
            if hasattr(self.site,"Address"):
                self.site.Address = self.form.siteAddress.text()
            elif hasattr(self.site,"SiteAddress"):
                self.site.SiteAddress = self.form.siteAddress.text()
            if hasattr(self.site,"Longitude"):
                self.site.Longitude = self.form.siteLongitude.value()
            elif hasattr(self.site,"RefLongitude"):
                self.site.RefLongitude = self.form.siteLongitude.value()
            if hasattr(self.site,"Latitude"):
                self.site.Latitude = self.form.siteLatitude.value()
            elif hasattr(self.site,"RefLatitude"):
                self.site.RefLatitude = self.form.siteLatitude.value()
            if hasattr(self.site, "NorthDeviation"):
                self.site.NorthDeviation = self.form.siteDeviation.value()
            elif hasattr(self.site, "Declination"):
                self.site.Declination = self.form.siteDeviation.value()
            elev = FreeCAD.Units.Quantity(self.form.siteElevation.text()).Value
            if hasattr(self.site, "Elevation"):
                self.site.Elevation = elev
            elif hasattr(self.site, "RefElevation"):
                self.site.RefElevation = elev

        # Building creation or edition
        if self.form.groupBuilding.isChecked():
            if not self.building:
                self.building = Arch.makeBuilding()
                if self.project:
                    from nativeifc import ifc_tools
                    if self.site:
                        self.building = ifc_tools.aggregate(self.building, self.site)
                    else:
                        self.building = ifc_tools.aggregate(self.building, self.project)
                elif self.site:
                    self.site.Group = [self.building]
            self.building.Label = self.form.buildingName.text()
            if hasattr(self.building, "BuildingType"):
                self.building.BuildingType = self.form.buildingUse.currentText()

            # Detecting existing contents
            if self.building:
                grp = [o for o in self.building.Group if o.Name.startswith("BuildingLayout")]
                if grp:
                    grp = grp[0]
            if grp:
                axes = [o for o in grp.Group if o.Name.startswith("Axis")]
                if axes:
                    for ax in axes:
                        if round(math.degrees(ax.Placement.Rotation.Angle),0) in [0, 180, 360]:
                            vaxes.append(ax)
                        elif round(math.degrees(ax.Placement.Rotation.Angle),0) in [90, 270]:
                            haxes.append(ax)
                outline = [o for o in grp.Group if o.Name.startswith("Rectangle")]
                if outline:
                    outline = outline[0]
                human = [o for o in grp.Group if o.Name.startswith("Human")]
                if human:
                    human = human[0]
            else:
                if self.form.addHumanFigure.isChecked() or self.form.countVAxes.value() \
                or self.form.countHAxes.value() or (buildingWidth and buildingLength):
                    grp = doc.addObject("App::DocumentObjectGroup", "BuildingLayout")
                    grp.Label = translate("BIM", "Building Layout")
                    if self.building:
                        if hasattr(self.building, "addObject"):
                            self.building.addObject(grp)

            # Human figure
            if self.form.addHumanFigure.isChecked():
                if not human:
                    # TODO embed this
                    humanpath = os.path.join(
                        os.path.dirname(__file__), "geometry", "human figure.brep"
                    )
                    if os.path.exists(humanpath):
                        humanshape = Part.Shape()
                        humanshape.importBrep(humanpath)
                        human = FreeCAD.ActiveDocument.addObject("Part::Feature", "Human")
                        human.Shape = humanshape
                        human.Placement.move(FreeCAD.Vector(500, 500, 0))
                    if human:
                        grp.addObject(human)
                    # TODO: nativeifc

            # Outline
            if buildingWidth and buildingLength:
                if not outline:
                    outline = Draft.makeRectangle(buildingLength, buildingWidth, face=False)
                    outline.Label = translate("BIM", "Building Outline")
                    outline.ViewObject.DrawStyle = "Dashed"
                    grp.addObject(outline)
                outline.ViewObject.LineColor = color
                outline.ViewObject.LineWidth = self.form.lineWidth.value() * 2
                outline.Length = buildingLength
                outline.Height = buildingWidth

            # Label
            if self.form.buildingName.text():
                buildingname = self.form.buildingName.text()
                outtext = Draft.make_text(
                    [buildingname],
                    FreeCAD.Vector(
                        params.get_param("textheight") * 0.3,
                        -params.get_param("textheight") * 1.43,
                        0,
                    ),
                )
                outtext.Label = translate("BIM", "Building Label")
                outtext.ViewObject.TextColor = color
                grp.addObject(outtext)

            # Axes
            axisV = None
            if self.form.countVAxes.value() and distVAxes:
                axisV = Arch.makeAxis(
                    num=self.form.countVAxes.value(), size=distVAxes, name="vaxis"
                )
                axisV.Label = translate("BIM", "Vertical Axes")
                axisV.ViewObject.BubblePosition = "Both"
                axisV.ViewObject.LineWidth = self.form.lineWidth.value()
                axisV.ViewObject.FontSize = params.get_param("textheight")
                axisV.ViewObject.BubbleSize = params.get_param("textheight") * 1.43
                axisV.ViewObject.LineColor = color
            axisH = None
            if self.form.countHAxes.value() and distHAxes:
                axisH = Arch.makeAxis(
                    num=self.form.countHAxes.value(), size=distHAxes, name="haxis"
                )
                axisH.Label = translate("BIM", "Horizontal Axes")
                axisH.ViewObject.BubblePosition = "Both"
                axisH.ViewObject.NumberingStyle = "A,B,C"
                axisH.ViewObject.LineWidth = self.form.lineWidth.value()
                axisH.ViewObject.FontSize = params.get_param("textheight")
                axisH.ViewObject.BubbleSize = params.get_param("textheight") * 1.43
                axisH.Placement.Rotation = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 90)
                axisH.ViewObject.LineColor = color
            if axisV and axisH:
                axisG = Arch.makeAxisSystem([axisH, axisV])
                axisG.Label = translate("BIM", "Axes")
                grp.addObject(axisG)
            else:
                if axisV:
                    grp.addObject(axisV)
                if axisH:
                    grp.addObject(axisH)
            if self.form.countLevels.value() and levelHeight:
                h = 0
                alabels = []
                groups = []
                for i in range(self.form.groupsList.count()):
                    groups.append(self.form.groupsList.item(i).text())
                for i in range(self.form.countLevels.value()):
                    lev = Arch.makeFloor()
                    lev.Label = translate("BIM", "Level") + " " + str(i)
                    alabels.append(lev.Label)
                    lev.Height = levelHeight
                    lev.Placement.move(FreeCAD.Vector(0, 0, h))
                    if self.project and self.building:
                        from nativeifc import ifc_tools
                        lev = ifc_tools.aggregate(lev, self.building)
                    elif self.building:
                        self.building.addObject(lev)
                    h += levelHeight
                    for group in groups:
                        levGroup = FreeCAD.ActiveDocument.addObject(
                            "App::DocumentObjectGroup"
                        )
                        levGroup.Label = group
                        if self.project:
                            from nativeifc import ifc_tools
                            ifc_tools.aggregate(levGroup, lev)
                        else:
                            lev.addObject(levGroup)
        FreeCAD.ActiveDocument.recompute()
        # fit zoom
        if outline:
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(outline)
            FreeCADGui.SendMsgToActiveView("ViewSelection")
            FreeCADGui.Selection.clearSelection()
        # aggregate layout group
        if self.building and grp:
            if hasattr(self.building, "IfcClass"):
                from nativeifc import ifc_tools
                ifc_tools.aggregate(grp, self.building)
        self.form.hide()
        FreeCAD.ActiveDocument.recompute()
        if hasattr(FreeCADGui, "Snapper"):
            FreeCADGui.Snapper.show()
        if self.form.radioNative3.isChecked():
            from nativeifc import ifc_status
            ifc_status.set_button(True,True)
        return True

    def addGroup(self):
        from PySide import QtCore, QtGui

        it = QtGui.QListWidgetItem(translate("BIM", "New Group"))
        it.setFlags(it.flags() | QtCore.Qt.ItemIsEditable)
        self.form.groupsList.addItem(it)

    def delGroup(self):
        r = self.form.groupsList.currentRow()
        if r >= 0:
            self.form.groupsList.takeItem(r)

    def savePreset(self):
        import Arch
        from PySide import QtGui

        res = QtGui.QInputDialog.getText(
            None,
            translate("BIM", "Save preset"),
            translate("BIM", "Preset name:"),
            QtGui.QLineEdit.Normal,
            "DefaultProject",
        )
        if res[1]:
            name = res[0]
            presetdir = os.path.join(FreeCAD.getUserAppDataDir(), "BIM")
            if not os.path.isdir(presetdir):
                os.mkdir(presetdir)

            groups = []
            for i in range(self.form.groupsList.count()):
                groups.append(self.form.groupsList.item(i).text())

            s = "# FreeCAD BIM Project setup preset " + name + "\n"
            s += (
                "groupNewDocument="
                + str(int(self.form.groupNewProject.isChecked()))
                + "\n"
            )
            s += "projectName=" + self.form.projectName.text() + "\n"
            s += "groupSite=" + str(int(self.form.groupSite.isChecked())) + "\n"

            s += "siteName=" + self.form.siteName.text() + "\n"
            s += "siteAddress=" + self.form.siteAddress.text() + "\n"
            s += "siteLongitude=" + str(self.form.siteLongitude.value()) + "\n"
            s += "siteLatitude=" + str(self.form.siteLatitude.value()) + "\n"
            s += "siteDeviation=" + str(self.form.siteDeviation.value()) + "\n"
            s += "siteElevation=" + self.form.siteElevation.text() + "\n"

            s += "groupBuilding=" + str(int(self.form.groupBuilding.isChecked())) + "\n"
            s += "buildingName=" + self.form.buildingName.text() + "\n"
            s += "buildingUse=" + str(self.form.buildingUse.currentIndex()) + "\n"
            s += "buildingLength=" + self.form.buildingLength.text() + "\n"
            s += "buildingWidth=" + self.form.buildingWidth.text() + "\n"
            s += "countVAxes=" + str(self.form.countVAxes.value()) + "\n"
            s += "distVAxes=" + self.form.distVAxes.text() + "\n"
            s += "countHAxes=" + str(self.form.countHAxes.value()) + "\n"
            s += "distHAxes=" + self.form.distHAxes.text() + "\n"
            s += "countLevels=" + str(self.form.countLevels.value()) + "\n"
            s += "levelHeight=" + self.form.levelHeight.text() + "\n"
            s += "lineWidth=" + str(self.form.lineWidth.value()) + "\n"
            s += (
                "lineColor="
                + str(self.form.lineColor.property("color").getRgbF()[:3])
                + "\n"
            )
            s += "groups=" + ";;".join(groups) + "\n"
            s += (
                "addHumanFigure="
                + str(int(self.form.addHumanFigure.isChecked()))
                + "\n"
            )

            f = open(os.path.join(presetdir, name + ".txt"), "w")
            f.write(s)
            f.close()
            self.fillPresets()

    def fillPresets(self):
        self.form.presets.clear()
        self.form.presets.addItem(translate("BIM", "User preset"))
        presetdir = os.path.join(FreeCAD.getUserAppDataDir(), "BIM")
        if os.path.isdir(presetdir):
            for f in os.listdir(presetdir):
                if f.endswith(".txt"):
                    self.form.presets.addItem(os.path.splitext(f)[0])

    def getPreset(self, preset):
        import Arch
        from PySide import QtGui

        preset = self.form.presets.itemText(preset)
        pfile = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", preset + ".txt")
        if os.path.exists(pfile):
            f = open(pfile, "r")
            buf = f.read()
            f.close()
            lines = buf.split("\n")
            for line in lines:
                if line:
                    if line[0] != "#":
                        s = line.split("=")
                        if s[0] == "groupNewDocument":
                            self.form.groupNewProject.setChecked(bool(int(s[1])))
                        elif s[0] == "projectName":
                            self.form.projectName.setText(s[1])
                        elif s[0] == "groupSite":
                            self.form.groupSite.setChecked(bool(int(s[1])))
                        elif s[0] == "siteName":
                            self.form.siteName.setText(s[1])
                        elif s[0] == "siteAddress":
                            self.form.siteAddress.setText(s[1])
                        elif s[0] == "siteLongitude":
                            self.form.siteLongitude.setValue(float(s[1]))
                        elif s[0] == "siteLatitude":
                            self.form.siteLatitude.setValue(float(s[1]))
                        elif s[0] == "siteDeviation":
                            self.form.siteDeviation.setValue(float(s[1]))
                        elif s[0] == "siteElevation":
                            self.form.siteElevation.setText(s[1])
                        elif s[0] == "groupBuilding":
                            self.form.groupBuilding.setChecked(bool(int(s[1])))
                        elif s[0] == "buildingName":
                            self.form.buildingName.setText(s[1])
                        elif s[0] == "buildingUse":
                            self.form.buildingUse.setCurrentIndex(int(s[1]))
                        elif s[0] == "buildingLength":
                            self.form.buildingLength.setText(s[1])
                        elif s[0] == "buildingWidth":
                            self.form.buildingWidth.setText(s[1])
                        elif s[0] == "countVAxes":
                            self.form.countVAxes.setValue(int(s[1]))
                        elif s[0] == "distVAxes":
                            self.form.distVAxes.setText(s[1])
                        elif s[0] == "countHAxes":
                            self.form.countHAxes.setValue(int(s[1]))
                        elif s[0] == "distHAxes":
                            self.form.distHAxes.setText(s[1])
                        elif s[0] == "countLevels":
                            self.form.countLevels.setValue(int(s[1]))
                        elif s[0] == "levelHeight":
                            self.form.levelHeight.setText(s[1])
                        elif s[0] == "lineWidth":
                            self.form.lineWidth.setValue(int(s[1]))
                        elif s[0] == "lineColor":
                            col = tuple(
                                [
                                    float(t)
                                    for t in s[1].strip("(").strip(")").split(",")
                                ]
                            )
                            col = QtGui.QColor.fromRgbF(*col)
                            self.form.lineColor.setProperty("color", col)
                        elif s[0] == "groups":
                            groups = s[1].split(";;")
                            self.form.groupsList.clear()
                            self.form.groupsList.addItems(groups)
                        elif s[0] == "addHumanFigure":
                            self.form.addHumanFigure.setChecked(bool(int(s[1])))

    def saveTemplate(self):
        """saves the contents of the current file as a template"""

        import WorkingPlane

        d = FreeCAD.ActiveDocument
        if not d:
            d = FreeCAD.newDocument()

        # build list of useful settings to store
        wp = WorkingPlane.get_working_plane()
        values = {}
        values["wpposition"] = str(wp.position)
        values["wpu"] = str(wp.u)
        values["wpv"] = str(wp.v)
        values["wpaxis"] = str(wp.axis)
        values["unit"] = str(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt(
                "UserSchema", 0
            )
        )
        values["textsize"] = str(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetFloat(
                "textheight", 10
            )
        )
        values["textfont"] = str(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetString(
                "textfont", "Sans"
            )
        )
        values["dimstyle"] = str(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetInt(
                "dimsymbol", 0
            )
        )
        values["arrowsize"] = str(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetFloat(
                "arrowsize", 5
            )
        )
        values["decimals"] = str(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt(
                "Decimals", 2
            )
        )
        values["grid"] = str(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetFloat(
                "gridSpacing", 10
            )
        )
        values["squares"] = str(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetInt(
                "gridEvery", 10
            )
        )
        values["linewidth"] = str(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").GetInt(
                "DefautShapeLineWidth", 2
            )
        )
        values["colFace"] = str(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").GetUnsigned(
                "DefaultShapeColor", 4294967295
            )
        )
        values["colLine"] = str(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").GetUnsigned(
                "DefaultShapeLineColor", 255
            )
        )
        values["colHelp"] = str(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetUnsigned(
                "ColorHelpers", 674321151
            )
        )
        values["colConst"] = str(
            FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Draft"
            ).GetUnsigned("constructioncolor", 746455039)
        )

        d.Meta = values
        from PySide import QtGui

        filename = QtGui.QFileDialog.getSaveFileName(
            QtGui.QApplication.activeWindow(),
            translate("BIM", "Save template file"),
            None,
            "FreeCAD file (*.FCStd)",
        )
        if filename:
            filename = filename[0]
            if not filename.lower().endswith(".FCStd"):
                filename += ".FCStd"
            d.saveCopy(filename)
            FreeCAD.Console.PrintMessage(
                translate("BIM", "Template saved successfully") + "\n"
            )
            self.reject()

    def loadTemplate(self):
        """loads the contents of a template into the current file"""

        from PySide import QtGui
        import FreeCADGui
        import WorkingPlane
        from FreeCAD import Vector  # required for following eval calls

        filename = QtGui.QFileDialog.getOpenFileName(
            QtGui.QApplication.activeWindow(),
            translate("BIM", "Open template file"),
            None,
            "FreeCAD file (*.FCStd)",
        )
        if filename:
            filename = filename[0]
            if FreeCAD.ActiveDocument:
                d = FreeCAD.ActiveDocument
                td = FreeCAD.openDocument(filename, True)  # hidden
                tname = td.Name
                values = td.Meta
                FreeCAD.closeDocument(tname)
                d.mergeProject(filename)
                FreeCADGui.ActiveDocument = FreeCADGui.getDocument(
                    d.Name
                )  # fix b/c hidden doc
            else:
                d = FreeCAD.openDocument(filename)
                FreeCAD.ActiveDocument = d
                values = d.Meta
            bimunit = 0
            wp = WorkingPlane.get_working_plane()
            if "wpposition" in values:
                wp.position = eval(values["wpposition"])
            if "wpu" in values:
                wp.u = eval(values["wpu"])
            if "wpv" in values:
                wp.v = eval(values["wpv"])
            if "wpaxis" in values:
                wp.axis = eval(values["wpaxis"])
            wp._handle_custom(_hist_add=True)  # update the widget
            if "unit" in values:
                FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").SetInt(
                    "UserSchema", int(values["unit"])
                )
                if hasattr(FreeCAD.Units, "setSchema"):
                    FreeCAD.Units.setSchema(int(values["unit"]))
                    bimunit = [0, 2, 3, 3, 1, 5, 0, 4][int(values["unit"])]
            if "textsize" in values:
                FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/Draft"
                ).SetFloat("textheight", float(values["textsize"]))
                if hasattr(FreeCADGui, "draftToolBar"):
                    FreeCADGui.draftToolBar.fontsizeButton.setValue(
                        float(values["textsize"])
                    )
            if "textfont" in values:
                FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/Draft"
                ).SetString("textfont", values["textfont"])
            if "dimstyle" in values:
                FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetInt(
                    "dimsymbol", int(values["dimstyle"])
                )
            if "arrowsize" in values:
                FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/Draft"
                ).SetFloat("arrowsize", float(values["arrowsize"]))
            if "decimals" in values:
                FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").SetInt(
                    "Decimals", int(values["decimals"])
                )
            if "grid" in values:
                FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/Draft"
                ).SetFloat("gridSpacing", float(values["grid"]))
            if "squares" in values:
                FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetInt(
                    "gridEvery", int(values["squares"])
                )
            if hasattr(FreeCADGui, "Snapper"):
                FreeCADGui.Snapper.setGrid()
            if "linewidth" in values:
                FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").SetInt(
                    "DefautShapeLineWidth", int(values["linewidth"])
                )
                if hasattr(FreeCADGui, "draftToolBar"):
                    FreeCADGui.draftToolBar.widthButton.setValue(
                        int(values["linewidth"])
                    )
            if "colFace" in values:
                FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").SetUnsigned(
                    "DefaultShapeColor", int(values["colFace"])
                )
            if "colLine" in values:
                FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").SetUnsigned(
                    "DefaultShapeLineColor", int(values["colLine"])
                )
            if "colHelp" in values:
                FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/Arch"
                ).SetUnsigned("ColorHelpers", int(values["colHelp"]))
            if "colConst" in values:
                FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/Draft"
                ).SetUnsigned("constructioncolor", int(values["colConst"]))

            # set the status bar widgets
            mw = FreeCADGui.getMainWindow()
            if mw:
                st = mw.statusBar()
                statuswidget = st.findChild(QtGui.QToolBar, "BIMStatusWidget")
                if statuswidget:
                    statuswidget.unitLabel.setText(statuswidget.unitsList[bimunit])
                    # change the unit of the nudge button
                    nudgeactions = statuswidget.nudge.menu().actions()
                    if bimunit in [2, 3, 5, 7]:
                        nudgelabels = statuswidget.nudgeLabelsI
                    else:
                        nudgelabels = statuswidget.nudgeLabelsM
                    for i in range(len(nudgelabels)):
                        nudgeactions[i].setText(nudgelabels[i])
                    if not "auto" in statuswidget.nudge.text().replace("&", "").lower():
                        statuswidget.nudge.setText(
                            FreeCAD.Units.Quantity(
                                statuswidget.nudge.text().replace("&", "")
                            ).UserString
                        )

            FreeCAD.Console.PrintMessage(
                translate("BIM", "Template successfully loaded into the current document")
                + "\n"
            )
            self.reject()


FreeCADGui.addCommand("BIM_ProjectManager", BIM_ProjectManager())
