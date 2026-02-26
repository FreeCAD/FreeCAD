# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "Tools for the work with Elmer solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"


from PySide.QtCore import QProcess, QProcessEnvironment
import os
import re
import shutil

import FreeCAD

from . import writer
from .. import settings

from femtools import membertools
from femtools.objecttools import ObjectTools


class ElmerTools(ObjectTools):

    name = "Elmer"

    def __init__(self, obj):
        super().__init__(obj)
        self.model_file = ""
        self._result_format = ""

    def prepare(self):
        w = writer.Writer(self.obj, self.obj.WorkingDirectory)
        w.write_solver_input()

        mesh = w.getSingleMember("Fem::FemMeshObject")
        if not mesh.FemMesh.Groups:
            raise ValueError(f"Mesh object '{mesh.Label}' has no groups, please remesh\n")

        mesh_file = os.path.join(self.obj.WorkingDirectory, "mesh.unv")
        mesh.FemMesh.write(mesh_file)

        grid_bin = settings.get_binary("ElmerGrid")
        env = QProcessEnvironment.systemEnvironment()
        p = QProcess()
        p.setProcessEnvironment(env)
        p.setWorkingDirectory(self.obj.WorkingDirectory)
        grid_args = ["8", "2", mesh_file, "-out", self.obj.WorkingDirectory]
        p.start(grid_bin, grid_args)
        p.waitForFinished(-1)
        num_proc = self.fem_param.GetGroup("Elmer").GetInt("NumberOfTasks", 1)
        if num_proc > 1:
            # MPI parallel computing version
            grid_args.extend(["-partdual", "-metiskway", str(num_proc)])
            p.start(grid_bin, grid_args)
            p.waitForFinished()

        self.model_file = os.path.join(self.obj.WorkingDirectory, writer._SIF_NAME)
        handled = w.getHandledConstraints()
        allConstraints = membertools.get_member(self.analysis, "Fem::Constraint")
        for obj in set(allConstraints) - handled:
            FreeCAD.Console.PrintWarning(f"Ignored constraint {obj.Label}")

    def compute(self):
        self._clear_results()
        elmer_bin = settings.get_binary("ElmerSolver")
        num_proc = self.fem_param.GetGroup("Elmer").GetInt("NumberOfTasks", 1)
        num_thr = self.fem_param.GetGroup("Elmer").GetInt("ThreadsPerTask", 1)
        env = QProcessEnvironment.systemEnvironment()
        env.insert("OMP_NUM_THREADS", str(num_thr))
        self.process.setProcessEnvironment(env)
        self.process.setWorkingDirectory(self.obj.WorkingDirectory)

        if num_proc > 1:
            # MPI parallel computing version
            mpi = shutil.which("mpiexec")
            self._result_format = ".pvtu"
            command_list = ["-n", str(num_proc), elmer_bin]
            self.process.start(mpi, command_list)
        else:
            self._result_format = ".vtu"
            command_list = []
            self.process.start(elmer_bin, command_list)

        if self.obj.SimulationType == "Transient":
            self._result_format = ".pvd"

        return self.process

    def update_properties(self):
        self._load_vtk_results()
        self._load_dat_results()

    def _clear_results(self):
        dir_content = os.listdir(self.obj.WorkingDirectory)
        for f in dir_content:
            path = os.path.join(self.obj.WorkingDirectory, f)
            base, ext = os.path.splitext(path)
            if ext in [".vtu", ".vtp", ".pvtu", ".pvd", ".dat"]:
                os.remove(path)

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

        files = os.listdir(self.obj.WorkingDirectory)
        for f in files:
            base, ext = os.path.splitext(f)
            if ext == self._result_format:
                res = os.path.join(self.obj.WorkingDirectory, f)
                pipeline.read(res)
                break

        if create:
            # default display mode
            pipeline.ViewObject.DisplayMode = "Surface"
            pipeline.ViewObject.SelectionStyle = "BoundBox"
            fields = pipeline.ViewObject.getEnumerationsOfProperty("Field")
            for f in fields:
                # beware of possible suffix Im or Re
                if f.lower().startswith(self._get_default_field()):
                    pipeline.ViewObject.Field = f
                    break

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

        files = os.listdir(self.obj.WorkingDirectory)
        for f in files:
            if f.endswith(".dat"):
                dat_file = os.path.join(self.obj.WorkingDirectory, f)
                with open(dat_file, "r") as file:
                    dat.Text = file.read()
                break

    def _get_default_field(self):
        default = "None"
        for eq in self.obj.Group:
            match eq.Proxy.Type:
                case "Fem::EquationElmerHeat":
                    default = "temperature"
                case "Fem::EquationElmerElasticity" | "Fem::EquationElmerDeformation":
                    default = "displacement"
                case "Fem::EquationElmerElectrostatic" | "Fem::EquationElmerStaticCurrent":
                    default = "potential"
                case "Fem::EquationElmerFlow":
                    default = "pressure"
                case "Fem::EquationElmerMagnetodynamic" | "Fem::EquationElmerMagnetodynamic2D":
                    default = "magnetic flux"
        return default

    def version(self):
        p = QProcess()
        elmer_bin = settings.get_binary("ElmerSolver")
        p.start(elmer_bin, ["-v"])
        p.waitForFinished()
        info = p.readAll().data().decode()
        reg_exp = re.compile(r"Version:\s*(?P<version>.*)$", re.M)
        m = reg_exp.search(info)
        ver = "Version: {}".format(m.group("version") if m else "")
        return ver
