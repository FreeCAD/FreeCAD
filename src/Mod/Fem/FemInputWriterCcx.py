# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Przemo Firszt <przemo@firszt.eu>                 *
# *   Copyright (c) 2015 - Bernd Hahnebach <bernd@bimstatik.org>            *
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


__title__ = "FemInputWriterCcx"
__author__ = "Przemo Firszt, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import FreeCAD
import os
import sys
import time
import FemMeshTools
import FemInputWriter


class FemInputWriterCcx(FemInputWriter.FemInputWriter):
    def __init__(self,
                 analysis_obj, solver_obj,
                 mesh_obj, mat_obj,
                 fixed_obj, displacement_obj,
                 contact_obj, planerotation_obj,
                 selfweight_obj, force_obj, pressure_obj,
                 temperature_obj, heatflux_obj, initialtemperature_obj,
                 beamsection_obj, shellthickness_obj,
                 analysis_type=None, dir_name=None
                 ):

        FemInputWriter.FemInputWriter.__init__(
            self,
            analysis_obj, solver_obj,
            mesh_obj, mat_obj,
            fixed_obj, displacement_obj,
            contact_obj, planerotation_obj,
            selfweight_obj, force_obj, pressure_obj,
            temperature_obj, heatflux_obj, initialtemperature_obj,
            beamsection_obj, shellthickness_obj,
            analysis_type, dir_name)
        self.file_name = self.dir_name + '/' + self.mesh_object.Name + '.inp'
        print('FemInputWriterCcx --> self.dir_name  -->  ' + self.dir_name)
        print('FemInputWriterCcx --> self.file_name  -->  ' + self.file_name)

    def write_calculix_input_file(self):
        self.femmesh.writeABAQUS(self.file_name)

        # reopen file with "append" and add the analysis definition
        inpfile = open(self.file_name, 'a')
        inpfile.write('\n\n')

        # node and element sets
        self.write_element_sets_material_and_femelement_type(inpfile)
        if self.fixed_objects:
            self.write_node_sets_constraints_fixed(inpfile)
        if self.displacement_objects:
            self.write_node_sets_constraints_displacement(inpfile)
        if self.planerotation_objects:
            self.write_node_sets_constraints_planerotation(inpfile)
        if self.contact_objects:
            self.write_surfaces_contraints_contact(inpfile)
        if self.analysis_type == "thermomech" and self.temperature_objects:
            self.write_node_sets_constraints_temperature(inpfile)

        # materials and fem element types
        self.write_materials(inpfile)
        if self.analysis_type == "thermomech" and self.initialtemperature_objects:
            self.write_constraints_initialtemperature(inpfile)
        self.write_femelementsets(inpfile)

        # constraints independent from steps
        if self.planerotation_objects:
            self.write_constraints_planerotation(inpfile)
        if self.contact_objects:
            self.write_constraints_contact(inpfile)

        # step begin
        if self.analysis_type == "frequency":
            self.write_step_begin_static_frequency(inpfile)
            self.write_analysis_frequency(inpfile)
        elif self.analysis_type == "static":
            self.write_step_begin_static_frequency(inpfile)
        elif self.analysis_type == "thermomech":
            self.write_step_begin_thermomech(inpfile)
            self.write_analysis_thermomech(inpfile)

        # constraints depend on step used in all analysis types
        if self.fixed_objects:
            self.write_constraints_fixed(inpfile)
        if self.displacement_objects:
            self.write_constraints_displacement(inpfile)

        # constraints depend on step and depending on analysis type
        if self.analysis_type == "frequency":
            pass
        elif self.analysis_type == "static":
            if self.selfweight_objects:
                self.write_constraints_selfweight(inpfile)
            if self.force_objects:
                self.write_constraints_force(inpfile)
            if self.pressure_objects:
                self.write_constraints_pressure(inpfile)
        elif self.analysis_type == "thermomech":
            if self.selfweight_objects:
                self.write_constraints_selfweight(inpfile)
            if self.force_objects:
                self.write_constraints_force(inpfile)
            if self.pressure_objects:
                self.write_constraints_pressure(inpfile)
            if self.temperature_objects:
                self.write_constraints_temperature(inpfile)
            if self.heatflux_objects:
                self.write_constraints_heatflux(inpfile)

        # output and step end
        self.write_outputs_types(inpfile)
        self.write_step_end(inpfile)

        # footer
        self.write_footer(inpfile)
        inpfile.close()
        return self.file_name

    def write_element_sets_material_and_femelement_type(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Element sets for materials and FEM element type (solid, shell, beam)\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        if len(self.material_objects) == 1:
            if self.beamsection_objects and len(self.beamsection_objects) == 1:          # single mat, single beam
                self.get_ccx_elsets_single_mat_single_beam()
            elif self.beamsection_objects and len(self.beamsection_objects) > 1:         # single mat, multiple beams
                self.get_ccx_elsets_single_mat_multiple_beam()
            elif self.shellthickness_objects and len(self.shellthickness_objects) == 1:  # single mat, single shell
                self.get_ccx_elsets_single_mat_single_shell()
            elif self.shellthickness_objects and len(self.shellthickness_objects) > 1:   # single mat, multiple shells
                self.get_ccx_elsets_single_mat_multiple_shell()
            else:                                                                        # single mat, solid
                self.get_ccx_elsets_single_mat_solid()
        else:
            if self.beamsection_objects and len(self.beamsection_objects) == 1:         # multiple mats, single beam
                self.get_ccx_elsets_multiple_mat_single_beam()
            elif self.beamsection_objects and len(self.beamsection_objects) > 1:        # multiple mats, multiple beams
                self.get_ccx_elsets_multiple_mat_multiple_beam()
            elif self.shellthickness_objects and len(self.shellthickness_objects) == 1:   # multiple mats, single shell
                self.get_ccx_elsets_multiple_mat_single_shell()
            elif self.shellthickness_objects and len(self.shellthickness_objects) > 1:  # multiple mats, multiple shells
                self.get_ccx_elsets_multiple_mat_multiple_shell()
            else:                                                                       # multiple mats, solid
                self.get_ccx_elsets_multiple_mat_solid()
        for ccx_elset in self.ccx_elsets:
            f.write('*ELSET,ELSET=' + ccx_elset['ccx_elset_name'] + '\n')
            if ccx_elset['ccx_elset']:
                if ccx_elset['ccx_elset'] == self.ccx_eall:
                    f.write(self.ccx_eall + '\n')
                else:
                    for elid in ccx_elset['ccx_elset']:
                        f.write(str(elid) + ',\n')
            else:
                f.write('**No elements found for these objects\n')

    def write_node_sets_constraints_fixed(self, f):
        # get nodes
        self.get_constraints_fixed_nodes()
        # write nodes to file
        f.write('\n***********************************************************\n')
        f.write('** Node set for fixed constraint\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for femobj in self.fixed_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            f.write('*NSET,NSET=' + femobj['Object'].Name + '\n')
            for n in femobj['Nodes']:
                f.write(str(n) + ',\n')

    def write_node_sets_constraints_displacement(self, f):
        # get nodes
        self.get_constraints_displacement_nodes()
        # write nodes to file
        f.write('\n***********************************************************\n')
        f.write('** Node sets for prescribed displacement constraint\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for femobj in self.displacement_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            f.write('*NSET,NSET=' + femobj['Object'].Name + '\n')
            for n in femobj['Nodes']:
                f.write(str(n) + ',\n')

    def write_node_sets_constraints_planerotation(self, f):
        # get nodes
        self.get_constraints_planerotation_nodes()
        # write nodes to file
        if not self.femnodes_mesh:
            self.femnodes_mesh = self.femmesh.Nodes
        f.write('\n***********************************************************\n')
        f.write('** Node set for plane rotation constraint\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        # info about self.constraint_conflict_nodes:
        # is used to check if MPC and constraint fixed and constraint displacement share same nodes,
        # because MPC's and constriants fixed an constraints displacement can't share same nodes.
        # thus call write_node_sets_constraints_planerotation has to be after constraint fixed and constraint displacement
        for femobj in self.planerotation_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            l_nodes = femobj['Nodes']
            fric_obj = femobj['Object']
            f.write('*NSET,NSET=' + fric_obj.Name + '\n')
            # Code to extract nodes and coordinates on the PlaneRotation support face
            nodes_coords = []
            for node in l_nodes:
                nodes_coords.append((node, self.femnodes_mesh[node].x, self.femnodes_mesh[node].y, self.femnodes_mesh[node].z))
            node_planerotation = FemMeshTools.get_three_non_colinear_nodes(nodes_coords)
            for i in range(len(l_nodes)):
                if l_nodes[i] not in node_planerotation:
                    node_planerotation.append(l_nodes[i])
            MPC_nodes = []
            for i in range(len(node_planerotation)):
                cnt = 0
                for j in range(len(self.constraint_conflict_nodes)):
                    if node_planerotation[i] == self.constraint_conflict_nodes[j]:
                        cnt = cnt + 1
                if cnt == 0:
                    MPC = node_planerotation[i]
                    MPC_nodes.append(MPC)
            for i in range(len(MPC_nodes)):
                f.write(str(MPC_nodes[i]) + ',\n')

    def write_surfaces_contraints_contact(self, f):
        # get surface nodes and write them to file
        f.write('\n***********************************************************\n')
        f.write('** Surfaces for contact constraint\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        obj = 0
        for femobj in self.contact_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            contact_obj = femobj['Object']
            cnt = 0
            obj = obj + 1
            for o, elem_tup in contact_obj.References:
                for elem in elem_tup:
                    ref_shape = o.Shape.getElement(elem)
                    cnt = cnt + 1
                    if ref_shape.ShapeType == 'Face':
                        if cnt == 1:
                            name = "DEP" + str(obj)
                        else:
                            name = "IND" + str(obj)
                        f.write('*SURFACE, NAME =' + name + '\n')
                        v = self.mesh_object.FemMesh.getccxVolumesByFace(ref_shape)
                        for i in v:
                            f.write("{},S{}\n".format(i[0], i[1]))

    def write_node_sets_constraints_temperature(self, f):
        # get nodes
        self.get_constraints_temperature_nodes()
        # write nodes to file
        f.write('\n***********************************************************\n')
        f.write('** Node sets for temperature constraints\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for femobj in self.temperature_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            f.write('*NSET,NSET=' + femobj['Object'].Name + '\n')
            for n in femobj['Nodes']:
                f.write(str(n) + ',\n')

    def write_materials(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Materials\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        f.write('** Young\'s modulus unit is MPa = N/mm2\n')
        if self.analysis_type == "frequency" or self.selfweight_objects:
            f.write('** Density\'s unit is t/mm^3\n')
        if self.analysis_type == "thermomech":
            f.write('** Thermal conductivity unit is kW/mm/K = t*mm/K*s^3\n')
            f.write('** Specific Heat unit is kJ/t/K = mm^2/s^2/K\n')
        for femobj in self.material_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            mat_obj = femobj['Object']
            mat_info_name = mat_obj.Material['Name']
            mat_name = mat_obj.Name
            # get material properties, Currently in SI units: M/kg/s/Kelvin
            YM = FreeCAD.Units.Quantity(mat_obj.Material['YoungsModulus'])
            YM_in_MPa = float(YM.getValueAs('MPa'))
            PR = float(mat_obj.Material['PoissonRatio'])
            if self.analysis_type == "frequency" or self.selfweight_objects:
                density = FreeCAD.Units.Quantity(mat_obj.Material['Density'])
                density_in_tonne_per_mm3 = float(density.getValueAs('t/mm^3'))
            if self.analysis_type == "thermomech":
                TC = FreeCAD.Units.Quantity(mat_obj.Material['ThermalConductivity'])
                TC_in_WmK = float(TC.getValueAs('W/m/K'))  # SvdW: Add factor to force units to results' base units of t/mm/s/K - W/m/K results in no factor needed
                TEC = FreeCAD.Units.Quantity(mat_obj.Material['ThermalExpansionCoefficient'])
                TEC_in_mmK = float(TEC.getValueAs('mm/mm/K'))
                SH = FreeCAD.Units.Quantity(mat_obj.Material['SpecificHeat'])
                SH_in_JkgK = float(SH.getValueAs('J/kg/K')) * 1e+06  # SvdW: Add factor to force units to results' base units of t/mm/s/K
            # write material properties
            f.write('** FreeCAD material name: ' + mat_info_name + '\n')
            f.write('*MATERIAL, NAME=' + mat_name + '\n')
            f.write('*ELASTIC \n')
            f.write('{0:.0f}, {1:.3f}\n'.format(YM_in_MPa, PR))
            if self.analysis_type == "frequency" or self.selfweight_objects:
                f.write('*DENSITY \n')
                f.write('{0:.3e}, \n'.format(density_in_tonne_per_mm3))
            if self.analysis_type == "thermomech":
                f.write('*CONDUCTIVITY \n')
                f.write('{0:.3f}, \n'.format(TC_in_WmK))
                f.write('*EXPANSION \n')
                f.write('{0:.3e}, \n'.format(TEC_in_mmK))
                f.write('*SPECIFIC HEAT \n')
                f.write('{0:.3e}, \n'.format(SH_in_JkgK))

    def write_constraints_initialtemperature(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Initial temperature constraint\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        f.write('*INITIAL CONDITIONS,TYPE=TEMPERATURE\n')
        for itobj in self.initialtemperature_objects:  # Should only be one
            inittemp_obj = itobj['Object']
            f.write('Nall,{}\n'.format(inittemp_obj.initialTemperature))  # OvG: Initial temperature

    def write_femelementsets(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Sections\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for ccx_elset in self.ccx_elsets:
            if ccx_elset['ccx_elset']:
                if 'beamsection_obj'in ccx_elset:  # beam mesh
                    beamsec_obj = ccx_elset['beamsection_obj']
                    elsetdef = 'ELSET=' + ccx_elset['ccx_elset_name'] + ', '
                    material = 'MATERIAL=' + ccx_elset['mat_obj_name']
                    height = beamsec_obj.Height.getValueAs('mm')
                    width = beamsec_obj.Width.getValueAs('mm')
                    if width == 0:
                        section_type = ', SECTION=CIRC'
                        setion_geo = str(height) + '\n'
                    else:
                        section_type = ', SECTION=RECT'
                        setion_geo = str(height) + ', ' + str(width) + '\n'
                    setion_def = '*BEAM SECTION, ' + elsetdef + material + section_type + '\n'
                    f.write(setion_def)
                    f.write(setion_geo)
                elif 'shellthickness_obj'in ccx_elset:  # shell mesh
                    shellth_obj = ccx_elset['shellthickness_obj']
                    elsetdef = 'ELSET=' + ccx_elset['ccx_elset_name'] + ', '
                    material = 'MATERIAL=' + ccx_elset['mat_obj_name']
                    setion_def = '*SHELL SECTION, ' + elsetdef + material + '\n'
                    setion_geo = str(shellth_obj.Thickness.getValueAs('mm')) + '\n'
                    f.write(setion_def)
                    f.write(setion_geo)
                else:  # solid mesh
                    elsetdef = 'ELSET=' + ccx_elset['ccx_elset_name'] + ', '
                    material = 'MATERIAL=' + ccx_elset['mat_obj_name']
                    setion_def = '*SOLID SECTION, ' + elsetdef + material + '\n'
                    f.write(setion_def)

    def write_step_begin_static_frequency(self, f):
        f.write('\n***********************************************************\n')
        f.write('** One step is needed to run the mechanical analysis of FreeCAD\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        static_frequency_step = '*STEP'
        if self.solver_obj.GeometricalNonlinearity == "nonlinear" and self.analysis_type == 'static':
            static_frequency_step += ', NLGEOM'   # https://www.comsol.com/blogs/what-is-geometric-nonlinearity/
        elif self.solver_obj.GeometricalNonlinearity == "nonlinear" and self.analysis_type == 'frequency':
            print('Analysis type frequency and geometrical nonlinear analyis are not allowed together, linear is used instead!')
        f.write(static_frequency_step + '\n')
        if self.solver_obj.IterationsControlParameterTimeUse:
            f.write('*CONTROLS, PARAMETERS=TIME INCREMENTATION\n')
            f.write(self.solver_obj.IterationsControlParameterIter + '\n')
            f.write(self.solver_obj.IterationsControlParameterCutb + '\n')
        analysis_static = '*STATIC'
        if self.solver_obj.MatrixSolverType == "default":
            pass
        elif self.solver_obj.MatrixSolverType == "spooles":
            analysis_static += ', SOLVER=SPOOLES'
        elif self.solver_obj.MatrixSolverType == "iterativescaling":
            analysis_static += ', SOLVER=ITERATIVE SCALING'
        elif self.solver_obj.MatrixSolverType == "iterativecholesky":
            analysis_static += ', SOLVER=ITERATIVE CHOLESKY'
        f.write(analysis_static + '\n')

    def write_step_begin_thermomech(self, f):
        f.write('\n***********************************************************\n')
        f.write('** One step is needed to run the thermomechanical analysis of FreeCAD\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        thermomech_step = '*STEP'
        if self.solver_obj.GeometricalNonlinearity == "nonlinear":
            thermomech_step += ', NLGEOM'
        thermomech_step += ', INC=' + str(self.solver_obj.IterationsMaximum)
        f.write(thermomech_step + '\n')
        if self.solver_obj.IterationsControlParameterTimeUse:
            f.write('*CONTROLS, PARAMETERS=TIME INCREMENTATION\n')
            f.write(self.solver_obj.IterationsControlParameterIter + '\n')
            f.write(self.solver_obj.IterationsControlParameterCutb + '\n')

    def write_analysis_frequency(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Frequency analysis\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        f.write('*FREQUENCY\n')
        f.write('{},{},{}\n'.format(self.solver_obj.EigenmodesCount, self.solver_obj.EigenmodeLowLimit, self.solver_obj.EigenmodeHighLimit))

    def write_analysis_thermomech(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Coupled temperature displacement analysis\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        thermomech_analysis = '*COUPLED TEMPERATURE-DISPLACEMENT'
        if self.solver_obj.MatrixSolverType == "default":
            pass
        elif self.solver_obj.MatrixSolverType == "spooles":
            thermomech_analysis += ', SOLVER=SPOOLES'
        elif self.solver_obj.MatrixSolverType == "iterativescaling":
            thermomech_analysis += ', SOLVER=ITERATIVE SCALING'
        elif self.solver_obj.MatrixSolverType == "iterativecholesky":
            thermomech_analysis += ', SOLVER=ITERATIVE CHOLESKY'
        if self.solver_obj.SteadyState:
            thermomech_analysis += ', STEADY STATE'
            self.solver_obj.TimeInitialStep = 1.0  # Set time to 1 and ignore user inputs for steady state
            self.solver_obj.TimeEnd = 1.0
        thermomech_time = '{},{}'.format(self.solver_obj.TimeInitialStep, self.solver_obj.TimeEnd)  # OvG: 1.0 increment, total time 1 for steady state will cut back automatically
        f.write(thermomech_analysis + '\n')
        f.write(thermomech_time + '\n')

    def write_constraints_fixed(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Fixed Constraints\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for femobj in self.fixed_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            fix_obj_name = femobj['Object'].Name
            f.write('*BOUNDARY\n')
            f.write(fix_obj_name + ',1\n')
            f.write(fix_obj_name + ',2\n')
            f.write(fix_obj_name + ',3\n')
            if self.beamsection_objects or self.shellthickness_objects:
                f.write(fix_obj_name + ',4\n')
                f.write(fix_obj_name + ',5\n')
                f.write(fix_obj_name + ',6\n')
            f.write('\n')

    def write_constraints_displacement(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Displacement constraint applied\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for femobj in self.displacement_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            disp_obj = femobj['Object']
            disp_obj_name = disp_obj.Name
            f.write('*BOUNDARY\n')
            if disp_obj.xFix:
                f.write(disp_obj_name + ',1\n')
            elif not disp_obj.xFree:
                f.write(disp_obj_name + ',1,1,' + str(disp_obj.xDisplacement) + '\n')
            if disp_obj.yFix:
                f.write(disp_obj_name + ',2\n')
            elif not disp_obj.yFree:
                f.write(disp_obj_name + ',2,2,' + str(disp_obj.yDisplacement) + '\n')
            if disp_obj.zFix:
                f.write(disp_obj_name + ',3\n')
            elif not disp_obj.zFree:
                f.write(disp_obj_name + ',3,3,' + str(disp_obj.zDisplacement) + '\n')

            if self.beamsection_objects or self.shellthickness_objects:
                if disp_obj.rotxFix:
                    f.write(disp_obj_name + ',4\n')
                elif not disp_obj.rotxFree:
                    f.write(disp_obj_name + ',4,4,' + str(disp_obj.xRotation) + '\n')
                if disp_obj.rotyFix:
                    f.write(disp_obj_name + ',5\n')
                elif not disp_obj.rotyFree:
                    f.write(disp_obj_name + ',5,5,' + str(disp_obj.yRotation) + '\n')
                if disp_obj.rotzFix:
                    f.write(disp_obj_name + ',6\n')
                elif not disp_obj.rotzFree:
                    f.write(disp_obj_name + ',6,6,' + str(disp_obj.zRotation) + '\n')
        f.write('\n')

    def write_constraints_contact(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Contact Constraints\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        obj = 0
        for femobj in self.contact_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            obj = obj + 1
            contact_obj = femobj['Object']
            f.write('*CONTACT PAIR, INTERACTION=INT' + str(obj) + ',TYPE=SURFACE TO SURFACE\n')
            ind_surf = "IND" + str(obj)
            dep_surf = "DEP" + str(obj)
            f.write(dep_surf + ',' + ind_surf + '\n')
            f.write('*SURFACE INTERACTION, NAME=INT' + str(obj) + '\n')
            f.write('*SURFACE BEHAVIOR,PRESSURE-OVERCLOSURE=LINEAR\n')
            slope = contact_obj.Slope
            f.write(str(slope) + ' \n')
            friction = contact_obj.Friction
            if friction > 0:
                f.write('*FRICTION \n')
                stick = (slope / 10.0)
                f.write(str(friction) + ', ' + str(stick) + ' \n')

    def write_constraints_planerotation(self, f):
        f.write('\n***********************************************************\n')
        f.write('** PlaneRotation Constraints\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for femobj in self.planerotation_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            fric_obj_name = femobj['Object'].Name
            f.write('*MPC\n')
            f.write('PLANE,' + fric_obj_name + '\n')

    def write_constraints_selfweight(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Self weight Constraint\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for femobj in self.selfweight_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            selwei_obj_name = femobj['Object'].Name
            f.write('** ' + selwei_obj_name + '\n')
            f.write('*DLOAD\n')
            f.write('Eall,GRAV,9810,0,0,-1\n')
            f.write('\n')
        # grav (erdbeschleunigung) is equal for all elements
        # should be only one constraint
        # different elment sets for different density are written in the material element sets allready

    def write_constraints_force(self, f):
        # check shape type of reference shape and get node loads
        self.get_constraints_force_nodeloads()
        # write node loads to file
        f.write('\n***********************************************************\n')
        f.write('** Node loads Constraints\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        f.write('*CLOAD\n')
        for femobj in self.force_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            frc_obj_name = femobj['Object'].Name
            direction_vec = femobj['Object'].DirectionVector
            f.write('** ' + frc_obj_name + '\n')
            for ref_shape in femobj['NodeLoadTable']:
                f.write('** ' + ref_shape[0] + '\n')
                for n in sorted(ref_shape[1]):
                    node_load = ref_shape[1][n]
                    if (direction_vec.x != 0.0):
                        v1 = "{:.13E}".format(direction_vec.x * node_load)
                        f.write(str(n) + ',1,' + v1 + '\n')
                    if (direction_vec.y != 0.0):
                        v2 = "{:.13E}".format(direction_vec.y * node_load)
                        f.write(str(n) + ',2,' + v2 + '\n')
                    if (direction_vec.z != 0.0):
                        v3 = "{:.13E}".format(direction_vec.z * node_load)
                        f.write(str(n) + ',3,' + v3 + '\n')
                f.write('\n')
            f.write('\n')

    def write_constraints_pressure(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Element + CalculiX face + load in [MPa]\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for femobj in self.pressure_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            prs_obj = femobj['Object']
            f.write('*DLOAD\n')
            for o, elem_tup in prs_obj.References:
                rev = -1 if prs_obj.Reversed else 1
                for elem in elem_tup:
                    ref_shape = o.Shape.getElement(elem)
                    if ref_shape.ShapeType == 'Face':
                        v = self.femmesh.getccxVolumesByFace(ref_shape)
                        f.write("** Load on face {}\n".format(elem))
                        for i in v:
                            f.write("{},P{},{}\n".format(i[0], i[1], rev * prs_obj.Pressure))

    def write_constraints_temperature(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Fixed temperature constraint applied\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for ftobj in self.temperature_objects:
            fixedtemp_obj = ftobj['Object']
            f.write('*BOUNDARY\n')
            f.write('{},11,11,{}\n'.format(fixedtemp_obj.Name, fixedtemp_obj.Temperature))
            f.write('\n')

    def write_constraints_heatflux(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Heatflux constraints\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for hfobj in self.heatflux_objects:
            heatflux_obj = hfobj['Object']
            f.write('*FILM\n')
            for o, elem_tup in heatflux_obj.References:
                for elem in elem_tup:
                    ho = o.Shape.getElement(elem)
                    if ho.ShapeType == 'Face':
                        v = self.mesh_object.FemMesh.getccxVolumesByFace(ho)
                        f.write("** Heat flux on face {}\n".format(elem))
                        for i in v:
                            f.write("{},F{},{},{}\n".format(i[0], i[1], heatflux_obj.AmbientTemp, heatflux_obj.FilmCoef * 0.001))  # SvdW add factor to force heatflux to units system of t/mm/s/K # OvG: Only write out the VolumeIDs linked to a particular face

    def write_outputs_types(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Outputs --> frd file\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        if self.beamsection_objects or self.shellthickness_objects:
            f.write('*NODE FILE, OUTPUT=2d\n')
        else:
            f.write('*NODE FILE\n')
        if self.analysis_type == "thermomech":  # MPH write out nodal temperatures if thermomechanical
            f.write('U, NT\n')
        else:
            f.write('U\n')
        f.write('*EL FILE\n')
        f.write('S, E\n')
        f.write('** outputs --> dat file\n')
        f.write('*NODE PRINT , NSET=Nall \n')
        f.write('U \n')
        f.write('*EL PRINT , ELSET=Eall \n')
        f.write('S \n')

    def write_step_end(self, f):
        f.write('\n***********************************************************\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        f.write('*END STEP \n')

    def write_footer(self, f):
        f.write('\n***********************************************************\n')
        f.write('** CalculiX Input file\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        f.write('**   written by    --> FreeCAD ' + self.fc_ver[0] + '.' + self.fc_ver[1] + '.' + self.fc_ver[2] + '\n')
        f.write('**   written on    --> ' + time.ctime() + '\n')
        f.write('**   file name     --> ' + os.path.basename(FreeCAD.ActiveDocument.FileName) + '\n')
        f.write('**   analysis name --> ' + self.analysis.Name + '\n')
        f.write('**\n')
        f.write('**\n')
        f.write('**\n')
        f.write('**   Units\n')
        f.write('**\n')
        f.write('**   Geometry (mesh data)        --> mm\n')
        f.write("**   Materials (Young's modulus) --> N/mm2 = MPa\n")
        f.write('**   Loads (nodal loads)         --> N\n')
        f.write('**\n')

    # self.ccx_elsets = [ {
    #                        'beamsection_obj' : 'beamsection_obj'       if exists
    #                        'shellthickness_obj' : shellthickness_obj'  if exists
    #                        'ccx_elset' : [e1, e2, e3, ... , en] or string self.ccx_eall
    #                        'ccx_elset_name' : 'ccx_identifier_elset'
    #                        'mat_obj_name' : 'mat_obj.Name'
    #                        'ccx_mat_name' : 'mat_obj.Material['Name']'   !!! not unique !!!
    #                     },
    #                     {}, ... , {} ]
    def get_ccx_elsets_single_mat_single_beam(self):
        mat_obj = self.material_objects[0]['Object']
        beamsec_obj = self.beamsection_objects[0]['Object']
        ccx_elset = {}
        ccx_elset['beamsection_obj'] = beamsec_obj
        ccx_elset['ccx_elset'] = self.ccx_eall
        ccx_elset['ccx_elset_name'] = get_ccx_elset_beam_name(mat_obj.Name, beamsec_obj.Name)
        ccx_elset['mat_obj_name'] = mat_obj.Name
        ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
        self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_single_mat_single_shell(self):
        mat_obj = self.material_objects[0]['Object']
        shellth_obj = self.shellthickness_objects[0]['Object']
        ccx_elset = {}
        ccx_elset['shellthickness_obj'] = shellth_obj
        ccx_elset['ccx_elset'] = self.ccx_eall
        ccx_elset['ccx_elset_name'] = get_ccx_elset_shell_name(mat_obj.Name, shellth_obj.Name)
        ccx_elset['mat_obj_name'] = mat_obj.Name
        ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
        self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_single_mat_solid(self):
        mat_obj = self.material_objects[0]['Object']
        ccx_elset = {}
        ccx_elset['ccx_elset'] = self.ccx_eall
        ccx_elset['ccx_elset_name'] = get_ccx_elset_solid_name(mat_obj.Name)
        ccx_elset['mat_obj_name'] = mat_obj.Name
        ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
        self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_single_mat_multiple_beam(self):
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
        mat_obj = self.material_objects[0]['Object']
        FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.beamsection_objects)
        for beamsec_data in self.beamsection_objects:
            beamsec_obj = beamsec_data['Object']
            ccx_elset = {}
            ccx_elset['beamsection_obj'] = beamsec_obj
            ccx_elset['ccx_elset'] = beamsec_data['FEMElements']
            ccx_elset['ccx_elset_name'] = get_ccx_elset_beam_name(mat_obj.Name, beamsec_obj.Name, None, beamsec_data['ShortName'])
            ccx_elset['mat_obj_name'] = mat_obj.Name
            ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_single_mat_multiple_shell(self):
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
        mat_obj = self.material_objects[0]['Object']
        FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.shellthickness_objects)
        for shellth_data in self.shellthickness_objects:
            shellth_obj = shellth_data['Object']
            ccx_elset = {}
            ccx_elset['shellthickness_obj'] = shellth_obj
            ccx_elset['ccx_elset'] = shellth_data['FEMElements']
            ccx_elset['ccx_elset_name'] = get_ccx_elset_shell_name(mat_obj.Name, shellth_obj.Name, None, shellth_data['ShortName'])
            ccx_elset['mat_obj_name'] = mat_obj.Name
            ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_single_beam(self):
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
        beamsec_obj = self.beamsection_objects[0]['Object']
        FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.material_objects)
        for mat_data in self.material_objects:
            mat_obj = mat_data['Object']
            ccx_elset = {}
            ccx_elset['beamsection_obj'] = beamsec_obj
            ccx_elset['ccx_elset'] = mat_data['FEMElements']
            ccx_elset['ccx_elset_name'] = get_ccx_elset_beam_name(mat_obj.Name, beamsec_obj.Name, mat_data['ShortName'])
            ccx_elset['mat_obj_name'] = mat_obj.Name
            ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_single_shell(self):
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
        shellth_obj = self.shellthickness_objects[0]['Object']
        FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.material_objects)
        for mat_data in self.material_objects:
            mat_obj = mat_data['Object']
            ccx_elset = {}
            ccx_elset['shellthickness_obj'] = shellth_obj
            ccx_elset['ccx_elset'] = mat_data['FEMElements']
            ccx_elset['ccx_elset_name'] = get_ccx_elset_shell_name(mat_obj.Name, shellth_obj.Name, mat_data['ShortName'])
            ccx_elset['mat_obj_name'] = mat_obj.Name
            ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_solid(self):
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
        FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.material_objects)
        for mat_data in self.material_objects:
            mat_obj = mat_data['Object']
            ccx_elset = {}
            ccx_elset['ccx_elset'] = mat_data['FEMElements']
            ccx_elset['ccx_elset_name'] = get_ccx_elset_solid_name(mat_obj.Name, None, mat_data['ShortName'])
            ccx_elset['mat_obj_name'] = mat_obj.Name
            ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_multiple_beam(self):
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
        FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.beamsection_objects)
        FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.material_objects)
        for beamsec_data in self.beamsection_objects:
            beamsec_obj = beamsec_data['Object']
            for mat_data in self.material_objects:
                mat_obj = mat_data['Object']
                ccx_elset = {}
                ccx_elset['beamsection_obj'] = beamsec_obj
                elemids = []
                for elemid in beamsec_data['FEMElements']:
                    if elemid in mat_data['FEMElements']:
                        elemids.append(elemid)
                ccx_elset['ccx_elset'] = elemids
                ccx_elset['ccx_elset_name'] = get_ccx_elset_beam_name(mat_obj.Name, beamsec_obj.Name, mat_data['ShortName'], beamsec_data['ShortName'])
                ccx_elset['mat_obj_name'] = mat_obj.Name
                ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
                self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_multiple_shell(self):
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
        FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.shellthickness_objects)
        FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.material_objects)
        for shellth_data in self.shellthickness_objects:
            shellth_obj = shellth_data['Object']
            for mat_data in self.material_objects:
                mat_obj = mat_data['Object']
                ccx_elset = {}
                ccx_elset['shellthickness_obj'] = shellth_obj
                elemids = []
                for elemid in shellth_data['FEMElements']:
                    if elemid in mat_data['FEMElements']:
                        elemids.append(elemid)
                ccx_elset['ccx_elset'] = elemids
                ccx_elset['ccx_elset_name'] = get_ccx_elset_shell_name(mat_obj.Name, shellth_obj.Name, mat_data['ShortName'], shellth_data['ShortName'])
                ccx_elset['mat_obj_name'] = mat_obj.Name
                ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
                self.ccx_elsets.append(ccx_elset)


# Helpers
def get_ccx_elset_beam_name(mat_name, beamsec_name, mat_short_name=None, beamsec_short_name=None):
    if not mat_short_name:
        mat_short_name = 'Mat0'
    if not beamsec_short_name:
        beamsec_short_name = 'Beam0'
    if len(mat_name + beamsec_name) > 20:   # max identifier lenght in CalculiX for beam elsets
        return mat_short_name + beamsec_short_name
    else:
        return mat_name + beamsec_name


def get_ccx_elset_shell_name(mat_name, shellth_name, mat_short_name=None, shellth_short_name=None):
    if not mat_short_name:
        mat_short_name = 'Mat0'
    if not shellth_short_name:
        shellth_short_name = 'Shell0'
    if len(mat_name + shellth_name) > 80:   # standard max identifier lenght in CalculiX
        return mat_short_name + shellth_short_name
    else:
        return mat_name + shellth_name


def get_ccx_elset_solid_name(mat_name, solid_name=None, mat_short_name=None):
    if not solid_name:
        solid_name = 'Solid'
    if not mat_short_name:
        mat_short_name = 'Mat0'
    if len(mat_name + solid_name) > 80:   # standard max identifier lenght in CalculiX
        return mat_short_name + solid_name
    else:
        return mat_name + solid_name
