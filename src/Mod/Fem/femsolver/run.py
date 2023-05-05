# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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
""" Execute Solver and obtain Reports and Results.

Integral part of the Solver Framework which contains components responsible for
executing the solver in the background. Also provides an asynchronous
communication system with the solver running in the background. The purpose of
this module is to be as generic as possible. It can execute every solver
supported by the fem workbench. The threading and communication support is
mainly implemented by the :mod:`femsolver.task` and :mod:`femsolver.signal`
modules.
"""

__title__ = "FreeCAD FEM solver run"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import os
import os.path
import shutil
import tempfile
# import threading  # not used ATM

import FreeCAD as App

from . import settings
from . import signal
from . import task
from femtools import femutils
from femtools import membertools
from femtools.errors import DirectoryDoesNotExistError
from femtools.errors import MustSaveError

if App.GuiUp:
    from PySide import QtGui
    import FreeCADGui


CHECK = 0
PREPARE = 1
SOLVE = 2
RESULTS = 3
DONE = 4


_machines = {}
_dirTypes = {}


def run_fem_solver(solver, working_dir=None):
    """ Execute *solver* of the solver framework.

    Uses :meth:`getMachine <femsolver.solverbase.Proxy.getMachine>` to obtain a
    :class:`Machine` instance of the solver. It than executes the Machine with
    using the ``RESULTS`` target (see :class:`Machine` for infos about
    different targets). This method is blocking, it waits for the solver to
    finished before returning. Be aware of :class:`Machine` caching when using
    the function.

    :param solver:
        A document object which must be a framework compliant solver. This means
        that it should be derived from the document object provided by
        :mod:`femsolver.solverbase` and implement all required methods
        correctly. Of particular importance is :meth:`getMachine
        <femsolver.solverbase.Proxy.getMachine>` as it is used by this method
        the get the :class:`Machine` used to execute the solver.

    :param working_dir:
        If specified it overwrites the automatic and user configurable working
        directory management of the Solver framework. Should always be a
        absolute path because the location of the binary is not consistent
        among platforms. If ``None`` the automatic working directory management
        is used.

    :note:
        There is some legacy code to execute the old Calculix solver
        (pre-framework) which behaives differently because it does not
        use a :class:`Machine`.
    """

    if solver.Proxy.Type == "Fem::SolverCcxTools":
        from femtools.ccxtools import CcxTools as ccx
        App.Console.PrintMessage("Run of CalxuliX ccx tools solver started.\n")
        fea = ccx(solver)
        fea.reset_mesh_purge_results_checked()
        if working_dir is None:
            fea.run()  # standard, no working dir is given in solver
        else:
            # not the standard way
            fea.update_objects()
            fea.setup_working_dir(working_dir)
            fea.setup_ccx()
            message = fea.check_prerequisites()
            if not message:
                fea.write_inp_file()
                fea.ccx_run()
                fea.load_results()
            else:
                App.Console.PrintError("Houston, we have a problem...!\n{}\n".format(message))
        App.Console.PrintMessage("Run of CalxuliX ccx tools solver finished.\n")
    else:
        # App.Console.PrintMessage("Frame work solver!\n")
        try:
            if working_dir is not None:
                machine = getMachine(solver, working_dir)
            else:
                machine = getMachine(solver)
        except MustSaveError:
            error_message = (
                "Please save the file before executing the solver. "
                "This must be done because the location of the working "
                "directory is set to \"Beside *.FCStd File\"."
            )
            App.Console.PrintError(error_message + "\n")
            if App.GuiUp:
                QtGui.QMessageBox.critical(
                    FreeCADGui.getMainWindow(),
                    "Can't start Solver",
                    error_message
                )
            return
        except DirectoryDoesNotExistError:
            error_message = "Selected working directory doesn't exist."
            App.Console.PrintError(error_message + "\n")
            if App.GuiUp:
                QtGui.QMessageBox.critical(
                    FreeCADGui.getMainWindow(),
                    "Can't start Solver",
                    error_message
                )
            return
        if not machine.running:
            machine.reset()
            machine.target = RESULTS
            machine.start()
            machine.join()  # wait for the machine to finish.
            if machine.failed is True:
                App.Console.PrintError("Machine failed to run.\n")
                from .report import displayLog
                displayLog(machine.report)
                if App.GuiUp:
                    error_message = (
                        "Failed to run. Please try again after all "
                        "of the following errors are resolved."
                    )
                    from .report import display
                    display(machine.report, "Run Report", error_message)


def getMachine(solver, path=None):
    """ Get or create :class:`Machine` using caching mechanism.

    :param solver:
        A document object which must be a framework compliant solver. This means
        that it should be derived from the document object provided by
        :mod:`femsolver.solverbase` and implement all required methods
        correctly. Of particular importance is :meth:`getMachine
        <femsolver.solverbase.Proxy.getMachine>` as it is used by this method
        to create a new :class:`Machine` on cache miss.

    :param path:
        A valid filesystem path which shall be associetad with the machine.
    """
    # print(path)
    _DocObserver.attach()
    m = _machines.get(solver)
    if m is None or not _isPathValid(m, path):
        m = _createMachine(solver, path, testmode=False)
        # print(m.__dir__())  # document these attributes somewhere
        # print(m.directory)
    return m


def _isPathValid(m, path):
    t = _dirTypes.get(m.directory)  # setting default None
    setting = settings.get_dir_setting()
    if path is not None:
        return t is None and m.directory == path
    if setting == settings.DirSetting.BESIDE:
        if t == settings.DirSetting.BESIDE:
            base = os.path.split(m.directory.rstrip("/"))[0]
            return base == _getBesideBase(m.solver)
        return False
    if setting == settings.DirSetting.TEMPORARY:
        return t == settings.DirSetting.TEMPORARY
    if setting == settings.DirSetting.CUSTOM:
        if t == settings.DirSetting.CUSTOM:
            firstBase = os.path.split(m.directory.rstrip("/"))[0]
            customBase = os.path.split(firstBase)[0]
            return customBase == _getCustomBase(m.solver)
        return False


def _createMachine(solver, path, testmode):
    global _dirTypes
    setting = settings.get_dir_setting()
    if path is not None:
        _dirTypes[path] = None
    elif setting == settings.DirSetting.BESIDE:
        path = _getBesideDir(solver)
        _dirTypes[path] = settings.DirSetting.BESIDE
    elif setting == settings.DirSetting.TEMPORARY:
        path = _getTempDir(solver)
        _dirTypes[path] = settings.DirSetting.TEMPORARY
    elif setting == settings.DirSetting.CUSTOM:
        path = _getCustomDir(solver)
        _dirTypes[path] = settings.DirSetting.CUSTOM
    m = solver.Proxy.createMachine(solver, path, testmode)
    oldMachine = _machines.get(solver)
    if oldMachine is not None and _dirTypes.get(oldMachine.directory) is not None:
        del _dirTypes[oldMachine.directory]
    _machines[solver] = m
    return m


def _getTempDir(solver):
    return tempfile.mkdtemp(prefix="fem")


def _getBesideDir(solver):
    base = _getBesideBase(solver)
    specificPath = os.path.join(base, solver.Label)
    specificPath = _getUniquePath(specificPath)
    if not os.path.isdir(specificPath):
        os.makedirs(specificPath)
    return specificPath


def _getBesideBase(solver):
    path = os.path.splitext(solver.Document.FileName)[0]
    # doc=App.newDocument()
    # doc.FileName
    # the above returns an empty string in FreeCAD 0.19
    # https://forum.freecad.org/viewtopic.php?f=10&t=48842
    if path == "":
        error_message = (
            "Please save the file before executing the solver. "
            "This must be done because the location of the working "
            "directory is set to \"Beside *.FCStd File\"."
        )
        App.Console.PrintError(error_message + "\n")
        if App.GuiUp:
            QtGui.QMessageBox.critical(
                FreeCADGui.getMainWindow(),
                "Can't start Solver",
                error_message
            )
        raise MustSaveError()
        # TODO may be do not abort but use a temporary directory
    return path


def _getCustomDir(solver):
    base = _getCustomBase(solver)
    specificPath = os.path.join(
        base, solver.Document.Name, solver.Label)
    specificPath = _getUniquePath(specificPath)
    if not os.path.isdir(specificPath):
        os.makedirs(specificPath)
    return specificPath


def _getCustomBase(solver):
    path = settings.get_custom_dir()
    if not os.path.isdir(path):
        error_message = "Selected working directory doesn't exist."
        App.Console.PrintError(error_message + "\n")
        if App.GuiUp:
            QtGui.QMessageBox.critical(
                FreeCADGui.getMainWindow(),
                "Can't start Solver",
                error_message
            )
        raise DirectoryDoesNotExistError("Invalid path")
    return path


def _getUniquePath(path):
    postfix = 1
    if path in _dirTypes:
        path += "_%03d" % postfix
    while path in _dirTypes:
        postfix += 1
        path = path[:-4] + "_%03d" % postfix
    return path


class BaseTask(task.Thread):

    def __init__(self):
        super(BaseTask, self).__init__()
        self.solver = None
        self.directory = None
        self.testmode = None

    @property
    def analysis(self):
        return self.solver.getParentGroup()


class Machine(BaseTask):

    def __init__(
            self, solver, directory, check,
            prepare, solve, results, testmode):
        super(Machine, self).__init__()
        self.solver = solver
        self.directory = directory
        self.signalState = set()
        self.check = check
        self.prepare = prepare
        self.solve = solve
        self.results = results
        self.target = RESULTS
        self._state = CHECK
        self._pendingState = None
        self._isReset = False
        self.testmode = testmode

    @property
    def state(self):
        return self._state

    def run(self):
        self._confTasks()
        self._isReset = False
        self._pendingState = self.state
        while (
            not self.aborted
            and not self.failed
            and self._pendingState <= self.target
        ):
            task = self._getTask(self._pendingState)
            self._runTask(task)
            self.report.extend(task.report)
            if task.failed:
                self.fail()
            elif task.aborted:
                self.abort()
            else:
                self._pendingState += 1
        self._applyPending()

    def reset(self, newState=CHECK):
        state = (self.state
                 if self._pendingState is None
                 else self._pendingState)
        if newState < state:
            self._isReset = True
            self._state = newState
            signal.notify(self.signalState)

    def _confTasks(self):
        tasks = [
            self.check,
            self.prepare,
            self.solve,
            self.results
        ]
        for t in tasks:
            t.solver = self.solver
            t.directory = self.directory
            t.testmode = self.testmode

    def _applyPending(self):
        if not self._isReset:
            self._state = self._pendingState
            signal.notify(self.signalState)
        self._isReset = False
        self._pendingState = None

    def _runTask(self, task):

        def statusProxy(line):
            self.pushStatus(line)

        def killer():
            task.abort()
        self.signalAbort.add(killer)
        task.signalStatus.add(statusProxy)
        task.start()
        task.join()
        self.signalAbort.remove(killer)
        task.signalStatus.remove(statusProxy)

    def _getTask(self, state):
        if state == CHECK:
            return self.check
        elif state == PREPARE:
            return self.prepare
        elif state == SOLVE:
            return self.solve
        elif state == RESULTS:
            return self.results
        return None


class Check(BaseTask):

    def get_several_member(self, t):
        return membertools.get_several_member(self.analysis, t)

    def check_mesh_exists(self):
        meshes = self.get_several_member("Fem::FemMeshObject")
        if len(meshes) == 0:
            self.report.error("Missing a mesh object.")
            self.fail()
            return False
        elif len(meshes) > 1:
            self.report.error(
                "Too many meshes. "
                "More than one mesh is not supported."
            )
            self.fail()
            return False
        return True

    def check_material_exists(self):
        objs = self.get_several_member("App::MaterialObjectPython")
        if len(objs) == 0:
            self.report.error(
                "Missing a material object. "
                "At least one material is required."
            )
            self.fail()
            return False
        return True

    def check_material_single(self):
        objs = self.get_several_member("App::MaterialObjectPython")
        if len(objs) > 1:
            self.report.error("Only one Material is supported for this solver.")
            self.fail()
            return False
        return True

    def check_geos_beamsection_no(self):
        objs = self.get_several_member("Fem::ElementGeometry1D")
        if len(objs) > 0:
            self.report.error("Beamsections are not supported for this solver.")
            self.fail()
            return False
        return True

    def check_geos_beamsection_single(self):
        objs = self.get_several_member("Fem::ElementGeometry1D")
        if len(objs) > 1:
            self.report.error("Only one beamsection is supported for this solver.")
            self.fail()
            return False
        return True

    def check_geos_shellthickness_no(self):
        objs = self.get_several_member("Fem::ElementGeometry2D")
        if len(objs) > 0:
            self.report.error("Shellsections are not supported for this solver.")
            self.fail()
            return False
        return True

    def check_geos_shellthickness_single(self):
        objs = self.get_several_member("Fem::ElementGeometry2D")
        if len(objs) > 1:
            self.report.error("Only one shellthickness is supported for this solver.")
            self.fail()
            return False
        return True

    def check_geos_beamsection_and_shellthickness(self):
        beamsec_obj = self.get_several_member("Fem::ElementGeometry1D")
        shellth_obj = self.get_several_member("Fem::ElementGeometry2D")
        if len(beamsec_obj) > 0 and len(shellth_obj) > 0:
            self.report.error(
                "Either beamsection or shellthickness objects are "
                "supported for this solver, but not both in one analysis."
            )
            self.fail()
            return False
        return True

    def checkSupported(self, allSupported):
        for m in self.analysis.Group:
            if femutils.is_of_type(m, "Fem::Constraint"):
                supported = False
                for sc in allSupported:
                    if femutils.is_of_type(m, *sc):
                        supported = True
                if not supported:
                    self.report.warning(
                        "Ignored unsupported constraint: {}"
                        .format(m.Label)
                    )
        return True


class Solve(BaseTask):

    def _observeSolver(self, process):
        output = ""
        line = femutils.pydecode(process.stdout.readline())
        self.pushStatus(line)
        output += line
        line = femutils.pydecode(process.stdout.readline())
        while line:
            line = "\n%s" % line.rstrip()
            self.pushStatus(line)
            output += line
            line = femutils.pydecode(process.stdout.readline())
        return output


class Prepare(BaseTask):
    pass


class Results(BaseTask):
    pass


class _DocObserver(object):

    _instance = None
    _WHITELIST = [
        "Fem::Constraint",
        "App::MaterialObject",
        "Fem::FemMeshObject",
    ]
    _BLACKLIST_PROPS = [
        "Label",
        "ElmerOutput",
        "ElmerResult"
    ]

    def __init__(self):
        self._saved = {}
        for doc in iter(App.listDocuments().values()):
            for obj in doc.Objects:
                if obj.isDerivedFrom("Fem::FemAnalysis"):
                    self._saved[obj] = obj.Group

    @classmethod
    def attach(cls):
        if cls._instance is None:
            cls._instance = cls()
            App.addDocumentObserver(cls._instance)

    def slotDeletedObject(self, obj):
        self._checkModel(obj)
        if obj in _machines:
            self._deleteMachine(obj)

    def slotChangedObject(self, obj, prop):
        if prop not in self._BLACKLIST_PROPS:
            self._checkAnalysis(obj)
            self._checkEquation(obj)
            self._checkSolver(obj)
            self._checkModel(obj)

    def slotDeletedDocument(self, doc):
        for obj in doc.Objects:
            if obj in _machines:
                self._deleteMachine(obj)

    def _deleteMachine(self, obj):
        m = _machines[obj]
        t = _dirTypes[m.directory]
        m.abort()
        if t == settings.DirSetting.TEMPORARY:
            shutil.rmtree(m.directory)
        del _machines[obj]
        del _dirTypes[m.directory]

    def _checkEquation(self, obj):
        for o in obj.Document.Objects:
            if (
                femutils.is_derived_from(o, "Fem::FemSolverObject")
                and hasattr(o, "Group")
                and obj in o.Group
            ):
                if o in _machines:
                    _machines[o].reset()

    def _checkSolver(self, obj):
        analysis = obj.getParentGroup()
        for m in iter(_machines.values()):
            if analysis == m.analysis and obj == m.solver:
                m.reset()

    def _checkAnalysis(self, obj):
        if femutils.is_derived_from(obj, "Fem::FemAnalysis"):
            deltaObjs = self._getAdded(obj)
            if deltaObjs:
                reset = False
                for o in deltaObjs:
                    if self._partOfModel(o):
                        reset = True
                if reset:
                    self._resetAll(obj)

    def _checkModel(self, obj):
        if self._partOfModel(obj):
            analysis = obj.getParentGroup()
            if analysis is not None:
                self._resetAll(analysis)

    def _getAdded(self, analysis):
        if analysis not in self._saved:
            self._saved[analysis] = []
        delta = set(analysis.Group) - set(self._saved[analysis])
        self._saved[analysis] = analysis.Group
        return delta

    def _resetAll(self, analysis):
        for m in iter(_machines.values()):
            if analysis == m.analysis:
                m.reset()

    def _partOfModel(self, obj):
        for t in self._WHITELIST:
            if femutils.is_derived_from(obj, t):
                return True
        return False
