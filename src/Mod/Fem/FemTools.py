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
from PySide import QtCore


class FemTools(QtCore.QRunnable, QtCore.QObject):

    finished = QtCore.Signal(int)

    def __init__(self, analysis=None):
        QtCore.QRunnable.__init__(self)
        QtCore.QObject.__init__(self)

        if analysis:
            self.analysis = analysis
        else:
            self.analysis = FemGui.getActiveAnalysis()
        if self.analysis:
            self.update_objects()
            self.base_name = ""
            self.results_present = False
            self.setup_working_dir()
            self.setup_ccx()
        else:
            raise Exception('FEM: No active analysis found!')

    def purge_results(self):
        for m in self.analysis.Member:
            if (m.isDerivedFrom('Fem::FemResultObject')):
                FreeCAD.ActiveDocument.removeObject(m.Name)
        self.results_present = False

    def reset_mesh_deformation(self):
        if self.mesh:
            self.mesh.ViewObject.applyDisplacement(0.0)

    def reset_mesh_color(self):
        if self.mesh:
            self.mesh.ViewObject.NodeColor = {}
            self.mesh.ViewObject.ElementColor = {}
            self.mesh.ViewObject.setNodeColorByScalars()

    def show_result(self, result_type="Sabs"):
        self.update_objects()
        if result_type == "None":
            self.reset_mesh_color()
            return
        if self.result_object:
            if result_type == "Sabs":
                values = self.result_object.StressValues
            elif result_type == "Uabs":
                values = self.result_object.DisplacementLengths
            else:
                match = {"U1": 0, "U2": 1, "U3": 2}
                d = zip(*self.result_object.DisplacementVectors)
                values = list(d[match[result_type]])
            self.mesh.ViewObject.setNodeColorByScalars(self.result_object.ElementNumbers, values)

    def show_displacement(self, displacement_factor=0.0):
        self.mesh.ViewObject.setNodeDisplacementByVectors(self.result_object.ElementNumbers,
                                                          self.result_object.DisplacementVectors)
        self.mesh.ViewObject.applyDisplacement(displacement_factor)

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

    def write_inp_file(self):
        import ccxInpWriter as iw
        import sys
        self.base_name = ""
        try:
            inp_writer = iw.inp_writer(self.analysis, self.mesh, self.material,
                                       self.fixed_constraints, self.force_constraints,
                                       self.pressure_constraints, self.working_dir)
            self.base_name = inp_writer.write_calculix_input_file()
        except:
            print "Unexpected error when writing CalculiX input file:", sys.exc_info()[0]
            raise

    def start_ccx(self):
        import multiprocessing
        import os
        import subprocess
        if self.base_name != "":
            ont_backup = os.environ.get('OMP_NUM_THREADS')
            if not ont_backup:
                ont_backup = ""
            _env = os.putenv('OMP_NUM_THREADS', str(multiprocessing.cpu_count()))
            # change cwd because ccx may crash if directory has no write permission
            # there is also a limit of the length of file names so jump to the document directory
            cwd = QtCore.QDir.currentPath()
            f = QtCore.QFileInfo(self.base_name)
            QtCore.QDir.setCurrent(f.path())
            p = subprocess.Popen([self.ccx_binary, "-i ", f.baseName()], shell=False, env=_env)
            self.ccx_stdout, self.ccx_stderr = p.communicate()
            os.putenv('OMP_NUM_THREADS', ont_backup)
            QtCore.QDir.setCurrent(cwd)
            return p.returncode
        return -1

    def setup_working_dir(self, working_dir=None):
        if working_dir is None:
            self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
            self.working_dir = self.fem_prefs.GetString("WorkingDir", "/tmp")
        else:
            self.working_dir = working_dir

    def setup_ccx(self, ccx_binary=None):
        if not ccx_binary:
            self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
            ccx_binary = self.fem_prefs.GetString("ccxBinaryPath", "")
        if not ccx_binary:
            from platform import system
            if system() == "Linux":
                ccx_binary = "ccx"
            elif system() == "Windows":
                ccx_binary = FreeCAD.getHomePath() + "bin/ccx.exe"
            else:
                ccx_binary = "ccx"
        self.ccx_binary = ccx_binary

    def load_results(self):
        import ccxFrdReader
        import os
        if os.path.isfile(self.base_name + ".frd"):
            ccxFrdReader.importFrd(self.base_name + ".frd", self.analysis)
            self.results_present = True
            for m in self.analysis.Member:
                if m.isDerivedFrom("Fem::FemResultObject"):
                    self.result_object = m
        else:
            self.results_present = False

    def use_results(self, results_name=None):
        for m in self.analysis.Member:
            if m.isDerivedFrom("Fem::FemResultObject") and m.Name == results_name:
                ro = m
        if not ro:
            print "{} doesn't exist".format(results_name)
        else:
            self.result_object = ro

    def run(self):
        ret_code = 0
        message = self.check_prerequisites()
        if not message:
            self.write_inp_file()
            from FreeCAD import Base
            progress_bar = Base.ProgressIndicator()
            progress_bar.start("Running CalculiX ccx...", 0)
            ret_code = self.start_ccx()
            self.finished.emit(ret_code)
            progress_bar.stop()
        else:
            print "Running analysis failed! " + message
        if ret_code:
            print "Analysis failed with exit code {}".format(ret_code)

    ## returns minimum, average and maximum value for provided result type
    #  @param self The python object self
    #  @result_type Type of FEM result, allowed U1, U2, U3, Uabs, Sabs and None
    def get_stats(self, result_type):
        stats = (0.0, 0.0, 0.0)
        for m in self.analysis.Member:
            if m.isDerivedFrom("Fem::FemResultObject"):
                match = {"U1": (m.Stats[0], m.Stats[1], m.Stats[2]),
                         "U2": (m.Stats[3], m.Stats[4], m.Stats[5]),
                         "U3": (m.Stats[6], m.Stats[7], m.Stats[8]),
                         "Uabs": (m.Stats[9], m.Stats[10], m.Stats[11]),
                         "Sabs": (m.Stats[12], m.Stats[13], m.Stats[14]),
                         "None": (0.0, 0.0, 0.0)}
                stats = match[result_type]
        return stats
