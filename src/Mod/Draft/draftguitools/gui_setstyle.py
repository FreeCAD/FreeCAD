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
if FreeCAD.GuiUp:
    import FreeCADGui
    import Draft_rc
def QT_TRANSLATE_NOOP(ctx,txt):
    return txt

__title__ = "FreeCAD Draft Workbench GUI Tools - Styling tools"
__author__ = ("Yorik van Havre")
__url__ = "https://www.freecadweb.org"


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
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Draft_Apply.svg"))
        self.form.applyButton.setIcon(QtGui.QIcon(":/icons/Draft_Apply.svg"))
        self.form.LineColor.setProperty("color",self.getPrefColor("View","DefaultShapeLineColor",255))
        self.form.LineWidth.setValue(FreeCAD.ParamGet(self.p+"View").GetInt("DefaultShapeLineWidth",2))
        self.form.DrawStyle.setCurrentIndex(FreeCAD.ParamGet(self.p+"Mod/Draft").GetInt("DefaultDrawStyle",0))
        self.form.DisplayMode.setCurrentIndex(FreeCAD.ParamGet(self.p+"Mod/Draft").GetInt("DefaultDisplayMode",0))
        self.form.ShapeColor.setProperty("color",self.getPrefColor("View","DefaultShapeColor",4294967295))
        self.form.Transparency.setValue(FreeCAD.ParamGet(self.p+"Mod/Draft").GetInt("DefaultTransparency",0))
        self.form.TextFont.setCurrentFont(QtGui.QFont(FreeCAD.ParamGet(self.p+"Mod/Draft").GetString("textfont","Sans")))
        self.form.TextSize.setText(FreeCAD.Units.Quantity(FreeCAD.ParamGet(self.p+"Mod/Draft").GetFloat("textheight",10),FreeCAD.Units.Length).UserString)
        self.form.TextColor.setProperty("color",self.getPrefColor("Mod/Draft","DefaultTextColor",255))
        self.form.ArrowStyle.setCurrentIndex(FreeCAD.ParamGet(self.p+"Mod/Draft").GetInt("dimsymbol",0))
        self.form.ArrowSize.setText(FreeCAD.Units.Quantity(FreeCAD.ParamGet(self.p+"/Mod/Draft").GetFloat("arrowsize",5),FreeCAD.Units.Length).UserString)
        self.form.ShowUnit.setChecked(FreeCAD.ParamGet(self.p+"Mod/Draft").GetBool("showUnit",True))
        self.form.UnitOverride.setText(FreeCAD.ParamGet(self.p+"Mod/Draft").GetString("overrideUnit",""))
        self.form.applyButton.clicked.connect(self.onApplyStyle)

    def getPrefColor(self,group,prop,default):

        c = FreeCAD.ParamGet(self.p+group).GetUnsigned(prop,default)
        r = ((c>>24)&0xFF)/255.0
        g = ((c>>16)&0xFF)/255.0
        b = ((c>>8)&0xFF)/255.0
        from PySide import QtGui
        return QtGui.QColor.fromRgbF(r,g,b)

    def reject(self):

        FreeCADGui.Control.closeDialog()

    def accept(self):

        FreeCAD.ParamGet(self.p+"View").SetUnsigned("DefaultShapeLineColor",self.form.LineColor.property("color").rgb()<<8)
        FreeCAD.ParamGet(self.p+"View").SetInt("DefaultShapeLineWidth",self.form.LineWidth.value())
        FreeCAD.ParamGet(self.p+"Mod/Draft").SetInt("DefaultDrawStyle",self.form.DrawStyle.currentIndex())
        FreeCAD.ParamGet(self.p+"Mod/Draft").SetInt("DefaultDisplayMode",self.form.DisplayMode.currentIndex())
        FreeCAD.ParamGet(self.p+"View").SetUnsigned("DefaultShapeColor",self.form.ShapeColor.property("color").rgb()<<8)
        FreeCAD.ParamGet(self.p+"Mod/Draft").SetInt("DefaultTransparency",self.form.Transparency.value())
        FreeCAD.ParamGet(self.p+"Mod/Draft").SetString("textfont",self.form.TextFont.currentFont().family())
        FreeCAD.ParamGet(self.p+"Mod/Draft").SetFloat("textheight",FreeCAD.Units.Quantity(self.form.TextSize.text()).Value)
        FreeCAD.ParamGet(self.p+"Mod/Draft").SetUnsigned("DefaultTextColor",self.form.TextColor.property("color").rgb()<<8)
        FreeCAD.ParamGet(self.p+"Mod/Draft").SetInt("dimsymbol",self.form.ArrowStyle.currentIndex())
        FreeCAD.ParamGet(self.p+"/Mod/Draft").SetFloat("arrowsize",FreeCAD.Units.Quantity(self.form.ArrowSize.text()).Value)
        FreeCAD.ParamGet(self.p+"Mod/Draft").SetBool("showUnit",self.form.ShowUnit.isChecked())
        FreeCAD.ParamGet(self.p+"Mod/Draft").SetString("overrideUnit",self.form.UnitOverride.text())
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.setStyleButton()
        self.reject()

    def onApplyStyle(self):

        for obj in FreeCADGui.Selection.getSelection():
            vobj = obj.ViewObject
            if vobj:
                if "LineColor" in vobj.PropertiesList:
                    vobj.LineColor = self.form.LineColor.property("color").rgb()<<8
                if "LineWidth" in vobj.PropertiesList:
                    vobj.LineWidth = self.form.LineWidth.value()
                if "DrawStyle" in vobj.PropertiesList:
                    vobj.DrawStyle = ["Solid","Dashed","Dotted","Dashdot"][self.form.DrawStyle.currentIndex()]
                if "DisplayMode" in vobj.PropertiesList:
                    dmodes = ["Flat Lines","Wireframe","Shaded","Points"]
                    dm = dmodes[self.form.DisplayMode.currentIndex()]
                    if hasattr(vobj,"Proxy") and hasattr(vobj.Proxy,"getDisplayModes"):
                        dmodes = vobj.Proxy.getDisplayModes(vobj)
                    if dm in dmodes:
                        try:
                            vobj.DisplayMode = dm
                        except:
                            pass
                if "ShapeColor" in vobj.PropertiesList:
                    vobj.ShapeColor = self.form.ShapeColor.property("color").rgb()<<8
                if "Transparency" in vobj.PropertiesList:
                    vobj.Transparency = self.form.Transparency.value()
                if "FontName" in vobj.PropertiesList:
                    vobj.FontName = self.form.TextFont.currentFont().family()
                if "TextSize" in vobj.PropertiesList:
                    vobj.TextSize = FreeCAD.Units.Quantity(self.form.TextSize.text()).Value
                if "FontSize" in vobj.PropertiesList:
                    vobj.FontSize = FreeCAD.Units.Quantity(self.form.TextSize.text()).Value
                if "TextColor" in vobj.PropertiesList:
                    vobj.TextColor = self.form.TextColor.property("color").rgb()<<8
                if "ArrowType" in vobj.PropertiesList:
                    vobj.ArrowType = ["Dot", "Circle", "Arrow", "Tick", "Tick-2"][self.form.ArrowStyle.currentIndex()]
                if "ArrowSize" in vobj.PropertiesList:
                    vobj.ArrowSize = FreeCAD.Units.Quantity(self.form.ArrowSize.text()).Value
                if "ShowUnit" in vobj.PropertiesList:
                    vobj.ShowUnit = self.form.ShowUnit.isChecked()
                if "UnitOverride" in vobj.PropertiesList:
                    vobj.UnitOverride = self.form.UnitOverride.text()


FreeCADGui.addCommand('Draft_SetStyle', Draft_SetStyle())

## @}
