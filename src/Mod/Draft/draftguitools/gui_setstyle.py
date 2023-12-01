# ***************************************************************************
# *   Copyright (c) 2020 Yorik van Havre <yorik@uncreated.net>              *
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

"""Provides GUI tools to set up default styles."""

## @package gui_setstyle
# \ingroup draftguitools
# \brief Provides GUI tools to set Draft styles such as color or line width

## \addtogroup draftguitools
# @{

from PySide import QtCore
from PySide import QtGui

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import os

from FreeCAD import Units as U
from draftutils import utils

def QT_TRANSLATE_NOOP(ctx,txt):
    return txt
translate = App.Qt.translate

__title__ = "FreeCAD Draft Workbench GUI Tools - Styling tools"
__author__ = ("Yorik van Havre")
__url__ = "https://www.freecad.org"

PRESETPATH = os.path.join(App.getUserAppDataDir(), "Draft", "StylePresets.json")


class Draft_SetStyle:

    """The Draft_SetStyle FreeCAD command definition."""

    def GetResources(self):

        return {
            "Pixmap": "Draft_Apply",
            "Accel": "S, S",
            "MenuText": QT_TRANSLATE_NOOP("Draft_SetStyle", "Set style"),
            "ToolTip": QT_TRANSLATE_NOOP("Draft_SetStyle", "Sets default styles")
        }

    def Activated(self):

        Gui.Control.showDialog(Draft_SetStyle_TaskPanel())


class Draft_SetStyle_TaskPanel:

    """The task panel for the Draft_SetStyle command"""

    def __init__(self):

        self.p = "User parameter:BaseApp/Preferences/"
        param_draft = App.ParamGet(self.p + "Mod/Draft")
        param_view  = App.ParamGet(self.p + "View")

        self.form = Gui.PySideUic.loadUi(":/ui/TaskPanel_SetStyle.ui")
        self.form.setWindowIcon(QtGui.QIcon.fromTheme("gtk-apply", QtGui.QIcon(":/icons/Draft_Apply.svg")))

        self.form.saveButton.setIcon(QtGui.QIcon.fromTheme("gtk-save", QtGui.QIcon(":/icons/document-save.svg")))
        self.form.applyButton.setIcon(QtGui.QIcon.fromTheme("gtk-apply", QtGui.QIcon(":/icons/Draft_Apply.svg")))
        self.form.annotButton.setIcon(QtGui.QIcon(":/icons/Draft_Text.svg"))

        self.form.ShapeColor.setProperty("color", self.getColor(param_view.GetUnsigned("DefaultShapeColor", 3435973887)))
        self.form.Transparency.setValue(param_view.GetInt("DefaultShapeTransparency", 0))
        self.form.LineColor.setProperty("color", self.getColor(param_view.GetUnsigned("DefaultShapeLineColor", 255)))
        self.form.LineWidth.setValue(param_view.GetInt("DefaultShapeLineWidth", 2))
        self.form.PointColor.setProperty("color", self.getColor(param_view.GetUnsigned("DefaultShapeVertexColor", 255)))
        self.form.PointSize.setValue(param_view.GetInt("DefaultShapePointSize", 2))
        self.form.DrawStyle.setCurrentIndex(param_draft.GetInt("DefaultDrawStyle", 0))
        self.form.DisplayMode.setCurrentIndex(param_draft.GetInt("DefaultDisplayMode", 0))
        self.form.TextColor.setProperty("color", self.getColor(param_draft.GetUnsigned("DefaultTextColor", 255)))
        self.form.TextFont.setCurrentFont(QtGui.QFont(param_draft.GetString("textfont", "Sans")))
        self.form.TextSize.setText(U.Quantity(param_draft.GetFloat("textheight", 3.5), U.Length).UserString)
        self.form.LineSpacing.setValue(param_draft.GetFloat("LineSpacing", 1))
        self.form.AnnoLineColor.setProperty("color", self.getColor(param_draft.GetUnsigned("DefaultAnnoLineColor", 255)))
        self.form.AnnoLineWidth.setValue(param_draft.GetInt("DefaultAnnoLineWidth", 2))
        self.form.ArrowStyle.setCurrentIndex(param_draft.GetInt("dimsymbol", 0))
        self.form.ArrowSize.setText(U.Quantity(param_draft.GetFloat("arrowsize", 1), U.Length).UserString)
        self.form.ShowUnit.setChecked(param_draft.GetBool("showUnit", True))
        self.form.UnitOverride.setText(param_draft.GetString("overrideUnit", ""))
        self.form.DimOvershoot.setText(U.Quantity(param_draft.GetFloat("dimovershoot", 0), U.Length).UserString)
        self.form.ExtLines.setText(U.Quantity(param_draft.GetFloat("extlines", -0.5), U.Length).UserString)
        self.form.ExtOvershoot.setText(U.Quantity(param_draft.GetFloat("extovershoot", 2), U.Length).UserString)
        self.form.TextSpacing.setText(U.Quantity(param_draft.GetFloat("dimspacing", 1), U.Length).UserString)

        self.form.saveButton.clicked.connect(self.onSaveStyle)
        self.form.applyButton.clicked.connect(self.onApplyStyle)
        self.form.annotButton.clicked.connect(self.onApplyAnnot)
        self.form.comboPresets.currentIndexChanged.connect(self.onLoadStyle)

        self.loadDefaults()

    def loadDefaults(self):

        presets = [self.form.comboPresets.itemText(0)]
        self.form.comboPresets.clear()
        pdict = self.load()
        pdict_keys = list(pdict)
        presets.extend(pdict_keys)
        self.form.comboPresets.addItems(presets)
        current = self.getValues()
        for name, preset in pdict.items():
            if all(item in current.items() for item in preset.items()):  # if preset == current:
                self.form.comboPresets.setCurrentIndex(1 + (pdict_keys.index(name)))
                break

    def getColor(self, c):

        return QtGui.QColor(utils.rgba_to_argb(c))

    def getValues(self):

        preset = {}
        preset["ShapeColor"] = utils.argb_to_rgba(self.form.ShapeColor.property("color").rgba())
        preset["Transparency"] = self.form.Transparency.value()
        preset["LineColor"] = utils.argb_to_rgba(self.form.LineColor.property("color").rgba())
        preset["LineWidth"] = self.form.LineWidth.value()
        preset["PointColor"] = utils.argb_to_rgba(self.form.PointColor.property("color").rgba())
        preset["PointSize"] = self.form.PointSize.value()
        preset["DrawStyle"] = self.form.DrawStyle.currentIndex()
        preset["DisplayMode"] = self.form.DisplayMode.currentIndex()
        preset["TextColor"] = utils.argb_to_rgba(self.form.TextColor.property("color").rgba())
        preset["TextFont"] = self.form.TextFont.currentFont().family()
        preset["TextSize"] = U.Quantity(self.form.TextSize.text()).Value
        preset["LineSpacing"] = self.form.LineSpacing.value()
        preset["AnnoLineColor"] = utils.argb_to_rgba(self.form.AnnoLineColor.property("color").rgba())
        preset["AnnoLineWidth"] = self.form.AnnoLineWidth.value()
        preset["ArrowStyle"] = self.form.ArrowStyle.currentIndex()
        preset["ArrowSize"] = U.Quantity(self.form.ArrowSize.text()).Value
        preset["ShowUnit"] = self.form.ShowUnit.isChecked()
        preset["UnitOverride"] = self.form.UnitOverride.text()
        preset["DimOvershoot"] = U.Quantity(self.form.DimOvershoot.text()).Value
        preset["ExtLines"] = U.Quantity(self.form.ExtLines.text()).Value
        preset["ExtOvershoot"] = U.Quantity(self.form.ExtOvershoot.text()).Value
        preset["TextSpacing"] = U.Quantity(self.form.TextSpacing.text()).Value
        return preset

    def setValues(self,preset):

        # For compatibility with V0.21 and earlier some properties, if missing, revert to others:
        #     'new' prop    -> old prop
        #     ---------------------------
        #     PointColor    -> LineColor
        #     PointSize     -> LineWidth
        #     AnnoLineColor -> LineColor
        #     AnnoLineWidth -> LineWidth
        self.form.ShapeColor.setProperty("color", self.getColor(preset.get("ShapeColor", 3435973887)))
        self.form.Transparency.setValue(preset.get("Transparency", 0))
        self.form.LineColor.setProperty("color", self.getColor(preset.get("LineColor", 255)))
        self.form.LineWidth.setValue(preset.get("LineWidth", 2))
        self.form.PointColor.setProperty("color", self.getColor(preset.get("PointColor", preset.get("LineColor", 255))))
        self.form.PointSize.setValue(preset.get("PointSize", preset.get("LineWidth", 2)))
        self.form.DrawStyle.setCurrentIndex(preset.get("DrawStyle", 0))
        self.form.DisplayMode.setCurrentIndex(preset.get("DisplayMode", 0))
        self.form.TextColor.setProperty("color", self.getColor(preset.get("TextColor", 255)))
        self.form.TextFont.setCurrentFont(QtGui.QFont(preset.get("TextFont", "Sans")))
        self.form.TextSize.setText(U.Quantity(preset.get("TextSize", 3.5),U.Length).UserString)
        self.form.LineSpacing.setValue(preset.get("LineSpacing", 1))
        self.form.AnnoLineColor.setProperty("color", self.getColor(preset.get("AnnoLineColor", preset.get("LineColor", 255))))
        self.form.AnnoLineWidth.setValue(preset.get("AnnoLineWidth", preset.get("LineWidth", 2)))
        self.form.ArrowStyle.setCurrentIndex(preset.get("ArrowStyle", 0))
        self.form.ArrowSize.setText(U.Quantity(preset.get("ArrowSize", 1), U.Length).UserString)
        self.form.ShowUnit.setChecked(preset.get("ShowUnit", True))
        self.form.UnitOverride.setText(preset.get("UnitOverride", ""))
        self.form.DimOvershoot.setText(U.Quantity(preset.get("DimOvershoot", 0), U.Length).UserString)
        self.form.ExtLines.setText(U.Quantity(preset.get("ExtLines", -0.5), U.Length).UserString)
        self.form.ExtOvershoot.setText(U.Quantity(preset.get("ExtOvershoot", 2), U.Length).UserString)
        self.form.TextSpacing.setText(U.Quantity(preset.get("TextSpacing", 1), U.Length).UserString)

    def reject(self):

        Gui.Control.closeDialog()

    def accept(self):

        param_draft = App.ParamGet(self.p + "Mod/Draft")
        param_view  = App.ParamGet(self.p + "View")

        param_view.SetUnsigned("DefaultShapeColor", utils.argb_to_rgba(self.form.ShapeColor.property("color").rgba()))
        param_view.SetInt("DefaultShapeTransparency", self.form.Transparency.value())
        param_view.SetUnsigned("DefaultShapeLineColor", utils.argb_to_rgba(self.form.LineColor.property("color").rgba()))
        param_view.SetInt("DefaultShapeLineWidth", self.form.LineWidth.value())
        param_view.SetUnsigned("DefaultShapeVertexColor", utils.argb_to_rgba(self.form.PointColor.property("color").rgba()))
        param_view.SetInt("DefaultShapePointSize", self.form.PointSize.value())
        param_draft.SetInt("DefaultDrawStyle", self.form.DrawStyle.currentIndex())
        param_draft.SetInt("DefaultDisplayMode", self.form.DisplayMode.currentIndex())
        param_draft.SetUnsigned("DefaultTextColor", utils.argb_to_rgba(self.form.TextColor.property("color").rgba()))
        param_draft.SetString("textfont", self.form.TextFont.currentFont().family())
        param_draft.SetFloat("textheight", U.Quantity(self.form.TextSize.text()).Value)
        param_draft.SetFloat("LineSpacing", self.form.LineSpacing.value())
        param_draft.SetUnsigned("DefaultAnnoLineColor", utils.argb_to_rgba(self.form.AnnoLineColor.property("color").rgba()))
        param_draft.SetInt("DefaultAnnoLineWidth", self.form.AnnoLineWidth.value())
        param_draft.SetInt("dimsymbol", self.form.ArrowStyle.currentIndex())
        param_draft.SetFloat("arrowsize", U.Quantity(self.form.ArrowSize.text()).Value)
        param_draft.SetBool("showUnit", self.form.ShowUnit.isChecked())
        param_draft.SetString("overrideUnit", self.form.UnitOverride.text())
        param_draft.SetFloat("dimovershoot", U.Quantity(self.form.DimOvershoot.text()).Value)
        param_draft.SetFloat("extlines", U.Quantity(self.form.ExtLines.text()).Value)
        param_draft.SetFloat("extovershoot", U.Quantity(self.form.ExtOvershoot.text()).Value)
        param_draft.SetFloat("dimspacing", U.Quantity(self.form.TextSpacing.text()).Value)
        self.reject()

    def onApplyStyle(self):

        for obj in Gui.Selection.getSelection():
            self.apply_style_to_obj(obj)

    def onApplyAnnot(self):

        if App.ActiveDocument is not None:  # Command can be called without a document.
            objs = App.ActiveDocument.Objects
            typs = ["Dimension", "LinearDimension", "AngularDimension",
                    "Text", "DraftText", "Label"]
            for obj in objs:
                if utils.get_type(obj) in typs:
                    self.apply_style_to_obj(obj)

    def apply_style_to_obj(self, obj):

        vobj = obj.ViewObject
        if not vobj:
            return

        properties = vobj.PropertiesList
        if "FontName" not in properties:  # Shapes
            if "ShapeColor" in properties:
                vobj.ShapeColor = self.form.ShapeColor.property("color").getRgbF()[:3] # Remove Alpha (which is 1 instead of 0).
            if "Transparency" in properties:
                vobj.Transparency = self.form.Transparency.value()
            if "LineColor" in properties:
                vobj.LineColor = self.form.LineColor.property("color").getRgbF()[:3]
            if "LineWidth" in properties:
                vobj.LineWidth = self.form.LineWidth.value()
            if "PointColor" in properties:
                vobj.PointColor = self.form.PointColor.property("color").getRgbF()[:3]
            if "PointSize" in properties:
                vobj.PointSize = self.form.PointSize.value()
            if "DrawStyle" in properties:
                dstyles = ["Solid", "Dashed", "Dotted", "Dashdot"]
                vobj.DrawStyle = dstyles[self.form.DrawStyle.currentIndex()]
            if "DisplayMode" in properties:
                dmodes = ["Flat Lines", "Shaded", "Wireframe", "Points"]
                dm = dmodes[self.form.DisplayMode.currentIndex()]
                if dm in vobj.getEnumerationsOfProperty("DisplayMode"):
                    vobj.DisplayMode = dm
        else:  # Annotations
            if "TextColor" in properties:
                vobj.TextColor = self.form.TextColor.property("color").getRgbF()[:3]
            if "FontName" in properties:
                vobj.FontName = self.form.TextFont.currentFont().family()
            if "FontSize" in properties:
                vobj.FontSize = U.Quantity(self.form.TextSize.text()).Value
            if "LineSpacing" in properties:
                vobj.LineSpacing = self.form.LineSpacing.value()
            if "LineColor" in properties:
                vobj.LineColor = self.form.AnnoLineColor.property("color").getRgbF()[:3]
            if "LineWidth" in properties:
                vobj.LineWidth = self.form.AnnoLineWidth.value()
            if "ArrowType" in properties:
                vobj.ArrowType = ["Dot", "Circle", "Arrow", "Tick", "Tick-2"][self.form.ArrowStyle.currentIndex()]
            if "ArrowSize" in properties:
                vobj.ArrowSize = U.Quantity(self.form.ArrowSize.text()).Value
            if "ShowUnit" in properties:
                vobj.ShowUnit = self.form.ShowUnit.isChecked()
            if "UnitOverride" in properties:
                vobj.UnitOverride = self.form.UnitOverride.text()
            if "DimOvershoot" in properties:
                vobj.DimOvershoot = U.Quantity(self.form.DimOvershoot.text()).Value
            if "ExtLines" in properties:
                vobj.ExtLines = U.Quantity(self.form.ExtLines.text()).Value
            if "ExtOvershoot" in properties:
                vobj.ExtOvershoot = U.Quantity(self.form.ExtOvershoot.text()).Value
            if "TextSpacing" in properties:
                vobj.TextSpacing = U.Quantity(self.form.TextSpacing.text()).Value

    def onLoadStyle(self,index):

        if index > 0:
            pdict = self.load()
            if self.form.comboPresets.itemText(index) in pdict:
                preset = pdict[self.form.comboPresets.itemText(index)]
                self.setValues(preset)

    def onSaveStyle(self):

        reply = QtGui.QInputDialog.getText(None,
                                           translate("Draft", "Save style"),
                                           translate("Draft", "Name of this new style:"))
        if reply[1]:
            name = reply[0]
            pdict = self.load()
            if pdict:
                if name in pdict:
                    reply = QtGui.QMessageBox.question(None,
                                                       translate("Draft", "Warning"),
                                                       translate("Draft", "Name exists. Overwrite?"),
                                                       QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                                                       QtGui.QMessageBox.No)
                    if reply == QtGui.QMessageBox.No:
                        return
            preset = self.getValues()
            pdict[name] = preset
            self.save(pdict)
            self.loadDefaults()

    def load(self):

        """loads the presets json file, returns a dict"""

        pdict = {}
        try:
            import json
            from json.decoder import JSONDecodeError
        except Exception:
            App.Console.PrintError(
                translate("Draft", "Error: json module not found. Unable to load style") + "\n"
            )
            return
        if os.path.exists(PRESETPATH):
            with open(PRESETPATH, "r") as f:
                try:
                    pdict = json.load(f)
                except JSONDecodeError:
                    return {}
        return pdict

    def save(self,d):

        """saves the given dict to the presets json file"""

        try:
            import json
        except Exception:
            App.Console.PrintError(
                translate("Draft", "Error: json module not found. Unable to save style") + "\n"
            )
            return
        folder = os.path.dirname(PRESETPATH)
        if not os.path.exists(folder):
            os.makedirs(folder)
        with open(PRESETPATH, "w") as f:
            json.dump(d, f, indent=4)


Gui.addCommand("Draft_SetStyle", Draft_SetStyle())

## @}
