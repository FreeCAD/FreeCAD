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


from PySide.QtCore import QProcess, QThread
import tempfile
import os
import shutil

import FreeCAD
import Fem

from . import writer
from .. import settings

# from feminout import importCcxDatResults
from femmesh import meshsetsgetter
from femtools import membertools


class CalculiXTools:

    name = "CalculiX"

    def __init__(self, obj):
        self.obj = obj
        self.process = QProcess()
        self.model_file = ""
        self.analysis = obj.getParentGroup()
        self.fem_param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        self._create_working_directory(obj)

    def _create_working_directory(self, obj):
        """
        Create working directory according to preferences
        """
        if not os.path.isdir(obj.WorkingDirectory):
            gen_param = self.fem_param.GetGroup("General")
            if gen_param.GetBool("UseTempDirectory"):
                self.obj.WorkingDirectory = tempfile.mkdtemp(prefix="fem_")
            elif gen_param.GetBool("UseBesideDirectory"):
                root, ext = os.path.splitext(obj.Document.FileName)
                if root:
                    self.obj.WorkingDirectory = os.path.join(root, obj.Label)
                    os.makedirs(self.obj.WorkingDirectory, exist_ok=True)
                else:
                    # file not saved, use temporary
                    self.obj.WorkingDirectory = tempfile.mkdtemp(prefix="fem_")
            elif gen_param.GetBool("UseCustomDirectory"):
                self.obj.WorkingDirectory = gen_param.GetString("CustomDirectoryPath")
                os.makedirs(self.obj.WorkingDirectory, exist_ok=True)

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
        env = self.process.processEnvironment()
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
        self._load_ccxfrd_results()
        self._load_ccxdat_results()

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

    def _load_ccxdat_results(self):
        # search dat output
        dat = None
        for res in self.obj.Results:
            if res.isDerivedFrom("App::TextDocument"):
                dat = res

        if not dat:
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

    def _load_ccxfrd_results(self):
        # search current pipeline
        pipeline = None
        create = False
        for res in self.obj.Results:
            if res.isDerivedFrom("Fem::FemPostPipeline"):
                pipeline = res

        if not pipeline:
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
                pipeline.read(vtm_file)
                pipeline.renameArrays(self.frd_var_conversion(self.obj.AnalysisType))
                break

        if create:
            # default display mode
            pipeline.ViewObject.DisplayMode = "Surface"
            pipeline.ViewObject.SelectionStyle = "BoundBox"
            pipeline.ViewObject.Field = self.get_default_field(self.obj.AnalysisType)

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

    def get_default_field(self, analysis_type):
        match analysis_type:
            case "static" | "frequency" | "buckling":
                return "Displacement"
            case "thermomech":
                return "Temperature"
            case "electromagnetic":
                return "Potential"

    def version(self):
        p = QProcess()
        ccx_bin = settings.get_binary("Calculix")
        p.start(ccx_bin, ["-v"])
        p.waitForFinished()
        info = p.readAll().data().decode()
        return info
