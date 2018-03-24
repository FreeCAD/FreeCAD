# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Ofentse Kgoa <kgoaot@eskom.co.za>                *
# *   Based on the FemElementGeometry1D by Bernd Hahnebach                        *
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

__title__ = "_ViewProviderFemElementFluid1D"
__author__ = "Ofentse Kgoa"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemElementFluid1D
#  \ingroup FEM

import FreeCAD
import FreeCADGui


# for the panel
from femobjects import _FemElementFluid1D
from PySide import QtCore
from PySide import QtGui


class _ViewProviderFemElementFluid1D:
    "A View Provider for the FemElementFluid1D object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-fluid-section.svg"

    def attach(self, vobj):
        from pivy import coin
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.standard = coin.SoGroup()
        vobj.addDisplayMode(self.standard, "Standard")

    def getDisplayModes(self, obj):
        return ["Standard"]

    def getDefaultDisplayMode(self):
        return "Standard"

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode=0):
        taskd = _TaskPanelFemElementFluid1D(self.Object)
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return

    def doubleClicked(self, vobj):
        doc = FreeCADGui.getDocument(vobj.Object.Document)
        if not doc.getInEdit():
            doc.setEdit(vobj.Object.Name)
        else:
            FreeCAD.Console.PrintError('Active Task Dialog found! Please close this one first!\n')
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _TaskPanelFemElementFluid1D:
    '''The TaskPanel for editing References property of FemElementFluid1D objects'''

    def __init__(self, obj):
        FreeCADGui.Selection.clearSelection()
        self.sel_server = None
        self.obj = obj
        self.obj_notvisible = []

        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElementFluid1D.ui")
        QtCore.QObject.connect(self.form.btn_add, QtCore.SIGNAL("clicked()"), self.add_references)
        QtCore.QObject.connect(self.form.btn_remove, QtCore.SIGNAL("clicked()"), self.remove_reference)
        QtCore.QObject.connect(self.form.cb_section_type, QtCore.SIGNAL("activated(int)"), self.sectiontype_changed)
        QtCore.QObject.connect(self.form.cb_liquid_section_type, QtCore.SIGNAL("activated(int)"), self.liquidsectiontype_changed)
        QtCore.QObject.connect(self.form.if_manning_area, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.manning_area_changed)
        QtCore.QObject.connect(self.form.if_manning_radius, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.manning_radius_changed)
        QtCore.QObject.connect(self.form.sb_manning_coefficient, QtCore.SIGNAL("valueChanged(double)"), self.manning_coefficient_changed)
        QtCore.QObject.connect(self.form.if_enlarge_area1, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.enlarge_area1_changed)
        QtCore.QObject.connect(self.form.if_enlarge_area2, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.enlarge_area2_changed)
        QtCore.QObject.connect(self.form.if_contract_area1, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.contract_area1_changed)
        QtCore.QObject.connect(self.form.if_contract_area2, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.contract_area2_changed)
        QtCore.QObject.connect(self.form.if_inletpressure, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.inlet_pressure_changed)
        QtCore.QObject.connect(self.form.if_outletpressure, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.outlet_pressure_changed)
        QtCore.QObject.connect(self.form.if_inletflowrate, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.inlet_flowrate_changed)
        QtCore.QObject.connect(self.form.if_outletflowrate, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.outlet_flowrate_changed)
        QtCore.QObject.connect(self.form.gb_inletpressure, QtCore.SIGNAL("clicked(bool)"), self.inlet_pressure_active)
        QtCore.QObject.connect(self.form.gb_outletpressure, QtCore.SIGNAL("clicked(bool)"), self.outlet_pressure_active)
        QtCore.QObject.connect(self.form.gb_inletflowrate, QtCore.SIGNAL("clicked(bool)"), self.inlet_flowrate_active)
        QtCore.QObject.connect(self.form.gb_outletflowrate, QtCore.SIGNAL("clicked(bool)"), self.outlet_flowrate_active)
        QtCore.QObject.connect(self.form.if_entrance_pipe_area, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.entrance_pipe_area_changed)
        QtCore.QObject.connect(self.form.if_entrance_area, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.entrance_area_changed)
        QtCore.QObject.connect(self.form.if_diaphragm_pipe_area, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.diaphragm_pipe_area_changed)
        QtCore.QObject.connect(self.form.if_diaphragm_area, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.diaphragm_area_changed)
        QtCore.QObject.connect(self.form.if_bend_pipe_area, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.bend_pipe_area_changed)
        QtCore.QObject.connect(self.form.sb_bradius_pdiameter, QtCore.SIGNAL("valueChanged(double)"), self.bradius_pdiameter_changed)
        QtCore.QObject.connect(self.form.sb_bend_angle, QtCore.SIGNAL("valueChanged(double)"), self.bend_angle_changed)
        QtCore.QObject.connect(self.form.sb_bend_loss_coefficient, QtCore.SIGNAL("valueChanged(double)"), self.bend_loss_coefficient_changed)
        QtCore.QObject.connect(self.form.if_gatevalve_pipe_area, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.gatevalve_pipe_area_changed)
        QtCore.QObject.connect(self.form.sb_gatevalve_closing_coeff, QtCore.SIGNAL("valueChanged(double)"), self.gatevalve_closing_coeff_changed)
        QtCore.QObject.connect(self.form.if_colebrooke_pipe_area, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.colebrooke_pipe_area_changed)
        QtCore.QObject.connect(self.form.if_colebrooke_radius, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.colebrooke_radius_changed)
        QtCore.QObject.connect(self.form.if_colebrooke_grain_diameter, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.colebrooke_grain_diameter_changed)
        QtCore.QObject.connect(self.form.sb_colebrooke_form_factor, QtCore.SIGNAL("valueChanged(double)"), self.colebrooke_form_factor_changed)
        QtCore.QObject.connect(self.form.tw_pump_characteristics, QtCore.SIGNAL("cellChanged(int, int)"), self.pump_characteristics_changed)
        self.form.list_References.itemSelectionChanged.connect(self.select_clicked_reference_shape)
        self.form.list_References.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.form.list_References.connect(self.form.list_References, QtCore.SIGNAL("customContextMenuRequested(QPoint)"), self.references_list_right_clicked)
        self.form.cb_section_type.addItems(_FemElementFluid1D._FemElementFluid1D.known_fluid_types)
        self.form.cb_liquid_section_type.addItems(_FemElementFluid1D._FemElementFluid1D.known_liquid_types)
        self.form.cb_gas_section_type.addItems(_FemElementFluid1D._FemElementFluid1D.known_gas_types)
        self.form.cb_channel_section_type.addItems(_FemElementFluid1D._FemElementFluid1D.known_channel_types)

        self.get_fluidsection_props()
        self.update()

    def accept(self):
        self.setback_listobj_visibility()
        self.set_fluidsection_props()
        if self.sel_server:
            FreeCADGui.Selection.removeObserver(self.sel_server)
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.recompute()
        return True

    def reject(self):
        self.setback_listobj_visibility()
        if self.sel_server:
            FreeCADGui.Selection.removeObserver(self.sel_server)
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def get_fluidsection_props(self):
        self.references = []
        if self.obj.References:
            self.tuplereferences = self.obj.References
            self.get_references()
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
        self.obj.References = self.references
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

    def update(self):
        'fills the widgets'
        index_sectiontype = self.form.cb_section_type.findText(self.SectionType)
        self.form.cb_section_type.setCurrentIndex(index_sectiontype)
        self.form.sw_section_type.setCurrentIndex(index_sectiontype)
        index_liquidsectiontype = self.form.cb_liquid_section_type.findText(self.LiquidSectionType)
        self.form.cb_liquid_section_type.setCurrentIndex(index_liquidsectiontype)
        self.form.sw_liquid_section_type.setCurrentIndex(index_liquidsectiontype)
        self.form.if_manning_area.setText(self.ManningArea.UserString)
        self.form.if_manning_radius.setText(self.ManningRadius.UserString)
        self.form.sb_manning_coefficient.setValue(self.ManningCoefficient)
        self.form.if_enlarge_area1.setText(self.EnlargeArea1.UserString)
        self.form.if_enlarge_area2.setText(self.EnlargeArea2.UserString)
        self.form.if_contract_area1.setText(self.ContractArea1.UserString)
        self.form.if_contract_area2.setText(self.ContractArea2.UserString)
        self.form.if_inletpressure.setText(FreeCAD.Units.Quantity(1000 * self.InletPressure, FreeCAD.Units.Pressure).UserString)
        self.form.if_outletpressure.setText(FreeCAD.Units.Quantity(1000 * self.OutletPressure, FreeCAD.Units.Pressure).UserString)
        self.form.if_inletflowrate.setText(str(self.InletFlowRate))
        self.form.if_outletflowrate.setText(str(self.OutletFlowRate))
        self.form.gb_inletpressure.setChecked(self.InletPressureActive)
        self.form.gb_outletpressure.setChecked(self.OutletPressureActive)
        self.form.gb_inletflowrate.setChecked(self.InletFlowRateActive)
        self.form.gb_outletflowrate.setChecked(self.OutletFlowRateActive)
        self.form.if_entrance_pipe_area.setText(self.EntrancePipeArea.UserString)
        self.form.if_entrance_area.setText(self.EntranceArea.UserString)
        self.form.if_diaphragm_pipe_area.setText(self.DiaphragmPipeArea.UserString)
        self.form.if_diaphragm_area.setText(self.DiaphragmArea.UserString)
        self.form.if_bend_pipe_area.setText(self.BendPipeArea.UserString)
        self.form.sb_bradius_pdiameter.setValue(self.BendRadiusDiameter)
        self.form.sb_bend_angle.setValue(self.BendAngle)
        self.form.sb_bend_loss_coefficient.setValue(self.BendLossCoefficient)
        self.form.if_gatevalve_pipe_area.setText(self.GateValvePipeArea.UserString)
        self.form.sb_gatevalve_closing_coeff.setValue(self.GateValveClosingCoeff)
        self.form.if_colebrooke_pipe_area.setText(self.ColebrookeArea.UserString)
        self.form.if_colebrooke_radius.setText(self.ColebrookeRadius.UserString)
        self.form.if_colebrooke_grain_diameter.setText(self.ColebrookeGrainDiameter.UserString)
        self.form.sb_colebrooke_form_factor.setValue(self.ColebrookeFormFactor)
        for i in range(len(self.PumpFlowRate)):
            self.form.tw_pump_characteristics.setItem(i, 0, QtGui.QTableWidgetItem(str(self.PumpFlowRate[i])))
            self.form.tw_pump_characteristics.setItem(i, 1, QtGui.QTableWidgetItem(str(self.PumpHeadLoss[i])))
        self.rebuild_list_References()

    def sectiontype_changed(self, index):
        if index < 0:
            return
        self.form.cb_section_type.setCurrentIndex(index)
        self.form.sw_section_type.setCurrentIndex(index)
        self.SectionType = str(self.form.cb_section_type.itemText(index))  # form returns unicode

    def liquidsectiontype_changed(self, index):
        if index < 0:
            return
        self.form.cb_liquid_section_type.setCurrentIndex(index)
        self.form.sw_liquid_section_type.setCurrentIndex(index)
        self.LiquidSectionType = str(self.form.cb_liquid_section_type.itemText(index))  # form returns unicode

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
        self.InletPressure = float(FreeCAD.Units.Quantity(base_quantity_value).getValueAs("MPa"))

    def outlet_pressure_changed(self, base_quantity_value):
        self.OutletPressure = float(FreeCAD.Units.Quantity(base_quantity_value).getValueAs("MPa"))

    def inlet_flowrate_changed(self, base_quantity_value):
        self.InletFlowRate = float(FreeCAD.Units.Quantity(base_quantity_value).getValueAs("kg/s"))

    def outlet_flowrate_changed(self, base_quantity_value):
        self.OutletFlowRate = float(FreeCAD.Units.Quantity(base_quantity_value).getValueAs("kg/s"))

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
            self.PumpFlowRate[row] = float(self.form.tw_pump_characteristics.item(row, column).text())
        else:
            self.PumpHeadLoss[row] = float(self.form.tw_pump_characteristics.item(row, column).text())

    def get_references(self):
        for ref in self.tuplereferences:
            for elem in ref[1]:
                self.references.append((ref[0], elem))

    def references_list_right_clicked(self, QPos):
        self.form.contextMenu = QtGui.QMenu()
        menu_item = self.form.contextMenu.addAction("Remove Reference")
        if not self.references:
            menu_item.setDisabled(True)
        self.form.connect(menu_item, QtCore.SIGNAL("triggered()"), self.remove_reference)
        parentPosition = self.form.list_References.mapToGlobal(QtCore.QPoint(0, 0))
        self.form.contextMenu.move(parentPosition + QPos)
        self.form.contextMenu.show()

    def remove_reference(self):
        if not self.references:
            return
        currentItemName = str(self.form.list_References.currentItem().text())
        for ref in self.references:
            refname_to_compare_listentry = ref[0].Name + ':' + ref[1]
            if refname_to_compare_listentry == currentItemName:
                self.references.remove(ref)
        self.rebuild_list_References()

    def add_references(self):
        '''Called if Button add_reference is triggered'''
        # in constraints EditTaskPanel the selection is active as soon as the taskpanel is open
        # here the addReference button EditTaskPanel has to be triggered to start selection mode
        self.setback_listobj_visibility()
        FreeCADGui.Selection.clearSelection()
        # start SelectionObserver and parse the function to add the References to the widget
        print_message = "Select Edges by single click on them to add them to the list"
        if not self.sel_server:
            # if we do not check, we would start a new SelectionObserver on every click on addReference button
            # but close only one SelectionObserver on leaving the task panel
            from . import FemSelectionObserver
            self.sel_server = FemSelectionObserver.FemSelectionObserver(self.selectionParser, print_message)

    def selectionParser(self, selection):
        # print('selection: ', selection[0].Shape.ShapeType, '  ', selection[0].Name, '  ', selection[1])
        if hasattr(selection[0], "Shape"):
            if selection[1]:
                elt = selection[0].Shape.getElement(selection[1])
                if elt.ShapeType == 'Edge':
                    if selection not in self.references:
                        self.references.append(selection)
                        self.rebuild_list_References()
                    else:
                        FreeCAD.Console.PrintMessage(selection[0].Name + ' --> ' + selection[1] + ' is in reference list already!\n')

    def rebuild_list_References(self):
        self.form.list_References.clear()
        items = []
        for ref in self.references:
            item_name = ref[0].Name + ':' + ref[1]
            items.append(item_name)
        for listItemName in sorted(items):
            self.form.list_References.addItem(listItemName)

    def select_clicked_reference_shape(self):
        self.setback_listobj_visibility()
        if self.sel_server:
            FreeCADGui.Selection.removeObserver(self.sel_server)
            self.sel_server = None
        if not self.sel_server:
            if not self.references:
                return
            currentItemName = str(self.form.list_References.currentItem().text())
            for ref in self.references:
                refname_to_compare_listentry = ref[0].Name + ':' + ref[1]
                if refname_to_compare_listentry == currentItemName:
                    # print( 'found: shape: ' + ref[0].Name + ' element: ' + ref[1])
                    if not ref[0].ViewObject.Visibility:
                        self.obj_notvisible.append(ref[0])
                        ref[0].ViewObject.Visibility = True
                    FreeCADGui.Selection.clearSelection()
                    FreeCADGui.Selection.addSelection(ref[0], ref[1])

    def setback_listobj_visibility(self):
        '''set back Visibility of the list objects
        '''
        for obj in self.obj_notvisible:
            obj.ViewObject.Visibility = False
        self.obj_notvisible = []
