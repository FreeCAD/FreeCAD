# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 Ofentse Kgoa <kgoaot@eskom.co.za>                  *
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Based on the FemElementGeometry1D by Bernd Hahnebach                  *
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

__title__ = "FreeCAD FEM element fluid 1D ViewProvider for the document object"
__author__ = "Ofentse Kgoa, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemElementFluid1D
#  \ingroup FEM
#  \brief FreeCAD ViewProviderFemElementFluid1D

import FreeCAD
import FreeCADGui
import FemGui  # needed to display the icons in TreeView
False if False else FemGui.__name__  # dummy usage of FemGui for flake8, just returns 'FemGui'

# for the panel
from femobjects import _FemElementFluid1D
from PySide import QtCore
from PySide import QtGui
from . import FemSelectionWidgets


class _ViewProviderFemElementFluid1D:
    "A View Provider for the FemElementFluid1D object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-element-fluid-1d.svg"

    def attach(self, vobj):
        from pivy import coin
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.standard = coin.SoGroup()
        vobj.addDisplayMode(self.standard, "Default")

    def getDisplayModes(self, obj):
        return ["Default"]

    def getDefaultDisplayMode(self):
        return "Default"

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode=0):
        # hide all meshes
        for o in FreeCAD.ActiveDocument.Objects:
            if o.isDerivedFrom("Fem::FemMeshObject"):
                o.ViewObject.hide()
        # show task panel
        taskd = _TaskPanelFemElementFluid1D(self.Object)
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return True

    def doubleClicked(self, vobj):
        guidoc = FreeCADGui.getDocument(vobj.Object.Document)
        # check if another VP is in edit mode
        # https://forum.freecadweb.org/viewtopic.php?t=13077#p104702
        if not guidoc.getInEdit():
            guidoc.setEdit(vobj.Object.Name)
        else:
            from PySide.QtGui import QMessageBox
            message = 'Active Task Dialog found! Please close this one before opening  a new one!'
            QMessageBox.critical(None, "Error in tree view", message)
            FreeCAD.Console.PrintError(message + '\n')
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _TaskPanelFemElementFluid1D:
    '''The TaskPanel for editing References property of FemElementFluid1D objects'''

    def __init__(self, obj):

        self.obj = obj

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElementFluid1D.ui"
        )
        QtCore.QObject.connect(
            self.parameterWidget.cb_section_type,
            QtCore.SIGNAL("activated(int)"),
            self.sectiontype_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.cb_liquid_section_type,
            QtCore.SIGNAL("activated(int)"),
            self.liquidsectiontype_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_manning_area,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.manning_area_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_manning_radius,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.manning_radius_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.sb_manning_coefficient,
            QtCore.SIGNAL("valueChanged(double)"),
            self.manning_coefficient_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_enlarge_area1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.enlarge_area1_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_enlarge_area2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.enlarge_area2_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_contract_area1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.contract_area1_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_contract_area2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.contract_area2_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_inletpressure,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.inlet_pressure_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_outletpressure,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.outlet_pressure_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_inletflowrate,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.inlet_flowrate_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_outletflowrate,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.outlet_flowrate_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.gb_inletpressure,
            QtCore.SIGNAL("clicked(bool)"),
            self.inlet_pressure_active
        )
        QtCore.QObject.connect(
            self.parameterWidget.gb_outletpressure,
            QtCore.SIGNAL("clicked(bool)"),
            self.outlet_pressure_active
        )
        QtCore.QObject.connect(
            self.parameterWidget.gb_inletflowrate,
            QtCore.SIGNAL("clicked(bool)"),
            self.inlet_flowrate_active
        )
        QtCore.QObject.connect(
            self.parameterWidget.gb_outletflowrate,
            QtCore.SIGNAL("clicked(bool)"),
            self.outlet_flowrate_active
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_entrance_pipe_area,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.entrance_pipe_area_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_entrance_area,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.entrance_area_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_diaphragm_pipe_area,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.diaphragm_pipe_area_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_diaphragm_area,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.diaphragm_area_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_bend_pipe_area,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.bend_pipe_area_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.sb_bradius_pdiameter,
            QtCore.SIGNAL("valueChanged(double)"),
            self.bradius_pdiameter_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.sb_bend_angle,
            QtCore.SIGNAL("valueChanged(double)"),
            self.bend_angle_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.sb_bend_loss_coefficient,
            QtCore.SIGNAL("valueChanged(double)"),
            self.bend_loss_coefficient_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_gatevalve_pipe_area,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.gatevalve_pipe_area_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.sb_gatevalve_closing_coeff,
            QtCore.SIGNAL("valueChanged(double)"),
            self.gatevalve_closing_coeff_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_colebrooke_pipe_area,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.colebrooke_pipe_area_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_colebrooke_radius,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.colebrooke_radius_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_colebrooke_grain_diameter,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.colebrooke_grain_diameter_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.sb_colebrooke_form_factor,
            QtCore.SIGNAL("valueChanged(double)"),
            self.colebrooke_form_factor_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.tw_pump_characteristics,
            QtCore.SIGNAL("cellChanged(int, int)"),
            self.pump_characteristics_changed
        )
        self.parameterWidget.cb_section_type.addItems(
            _FemElementFluid1D._FemElementFluid1D.known_fluid_types
        )
        self.parameterWidget.cb_liquid_section_type.addItems(
            _FemElementFluid1D._FemElementFluid1D.known_liquid_types
        )
        self.parameterWidget.cb_gas_section_type.addItems(
            _FemElementFluid1D._FemElementFluid1D.known_gas_types
        )
        self.parameterWidget.cb_channel_section_type.addItems(
            _FemElementFluid1D._FemElementFluid1D.known_channel_types
        )
        self.get_fluidsection_props()
        self.updateParameterWidget()

        # geometry selection widget
        self.selectionWidget = FemSelectionWidgets.GeometryElementsSelection(
            obj.References, ['Edge']
        )

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

    def accept(self):
        self.set_fluidsection_props()
        self.obj.References = self.selectionWidget.references
        self.recompute_and_set_back_all()
        return True

    def reject(self):
        self.recompute_and_set_back_all()
        return True

    def recompute_and_set_back_all(self):
        doc = FreeCADGui.getDocument(self.obj.Document)
        doc.Document.recompute()
        self.selectionWidget.setback_listobj_visibility()
        if self.selectionWidget.sel_server:
            FreeCADGui.Selection.removeObserver(self.selectionWidget.sel_server)
        doc.resetEdit()

    def get_fluidsection_props(self):
        self.SectionType = self.obj.SectionType
        self.LiquidSectionType = self.obj.LiquidSectionType
        self.ManningArea = self.obj.ManningArea
        self.ManningRadius = self.obj.ManningRadius
        self.ManningCoefficient = self.obj.ManningCoefficient
        self.EnlargeArea1 = self.obj.EnlargeArea1
        self.EnlargeArea2 = self.obj.EnlargeArea2
        self.ContractArea1 = self.obj.ContractArea1
        self.ContractArea2 = self.obj.ContractArea2
        self.OutletPressure = self.obj.OutletPressure
        self.InletPressure = self.obj.InletPressure
        self.OutletFlowRate = self.obj.OutletFlowRate
        self.InletFlowRate = self.obj.InletFlowRate
        self.OutletPressureActive = self.obj.OutletPressureActive
        self.InletPressureActive = self.obj.InletPressureActive
        self.OutletFlowRateActive = self.obj.OutletFlowRateActive
        self.InletFlowRateActive = self.obj.InletFlowRateActive
        self.EntrancePipeArea = self.obj.EntrancePipeArea
        self.EntranceArea = self.obj.EntranceArea
        self.DiaphragmPipeArea = self.obj.DiaphragmPipeArea
        self.DiaphragmArea = self.obj.DiaphragmArea
        self.BendPipeArea = self.obj.BendPipeArea
        self.BendRadiusDiameter = self.obj.BendRadiusDiameter
        self.BendAngle = self.obj.BendAngle
        self.BendLossCoefficient = self.obj.BendLossCoefficient
        self.GateValvePipeArea = self.obj.GateValvePipeArea
        self.GateValveClosingCoeff = self.obj.GateValveClosingCoeff
        self.ColebrookeArea = self.obj.ColebrookeArea
        self.ColebrookeRadius = self.obj.ColebrookeRadius
        self.ColebrookeGrainDiameter = self.obj.ColebrookeGrainDiameter
        self.ColebrookeFormFactor = self.obj.ColebrookeFormFactor
        self.PumpFlowRate = self.obj.PumpFlowRate
        self.PumpHeadLoss = self.obj.PumpHeadLoss

    def set_fluidsection_props(self):
        self.obj.LiquidSectionType = self.LiquidSectionType
        self.obj.SectionType = self.SectionType
        self.obj.ManningArea = self.ManningArea
        self.obj.ManningRadius = self.ManningRadius
        self.obj.ManningCoefficient = self.ManningCoefficient
        self.obj.EnlargeArea1 = self.EnlargeArea1
        self.obj.EnlargeArea2 = self.EnlargeArea2
        self.obj.ContractArea1 = self.ContractArea1
        self.obj.ContractArea2 = self.ContractArea2
        self.obj.OutletPressure = self.OutletPressure
        self.obj.InletPressure = self.InletPressure
        self.obj.OutletFlowRate = self.OutletFlowRate
        self.obj.InletFlowRate = self.InletFlowRate
        self.obj.OutletPressureActive = self.OutletPressureActive
        self.obj.InletPressureActive = self.InletPressureActive
        self.obj.OutletFlowRateActive = self.OutletFlowRateActive
        self.obj.InletFlowRateActive = self.InletFlowRateActive
        self.obj.EntrancePipeArea = self.EntrancePipeArea
        self.obj.EntranceArea = self.EntranceArea
        self.obj.DiaphragmPipeArea = self.DiaphragmPipeArea
        self.obj.DiaphragmArea = self.DiaphragmArea
        self.obj.BendPipeArea = self.BendPipeArea
        self.obj.BendRadiusDiameter = self.BendRadiusDiameter
        self.obj.BendAngle = self.BendAngle
        self.obj.BendLossCoefficient = self.BendLossCoefficient
        self.obj.GateValvePipeArea = self.GateValvePipeArea
        self.obj.GateValveClosingCoeff = self.GateValveClosingCoeff
        self.obj.ColebrookeArea = self.ColebrookeArea
        self.obj.ColebrookeRadius = self.ColebrookeRadius
        self.obj.ColebrookeGrainDiameter = self.ColebrookeGrainDiameter
        self.obj.ColebrookeFormFactor = self.ColebrookeFormFactor
        self.obj.PumpFlowRate = self.PumpFlowRate
        self.obj.PumpHeadLoss = self.PumpHeadLoss

    def updateParameterWidget(self):
        'fills the widgets'
        index_sectiontype = self.parameterWidget.cb_section_type.findText(self.SectionType)
        self.parameterWidget.cb_section_type.setCurrentIndex(index_sectiontype)
        self.parameterWidget.sw_section_type.setCurrentIndex(index_sectiontype)
        index_liquidsectiontype = self.parameterWidget.cb_liquid_section_type.findText(
            self.LiquidSectionType
        )
        self.parameterWidget.cb_liquid_section_type.setCurrentIndex(index_liquidsectiontype)
        self.parameterWidget.sw_liquid_section_type.setCurrentIndex(index_liquidsectiontype)
        self.parameterWidget.if_manning_area.setText(self.ManningArea.UserString)
        self.parameterWidget.if_manning_radius.setText(self.ManningRadius.UserString)
        self.parameterWidget.sb_manning_coefficient.setValue(self.ManningCoefficient)
        self.parameterWidget.if_enlarge_area1.setText(self.EnlargeArea1.UserString)
        self.parameterWidget.if_enlarge_area2.setText(self.EnlargeArea2.UserString)
        self.parameterWidget.if_contract_area1.setText(self.ContractArea1.UserString)
        self.parameterWidget.if_contract_area2.setText(self.ContractArea2.UserString)
        self.parameterWidget.if_inletpressure.setText(FreeCAD.Units.Quantity(
            1000 * self.InletPressure, FreeCAD.Units.Pressure).UserString
        )
        self.parameterWidget.if_outletpressure.setText(FreeCAD.Units.Quantity(
            1000 * self.OutletPressure, FreeCAD.Units.Pressure).UserString
        )
        self.parameterWidget.if_inletflowrate.setText(str(self.InletFlowRate))
        self.parameterWidget.if_outletflowrate.setText(str(self.OutletFlowRate))
        self.parameterWidget.gb_inletpressure.setChecked(self.InletPressureActive)
        self.parameterWidget.gb_outletpressure.setChecked(self.OutletPressureActive)
        self.parameterWidget.gb_inletflowrate.setChecked(self.InletFlowRateActive)
        self.parameterWidget.gb_outletflowrate.setChecked(self.OutletFlowRateActive)
        self.parameterWidget.if_entrance_pipe_area.setText(self.EntrancePipeArea.UserString)
        self.parameterWidget.if_entrance_area.setText(self.EntranceArea.UserString)
        self.parameterWidget.if_diaphragm_pipe_area.setText(self.DiaphragmPipeArea.UserString)
        self.parameterWidget.if_diaphragm_area.setText(self.DiaphragmArea.UserString)
        self.parameterWidget.if_bend_pipe_area.setText(self.BendPipeArea.UserString)
        self.parameterWidget.sb_bradius_pdiameter.setValue(self.BendRadiusDiameter)
        self.parameterWidget.sb_bend_angle.setValue(self.BendAngle)
        self.parameterWidget.sb_bend_loss_coefficient.setValue(self.BendLossCoefficient)
        self.parameterWidget.if_gatevalve_pipe_area.setText(self.GateValvePipeArea.UserString)
        self.parameterWidget.sb_gatevalve_closing_coeff.setValue(self.GateValveClosingCoeff)
        self.parameterWidget.if_colebrooke_pipe_area.setText(self.ColebrookeArea.UserString)
        self.parameterWidget.if_colebrooke_radius.setText(self.ColebrookeRadius.UserString)
        self.parameterWidget.if_colebrooke_grain_diameter.setText(
            self.ColebrookeGrainDiameter.UserString
        )
        self.parameterWidget.sb_colebrooke_form_factor.setValue(self.ColebrookeFormFactor)
        for i in range(len(self.PumpFlowRate)):
            self.parameterWidget.tw_pump_characteristics.setItem(
                i, 0, QtGui.QTableWidgetItem(str(self.PumpFlowRate[i]))
            )
            self.parameterWidget.tw_pump_characteristics.setItem(
                i, 1, QtGui.QTableWidgetItem(str(self.PumpHeadLoss[i]))
            )

    def sectiontype_changed(self, index):
        if index < 0:
            return
        self.parameterWidget.cb_section_type.setCurrentIndex(index)
        self.parameterWidget.sw_section_type.setCurrentIndex(index)
        # parameterWidget returns unicode
        self.SectionType = str(self.parameterWidget.cb_section_type.itemText(index))

    def liquidsectiontype_changed(self, index):
        if index < 0:
            return
        self.parameterWidget.cb_liquid_section_type.setCurrentIndex(index)
        self.parameterWidget.sw_liquid_section_type.setCurrentIndex(index)
        # parameterWidget returns unicode
        self.LiquidSectionType = str(
            self.parameterWidget.cb_liquid_section_type.itemText(index)
        )

    def manning_area_changed(self, base_quantity_value):
        self.ManningArea = base_quantity_value

    def manning_radius_changed(self, base_quantity_value):
        self.ManningRadius = base_quantity_value

    def manning_coefficient_changed(self, base_quantity_value):
        self.ManningCoefficient = base_quantity_value

    def enlarge_area1_changed(self, base_quantity_value):
        self.EnlargeArea1 = base_quantity_value

    def enlarge_area2_changed(self, base_quantity_value):
        self.EnlargeArea2 = base_quantity_value

    def contract_area1_changed(self, base_quantity_value):
        self.ContractArea1 = base_quantity_value

    def contract_area2_changed(self, base_quantity_value):
        self.ContractArea2 = base_quantity_value

    def inlet_pressure_changed(self, base_quantity_value):
        self.InletPressure = float(
            FreeCAD.Units.Quantity(base_quantity_value).getValueAs("MPa")
        )

    def outlet_pressure_changed(self, base_quantity_value):
        self.OutletPressure = float(
            FreeCAD.Units.Quantity(base_quantity_value).getValueAs("MPa")
        )

    def inlet_flowrate_changed(self, base_quantity_value):
        self.InletFlowRate = float(
            FreeCAD.Units.Quantity(base_quantity_value).getValueAs("kg/s")
        )

    def outlet_flowrate_changed(self, base_quantity_value):
        self.OutletFlowRate = float(
            FreeCAD.Units.Quantity(base_quantity_value).getValueAs("kg/s")
        )

    def inlet_pressure_active(self, active):
        self.InletPressureActive = active

    def outlet_pressure_active(self, active):
        self.OutletPressureActive = active

    def inlet_flowrate_active(self, active):
        self.InletFlowRateActive = active

    def outlet_flowrate_active(self, active):
        self.OutletFlowRateActive = active

    def entrance_pipe_area_changed(self, base_quantity_value):
        self.EntrancePipeArea = base_quantity_value

    def entrance_area_changed(self, base_quantity_value):
        self.EntranceArea = base_quantity_value

    def diaphragm_pipe_area_changed(self, base_quantity_value):
        self.DiaphragmPipeArea = base_quantity_value

    def diaphragm_area_changed(self, base_quantity_value):
        self.DiaphragmArea = base_quantity_value

    def bend_pipe_area_changed(self, base_quantity_value):
        self.BendPipeArea = base_quantity_value

    def bradius_pdiameter_changed(self, base_quantity_value):
        self.BendRadiusDiameter = base_quantity_value

    def bend_angle_changed(self, base_quantity_value):
        self.BendAngle = base_quantity_value

    def bend_loss_coefficient_changed(self, base_quantity_value):
        self.BendLossCoefficient = base_quantity_value

    def gatevalve_pipe_area_changed(self, base_quantity_value):
        self.GateValvePipeArea = base_quantity_value

    def gatevalve_closing_coeff_changed(self, base_quantity_value):
        self.GateValveClosingCoeff = base_quantity_value

    def colebrooke_pipe_area_changed(self, base_quantity_value):
        self.ColebrookeArea = base_quantity_value

    def colebrooke_radius_changed(self, base_quantity_value):
        self.ColebrookeRadius = base_quantity_value

    def colebrooke_grain_diameter_changed(self, base_quantity_value):
        self.ColebrookeGrainDiameter = base_quantity_value

    def colebrooke_form_factor_changed(self, base_quantity_value):
        self.ColebrookeFormFactor = base_quantity_value

    def pump_characteristics_changed(self, row, column):
        if column == 0:
            self.PumpFlowRate[row] = float(
                self.parameterWidget.tw_pump_characteristics.item(row, column).text()
            )
        else:
            self.PumpHeadLoss[row] = float(
                self.parameterWidget.tw_pump_characteristics.item(row, column).text()
            )
