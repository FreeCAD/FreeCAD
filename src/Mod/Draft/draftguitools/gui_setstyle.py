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

import FreeCAD
import os
import draftutils.utils as utils

if FreeCAD.GuiUp:
    import FreeCADGui
    import Draft_rc
def QT_TRANSLATE_NOOP(ctx,txt):
    return txt
translate = FreeCAD.Qt.translate

__title__ = "FreeCAD Draft Workbench GUI Tools - Styling tools"
__author__ = ("Yorik van Havre")
__url__ = "https://www.freecad.org"

PRESETPATH = os.path.join(FreeCAD.getUserAppDataDir(),"Draft","StylePresets.json")


class Draft_SetStyle:

    """The Draft_SetStyle FreeCAD command definition."""

    def GetResources(self):

        d = {'Pixmap': 'Draft_Apply',
             'Accel': "S, S",
             'MenuText': QT_TRANSLATE_NOOP("Draft_SetStyle", "Set style"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_SetStyle", "Sets default styles")}
        return d

    def Activated(self):

        FreeCADGui.Control.showDialog(Draft_SetStyle_TaskPanel())



class Draft_SetStyle_TaskPanel:

    """The task panel for the Draft_SetStyle command"""

    def __init__(self):

        from PySide import QtCore,QtGui
        self.p = "User parameter:BaseApp/Preferences/"
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/TaskPanel_SetStyle.ui")
        self.form.setWindowIcon(QtGui.QIcon.fromTheme("gtk-apply", QtGui.QIcon(":/icons/Draft_Apply.svg")))
        self.form.applyButton.setIcon(QtGui.QIcon.fromTheme("gtk-apply", QtGui.QIcon(":/icons/Draft_Apply.svg")))
        self.form.annotButton.setIcon(QtGui.QIcon(":/icons/Draft_Text.svg"))
        self.form.LineColor.setProperty("color",self.getPrefColor("View","DefaultShapeLineColor",255))
        self.form.LineWidth.setValue(FreeCAD.ParamGet(self.p+"View").GetInt("DefaultShapeLineWidth",2))
        self.form.DrawStyle.setCurrentIndex(FreeCAD.ParamGet(self.p+"Mod/Draft").GetInt("DefaultDrawStyle",0))
        self.form.DisplayMode.setCurrentIndex(FreeCAD.ParamGet(self.p+"Mod/Draft").GetInt("DefaultDisplayMode",0))
        self.form.ShapeColor.setProperty("color",self.getPrefColor("View","DefaultShapeColor",4294967295))
        self.form.Transparency.setValue(FreeCAD.ParamGet(self.p+"View").GetInt("DefaultShapeTransparency",0))
        self.form.TextFont.setCurrentFont(QtGui.QFont(FreeCAD.ParamGet(self.p+"Mod/Draft").GetString("textfont","Sans")))
        self.form.TextSize.setText(FreeCAD.Units.Quantity(FreeCAD.ParamGet(self.p+"Mod/Draft").GetFloat("textheight",10),FreeCAD.Units.Length).UserString)
        self.form.TextColor.setProperty("color",self.getPrefColor("Mod/Draft","DefaultTextColor",255))
        self.form.ArrowStyle.setCurrentIndex(FreeCAD.ParamGet(self.p+"Mod/Draft").GetInt("dimsymbol",0))
        self.form.ArrowSize.setText(FreeCAD.Units.Quantity(FreeCAD.ParamGet(self.p+"Mod/Draft").GetFloat("arrowsize",5),FreeCAD.Units.Length).UserString)
        self.form.ShowUnit.setChecked(FreeCAD.ParamGet(self.p+"Mod/Draft").GetBool("showUnit",True))
        self.form.UnitOverride.setText(FreeCAD.ParamGet(self.p+"Mod/Draft").GetString("overrideUnit",""))
        self.form.saveButton.setIcon(QtGui.QIcon.fromTheme("gtk-save", QtGui.QIcon(":/icons/document-save.svg")))
        self.form.TextSpacing.setText(FreeCAD.Units.Quantity(FreeCAD.ParamGet(self.p+"Mod/Draft").GetFloat("dimspacing",1),FreeCAD.Units.Length).UserString)
        self.form.LineSpacing.setValue(FreeCAD.ParamGet(self.p+"Mod/Draft").GetFloat("LineSpacing",1))
        self.form.DimOvershoot.setText(FreeCAD.Units.Quantity(FreeCAD.ParamGet(self.p+"Mod/Draft").GetFloat("dimovershoot",0),FreeCAD.Units.Length).UserString)
        self.form.ExtLines.setText(FreeCAD.Units.Quantity(FreeCAD.ParamGet(self.p+"Mod/Draft").GetFloat("extlines",0),FreeCAD.Units.Length).UserString)
        self.form.ExtOvershoot.setText(FreeCAD.Units.Quantity(FreeCAD.ParamGet(self.p+"Mod/Draft").GetFloat("extovershoot",0),FreeCAD.Units.Length).UserString)
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
            if all(item in current.items() for item in preset.items()): #if preset == current:
                self.form.comboPresets.setCurrentIndex(1 + (pdict_keys.index(name)))
                break

    def getPrefColor(self,group,prop,default):

        c = FreeCAD.ParamGet(self.p+group).GetUnsigned(prop,default)
        return self.getColor(c)

    def getColor(self,c):

        from PySide import QtGui
        return QtGui.QColor(utils.rgba_to_argb(c))

    def getValues(self):

        preset = {}
        preset["LineColor"] = utils.argb_to_rgba(self.form.LineColor.property("color").rgba())
        preset["LineWidth"] = self.form.LineWidth.value()
        preset["DrawStyle"] = self.form.DrawStyle.currentIndex()
        preset["DisplayMode"] = self.form.DisplayMode.currentIndex()
        preset["ShapeColor"] = utils.argb_to_rgba(self.form.ShapeColor.property("color").rgba())
        preset["Transparency"] = self.form.Transparency.value()
        preset["TextFont"] = self.form.TextFont.currentFont().family()
        preset["TextSize"] = FreeCAD.Units.Quantity(self.form.TextSize.text()).Value
        preset["TextColor"] = utils.argb_to_rgba(self.form.TextColor.property("color").rgba())
        preset["ArrowStyle"] = self.form.ArrowStyle.currentIndex()
        preset["ArrowSize"] = FreeCAD.Units.Quantity(self.form.ArrowSize.text()).Value
        preset["ShowUnit"] = self.form.ShowUnit.isChecked()
        preset["UnitOverride"] = self.form.UnitOverride.text()
        preset["TextSpacing"] = FreeCAD.Units.Quantity(self.form.TextSpacing.text()).Value
        preset["LineSpacing"] = self.form.LineSpacing.value()
        preset["DimOvershoot"] = FreeCAD.Units.Quantity(self.form.DimOvershoot.text()).Value
        preset["ExtLines"] = FreeCAD.Units.Quantity(self.form.ExtLines.text()).Value
        preset["ExtOvershoot"] = FreeCAD.Units.Quantity(self.form.ExtOvershoot.text()).Value
        return preset

    def setValues(self,preset):

        from PySide import QtCore,QtGui
        self.form.LineColor.setProperty("color",self.getColor(preset.get("LineColor",255)))
        self.form.LineWidth.setValue(preset.get("LineWidth",1))
        self.form.DrawStyle.setCurrentIndex(preset.get("DrawStyle",0))
        self.form.DisplayMode.setCurrentIndex(preset.get("DisplayMode",0))
        self.form.ShapeColor.setProperty("color",self.getColor(preset.get("ShapeColor",4294967295)))
        self.form.Transparency.setValue(preset.get("Transparency",0))
        self.form.TextFont.setCurrentFont(QtGui.QFont(preset.get("TextFont","sans")))
        self.form.TextColor.setProperty("color",self.getColor(preset.get("TextColor",255)))
        self.form.ArrowStyle.setCurrentIndex(preset.get("ArrowStyle",0))
        self.form.ShowUnit.setChecked(preset.get("ShowUnit",False))
        self.form.UnitOverride.setText(preset.get("UnitOverride",""))
        self.form.TextSize.setText(FreeCAD.Units.Quantity(preset.get("TextSize",25),FreeCAD.Units.Length).UserString)
        self.form.ArrowSize.setText(FreeCAD.Units.Quantity(preset.get("ArrowSize",5),FreeCAD.Units.Length).UserString)
        self.form.TextSpacing.setText(FreeCAD.Units.Quantity(preset.get("TextSpacing",25),FreeCAD.Units.Length).UserString)
        self.form.LineSpacing.setValue(preset.get("LineSpacing",3))
        self.form.DimOvershoot.setText(FreeCAD.Units.Quantity(preset.get("DimOvershoot",0),FreeCAD.Units.Length).UserString)
        self.form.ExtLines.setText(FreeCAD.Units.Quantity(preset.get("ExtLines",0),FreeCAD.Units.Length).UserString)
        self.form.ExtOvershoot.setText(FreeCAD.Units.Quantity(preset.get("ExtOvershoot",0),FreeCAD.Units.Length).UserString)

    def reject(self):

        FreeCADGui.Control.closeDialog()

    def accept(self):

        param_draft = FreeCAD.ParamGet(self.p + "Mod/Draft")
        param_view  = FreeCAD.ParamGet(self.p + "View")

        param_view.SetUnsigned("DefaultShapeLineColor",utils.argb_to_rgba(self.form.LineColor.property("color").rgba()))
        param_view.SetInt("DefaultShapeLineWidth",self.form.LineWidth.value())
        param_view.SetUnsigned("DefaultShapeVertexColor",utils.argb_to_rgba(self.form.LineColor.property("color").rgba()))
        param_view.SetInt("DefaultShapePointSize",self.form.LineWidth.value())
        param_draft.SetInt("DefaultDrawStyle",self.form.DrawStyle.currentIndex())
        param_draft.SetInt("DefaultDisplayMode",self.form.DisplayMode.currentIndex())
        param_view.SetUnsigned("DefaultShapeColor",utils.argb_to_rgba(self.form.ShapeColor.property("color").rgba()))
        param_view.SetInt("DefaultShapeTransparency",self.form.Transparency.value())
        param_draft.SetString("textfont",self.form.TextFont.currentFont().family())
        param_draft.SetFloat("textheight",FreeCAD.Units.Quantity(self.form.TextSize.text()).Value)
        param_draft.SetUnsigned("DefaultTextColor",utils.argb_to_rgba(self.form.TextColor.property("color").rgba()))
        param_draft.SetInt("dimsymbol",self.form.ArrowStyle.currentIndex())
        param_draft.SetFloat("arrowsize",FreeCAD.Units.Quantity(self.form.ArrowSize.text()).Value)
        param_draft.SetBool("showUnit",self.form.ShowUnit.isChecked())
        param_draft.SetString("overrideUnit",self.form.UnitOverride.text())
        param_draft.SetFloat("dimspacing",FreeCAD.Units.Quantity(self.form.TextSpacing.text()).Value)
        param_draft.SetFloat("LineSpacing",self.form.LineSpacing.value())
        param_draft.SetFloat("dimovershoot",FreeCAD.Units.Quantity(self.form.DimOvershoot.text()).Value)
        param_draft.SetFloat("extlines",FreeCAD.Units.Quantity(self.form.ExtLines.text()).Value)
        param_draft.SetFloat("extovershoot",FreeCAD.Units.Quantity(self.form.ExtOvershoot.text()).Value)
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.setStyleButton()
        self.reject()

    def onApplyStyle(self):

        for obj in FreeCADGui.Selection.getSelection():
            self.apply_style_to_obj(obj)

    def onApplyAnnot(self):

        if FreeCAD.ActiveDocument is not None: # Command can be called without a document.
            from draftutils import utils
            objs = FreeCAD.ActiveDocument.Objects
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
        if "LineColor" in properties:
            vobj.LineColor = self.form.LineColor.property("color").getRgbF()[:3] # Remove Alpha (which is 1 instead of 0).
        if "LineWidth" in properties:
            vobj.LineWidth = self.form.LineWidth.value()
        if "PointColor" in properties:
            vobj.PointColor = self.form.LineColor.property("color").getRgbF()[:3]
        if "PointSize" in properties:
            vobj.PointSize = self.form.LineWidth.value()
        if "DrawStyle" in properties:
            vobj.DrawStyle = ["Solid", "Dashed", "Dotted", "Dashdot"][self.form.DrawStyle.currentIndex()]
        if "DisplayMode" in properties:
            dmodes = ["Flat Lines", "Wireframe", "Shaded", "Points"]
            dm = dmodes[self.form.DisplayMode.currentIndex()]
            if dm in vobj.getEnumerationsOfProperty("DisplayMode"):
                vobj.DisplayMode = dm
        if "ShapeColor" in properties:
            vobj.ShapeColor = self.form.ShapeColor.property("color").getRgbF()[:3]
        if "Transparency" in properties:
            vobj.Transparency = self.form.Transparency.value()
        if "FontName" in properties:
            vobj.FontName = self.form.TextFont.currentFont().family()
        if "FontSize" in properties:
            vobj.FontSize = FreeCAD.Units.Quantity(self.form.TextSize.text()).Value
        if "TextColor" in properties:
            vobj.TextColor = self.form.TextColor.property("color").getRgbF()[:3]
        if "ArrowType" in properties:
            vobj.ArrowType = ["Dot", "Circle", "Arrow", "Tick", "Tick-2"][self.form.ArrowStyle.currentIndex()]
        if "ArrowSize" in properties:
            vobj.ArrowSize = FreeCAD.Units.Quantity(self.form.ArrowSize.text()).Value
        if "ShowUnit" in properties:
            vobj.ShowUnit = self.form.ShowUnit.isChecked()
        if "UnitOverride" in properties:
            vobj.UnitOverride = self.form.UnitOverride.text()
        if "TextSpacing" in properties:
            vobj.TextSpacing = FreeCAD.Units.Quantity(self.form.TextSpacing.text()).Value
        if "LineSpacing" in properties:
            vobj.LineSpacing = self.form.LineSpacing.value()
        if "DimOvershoot" in properties:
            vobj.DimOvershoot = FreeCAD.Units.Quantity(self.form.DimOvershoot.text()).Value
        if "ExtLines" in properties:
            vobj.ExtLines = FreeCAD.Units.Quantity(self.form.ExtLines.text()).Value
        if "ExtOvershoot" in properties:
            vobj.ExtOvershoot = FreeCAD.Units.Quantity(self.form.ExtOvershoot.text()).Value

    def onLoadStyle(self,index):

        if index > 0:
            pdict = self.load()
            if self.form.comboPresets.itemText(index) in pdict:
                preset = pdict[self.form.comboPresets.itemText(index)]
                self.setValues(preset)

    def onSaveStyle(self):

        from PySide import QtCore,QtGui
        reply = QtGui.QInputDialog.getText(None,
                                           translate("Draft","Save style"),
                                           translate("Draft","Name of this new style:"))
        if reply[1]:
            name = reply[0]
            pdict = self.load()
            if pdict:
                if name in pdict:
                    reply = QtGui.QMessageBox.question(None,
                                                       translate("Draft","Warning"),
                                                       translate("Draft","Name exists. Overwrite?"),
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
            FreeCAD.Console.PrintError(translate("Draft","Error: json module not found. Unable to load style")+"\n")
            return
        if os.path.exists(PRESETPATH):
            with open(PRESETPATH,"r") as f:
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
            FreeCAD.Console.PrintError(translate("Draft","Error: json module not found. Unable to save style")+"\n")
            return
        folder = os.path.dirname(PRESETPATH)
        if not os.path.exists(folder):
            os.makedirs(folder)
        with open(PRESETPATH,"w") as f:
            json.dump(d,f,indent=4)


FreeCADGui.addCommand('Draft_SetStyle', Draft_SetStyle())

## @}
