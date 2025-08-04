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

"""The BIM Setup command"""

import os
import sys

import FreeCAD
import FreeCADGui

translate = FreeCAD.Qt.translate
QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Setup:

    def GetResources(self):
        return {
            "Pixmap": ":icons/preferences-system.svg",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Setup", "BIM Setup"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Setup", "Sets common FreeCAD preferences for a BIM workflow"
            ),
        }

    def Activated(self):

        TARGETVERSION = 0.19
        TECHDRAWDIMFACTOR = (
            0.16  # How many times TechDraw dim arrows are smaller than Draft
        )

        from PySide import QtGui
        import WorkingPlane

        # load dialog
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogSetup.ui")

        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.form.move(
            mw.frameGeometry().topLeft()
            + mw.rect().center()
            - self.form.rect().center()
        )

        # connect signals / slots
        self.form.comboPresets.currentIndexChanged[int].connect(self.setPreset)
        self.form.labelIfcOpenShell.linkActivated.connect(self.handleLink)

        # fill default values
        self.setPreset(None)

        # check missing addons
        self.form.labelMissingWorkbenches.hide()
        self.form.labelIfcOpenShell.hide()
        self.form.labelSnapTip.hide()
        self.form.labelVersion.hide()
        m = []
        try:
            import RebarTools
        except ImportError:
            m.append("Reinforcement")
        # disabled as WebTools can currentyl not be installed because of WebGui dependency
        #try:
        #    import BIMServer
        #except ImportError:
        #    m.append("WebTools")
        if sys.version_info.major < 3:
            try:
                import CommandsFrame
            except ImportError:
                m.append("Flamingo")
        else:
            try:
                import CFrame
            except ImportError:
                m.append("Dodo")
        try:
            import FastenerBase
        except ImportError:
            m.append("Fasteners")
        try:
            import report
        except ImportError:
            m.append("Reporting")
        try:
            import ifcopenshell
        except ImportError:
            ifcok = False
        else:
            ifcok = True
        libok = False
        librarypath = FreeCAD.ParamGet(
            "User parameter:Plugins/parts_library"
        ).GetString("destination", "")
        if librarypath and os.path.exists(librarypath):
            libok = True
        else:
            # check if the library is at the standard addon location
            librarypath = os.path.join(
                FreeCAD.getUserAppDataDir(), "Mod", "parts_library"
            )
            if os.path.exists(librarypath):
                FreeCAD.ParamGet("User parameter:Plugins/parts_library").SetString(
                    "destination", librarypath
                )
                libok = True
        if not libok:
            m.append("parts_library")
        if m:
            t = (
                translate(
                    "BIM",
                    "Some additional workbenches are not installed, that extend BIM functionality:",
                )
                + " <b>"
                + ",".join(m)
                + "</b>. "
                + translate(
                    "BIM", "Install them from menu Tools -> Addon Manager."
                )
            )
            self.form.labelMissingWorkbenches.setText(t)
            self.form.labelMissingWorkbenches.show()
        if not ifcok:
            self.form.labelIfcOpenShell.show()
        if (
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetString(
                "snapModes", "111111111101111"
            )
            == "111111111101111"
        ):
            self.form.labelSnapTip.show()
        version = float(str(FreeCAD.Version()[0]) + "." + str(FreeCAD.Version()[1]))
        if version < TARGETVERSION:
            t = self.form.labelVersion.text
            self.form.labelVersion.text = t.replace("%1", str(version)).replace(
                "%2", str(TARGETVERSION)
            )
            self.form.labelVersion.show()

        # show dialog and exit if cancelled
        FreeCADGui.BIMSetupDialog = (
            True  # this is there to be easily detected by the BIM tutorial
        )
        result = self.form.exec_()
        del FreeCADGui.BIMSetupDialog
        if not result:
            return

        # set preference values
        unit = self.form.settingUnits.currentIndex()
        unit = [0, 4, 1, 3, 7, 5][unit]  # less choices in our simplified dialog
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").SetInt(
            "UserSchema", unit
        )
        if FreeCAD.ActiveDocument is not None:
            docs_dict = FreeCAD.listDocuments()
            for doc in docs_dict.values():
                doc.UnitSystem = unit
            if len(docs_dict) == 1:
                FreeCAD.Console.PrintWarning(
                    translate("BIM", "Unit system updated for active document") + "\n"
                )
            else:
                FreeCAD.Console.PrintWarning(
                    translate("BIM", "Unit system updated for all opened documents") + "\n"
                )
        if hasattr(FreeCAD.Units, "setSchema"):
            FreeCAD.Units.setSchema(unit)
        decimals = self.form.settingDecimals.value()
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").SetInt(
            "Decimals", decimals
        )
        FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/TechDraw/Dimensions"
        ).SetBool("UseGlobalDecimals", True)
        grid = self.form.settingGrid.text()
        FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/General"
        ).SetString(
            "GridSize", grid
        )  # Also set sketcher grid
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetString(
            "gridSpacing", grid
        )
        squares = self.form.settingSquares.value()
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetInt(
            "gridEvery", squares
        )
        wp = self.form.settingWP.currentIndex()
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetInt(
            "defaultWP", wp
        )
        tsize = self.form.settingText.text()
        tsize = FreeCAD.Units.Quantity(tsize).Value
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetFloat(
            "textheight", tsize
        )
        FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/TechDraw/Dimensions"
        ).SetFloat(
            "FontSize", tsize
        )  # TODO - check if this needs a mult factor?
        font = self.form.settingFont.currentFont().family()
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetString(
            "textfont", font
        )
        FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/TechDraw/Labels"
        ).SetString("LabelFont", font)
        linewidth = self.form.settingLinewidth.value()
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").SetInt(
            "DefaultShapeLineWidth", linewidth
        )
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetInt(
            "linewidth", linewidth
        )
        # TODO - TechDraw default line styles
        dimstyle = self.form.settingDimstyle.currentIndex()
        ddimstyle = [0, 2, 3, 4][dimstyle]  # less choices in our simplified dialog
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetInt(
            "dimsymbol", ddimstyle
        )
        tdimstyle = [3, 0, 2, 2][dimstyle]  # TechDraw has different order than Draft
        FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/TechDraw/Dimensions"
        ).SetInt("dimsymbol", tdimstyle)
        asize = self.form.settingArrowsize.text()
        asize = FreeCAD.Units.Quantity(asize).Value
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetFloat(
            "arrowsize", asize
        )
        FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/TechDraw/Dimensions"
        ).SetFloat("ArrowSize", asize * TECHDRAWDIMFACTOR)
        author = self.form.settingAuthor.text()
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Document").SetString(
            "prefAuthor", author
        )
        lic = self.form.settingLicense.currentIndex()
        lic = [0, 1, 2, 4, 5][lic]  # less choices in our simplified dialog
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Document").SetInt(
            "prefLicenseType", lic
        )
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Document").SetString(
            "prefLicenseUrl", ""
        )  # TODO - set correct license URL
        newdoc = self.form.settingNewdocument.isChecked()
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Document").SetBool(
            "CreateNewDoc", newdoc
        )
        bkp = self.form.settingBackupfiles.value()
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Document").SetInt(
            "CountBackupFiles", bkp
        )
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").SetUnsigned(
            "BackgroundColor2", self.form.colorButtonTop.property("color").rgb() << 8
        )
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").SetUnsigned(
            "BackgroundColor3", self.form.colorButtonBottom.property("color").rgb() << 8
        )
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").SetUnsigned(
            "DefaultShapeColor", self.form.colorButtonFaces.property("color").rgb() << 8
        )
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetUnsigned(
            "color", self.form.colorButtonFaces.property("color").rgb() << 8
        )
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").SetUnsigned(
            "DefaultShapeLineColor",
            self.form.colorButtonLines.property("color").rgb() << 8,
        )
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").SetUnsigned(
            "ColorHelpers", self.form.colorButtonHelpers.property("color").rgb() << 8
        )
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetUnsigned(
            "constructioncolor",
            self.form.colorButtonConstruction.property("color").rgb() << 8,
        )
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").SetUnsigned(
            "ConstructionColor",
            self.form.colorButtonConstruction.property("color").rgb() << 8,
        )
        height = self.form.settingCameraHeight.value()
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetInt(
            "defaultCameraHeight", height
        )
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetUnsigned(
            "DefaultTextColor", self.form.colorButtonText.property("color").rgb() << 8
        )
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").SetUnsigned(
            "BackgroundColor", self.form.colorButtonSimple.property("color").rgb() << 8
        )

        # set the working plane
        wplane = WorkingPlane.get_working_plane()
        if wp == 1:
            wplane.set_to_top()
        elif wp == 2:
            wplane.set_to_front()
        elif wp == 3:
            wplane.set_to_side()
        else:
            wplane.set_to_auto()

        # set Draft toolbar
        if hasattr(FreeCADGui, "draftToolBar"):
            if hasattr(FreeCADGui.draftToolBar, "setStyleButton"):
                FreeCADGui.draftToolBar.setStyleButton()
            else:
                # pre-v0.19
                FreeCADGui.draftToolBar.widthButton.setValue(linewidth)
                FreeCADGui.draftToolBar.fontsizeButton.setValue(tsize)

        # set the grid
        if hasattr(FreeCADGui, "Snapper"):
            FreeCADGui.Snapper.setGrid()

        # set the status bar widgets
        mw = FreeCADGui.getMainWindow()
        if mw:
            st = mw.statusBar()
            statuswidget = st.findChild(QtGui.QToolBar, "BIMStatusWidget")
            if statuswidget:
                if hasattr(statuswidget, "unitLabel"):
                    statuswidget.unitLabel.setText(
                        statuswidget.unitsList[self.form.settingUnits.currentIndex()]
                    )
                # change the unit of the nudge button
                nudgeactions = statuswidget.nudge.menu().actions()
                if unit in [2, 3, 5, 7]:
                    nudgelabels = statuswidget.nudgeLabelsI
                else:
                    nudgelabels = statuswidget.nudgeLabelsM
                for i in range(len(nudgelabels)):
                    nudgeactions[i].setText(nudgelabels[i])
                try:
                    t = FreeCAD.Units.Quantity(
                        statuswidget.nudge.text().replace("&", "")
                    ).UserString
                except:
                    pass  # auto mode
                else:
                    statuswidget.nudge.setText(t)

        # Set different default values
        if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM").GetBool(
            "FirstTime", True
        ):
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetString(
                "svgDashedLine", "3,1"
            )
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetString(
                "svgDashdotLine", "3,1,0.2,1"
            )
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetString(
                "svgDottedLine", "0.5,1"
            )
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetFloat(
                "HatchPatternSize", 0.025
            )

        # finish
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM").SetBool(
            "FirstTime", False
        )

    def setPreset(self, preset=None):
        from PySide import QtGui

        unit = None
        decimals = None
        grid = None
        squares = None
        wp = None
        tsize = None
        font = None
        linewidth = None
        dimstyle = None
        asize = None
        author = None
        lic = None
        bimdefault = None
        newdoc = None
        bkp = None
        colTop = None
        colBottom = None
        colFace = None
        colLine = None
        colHelp = None
        colConst = None
        height = None
        colSimple = None
        colText = None

        if preset == 0:
            # the "Choose..." item from the presets box. Do nothing
            return

        elif preset == 1:
            # centimeters
            unit = 1
            decimals = 2
            grid = "10cm"
            squares = 10
            tsize = "20cm"
            linewidth = 1
            dimstyle = 0
            asize = "4cm"
            bkp = 2
            bimdefault = 2
            newdoc = False
            height = 4500

        elif preset == 2:
            # meters
            unit = 2
            decimals = 2
            grid = "0.1m"
            squares = 10
            tsize = "0.2m"
            linewidth = 1
            dimstyle = 0
            asize = "0.04m"
            bkp = 2
            bimdefault = 2
            newdoc = False
            height = 4500

        elif preset == 3:
            # US
            unit = 5
            decimals = 2
            grid = "1in"
            squares = 12
            tsize = "8in"
            linewidth = 1
            dimstyle = 3
            asize = "2in"
            bkp = 2
            bimdefault = 2
            newdoc = False
            height = 4500

        elif preset is None:
            # get values from settings
            unit = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt(
                "UserSchema", 0
            )
            unit = [0, 2, 3, 3, 1, 5, 0, 4, 0, 2][
                unit
            ]  # less choices in our simplified dialog
            decimals = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Units"
            ).GetInt("Decimals", 2)
            grid = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Draft"
            ).GetString("gridSpacing", "1 cm")
            grid = FreeCAD.Units.Quantity(grid).UserString
            squares = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Draft"
            ).GetInt("gridEvery", 10)
            wp = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Draft"
            ).GetInt("defaultWP", 1)
            tsize = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Draft"
            ).GetFloat("textheight", 10)
            tsize = FreeCAD.Units.Quantity(tsize, FreeCAD.Units.Length).UserString
            font = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Draft"
            ).GetString("textfont", "Sans")
            linewidth = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/View"
            ).GetInt("DefaultShapeLineWidth", 2)
            dimstyle = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Draft"
            ).GetInt("dimsymbol", 0)
            dimstyle = [0, 0, 1, 2, 3][
                dimstyle
            ]  # less choices in our simplified dialog
            asize = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Draft"
            ).GetFloat("arrowsize", 5)
            asize = FreeCAD.Units.Quantity(asize, FreeCAD.Units.Length).UserString
            author = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Document"
            ).GetString("prefAuthor", "")
            lic = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Document"
            ).GetInt("prefLicenseType", 0)
            lic = [0,
                   1, 2, 1, 3, 4, 1,
                   1, 2, 1, 3, 4, 1,
                   0, 0, 0, 0, 0, 0][
                lic
            ]  # less choices in our simplified dialog
            newdoc = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Document"
            ).GetBool("CreateNewDoc", False)
            bkp = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Document"
            ).GetInt("CountBackupFiles", 2)
            colTop = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/View"
            ).GetUnsigned("BackgroundColor2", 775244287)
            colBottom = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/View"
            ).GetUnsigned("BackgroundColor3", 1905041919)
            colFace = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/View"
            ).GetUnsigned("DefaultShapeColor", 4294967295)
            colLine = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/View"
            ).GetUnsigned("DefaultShapeLineColor", 255)
            colHelp = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Arch"
            ).GetUnsigned("ColorHelpers", 674321151)
            colConst = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Draft"
            ).GetUnsigned("constructioncolor", 746455039)
            height = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Draft"
            ).GetInt("defaultCameraHeight", 5)
            colSimple = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/View"
            ).GetUnsigned("BackgroundColor", 4294967295)
            colText = FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Draft"
            ).GetUnsigned("DefaultTextColor", 255)

        if unit != None:
            self.form.settingUnits.setCurrentIndex(unit)
        if decimals != None:
            self.form.settingDecimals.setValue(decimals)
        if grid != None:
            self.form.settingGrid.setText(grid)
        if squares != None:
            self.form.settingSquares.setValue(squares)
        if wp != None:
            self.form.settingWP.setCurrentIndex(wp)
        if tsize != None:
            self.form.settingText.setText(tsize)
        if font != None:
            self.form.settingFont.setCurrentFont(QtGui.QFont(font))
        if linewidth != None:
            self.form.settingLinewidth.setValue(linewidth)
        if dimstyle != None:
            self.form.settingDimstyle.setCurrentIndex(dimstyle)
        if asize != None:
            self.form.settingArrowsize.setText(asize)
        if author != None:
            self.form.settingAuthor.setText(author)
        if lic != None:
            self.form.settingLicense.setCurrentIndex(lic)
        if newdoc != None:
            self.form.settingNewdocument.setChecked(newdoc)
        if bkp != None:
            self.form.settingBackupfiles.setValue(bkp)
        if colTop != None:
            self.form.colorButtonTop.setProperty("color", self.getPrefColor(colTop))
        if colBottom != None:
            self.form.colorButtonBottom.setProperty("color", self.getPrefColor(colBottom))
        if colFace != None:
            self.form.colorButtonFaces.setProperty("color", self.getPrefColor(colFace))
        if colLine != None:
            self.form.colorButtonLines.setProperty("color", self.getPrefColor(colLine))
        if colHelp != None:
            self.form.colorButtonHelpers.setProperty("color", self.getPrefColor(colHelp))
        if colConst != None:
            self.form.colorButtonConstruction.setProperty(
                "color", self.getPrefColor(colConst)
            )
        if colSimple != None:
            self.form.colorButtonSimple.setProperty("color", self.getPrefColor(colSimple))
        if colText != None:
            self.form.colorButtonText.setProperty("color", self.getPrefColor(colText))
        if height:
            self.form.settingCameraHeight.setValue(height)
        # TODO - antialiasing?

    def handleLink(self, link):
        if hasattr(self, "form"):
            if "#install" in link:
                self.getIfcOpenShell()
            else:
                # print("Opening link:",link)
                from PySide import QtCore, QtGui

                url = QtCore.QUrl(link)
                QtGui.QDesktopServices.openUrl(url)


    def getPrefColor(self,color):
        r = ((color >> 24) & 0xFF) / 255.0
        g = ((color >> 16) & 0xFF) / 255.0
        b = ((color >> 8) & 0xFF) / 255.0
        from PySide import QtGui

        return QtGui.QColor.fromRgbF(r, g, b)


    def getIfcOpenShell(self,force=False):
        """downloads and installs IfcOpenShell"""

        # TODO WARNING the IfcOpenBot repo below is not actively kept updated.
        # We need to use PIP

        ifcok = False
        if not force:
            try:
                import ifcopenshell
            except:
                ifcok = False
            else:
                ifcok = False
                v = [int(i) for i in ifcopenshell.version.split(".")]
                if v[0] < 1:
                    if v[1] > 6:
                        ifcok = True
        if not ifcok:
            # ifcopenshell not installed
            import json
            import re
            from urllib import request
            import zipfile
            from PySide import QtGui

            if not FreeCAD.GuiUp:
                reply = QtGui.QMessageBox.Yes
            else:
                reply = QtGui.QMessageBox.question(
                    None,
                    translate("BIM", "IfcOpenShell not found"),
                    translate(
                        "BIM",
                        "IfcOpenShell is needed to import and export IFC files. It appears to be missing on the system. Download and install it now? It will be installed in FreeCAD's macros directory.",
                    ),
                    QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                    QtGui.QMessageBox.No,
                )
            if reply == QtGui.QMessageBox.Yes:
                print(
                    "Loading list of latest IfcOpenBot builds from https://github.com/IfcOpenBot/IfcOpenShell..."
                )
                url1 = "https://api.github.com/repos/IfcOpenBot/IfcOpenShell/comments?per_page=100"
                u = request.urlopen(url1)
                if u:
                    r = u.read()
                    u.close()
                    d = json.loads(r)
                    l = d[-1]["body"]
                    links = re.findall(r"http.*?zip", l)
                    pyv = (
                        "python-"
                        + str(sys.version_info.major)
                        + str(sys.version_info.minor)
                    )
                    if sys.platform.startswith("linux"):
                        plat = "linux"
                    elif sys.platform.startswith("win"):
                        plat = "win"
                    elif sys.platform.startswith("darwin"):
                        plat = "macos"
                    else:
                        FreeCAD.Console.PrintError("Error - unknown platform")
                        return
                    if sys.maxsize > 2**32:
                        plat += "64"
                    else:
                        plat += "32"
                    print("Looking for", plat, pyv)
                    for link in links:
                        if ("ifcopenshell-" + pyv in link) and (plat in link):
                            print("Downloading " + link + "...")
                            p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro")
                            fp = p.GetString(
                                "MacroPath",
                                os.path.join(FreeCAD.getUserAppDataDir(), "Macros"),
                            )
                            u = request.urlopen(link)
                            if u:
                                if sys.version_info.major < 3:
                                    import StringIO as io

                                    _stringio = io.StringIO
                                else:
                                    import io

                                    _stringio = io.BytesIO
                                zfile = _stringio()
                                zfile.write(u.read())
                                zfile = zipfile.ZipFile(zfile)
                                zfile.extractall(fp)
                                u.close()
                                zfile.close()
                                print("Successfully installed IfcOpenShell to", fp)
                                break
                    else:
                        FreeCAD.Console.PrintWarning(
                            "Unable to find a build for this version, therefore falling back to a pip install"
                        )
                        try:
                            import pip
                        except ModuleNotFoundError:
                            FreeCAD.Console.PrintError(
                                "Pnstall pip on your system, restart FreeCAD,"
                                " change to the BIM workbench and navigate the menu: Utils > ifcOpenShell Update"
                            )
                            return
                        from nativeifc import ifc_openshell

                        FreeCADGui.runCommand('IFC_UpdateIOS',1)

FreeCADGui.addCommand("BIM_Setup", BIM_Setup())
