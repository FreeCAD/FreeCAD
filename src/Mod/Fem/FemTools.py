#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - Przemo Firszt <przemo@firszt.eu>                 *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************


import FreeCAD
import FemGui


class FemTools:
    def __init__(self, analysis_object=None):
        if analysis_object:
            self.analysis = analysis_object
        else:
            self.analysis = FemGui.getActiveAnalysis()
        self.mesh = None
        for m in self.analysis.Member:
            if m.isDerivedFrom("Fem::FemMeshObject"):
                self.mesh = m

    def purge_results(self):
        for m in self.fem_analysis.Member:
            if (m.isDerivedFrom('Fem::FemResultObject')):
                FreeCAD.ActiveDocument.removeObject(m.Name)

    def reset_mesh_deformation(self):
        if self.mesh:
            self.mesh.ViewObject.applyDisplacement(0.0)

    def reset_mesh_color(self):
        if self.mesh:
            self.mesh.ViewObject.NodeColor = {}
            self.mesh.ViewObject.ElementColor = {}
            self.mesh.ViewObject.setNodeColorByScalars()

    def update_objects(self):
        # [{'Object':material}, {}, ...]
        # [{'Object':fixed_constraints, 'NodeSupports':bool}, {}, ...]
        # [{'Object':force_constraints, 'NodeLoad':value}, {}, ...
        # [{'Object':pressure_constraints, 'xxxxxxxx':value}, {}, ...]
        self.mesh = None
        self.material = []
        self.fixed_constraints = []
        self.force_constraints = []
        self.pressure_constraints = []

        for m in self.analysis.Member:
            if m.isDerivedFrom("Fem::FemMeshObject"):
                self.mesh = m
            elif m.isDerivedFrom("App::MaterialObjectPython"):
                material_dict = {}
                material_dict['Object'] = m
                self.material.append(material_dict)
            elif m.isDerivedFrom("Fem::ConstraintFixed"):
                fixed_constraint_dict = {}
                fixed_constraint_dict['Object'] = m
                self.fixed_constraints.append(fixed_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintForce"):
                force_constraint_dict = {}
                force_constraint_dict['Object'] = m
                self.force_constraints.append(force_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintPressure"):
                PressureObjectDict = {}
                PressureObjectDict['Object'] = m
                self.pressure_constraints.append(PressureObjectDict)

    def check_prerequisites(self):
        message = ""
        if not self.analysis:
            message += "No active Analysis\n"
        if not self.mesh:
            message += "No mesh object in the Analysis\n"
        if not self.material:
            message += "No material object in the Analysis\n"
        if not self.fixed_constraints:
            message += "No fixed-constraint nodes defined in the Analysis\n"
        if not (self.force_constraints or self.pressure_constraints):
            message += "No force-constraint or pressure-constraint defined in the Analysis\n"
        return message

    def write_inp_file(self, working_dir):
        import ccxInpWriter as iw
        import sys
        self.working_dir = working_dir
        self.base_name = ""
        try:
            inp_writer = iw.inp_writer(self.analysis, self.mesh, self.material,
                                       self.fixed_constraints, self.force_constraints,
                                       self.pressure_constraints, self.working_dir)
            self.base_name = inp_writer.write_calculix_input_file()
        except:
            print "Unexpected error when writing CalculiX input file:", sys.exc_info()[0]
            raise
