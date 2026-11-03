# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Nicholas Herrmann <nicholas.herrmann@proton.me>    *
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

__title__ = "FreeCAD FEM solver OpenRadioss tasks"
__author__ = "Nicholas Herrmann"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import os
import subprocess

import FreeCAD

from . import model
from . import results
from . import runner
from . import writer
from .. import run
from femtools import membertools

JOB_NAME = "radioss"
RESULT_PREFIX = "OpenRadiossResult"


class Check(run.Check):

    def run(self):
        self.pushStatus("Checking analysis members...\n")
        self.check_mesh_exists()
        self.check_material_exists()
        self.checkSupported([(t,) for t in model.SUPPORTED_CONSTRAINTS])
        if self.testmode:
            return  # writing the deck needs no solver
        problems = runner.get_installation().problems()
        if problems:
            self.report.error(
                "; ".join(problems) + ". Set the solver folder (or download OpenCourant) in "
                "Edit > Preferences > FEM > OpenRadioss."
            )
            self.fail()


class Prepare(run.Prepare):

    def run(self):
        self.pushStatus("Writing OpenRadioss deck...\n")
        try:
            deck_model, warnings = model.build_model(self.analysis, self.solver)
        except model.ModelError as e:
            self.report.error(str(e))
            self.fail()
            return
        for warning in warnings:
            self.report.warning(warning)
            self.pushStatus(f"Warning: {warning}\n")
        # Results of an earlier run in this directory would be mixed into the new ones
        for f in os.listdir(self.directory):
            if runner.is_result_file(f, JOB_NAME):
                os.remove(os.path.join(self.directory, f))
        path = os.path.join(self.directory, JOB_NAME + ".k")
        with open(path, "w") as f:
            f.write(writer.write_deck(deck_model))
        n_elements = sum(len(p["elements"]) for p in deck_model["parts"])
        self.pushStatus(
            f"Deck written: {path} ({n_elements} elements, {len(deck_model['parts'])} parts)\n"
        )


class Solve(run.Solve):

    def run(self):
        inst = runner.get_installation()
        copied = inst.prepare_flatpak()
        if copied:
            self.pushStatus(f"Copied host libraries for the Flatpak sandbox: {', '.join(copied)}\n")
        threads = int(self.solver.Threads)
        env = inst.environment(threads)
        deck = os.path.join(self.directory, JOB_NAME + ".k")
        starter_listing = os.path.join(self.directory, f"{JOB_NAME}_0000.out")

        self.pushStatus("Running OpenRadioss starter...\n")
        if not self._execute(inst.starter_command(deck, threads), env) and not self.aborted:
            for message in runner.listing_messages(starter_listing):
                self.report.error(message)
            self.report.error(f"The starter failed, see {starter_listing}")
            self.fail()
            return
        errors, warnings = runner.listing_summary(starter_listing)
        self.pushStatus(f"\nStarter: {errors} errors, {warnings} warnings\n")
        if errors:
            self.report.error(f"The starter found {errors} error(s) in the deck.")
            self.fail()
            return

        for engine_input in runner.engine_inputs(self.directory, JOB_NAME):
            if self.aborted:
                return
            self.pushStatus(f"Running OpenRadioss engine ({os.path.basename(engine_input)})...\n")
            ok = self._execute(inst.engine_command(engine_input, threads), env)
            out = os.path.splitext(engine_input)[0] + ".out"
            if self.aborted:
                return
            if not ok or not runner.normal_termination(out):
                self.report.error(f"The engine did not terminate normally, see {out}")
                self.fail()
                return

        animations = runner.animation_files(self.directory, JOB_NAME)
        self.pushStatus(f"\nConverting {len(animations)} result frames to VTK...\n")
        frames = []
        for animation in animations:
            if self.aborted:
                return
            frames.append(runner.convert_animation(inst, animation, env, self.directory))
        if frames:
            runner.write_pvd(frames, os.path.join(self.directory, JOB_NAME + ".pvd"))
        self.pushStatus(f"Done: {len(frames)} frames, series {JOB_NAME}.pvd\n")

    def _execute(self, argv, env):
        """Run a solver step, stream its output to the status; returns True on exit code 0."""
        process = subprocess.Popen(
            argv,
            cwd=self.directory,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        self.signalAbort.add(process.terminate)
        try:
            self._observeSolver(process)
            process.wait()
        finally:
            self.signalAbort.remove(process.terminate)
        return process.returncode == 0


class Results(run.Results):

    def run(self):
        frames = sorted(
            f
            for f in os.listdir(self.directory)
            if f.startswith(JOB_NAME + "A") and f.endswith(".vtu")
        )
        if not frames:
            self.report.error("No result frames found.")
            self.fail()
            return
        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
        doc = self.analysis.Document
        if not prefs.GetBool("KeepResultsOnReRun", False):
            old = [obj.Name for obj in self.analysis.Group if obj.Name.startswith(RESULT_PREFIX)]
            old_pipelines = [
                obj.Name
                for obj in self.analysis.Group
                if obj.Name.startswith("Pipeline_" + RESULT_PREFIX)
            ]
            for name in old:
                doc.removeObject(name)
            # pipelines have a 3D view: remove them in the main thread (queued before the new one)
            results.in_main_thread(
                lambda: [doc.removeObject(n) for n in old_pipelines if doc.getObject(n)]
            )
        last = os.path.join(self.directory, frames[-1])
        mesh_obj = membertools.get_mesh_to_solve(self.analysis)[0]
        end_time = self.solver.EndTime.getValueAs("s").Value
        result, _pipeline = results.load_result(
            doc,
            last,
            RESULT_PREFIX,
            self.analysis,
            femmesh=mesh_obj.FemMesh,
            time=end_time,
            pipeline_label=f"Pipeline {RESULT_PREFIX} (t = {end_time:g} s)",
        )
        result.Label = f"{RESULT_PREFIX} (t = {end_time:g} s)"
        max_disp = max(result.DisplacementLengths or [0])
        max_mises = max(result.vonMises or [0])
        pvd = os.path.join(self.directory, JOB_NAME + ".pvd")
        self.pushStatus(
            f"Loaded {frames[-1]} as '{result.Label}' (max. displacement {max_disp:.4g} mm, "
            f"max. von Mises {max_mises:.4g} MPa). All {len(frames)} frames: {pvd}\n"
        )


##  @}
