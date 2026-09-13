# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Tim Swait <t.swait@sheffield.ac.uk                 *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "Tools to use with the Code Aster solver"
__author__ = "Tim Swait"
__url__ = "https://www.freecad.org"


import os
import re
from traceback import format_exception_only

from PySide.QtCore import QProcess, QProcessEnvironment

from vtkmodules.util import numpy_support as vtk_np
from vtkmodules.vtkCommonDataModel import vtkUnstructuredGrid

import FreeCAD
import ObjectsFem

from . import writer
from .. import settings

from femmesh import meshsetsgetter
from femtools import membertools
from femtools.objecttools import ObjectTools
from feminout import importMedResults
from feminout import importToolsFem
from femresult import resulttools


class CodeAsterTools(ObjectTools):

    name = "Code Aster"

    def __init__(self, obj):
        super().__init__(obj)
        # self.model_file = ""
        self.input_file = ""
        self.result_file = ""

    def prepare(self):
        self._clear_results()
        mesh_obj = membertools.get_mesh_to_solve(self.analysis)
        if mesh_obj is None:
            raise RuntimeError(msg)

        meshdatagetter = meshsetsgetter.MeshSetsGetter(
            self.analysis,
            self.obj,
            mesh_obj,
            membertools.AnalysisMember(self.analysis),
        )

        # set masks
        # masks = define_masks(self.obj)
        # meshdatagetter.mask_tria3 = masks["tria3"]

        # write solver input
        w = writer.FemInputWriterCodeAster(
            self.analysis,
            self.obj,
            mesh_obj,
            meshdatagetter.member,
            self.obj.WorkingDirectory,
        )
        commpath, expath = w.write_solver_input()
        self.input_file = os.path.splitext(os.path.basename(expath))[0]

    def compute(self):

        infile = self.input_file + ".export"
        self._clear_results()
        codeaster_bin = settings.get_binary("CodeAster")
        env = QProcessEnvironment.systemEnvironment()
        self.process.setProcessEnvironment(env)
        self.process.setWorkingDirectory(self.obj.WorkingDirectory)
        command_list = [infile]
        self.process.start(codeaster_bin, command_list)

        return self.process

    def update_properties(self):
        self.result_file = os.path.join(self.obj.WorkingDirectory, self.input_file + ".rmed")
        if not os.path.isfile(self.result_file):
            FreeCAD.Console.PrintError("Result file not found")
        self._load_vtk_results()
        # self._load_dat_results()

    def _clear_results(self):
        result_file = os.path.join(self.obj.WorkingDirectory, self.input_file + ".rmed")
        if os.path.isfile(result_file):
            # self.pushStatus("Deleting old result file at {}\n".format(result_file))
            FreeCAD.Console.PrintMessage("Deleting old result file at {}\n".format(result_file))
            os.remove(result_file)

    def _load_dat_results(self):
        print("_load_dat_results")
        # search dat output
        # keep_result = self.fem_param.GetGroup("General").GetBool("KeepResultsOnReRun", False)
        # dat = None
        # for res in self.obj.Results:
        # if res.isDerivedFrom("App::TextDocument"):
        # dat = res

        # if not dat or keep_result:
        # create dat output
        # dat = self.obj.Document.addObject("App::TextDocument", self.obj.Name + "Output")
        # self.analysis.addObject(dat)
        # tmp = self.obj.Results
        # tmp.append(dat)
        # self.obj.Results = tmp

        # load stress results. If test mode, load log file
        # target_file = self.log_file if self.obj.AnalysisType == "test" else self.stress_file
        # clear text data
        # dat.Text = ""
        # self.append_section(dat, self.print_file)
        # self.append_section(dat, target_file)

    def _load_vtk_results(self):
        print("_load_vtk_results")
        # search current pipeline
        keep_result = self.fem_param.GetGroup("General").GetBool("KeepResultsOnReRun", False)
        pipeline = None
        create = False
        for res in self.obj.Results:
            if res.isDerivedFrom("Fem::FemPostPipeline"):
                pipeline = res

        if not pipeline or keep_result:
            # create pipeline
            pipeline = self.obj.Document.addObject("Fem::FemPostPipeline", self.obj.Name + "Result")
            self.analysis.addObject(pipeline)
            tmp = self.obj.Results
            tmp.append(pipeline)
            self.obj.Results = tmp
            create = True

        grid = self.load_mesh()
        importMedResults.read_med_resultVTK(self.result_file, grid)
        pipeline.Data = grid

        if create and FreeCAD.GuiUp:
            # default display mode
            view_obj = pipeline.ViewObject
            view_obj.DisplayMode = "Surface"
            view_obj.SelectionStyle = "BoundBox"
            enum_field = view_obj.getEnumerationsOfProperty("Field")
            default_field = self._get_default_field()
            if default_field in enum_field:
                view_obj.Field = default_field

        # mesh = importMedResults.read_med_mesh(self.result_file)
        # result_set = importMedResults.read_med_result(self.result_file)
        # results_name = "CodeAsterResults"
        # res_obj = ObjectsFem.makeResultMechanical(self.analysis.Document, results_name)
        # result_mesh_object = ObjectsFem.makeMeshResult(
        #    self.analysis.Document, results_name + "_Mesh"
        # )
        # result_mesh_object.FemMesh = mesh
        # res_obj.Mesh = result_mesh_object
        # res_obj = importToolsFem.fill_femresult_mechanical(res_obj, result_set)
        # res_obj = resulttools.fill_femresult_stats(res_obj)
        # self.analysis.addObject(res_obj)

    def _get_default_field(self):
        return "Displacement"

    def load_mesh(self):
        try:
            grid = importMedResults.read_med_meshVTK(self.result_file)
            return grid
        except Exception as e:
            FreeCAD.Console.PrintError("".join(format_exception_only(e)))
            return vtkUnstructuredGrid()

    def load_result(self, grid):
        result_set = importMedResults.read_med_result(self.result_file)
        if "disp" in result_set.keys():
            self.load_displacement(grid, result_set)

    def load_displacement(self, grid, result_set):
        print(result_set)
        disp_vtk = vtk_np.numpy_to_vtk(result_set["disp"], deep=True)
        disp_vtk.SetName("Displacement")
        grid.GetPointData().AddArray(disp_vtk)

    def load_force(self, grid):
        """
        path_o4 = os.path.join(self.obj.WorkingDirectory, self.force_file)
        force = z88utils.read_nodal_result(path_o4, grid)
        if force is None:
            return

        # plate model force_x -> force_z, force_y -> moment_x, force_z -> moment_y
        if self.obj.ModelSpace == "plate":
            force_vtk = vtk_np.numpy_to_vtk(force[:, 1:2], deep=True)
            force_vtk.SetName("Nodal Force")
            grid.GetPointData().AddArray(force_vtk)
            moment_vtk = vtk_np.numpy_to_vtk(force[:, 2:4], deep=True)
            moment_vtk.SetName("Nodal Moment")
            grid.GetPointData().AddArray(moment_vtk)
        else:
            force_vtk = vtk_np.numpy_to_vtk(force[:, 1:4], deep=True)
            force_vtk.SetName("Nodal Force")
            grid.GetPointData().AddArray(force_vtk)
            moment_vtk = vtk_np.numpy_to_vtk(force[:, 4:], deep=True)
            moment_vtk.SetName("Nodal Moment")
            grid.GetPointData().AddArray(moment_vtk)
        """

    def generate_disp_mesh(self, grid):
        """
        if not grid.GetPointData().HasArray("Displacement"):
            return

        points = grid.GetPoints()
        pd = points.GetData()
        disp = grid.GetPointData().GetAbstractArray("Displacement")
        # check 2d analysis
        disp = vtk_np.vtk_to_numpy(disp)
        disp = disp.reshape((len(disp), -1))
        rows, cols = disp.shape
        if cols == 2:
            # plane stress, axisymmetric models
            disp = np.hstack((disp, np.zeros((rows, 1))))
        elif cols == 1:
            # plate model
            disp = np.hstack((np.zeros((rows, 2)), disp))

        disp_points = vtk_np.vtk_to_numpy(pd) + disp
        disp_points = vtk_np.numpy_to_vtk(disp_points, deep=True)
        points.SetData(disp_points)
        """

    def save_section_print(self, grid):
        """
        path_section_print = os.path.join(self.obj.WorkingDirectory, self.section_print_file)
        if not os.path.exists(path_section_print):
            return

        sp_dict = np.load(path_section_print, allow_pickle=True).item()
        path_print = os.path.join(self.obj.WorkingDirectory, self.print_file)
        data_file = open(path_print, "w")

        pd = grid.GetPointData()
        if pd.HasArray("Nodal Force"):
            force = pd.GetAbstractArray("Nodal Force")
            force = vtk_np.vtk_to_numpy(force)
            max_len_name = max(map(len, sp_dict.keys()))
            if self.obj.ModelSpace == "plate":
                cols = 1
                names = ["Fz"]
            else:
                cols = 3
                names = ["Fx", "Fy", "Fz"]

            row_format = f"{{:<{max_len_name}}}" + "{:>15}" * cols + "\n"
            data_file.write(row_format.format("", *names))

            for feat, nodes in sp_dict.items():
                result = (
                    force[list(nodes)]
                    .sum(axis=0)
                    .reshape(
                        -1,
                    )
                )
                force_row = list(map(lambda x: "{:E}".format(x), result))
                data_file.write(row_format.format(feat, *force_row))

        data_file.close()
        """

    def append_section(self, obj, target_file):
        """
        files = os.listdir(self.obj.WorkingDirectory)
        for f in files:
            if f.lower() == target_file:
                section_header = "#" * 80 + "\n" + "Output file: " + f + "\n\n"
                data_file = os.path.join(self.obj.WorkingDirectory, f)
                with open(data_file, "r") as file:
                    obj.Text = obj.Text + section_header + file.read() + "\n"
                break
        """

    def version(self):
        return "Version: ?"
