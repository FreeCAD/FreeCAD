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
FemConstraint can be derived from python? need significant work
Mesh object can not update without click
pyFoamClearCase.py
SetField  BoxToFace BoundingBox  runApplication setFields
"""

from pyfoam import 
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
                    
def write_bc_faces(unv_mesh_file,mesh_object,bc_objects):
    FreeCAD.Console.PrintMessage('write face_set or patches for boundary\n')
    f=unv_mesh_file
    for fobj in bc_objects:
        bc_obj = fobj['Object']
        for o, e in bc_obj.References:
            elem = o.Shape.getElement(e)
            if elem.ShapeType == 'Face': #CFD study heeds only face boundary for 3D model, normally
                vols,faces = mesh_object.FemMesh.getVolumesByFace(elem) #return a tuple of VolomeSet and faceset
                f.write() #UNV faceset type and name
                for i in faces:
                    f.write("{},{},{}\n".format(i[0], i[1], i[2]) #to-do
                    
def set_bc_velocity(bc):
    """vector velocity is used"""
    pass # ignore in phase I
    
def set_bc_pressure(bc):
    """rev = -1 if prs_obj.Reversed else 1 #inlet and outlet"""
    pass
    
def set_bc_temperature(bc):
    """"""
    pass
    
def write_bc_wall(bc):
    pass
 
 def create_empty_case(zipped_template_file,output_path):
    #copy or command to generate an empty case folder structure
    import zipfile
    with zipfile.ZipFile(zipped_template_file, "r") as z:
        z.extractall(output_path)
    #remove old case and mesh? auto replace without warning

def is_solid_mesh(fem_mesh):
    if fem_mesh.VolumeCount > 0:  # solid mesh
        return True
        
class SolverCaseWriter:
    """
    """
    def __init__(self, analysis_obj, mesh_obj, mat_obj, boundaryConditionGroup):
        """analysis_obj should contains all the information needed,
        boundaryConditionList is a list of all boundary Conditions objects(FemConstraint)
        """
        self.case_file_name="test_openfoam" #get all these info from solver_object
        self.working_dir=home_path +"/Mod/Fem/cfd_files"
        self.analysis_obj=analysis_obj # later, changed to CfdAnalysis
        self.mesh_obj=mesh_obj #FreeCAD.getActiveDocument().getObject("Box_Mesh")
        self.bc_group=boundaryConditionGroup
        self.mesh_generatd=False
                        
    def write_case(self): 
        QApplication.setOverrideCursor(Qt.WaitCursor)
        create_empty_case(home_path + "/Mod/Fem/icoFoam_case_template.zip", self.working_dir+os.path.sep+self.case_file_name)
        self.write_mesh()
        self.write_material()
        self.write_boundary_condition_group()
        #solver control: time, 
        self.write_solver_control()
        self.write_time_control()
        QApplication.restoreOverrideCursor()
        print "Sucessfully write {} case to folder".format(self.solver_object.Name, self.working_dir)
        
    def write_bc_mesh(self, unv_mesh_file):
        FreeCAD.Console.PrintMessage('write face_set or patches for boundary\n')
        f=open(unv_mesh_file,'a') #appending to the volume mesh
        for bc in self.bcgroup:
            write_bc_faces(f, self.mesh_object,bc)
                    
    def write_mesh(self,mesh_obj=None): 
        #
        if mesh_obj==None:
            mesh_obj=self.mesh_obj
        __objs__=[]
        __objs__.append(mesh_obj)
        import Fem
        mesh_file_name=self.working_dir+os.path.sep+self.case_file_name+u".unv"
        Fem.export(__objs__,mesh_file_name)
        del __objs__
        
        #should repen this unv file and write the boundary faces
        
        #convert from UNV to OpenFoam  
        cmdline="ideasUnvToFoam -case {}  {}".format(self.working_dir, self.case_file_name+u".unv")
        #icoFoam -case WorkingDir/case_file_name
        launch_cmdline(cmdline)
        FreeCAD.Console.PrintMessage('mesh file {} converted\n'.format(mesh_file_name))
        
    def write_material(self, material=None): 
        #Air, Water, CustomedFluid, first step, default to Air
        pass
        
    def write_boundary_condition_group(self, bcgroup):
        """ switch case to deal diff boundary condition
        """
        for bc in bcgroup:
            pass
            
    def write_solver_control(self):
        pass
        
    def write_time_control(self):
        pass
        