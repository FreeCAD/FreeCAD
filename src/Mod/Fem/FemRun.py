# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
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


__title__ = "FemSolve"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


import os
import os.path
import tempfile
import threading
import shutil

import FreeCAD as App
import FemSettings
import FemMisc
import FemSignal
import FemTask


CHECK = 0
PREPARE = 1
SOLVE = 2
RESULTS = 3
DONE = 4


_machines = {}
_dirTypes = {}


def getMachine(solver, path=None):
    _DocObserver.attach()
    m = _machines.get(solver)
    if m is None or not _isPathValid(m, path):
        m = _createMachine(solver, path)
    return m


def _isPathValid(m, path):
    t = _dirTypes[m.directory]
    setting = FemSettings.getDirSetting()
    if path is not None:
        return t is None and m.directory == path
    if setting == FemSettings.BESIDE:
        if t == FemSettings.BESIDE:
            base = os.path.split(m.directory.rstrip("/"))[0]
            return base == _getBesideBase(m.solver)
        return False
    if setting == FemSettings.TEMPORARY:
        return t == FemSettings.TEMPORARY
    if setting == FemSettings.CUSTOM:
        if t == FemSettings.CUSTOM:
            firstBase = os.path.split(m.directory.rstrip("/"))[0]
            customBase = os.path.split(firstBase)[0]
            return customBase == _getCustomBase(m.solver)
        return False


def _createMachine(solver, path):
    global _dirTypes
    setting = FemSettings.getDirSetting()
    if path is not None:
        _dirTypes[path] = None
    elif setting == FemSettings.BESIDE:
        path = _getBesideDir(solver)
        _dirTypes[path] = FemSettings.BESIDE
    elif setting == FemSettings.TEMPORARY:
        path = _getTempDir(solver)
        _dirTypes[path] = FemSettings.TEMPORARY
    elif setting == FemSettings.CUSTOM:
        path = _getCustomDir(solver)
        _dirTypes[path] = FemSettings.CUSTOM
    m = solver.Proxy.createMachine(solver, path)
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
    fcstdPath = solver.Document.FileName
    if fcstdPath == "":
        raise MustSaveError()
    return os.path.splitext(fcstdPath)[0]


def _getCustomDir(solver):
    base = _getCustomBase(solver)
    specificPath = os.path.join(
        base, solver.Document.Name, solver.Label)
    specificPath = _getUniquePath(specificPath)
    if not os.path.isdir(specificPath):
        os.makedirs(specificPath)
    return specificPath


def _getCustomBase(solver):
    path = FemSettings.getCustomDir()
    if not os.path.isdir(path):
        raise DirectoryDoesNotExist("Invalid path")
    return path


def _getUniquePath(path):
    postfix = 1
    if path in _dirTypes:
        path += "_%03d" % postfix
    while path in _dirTypes:
        postfix += 1
        path = path[:-4] + "_%03d" % postfix
    return path


class BaseTask(FemTask.Thread):

    def __init__(self):
        super(BaseTask, self).__init__()
        self.solver = None
        self.directory = None

    @property
    def analysis(self):
        return FemMisc.findAnalysisOfMember(self.solver)


class Machine(BaseTask):

    def __init__(
            self, solver, directory, check,
            prepare, solve, results):
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

    @property
    def state(self):
        return self._state

    def run(self):
        self._confTasks()
        self._isReset = False
        self._pendingState = self.state
        while (not self.aborted and not self.failed
                and self._pendingState <= self.target):
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
            FemSignal.notify(self.signalState)

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

    def _applyPending(self):
        if not self._isReset:
            self._state = self._pendingState
            FemSignal.notify(self.signalState)
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

    def checkMesh(self):
        meshes = FemMisc.getMember(
            self.analysis, "Fem::FemMeshObject")
        if len(meshes) == 0:
            self.report.error("Missing a mesh object.")
            self.fail()
            return False
        elif len(meshes) > 1:
            self.report.error(
                "Too many meshes. "
                "More than one mesh is not supported.")
            self.fail()
            return False
        return True

    def checkMaterial(self):
        matObjs = FemMisc.getMember(
            self.analysis, "App::MaterialObjectPython")
        if len(matObjs) == 0:
            self.report.error(
                "No material object found. "
                "At least one material is required.")
            self.fail()
            return False
        return True

    def checkSupported(self, allSupported):
        for m in self.analysis.Member:
            if FemMisc.isOfType(m, "Fem::Constraint"):
                supported = False
                for sc in allSupported:
                    if FemMisc.isOfType(m, *sc):
                        supported = True
                if not supported:
                    self.report.warning(
                        "Ignored unsupported constraint: %s" % m.Label)
        return True


class Solve(BaseTask):
    pass


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
        for doc in App.listDocuments().itervalues():
            for obj in doc.Objects:
                if obj.isDerivedFrom("Fem::FemAnalysis"):
                    self._saved[obj] = obj.Member

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

        def delegate():
            m.join()
            if t == FemSettings.TEMPORARY:
                shutil.rmtree(m.directory)
            del _dirTypes[m.directory]
            del _machines[obj]
        m.abort()
        thread = threading.Thread(target=delegate)
        thread.daemon = False
        thread.start()

    def _checkEquation(self, obj):
        for o in obj.Document.Objects:
            if (FemMisc.isDerivedFrom(o, "Fem::FemSolverObject")
                    and hasattr(o, "Group") and obj in o.Group):
                if o in _machines:
                    _machines[o].reset()

    def _checkSolver(self, obj):
        analysis = FemMisc.findAnalysisOfMember(obj)
        for m in _machines.itervalues():
            if analysis == m.analysis and obj == m.solver:
                m.reset()

    def _checkAnalysis(self, obj):
        if FemMisc.isDerivedFrom(obj, "Fem::FemAnalysis"):
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
            analysis = FemMisc.findAnalysisOfMember(obj)
            if analysis is not None:
                self._resetAll(analysis)

    def _getAdded(self, analysis):
        if analysis not in self._saved:
            self._saved[analysis] = []
        delta = set(analysis.Member) - set(self._saved[analysis])
        self._saved[analysis] = analysis.Member
        return delta

    def _resetAll(self, analysis):
        for m in _machines.itervalues():
            if analysis == m.analysis:
                m.reset()

    def _partOfModel(self, obj):
        for t in self._WHITELIST:
            if FemMisc.isDerivedFrom(obj, t):
                return True
        return False


class MustSaveError(Exception):
    pass


class DirectoryDoesNotExist(Exception):
    pass
