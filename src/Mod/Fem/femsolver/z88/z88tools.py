# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "Tools for the work with Z88 solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"


import os
import re
from traceback import format_exception_only

from PySide.QtCore import QProcess, QProcessEnvironment

import numpy as np

from vtkmodules.util import numpy_support as vtk_np
from vtkmodules.vtkCommonDataModel import vtkUnstructuredGrid

import FreeCAD

from . import writer
from .. import settings
from . import z88utils

from femmesh import meshsetsgetter
from femtools import membertools
from femtools.objecttools import ObjectTools


class Z88Tools(ObjectTools):

    name = "Z88"

    def __init__(self, obj):
        super().__init__(obj)
        self.model_file = ""

    def prepare(self):
        self._clear_results()

        mesh_obj, msg = membertools.get_mesh_to_solve(self.analysis)
        if mesh_obj is None:
            raise RuntimeError(msg)

        meshdatagetter = meshsetsgetter.MeshSetsGetter(
            self.analysis,
            self.obj,
            mesh_obj,
            membertools.AnalysisMember(self.analysis),
        )

        # set masks
        masks = z88utils.define_masks()
        meshdatagetter.mask_tria6 = masks["tria6"]
        meshdatagetter.mask_quad8 = masks["quad8"]
        meshdatagetter.mask_tetra4 = masks["tetra4"]
        meshdatagetter.mask_tetra10 = masks["tetra10"]
        meshdatagetter.mask_hexa8 = masks["hexa8"]
        meshdatagetter.mask_hexa20 = masks["hexa20"]

        meshdatagetter.get_mesh_sets()

        # write solver input
        w = writer.FemInputWriterZ88(
            self.analysis,
            self.obj,
            mesh_obj,
            meshdatagetter.member,
            self.obj.WorkingDirectory,
        )
        w.write_solver_input()

    def compute(self):
        self._clear_results()
        z88_bin = settings.get_binary("Z88")
        env = QProcessEnvironment.systemEnvironment()
        self.process.setProcessEnvironment(env)
        self.process.setWorkingDirectory(self.obj.WorkingDirectory)
        command_list = ["-t", "-" + self.obj.SolverType]

        if self.obj.AnalysisType == "test":
            self.process.start(z88_bin, command_list)
        else:
            # first run test mode (mandatory) in another process to prevent self.process
            # emit signals
            prepare_process = QProcess()
            prepare_env = QProcessEnvironment.systemEnvironment()
            prepare_process.setProcessEnvironment(prepare_env)
            prepare_process.setWorkingDirectory(self.obj.WorkingDirectory)
            prepare_process.start(z88_bin, command_list)
            prepare_process.waitForFinished(-1)

            # compute mode
            command_list = ["-c", "-" + self.obj.SolverType]
            self.process.start(z88_bin, command_list)

        return self.process

    def update_properties(self):
        self._load_vtk_results()
        self._load_dat_results()

    def _clear_results(self):
        # results are z88oN.txt files
        dir_content = os.listdir(self.obj.WorkingDirectory)
        for f in dir_content:
            if re.match(r"^z88o\d+\.txt$", f) or re.match(r"^.+\.dat$", f):
                path = os.path.join(self.obj.WorkingDirectory, f)
                os.remove(path)

    def _load_dat_results(self):
        # search dat output
        keep_result = self.fem_param.GetGroup("General").GetBool("KeepResultsOnReRun", False)
        dat = None
        for res in self.obj.Results:
            if res.isDerivedFrom("App::TextDocument"):
                dat = res

        if not dat or keep_result:
            # create dat output
            dat = self.obj.Document.addObject("App::TextDocument", self.obj.Name + "Output")
            self.analysis.addObject(dat)
            tmp = self.obj.Results
            tmp.append(dat)
            self.obj.Results = tmp

        # load stress results. If test mode, load log file
        target_file = self.log_file if self.obj.AnalysisType == "test" else self.stress_file
        # clear text data
        dat.Text = ""
        self.append_section(dat, self.print_file)
        self.append_section(dat, target_file)

    def _load_vtk_results(self):
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

        if self.obj.AnalysisType == "test":
            pipeline.Data = vtkUnstructuredGrid()
            return

        grid = self.load_mesh()
        self.load_result(grid)
        if self.obj.DisplaceMesh:
            self.generate_disp_mesh(grid)
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

    def _get_default_field(self):
        return "Displacement"

    def load_mesh(self):
        try:
            path_o0 = os.path.join(self.obj.WorkingDirectory, self.mesh_file)
            grid = z88utils.load_mesh(path_o0)
            return grid
        except Exception as e:
            FreeCAD.Console.PrintError("".join(format_exception_only(e)))
            return vtkUnstructuredGrid()

    def load_result(self, grid):
        try:
            self.load_displacement(grid)
        except Exception as e:
            FreeCAD.Console.PrintError("".join(format_exception_only(e)))
        try:
            self.load_force(grid)
        except Exception as e:
            FreeCAD.Console.PrintError("".join(format_exception_only(e)))
        try:
            self.save_section_print(grid)
        except Exception as e:
            FreeCAD.Console.PrintError("".join(format_exception_only(e)))

    def load_displacement(self, grid):
        path_o2 = os.path.join(self.obj.WorkingDirectory, self.disp_file)
        disp = z88utils.read_nodal_result(path_o2, grid)
        if disp is None:
            return

        # plate model disp_x -> disp_z, disp_y -> rot_x, disp_z -> rot_y
        if self.obj.ModelSpace == "plate":
            disp_vtk = vtk_np.numpy_to_vtk(disp[:, 1:2], deep=True)
            disp_vtk.SetName("Displacement")
            grid.GetPointData().AddArray(disp_vtk)
            # set rotations in degrees here until we have a general method to set units
            disp_vtk = vtk_np.numpy_to_vtk(disp[:, 2:4] * 180 / np.pi, deep=True)
            disp_vtk.SetName("Rotation")
            grid.GetPointData().AddArray(disp_vtk)
        else:
            disp_vtk = vtk_np.numpy_to_vtk(disp[:, 1:4], deep=True)
            disp_vtk.SetName("Displacement")
            grid.GetPointData().AddArray(disp_vtk)

    def load_force(self, grid):
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

    def generate_disp_mesh(self, grid):
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

    def save_section_print(self, grid):
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

    def append_section(self, obj, target_file):
        files = os.listdir(self.obj.WorkingDirectory)
        for f in files:
            if f.lower() == target_file:
                section_header = "#" * 80 + "\n" + "Output file: " + f + "\n\n"
                data_file = os.path.join(self.obj.WorkingDirectory, f)
                with open(data_file, "r") as file:
                    obj.Text = obj.Text + section_header + file.read() + "\n"
                break

    def version(self):
        return "Version: ?"

    mesh_file = "z88o0.txt"
    disp_file = "z88o2.txt"
    stress_file = "z88o3.txt"
    force_file = "z88o4.txt"
    section_print_file = "z88section_print.npy"
    print_file = "z88print.dat"
    log_file = "z88r.log"
