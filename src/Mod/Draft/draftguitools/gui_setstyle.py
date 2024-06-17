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

import os
from PySide import QtCore
from PySide import QtGui
from PySide import QtWidgets

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
from FreeCAD import Units as U
from draftutils import params
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

        self.form = Gui.PySideUic.loadUi(":/ui/TaskPanel_SetStyle.ui")
        self.form.setWindowIcon(QtGui.QIcon.fromTheme("gtk-apply", QtGui.QIcon(":/icons/Draft_Apply.svg")))

        self.form.saveButton.setIcon(QtGui.QIcon.fromTheme("gtk-save", QtGui.QIcon(":/icons/document-save.svg")))
        self.form.applyButton.setIcon(QtGui.QIcon.fromTheme("gtk-apply", QtGui.QIcon(":/icons/Draft_Apply.svg")))
        self.form.annotButton.setIcon(QtGui.QIcon(":/icons/Draft_Text.svg"))

        self.form.ShapeColor.setProperty("color", self.getColor(params.get_param_view("DefaultShapeColor")))
        self.form.AmbientColor.setProperty("color", self.getColor(params.get_param_view("DefaultAmbientColor")))
        self.form.EmissiveColor.setProperty("color", self.getColor(params.get_param_view("DefaultEmissiveColor")))
        self.form.SpecularColor.setProperty("color", self.getColor(params.get_param_view("DefaultSpecularColor")))
        self.form.Transparency.setValue(params.get_param_view("DefaultShapeTransparency"))
        self.form.Shininess.setValue(params.get_param_view("DefaultShapeShininess"))

        self.form.LineColor.setProperty("color", self.getColor(params.get_param_view("DefaultShapeLineColor")))
        self.form.LineWidth.setValue(params.get_param_view("DefaultShapeLineWidth"))
        self.form.PointColor.setProperty("color", self.getColor(params.get_param_view("DefaultShapeVertexColor")))
        self.form.PointSize.setValue(params.get_param_view("DefaultShapePointSize"))
        self.form.DrawStyle.setCurrentIndex(params.get_param("DefaultDrawStyle"))
        self.form.DisplayMode.setCurrentIndex(params.get_param("DefaultDisplayMode"))

        self.form.TextColor.setProperty("color", self.getColor(params.get_param("DefaultTextColor")))
        self.form.TextFont.setCurrentFont(QtGui.QFont(params.get_param("textfont")))
        self.form.TextSize.setText(U.Quantity(params.get_param("textheight"), U.Length).UserString)
        self.form.LineSpacing.setValue(params.get_param("LineSpacing"))
        self.form.ScaleMultiplier.setValue(params.get_param("DefaultAnnoScaleMultiplier"))

        self.form.AnnoLineColor.setProperty("color", self.getColor(params.get_param("DefaultAnnoLineColor")))
        self.form.AnnoLineWidth.setValue(params.get_param("DefaultAnnoLineWidth"))
        self.form.ArrowStyle.setCurrentIndex(params.get_param("dimsymbol"))
        self.form.ArrowSize.setText(U.Quantity(params.get_param("arrowsize"), U.Length).UserString)
        self.form.ShowUnit.setChecked(params.get_param("showUnit"))
        self.form.UnitOverride.setText(params.get_param("overrideUnit"))
        self.form.DimOvershoot.setText(U.Quantity(params.get_param("dimovershoot"), U.Length).UserString)
        self.form.ExtLines.setText(U.Quantity(params.get_param("extlines"), U.Length).UserString)
        self.form.ExtOvershoot.setText(U.Quantity(params.get_param("extovershoot"), U.Length).UserString)
        self.form.TextSpacing.setText(U.Quantity(params.get_param("dimspacing"), U.Length).UserString)

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

        return {
            "ShapeColor":      utils.argb_to_rgba(self.form.ShapeColor.property("color").rgba()),
            "AmbientColor":    utils.argb_to_rgba(self.form.AmbientColor.property("color").rgba()),
            "EmissiveColor":   utils.argb_to_rgba(self.form.EmissiveColor.property("color").rgba()),
            "SpecularColor":   utils.argb_to_rgba(self.form.SpecularColor.property("color").rgba()),
            "Transparency":    self.form.Transparency.value(),
            "Shininess":       self.form.Shininess.value(),

            "LineColor":       utils.argb_to_rgba(self.form.LineColor.property("color").rgba()),
            "LineWidth":       self.form.LineWidth.value(),
            "PointColor":      utils.argb_to_rgba(self.form.PointColor.property("color").rgba()),
            "PointSize":       self.form.PointSize.value(),
            "DrawStyle":       self.form.DrawStyle.currentIndex(),
            "DisplayMode":     self.form.DisplayMode.currentIndex(),

            "TextColor":       utils.argb_to_rgba(self.form.TextColor.property("color").rgba()),
            "TextFont":        self.form.TextFont.currentFont().family(),
            "TextSize":        U.Quantity(self.form.TextSize.text()).Value,
            "LineSpacing":     self.form.LineSpacing.value(),
            "ScaleMultiplier": self.form.ScaleMultiplier.value(),

            "AnnoLineColor":   utils.argb_to_rgba(self.form.AnnoLineColor.property("color").rgba()),
            "AnnoLineWidth":   self.form.AnnoLineWidth.value(),
            "ArrowStyle":      self.form.ArrowStyle.currentIndex(),
            "ArrowSize":       U.Quantity(self.form.ArrowSize.text()).Value,
            "ShowUnit":        self.form.ShowUnit.isChecked(),
            "UnitOverride":    self.form.UnitOverride.text(),
            "DimOvershoot":    U.Quantity(self.form.DimOvershoot.text()).Value,
            "ExtLines":        U.Quantity(self.form.ExtLines.text()).Value,
            "ExtOvershoot":    U.Quantity(self.form.ExtOvershoot.text()).Value,
            "TextSpacing":     U.Quantity(self.form.TextSpacing.text()).Value
        }

    def setValues(self, preset):

        # For compatibility with <= V0.21 some properties, if missing, reference others:
        #     'new' prop    -> old prop
        #     ---------------------------
        #     PointColor    -> LineColor
        #     PointSize     -> LineWidth
        #     AnnoLineColor -> LineColor
        #     AnnoLineWidth -> LineWidth

        def getDefDraft(entry):
            return params.get_param(entry, ret_default=True)

        def getDefView(entry):
            return params.get_param_view(entry, ret_default=True)

        # ---------------------------------------------------------------------

        self.form.ShapeColor.setProperty(
            "color",
            self.getColor(preset.get("ShapeColor", getDefView("DefaultShapeColor")))
        )
        self.form.AmbientColor.setProperty(
            "color",
            self.getColor(preset.get("AmbientColor", getDefView("DefaultAmbientColor")))
        )
        self.form.EmissiveColor.setProperty(
            "color",
            self.getColor(preset.get("EmissiveColor", getDefView("DefaultEmissiveColor")))
        )
        self.form.SpecularColor.setProperty(
            "color",
            self.getColor(preset.get("SpecularColor", getDefView("DefaultSpecularColor")))
        )
        self.form.Transparency.setValue(
            preset.get("Transparency", getDefView("DefaultShapeTransparency"))
        )
        self.form.Shininess.setValue(
            preset.get("Shininess", getDefView("DefaultShapeShininess"))
        )

        # ---------------------------------------------------------------------

        self.form.LineColor.setProperty(
            "color",
            self.getColor(preset.get("LineColor", getDefView("DefaultShapeLineColor")))
        )
        self.form.LineWidth.setValue(
            preset.get("LineWidth", getDefView("DefaultShapeLineWidth"))
        )
        self.form.PointColor.setProperty(
            "color",
            self.getColor(preset.get("PointColor", preset.get("LineColor", getDefView("DefaultShapeVertexColor"))))
        )
        self.form.PointSize.setValue(
            preset.get("PointSize", preset.get("LineWidth", getDefView("DefaultShapePointSize")))
        )
        self.form.DrawStyle.setCurrentIndex(
            preset.get("DrawStyle", getDefDraft("DefaultDrawStyle"))
        )
        self.form.DisplayMode.setCurrentIndex(
            preset.get("DisplayMode", getDefDraft("DefaultDisplayMode"))
        )

        # ---------------------------------------------------------------------

        self.form.TextColor.setProperty(
            "color",
            self.getColor(preset.get("TextColor", getDefDraft("DefaultTextColor")))
        )
        self.form.TextFont.setCurrentFont(
            QtGui.QFont(preset.get("TextFont", getDefDraft("textfont")))
        )
        self.form.TextSize.setText(
            U.Quantity(preset.get("TextSize", getDefDraft("textheight")),U.Length).UserString
        )
        self.form.LineSpacing.setValue(
            preset.get("LineSpacing", getDefDraft("LineSpacing"))
        )
        self.form.ScaleMultiplier.setValue(
            preset.get("ScaleMultiplier", getDefDraft("DefaultAnnoScaleMultiplier"))
        )

        # ---------------------------------------------------------------------

        self.form.AnnoLineColor.setProperty(
            "color",
            self.getColor(preset.get("AnnoLineColor", preset.get("LineColor", getDefDraft("DefaultAnnoLineColor"))))
        )
        self.form.AnnoLineWidth.setValue(
            preset.get("AnnoLineWidth", preset.get("LineWidth", getDefDraft("DefaultAnnoLineWidth")))
        )
        self.form.ArrowStyle.setCurrentIndex(
            preset.get("ArrowStyle", getDefDraft("dimsymbol"))
        )
        self.form.ArrowSize.setText(
            U.Quantity(preset.get("ArrowSize", getDefDraft("arrowsize")), U.Length).UserString
        )
        self.form.ShowUnit.setChecked(
            preset.get("ShowUnit", getDefDraft("showUnit"))
        )
        self.form.UnitOverride.setText(
            preset.get("UnitOverride", getDefDraft("overrideUnit"))
        )
        self.form.DimOvershoot.setText(
            U.Quantity(preset.get("DimOvershoot", getDefDraft("dimovershoot")), U.Length).UserString
        )
        self.form.ExtLines.setText(
            U.Quantity(preset.get("ExtLines", getDefDraft("extlines")), U.Length).UserString
        )
        self.form.ExtOvershoot.setText(
            U.Quantity(preset.get("ExtOvershoot", getDefDraft("extovershoot")), U.Length).UserString
        )
        self.form.TextSpacing.setText(
            U.Quantity(preset.get("TextSpacing", getDefDraft("dimspacing")), U.Length).UserString
        )

    def reject(self):

        Gui.Control.closeDialog()

    def accept(self):

        params.set_param_view("DefaultShapeColor", utils.argb_to_rgba(self.form.ShapeColor.property("color").rgba()))
        params.set_param_view("DefaultAmbientColor", utils.argb_to_rgba(self.form.AmbientColor.property("color").rgba()))
        params.set_param_view("DefaultEmissiveColor", utils.argb_to_rgba(self.form.EmissiveColor.property("color").rgba()))
        params.set_param_view("DefaultSpecularColor", utils.argb_to_rgba(self.form.SpecularColor.property("color").rgba()))
        params.set_param_view("DefaultShapeTransparency", self.form.Transparency.value())
        params.set_param_view("DefaultShapeShininess", self.form.Shininess.value())

        params.set_param_view("DefaultShapeLineColor", utils.argb_to_rgba(self.form.LineColor.property("color").rgba()))
        params.set_param_view("DefaultShapeLineWidth", self.form.LineWidth.value())
        params.set_param_view("DefaultShapeVertexColor", utils.argb_to_rgba(self.form.PointColor.property("color").rgba()))
        params.set_param_view("DefaultShapePointSize", self.form.PointSize.value())
        params.set_param("DefaultDrawStyle", self.form.DrawStyle.currentIndex())
        params.set_param("DefaultDisplayMode", self.form.DisplayMode.currentIndex())

        params.set_param("DefaultTextColor", utils.argb_to_rgba(self.form.TextColor.property("color").rgba()))
        params.set_param("textfont", self.form.TextFont.currentFont().family())
        params.set_param("textheight", U.Quantity(self.form.TextSize.text()).Value)
        params.set_param("LineSpacing", self.form.LineSpacing.value())
        params.set_param("DefaultAnnoScaleMultiplier", self.form.ScaleMultiplier.value())

        params.set_param("DefaultAnnoLineColor", utils.argb_to_rgba(self.form.AnnoLineColor.property("color").rgba()))
        params.set_param("DefaultAnnoLineWidth", self.form.AnnoLineWidth.value())
        params.set_param("dimsymbol", self.form.ArrowStyle.currentIndex())
        params.set_param("arrowsize", U.Quantity(self.form.ArrowSize.text()).Value)
        params.set_param("showUnit", self.form.ShowUnit.isChecked())
        params.set_param("overrideUnit", self.form.UnitOverride.text())
        params.set_param("dimovershoot", U.Quantity(self.form.DimOvershoot.text()).Value)
        params.set_param("extlines", U.Quantity(self.form.ExtLines.text()).Value)
        params.set_param("extovershoot", U.Quantity(self.form.ExtOvershoot.text()).Value)
        params.set_param("dimspacing", U.Quantity(self.form.TextSpacing.text()).Value)

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
            if "ShapeAppearance" in properties:
                material = App.Material()
                material.DiffuseColor = self.form.ShapeColor.property("color").getRgbF()[:3]  # Remove Alpha (which is 1 instead of 0).
                material.AmbientColor = self.form.AmbientColor.property("color").getRgbF()[:3]
                material.EmissiveColor = self.form.EmissiveColor.property("color").getRgbF()[:3]
                material.SpecularColor = self.form.SpecularColor.property("color").getRgbF()[:3]
                material.Transparency = self.form.Transparency.value() / 100
                material.Shininess = self.form.Shininess.value() / 100
                vobj.ShapeAppearance = (material, )
            if "LineColor" in properties:
                vobj.LineColor = self.form.LineColor.property("color").getRgbF()[:3]
            if "LineWidth" in properties:
                vobj.LineWidth = self.form.LineWidth.value()
            if "PointColor" in properties:
                vobj.PointColor = self.form.PointColor.property("color").getRgbF()[:3]
            if "PointSize" in properties:
                vobj.PointSize = self.form.PointSize.value()
            if "DrawStyle" in properties:
                vobj.DrawStyle = utils.DRAW_STYLES[self.form.DrawStyle.currentIndex()]
            if "DisplayMode" in properties:
                dm = utils.DISPLAY_MODES[self.form.DisplayMode.currentIndex()]
                if dm in vobj.listDisplayModes():
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
            if "ScaleMultiplier" in properties:
                vobj.ScaleMultiplier = self.form.ScaleMultiplier.value()
            if "LineColor" in properties:
                vobj.LineColor = self.form.AnnoLineColor.property("color").getRgbF()[:3]
            if "LineWidth" in properties:
                vobj.LineWidth = self.form.AnnoLineWidth.value()
            if "ArrowType" in properties:
                vobj.ArrowType = utils.ARROW_TYPES[self.form.ArrowStyle.currentIndex()]
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

        reply = QtWidgets.QInputDialog.getText(None,
                                           translate("Draft", "Save style"),
                                           translate("Draft", "Name of this new style:"))
        if reply[1]:
            name = reply[0]
            pdict = self.load()
            if pdict:
                if name in pdict:
                    reply = QtWidgets.QMessageBox.question(None,
                                                       translate("Draft", "Warning"),
                                                       translate("Draft", "Name exists. Overwrite?"),
                                                       QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
                                                       QtWidgets.QMessageBox.No)
                    if reply == QtWidgets.QMessageBox.No:
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
