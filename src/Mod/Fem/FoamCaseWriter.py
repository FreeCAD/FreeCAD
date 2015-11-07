# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Qingfeng Xia <qingfeng.xia eng ox ac uk>                 *       *
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

import FreeCAD
import os
import sys
import time

__title__ = "FoamCaseWriter"
__author__ = "Qingfeng Xia"
__url__ = "http://www.freecadweb.org"

import FreeCAD
import os.path

"""
Mesh object can not update without click
pyFoamClearCase.py
SetField  BoxToFace BoundingBox  runApplication setFields
"""

#from pyfoam import
from subprocess import Popen, PIPE, ProcessException


def launch_cmdline(cmdline):
    process = Popen([].append(cmdline), stdout=PIPE, stderr=PIPE)
    stdout, stderr = process.communicate()
    exitCode = process.returncode

    if (exitCode == 0):
        print stdout
    else:
        print stdout
        print stderr
        raise ProcessException(cmdline, exitCode)


def write_bc_faces(unv_mesh_file, bc_id, bc_objects):
    FreeCAD.Console.PrintMessage('write face_set or patches for boundary\n')
    f = unv_mesh_file
    facet_list = []
    for fobj in bc_objects:
        bc_obj = fobj['Object']
        for o, e in bc_obj.References:
            elem = o.Shape.getElement(e)
            if elem.ShapeType == 'Face':  # CFD study heeds only 2D face boundary for 3D model, normally
                ret = mesh_object.FemMesh.getVolumesByFace(elem)
                # return a list of tuple (vol->GetID(), face->GetID())
                facet_list.append([i[1] for i in ret])
    nr_facets = len(facet_list)
    f.writeline("{:>10d}         0         0         0         0         0         0{:>10d}".format(bc_id,  nr_facets))
    f.writeline(bc_objects.Name)
    for i in int(nr_facets/2):
        f.write("         8{:>10d}         0         0         ".format(facet_list[2*i].id))
        f.writeline("         8{:>10d}         0         0         ".format(facet_list[2*i+1].id))
    if nr_facets%2:
        f.writeline("         8{:>10d}         0         0         ".format(facet_list[-1].id))

def set_bc_velocity(bc):
    """ignored in phase I"""
    pass


def set_bc_pressure(bc):
    """rev = -1 if prs_obj.Reversed else 1 #inlet and outlet"""
    pass


def set_bc_temperature(bc):
    """ignored in phase I"""
    pass


def write_bc_wall(bc):
    pass


def create_empty_case(zipped_template_file,output_path):
    """copy or command to generate an empty case folder structure"""
    import zipfile
    with zipfile.ZipFile(zipped_template_file, "r") as z:
        z.extractall(output_path)
    # remove old case and mesh? auto replace without warning


def is_solid_mesh(fem_mesh):
    if fem_mesh.VolumeCount > 0:  # solid mesh
        return True


class SolverCaseWriter:
    """write_case() is the only public API
    """
    def __init__(self, analysis_obj):
        """analysis_obj should contains all the information needed,
        boundaryConditionList is a list of all boundary Conditions objects(FemConstraint)
        """
        self.analysis_obj = analysis_obj
        self.solver_obj = CaeTools.getSolver(analysis_obj)
        self.mesh_obj = CaeTools.getMesh(analysis_obj)
        self.bc_group = CaeTools.getConstraintGroup(analysis_obj)
        self.mesh_generatd = False

    def write_case(self):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        try:
            create_empty_case(FreeCAD.getHomePath() + "/Mod/Fem/icoFoam_case_template.zip", self.working_dir+os.path.sep+self.solver_obj.InputCaseName)
            self.write_mesh()
            self.write_material()
            self.write_boundary_condition()
            # solver control: time,
            self.write_solver_control()
            self.write_time_control()
        finally:
            QApplication.restoreOverrideCursor()
        FreeCAD.Console.PrintMessage("Sucessfully write {} case to folder".format(self.solver_object.Name, self.working_dir))

    def write_bc_mesh(self, unv_mesh_file):
        FreeCAD.Console.PrintMessage('write face_set or patches for boundary\n')
        f=open(unv_mesh_file,'a')   # appending bc to the volume mesh, which contains node and element definition, ends with '-1' 
        f.writeline("{:6d}".format(-1))  # start of a section 
        f.writeline("{:6d}".format(2467))  # group section 
        for bc_number, bc_obj in enumerate(self.bcgroup):
            write_bc_faces(f, bc_number+1, bc_obj)
        f.writeline("{:6d}".format(-1))  # end of a section 
        f.writeline("{:6d}".format(-1))  # end of file
        f.close()

    def write_mesh(self, mesh_obj=None):
        """This is FreeCAD specific code"""
        if mesh_obj == None:
            mesh_obj = self.mesh_obj
        __objs__ = []
        __objs__.append(mesh_obj)
        import Fem
        mesh_file_name = self.solver_obj.WorkingDir + os.path.sep + self.solver_obj.CaseInputFile + u".unv"
        Fem.export(__objs__, mesh_file_name)
        del __objs__

        # repen this unv file and write the boundary faces
        write_bc_mesh(self, mesh_file_name)

        # convert from UNV to OpenFoam
        cmdline="ideasUnvToFoam -case {}  {}".format(self.solver_obj.WorkingDir, self.solver_obj.CaseInputFile + u".unv")
        # icoFoam -case WorkingDir/case_file_name
        launch_cmdline(cmdline)
        FreeCAD.Console.PrintMessage('mesh file {} converted\n'.format(mesh_file_name))
        self.mesh_generatd = True

    def write_material(self, material=None):
        """Air, Water, CustomedFluid, first step, default to Air"""
        pass

    def write_boundary_condition(self, bcgroup):
        """switch case to deal diff boundary condition, mapping FEM constrain to CFD boundary conditon
        thermal BC must be defined, heat flux and or fixed temperature
        volume force / load? 
        """
        for bc in bcgroup:
            if bc.isDerivedFrom("Fem::FemConstraintPressure"):  # pressure inlet or outlet (revsered = True)
                set_bc_pressure()
            elif bc.isDerivedFrom("Fem::FemConstraintForce"):  # velocity
                set_bc_velocity()
            elif bc.isDerivedFrom("Fem::FemConstraintFixed"):  # wall
                set_bc_wall()
            else:
                FreeCAD.Console.PrintMessage('boundary condition not supported yet\n')
        # non-group boundary surface should be wall, print a warning msg

    def write_solver_control(self):
        """ 
        """
        pass

    def write_time_control(self):
        if self.solver_obj.Transient == False:
            pass
