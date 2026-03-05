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

__title__ = "Tools for the work with CalculiX solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"


from PySide.QtCore import QProcess, QThread, QProcessEnvironment
import os
import shutil

from vtkmodules.util import numpy_support as vtk_np
from vtkmodules.vtkIOXML import vtkXMLMultiBlockDataReader
from vtkmodules.vtkCommonDataModel import vtkMultiBlockDataSet
import numpy as np

import FreeCAD
import Fem

from . import writer
from .. import settings

from femmesh import meshsetsgetter
from femtools import membertools
from femtools.objecttools import ObjectTools


class CalculiXTools(ObjectTools):

    name = "CalculiX"

    def __init__(self, obj):
        super().__init__(obj)
        self.model_file = ""

    def prepare(self):
        from femtools.checksanalysis import check_member_for_solver_calculix

        self._clear_results()

        message = check_member_for_solver_calculix(
            self.analysis,
            self.obj,
            membertools.get_mesh_to_solve(self.analysis)[0],
            membertools.AnalysisMember(self.analysis),
        )

        mesh_obj = membertools.get_mesh_to_solve(self.analysis)[0]
        meshdatagetter = meshsetsgetter.MeshSetsGetter(
            self.analysis,
            self.obj,
            mesh_obj,
            membertools.AnalysisMember(self.analysis),
        )
        meshdatagetter.get_mesh_sets()

        # write solver input
        w = writer.FemInputWriterCcx(
            self.analysis,
            self.obj,
            mesh_obj,
            meshdatagetter.member,
            self.obj.WorkingDirectory,
            meshdatagetter.mat_geo_sets,
        )
        self.model_file = w.write_solver_input()
        # report to user if task succeeded
        self.input_deck = os.path.splitext(os.path.basename(self.model_file))[0]

    def compute(self):
        self._clear_results()
        ccx_bin = settings.get_binary("Calculix")
        env = QProcessEnvironment.systemEnvironment()
        num_cpu = self.fem_param.GetGroup("Ccx").GetInt(
            "AnalysisNumCPUs", QThread.idealThreadCount()
        )
        env.insert("OMP_NUM_THREADS", str(num_cpu))
        pastix_prec = "1" if self.obj.PastixMixedPrecision else "0"
        env.insert("PASTIX_MIXED_PRECISION", pastix_prec)
        self.process.setProcessEnvironment(env)
        self.process.setWorkingDirectory(self.obj.WorkingDirectory)

        command_list = ["-i", os.path.join(self.obj.WorkingDirectory, self.input_deck)]
        self.process.start(ccx_bin, command_list)

        return self.process

    def update_properties(self):
        # TODO at the moment, only one .vtm file is assumed
        self._load_vtk_results()
        self._load_dat_results()

    def _clear_results(self):
        # result is a 'Result.vtm' file and a 'Result' directory
        # with the .vtu files
        dir_content = os.listdir(self.obj.WorkingDirectory)
        for f in dir_content:
            path = os.path.join(self.obj.WorkingDirectory, f)
            base, ext = os.path.splitext(path)
            if ext == ".vtm":
                # remove .vtm file
                os.remove(path)
                # remove dir with .vtu files
                shutil.rmtree(base)
            elif ext == ".dat":
                # remove .dat file
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

        files = os.listdir(self.obj.WorkingDirectory)
        for f in files:
            if f.endswith(".dat"):
                dat_file = os.path.join(self.obj.WorkingDirectory, f)
                with open(dat_file, "r") as file:
                    dat.Text = file.read()
                break

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

        frd_result_prefix = os.path.join(self.obj.WorkingDirectory, self.input_deck)
        binary_mode = self.fem_param.GetGroup("Ccx").GetBool("BinaryOutput", False)
        Fem.frdToVTK(frd_result_prefix + ".frd", binary_mode)
        files = os.listdir(self.obj.WorkingDirectory)
        for f in files:
            if f.endswith(".vtm"):
                vtm_file = os.path.join(self.obj.WorkingDirectory, f)
                reader = vtkXMLMultiBlockDataReader()
                reader.SetFileName(vtm_file)
                reader.Update()
                multi_block = reader.GetOutput()
                multi_block = self._generate_derived_result(multi_block)
                if self.obj.DisplaceMesh:
                    multi_block = self._generate_disp_mesh(multi_block)
                pipeline.Data = multi_block
                break

        pipeline.renameArrays(self.frd_var_conversion(self.obj.AnalysisType))
        self._set_time_info(pipeline)

        if create and FreeCAD.GuiUp:
            # default display mode
            view_obj = pipeline.ViewObject
            view_obj.DisplayMode = "Surface"
            view_obj.SelectionStyle = "BoundBox"
            enum_field = view_obj.getEnumerationsOfProperty("Field")
            default_field = self._get_default_field()
            if default_field in enum_field:
                view_obj.Field = default_field

    def frd_var_conversion(self, analysis_type):
        common = {
            "CONTACT": "Contact Displacement",
            "PE": "Plastic Strain",
            "CELS": "Contact Energy",
            "ECD": "Current Density",
            "EMFB": "Magnetic Field",
            "EMFE": "Electric Field",
            "ENER": "Internal Energy Density",
            "DISP": "Displacement",
            "TOSTRAIN": "Strain",
            "STRESS": "Stress",
            "STR(%)": "Error",
        }
        thermo = {"FLUX": "Heat Flux", "T": "Temperature"}
        electrostatic = {"T": "Potential", "FLUX": "Electric Flux Density"}

        match analysis_type:
            case "thermomech":
                common.update(thermo)
            case "electromagnetic":
                common.update(electrostatic)

        return common

    def _get_default_field(self):
        match self.obj.AnalysisType:
            case "static" | "frequency" | "buckling":
                return "Displacement"
            case "thermomech":
                return "Temperature"
            case "electromagnetic":
                return "Potential"
            case _:
                return "None"

    def _generate_disp_mesh(self, mb):
        for i in range(mb.GetNumberOfBlocks()):
            grid = mb.GetBlock(i)
            if grid is None:
                return mb
            pd = grid.GetPointData()
            if (pd is None) or not pd.HasArray("DISP"):
                return mb
            points = grid.GetPoints()
            pos = points.GetData()
            disp = pd.GetAbstractArray("DISP")
            disp_points = vtk_np.vtk_to_numpy(pos) + vtk_np.vtk_to_numpy(disp)
            disp_points = vtk_np.numpy_to_vtk(disp_points)
            points.SetData(disp_points)

        mb_out = vtkMultiBlockDataSet()
        mb_out.DeepCopy(mb)

        return mb_out

    def _generate_derived_result(self, mb):
        for i in range(mb.GetNumberOfBlocks()):
            grid = mb.GetBlock(i)
            if grid is None:
                return mb
            pd = grid.GetPointData()
            if (pd is None) or not pd.HasArray("STRESS"):
                return mb

            stress = pd.GetAbstractArray("STRESS")
            s_xx, s_yy, s_zz, s_xy, s_yz, s_xz = vtk_np.vtk_to_numpy(stress).transpose()
            # create symmetric matrix from [Sxx, Syy, Szz, Sxy, Syz, Sxz]
            sym_stress = (
                np.array([s_xx, s_xy, s_xz, s_xy, s_yy, s_yz, s_xz, s_yz, s_zz])
                .transpose()
                .reshape(-1, 3, 3)
            )
            prin_val, prin_axes = np.linalg.eigh(sym_stress)

            sigma_1 = prin_val[:, 2]
            sigma_3 = prin_val[:, 0]
            sigma_2 = prin_val[:, 1]
            sigma_1_axis = prin_axes[:, :, 2]
            sigma_3_axis = prin_axes[:, :, 0]
            sigma_2_axis = prin_axes[:, :, 1]

            sigma_1_vtk = vtk_np.numpy_to_vtk(sigma_1)
            sigma_1_vtk.SetName("Major Principal Stress")
            sigma_3_vtk = vtk_np.numpy_to_vtk(sigma_3)
            sigma_3_vtk.SetName("Minor Principal Stress")
            sigma_2_vtk = vtk_np.numpy_to_vtk(sigma_2)
            sigma_2_vtk.SetName("Intermediate Principal Stress")
            sigma_1_axis_vtk = vtk_np.numpy_to_vtk(sigma_1_axis)
            sigma_1_axis_vtk.SetName("Major Principal Stress Direction")
            sigma_3_axis_vtk = vtk_np.numpy_to_vtk(sigma_3_axis)
            sigma_3_axis_vtk.SetName("Minor Principal Stress Direction")
            sigma_2_axis_vtk = vtk_np.numpy_to_vtk(sigma_2_axis)
            sigma_2_axis_vtk.SetName("Intermediate Principal Stress Direction")

            # add principal stress
            pd.AddArray(sigma_1_vtk)
            pd.AddArray(sigma_3_vtk)
            pd.AddArray(sigma_2_vtk)
            # add principal directions
            pd.AddArray(sigma_1_axis_vtk)
            pd.AddArray(sigma_3_axis_vtk)
            pd.AddArray(sigma_2_axis_vtk)

            sigma_diff = np.array([sigma_1 - sigma_2, sigma_2 - sigma_3, sigma_3 - sigma_1])

            # von Mises stress
            von_mises = np.linalg.norm(sigma_diff, axis=0) / np.sqrt(2)
            von_mises_vtk = vtk_np.numpy_to_vtk(von_mises)
            von_mises_vtk.SetName("von Mises Stress")
            pd.AddArray(von_mises_vtk)

            # max shear stress
            max_shear = 1 / 2 * np.max(np.abs(sigma_diff), axis=0)
            max_shear_vtk = vtk_np.numpy_to_vtk(max_shear)
            max_shear_vtk.SetName("Tresca Stress")
            pd.AddArray(max_shear_vtk)

            # max abs principal stress
            max_abs_sigma = np.max(np.abs(np.array([sigma_1, sigma_2, sigma_3])), axis=0)
            max_abs_sigma_vtk = vtk_np.numpy_to_vtk(max_abs_sigma)
            max_abs_sigma_vtk.SetName("Maximum Absolute Principal Stress")
            pd.AddArray(max_abs_sigma_vtk)

        mb_out = vtkMultiBlockDataSet()
        mb_out.DeepCopy(mb)

        return mb_out

    def _set_time_info(self, pipeline):
        match self.obj.AnalysisType:
            case "frequency":
                pipeline.setTimeInfo("Frequency", FreeCAD.Units.Frequency)
            case "buckling":
                pipeline.setTimeInfo("Buckling factor", FreeCAD.Units.Unit())
            case _:
                pipeline.setTimeInfo("TimeStep", FreeCAD.Units.TimeSpan)

    def version(self):
        p = QProcess()
        ccx_bin = settings.get_binary("Calculix")
        p.start(ccx_bin, ["-v"])
        p.waitForFinished()
        info = p.readAll().data().decode()
        return info
