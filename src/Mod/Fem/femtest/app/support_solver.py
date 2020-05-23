# ***************************************************************************
# *   Copyright (c) 2019 Markus Hovorka <m.hovorka@live.de>                 *
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

""" Support for textbook simulation tests.

The TestCase derived base class :py:class:`SolverTest` should make it very easy
to do redo textbook simulations in FreeCAD and compare the results of the
analytic calculation to the approximation of the integrated solvers. See the
documentation for :py:class:`SolverTest` for more information.

Textbook simulations refere to simple simulations whose results can be
calculated on paper. The advantage of running such simulations when testing is
that we know what the results are supposed to be. After running the simulation
with an integrated solver like Elmer or Calculix we can compare the results
with the results calculated on paper.

Also contains the exception :py:exception:`TestSetupError` raised by some
methods of :py:class:`SolverTest`.
"""


__title__ = "Support for solver tests"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


# import unittest
import os.path

import FreeCAD
import femsolver.run
from femtools import femutils
import AppTestSupport


class TestSetupError(Exception):
    """ Error when preparing the textbook simulation. """
    pass


class SolverTest(AppTestSupport.BaseTest):
    """ Base class for textbook simulation TestCases.

    This class contains facilities for it's subclasses to put together a
    simulation, run it and after it finished assert the results of the
    simulation. It is derived from unittest.TestCase so the subclass has all
    the usual functionality of a python unittest at it's disposal as well.

    For convenience preparing the simulation is split into two parts: the
    solver generic and solver specific part. The solver generic part should be
    put into a analysis in a .FCStd inside
    :file:`Mod/Fem/femtest/data/problems/` for example
    :file:`data/problems/cantilever_uniform_load.FCStd`. This part is the same
    regardless of which solver is used to solve the problem an can be loaded by
    calling :py:func:`femtest.app.support_solver.loadSimulation`. For our
    example before the call would be
    `self.loadSimulation("cantilever_end_load")`.

    The solver generic FCStd file is supposed to contain a document with
    exactly one analysis containing all meshes, materials and constraints that
    define the example.

    The solver specific part is than added dynamically via python code. The
    solver specific part includes at least the solver object but may also
    include equations or other objects necessary to make the simulation work
    for the solver under test. For this purpose use
    :py:func:`femtest.app..support_solver.addSolver` and
    :py:func:`femtest.app.support_solver.addEquation` for solver and equation
    objects and :py:func:`femtest.app.support_solver.addMember` for all other
    analysis members.

    To run the simulation use
    :py:func:`femtest.app.support_solver.runSimulation`.  It automatically uses
    the right alanysis and solver. It blocks till the simulation is finished.

    When the simulation has finished use
    :py:func:`femtest.app.support_solver.assertDataAtPoint` to check whether
    the solver solved the simulation correctely.

    No other measures are necessary than the ones outlined above. The handling
    of the document, the analysis and detection of the right solver and result
    is done automatically.
    """

    TEST_DIR = os.path.join(
        FreeCAD.getHomePath(),
        "Mod/Fem/femtest/data/problems")
    EXTENSION = ".FCStd"

    def setUp(self):
        self.doc = None
        self.solver = None
        self.pipe = None

    def tearDown(self):
        if self.doc is not None:
            FreeCAD.closeDocument(self.doc.Name)

    def addMember(self, proxy):
        """ Add member to the simulations analysis.

        Create the object specified by *proxy* and add it to the analysis of
        the simulation. A analysis must not be created or specified, it is
        managed automatically. To add solver or equation objects use
        :py:func:`femtest.app.support_solver.addSolver` and
        :py:func:`femtest.app.support_solver.addEquation` instead of this method.

        :param proxy:
            The python proxy object for the DocumentObject that shall become a
            new member of the documentation. It must be supported by the fem
            object creation mechanism (must have a BaseType member).

        :raises TestSetupError:
            In case there are no or multiple analysis in the document. This
            usually means that the solver generic .FCStd doesn't contain a
            analysis or the loading process failed.

        :returns: The newly created DocumentObject.
        """
        if self.doc is None:
            raise TestSetupError("no document loaded")
        analyses = self.doc.findObjects("Fem::FemAnalysis")
        if len(analyses) == 0:
            raise TestSetupError("no Analysis found")
        if len(analyses) > 1:
            raise TestSetupError("multiple Analyses found")
        member = femutils.createObject(self.doc, "", proxy)
        analyses[0].addObject(member)
        return member

    def addSolver(self, proxy):
        """ Add solver to the simulations analysis.

        Same as :py:func:`femtest.app.support_solver.addMember` but tells the
        system that the newly created object is the one and only solver of the
        simulation.

        :param proxy:
            The python proxy object for the solver DocumentObject that shall
            become a new member of the documentation. It must be supported by
            the fem object creation mechanism (must have a BaseType member).

        :returns: The newly created solver DocumentObject.
        """
        self.solver = self.addMember(proxy)

    def addEquation(self, eqId):
        """ Add equation to the solver of the simulation.

        :param eqId:
            The string id of the equation to add to the solver. Equation ids
            are interpreted by the solver. The same id should be interpreted
            the same by all solver. If not this is a bug.

        :raises TestSetupError: If no document is loaded or no solver was added
        jet. Missing the document can happen if the solver generic FCStd was
        not loaded jet or loading failed for some reason or a solv.
        """
        if self.doc is None:
            raise TestSetupError("no document loaded")
        if self.solver is None:
            raise TestSetupError("no solver found")
        self.solver.Proxy.addEquation(self.solver, eqId)

    def loadSimulation(self, name):
        """ Load a FCStd containing a analysis.

        Load the simulation *name* from :file:`Mod/Fem/femtest/data/problems/`.
        The file must exist in this directory in the following way:
        :file:`data/problems/<name>.FCStd`. The file must contain at least a
        analysis object but should define all solver generic parts of the
        simulation which usually means everything that defines the textbook
        example.

        :param name:
            The file name of the .FCStd without the path and the FCStd
            extension as a string.
        """
        file_name = name + self.EXTENSION
        path = os.path.join(self.TEST_DIR, file_name)
        self.doc = FreeCAD.open(path)

    def runSimulation(self):
        """ Run the simulation under test.

        Run the simulation previously but together with the facilities of this
        class. At least :py:func:`femtest.app.support_solver.addSolver` must have
        been called before calling this function. This method blocks till the
        solver has finished and the results where loaded.
        """
        self.doc.recompute()
        femsolver.run.run_fem_solver(self.solver)
        self._convertResults()
        self._loadResults()

    def assertDataAtPoint(self, name, point, expect, rel_tol=1e-3, abs_tol=0.0):
        """ Check if simulation results match the expected values.

        Works just like all other assert methods of the unittest module.
        Automatically uses the right result object.
        :py:func:`femtest.app.test_support.runSimulation` must have been called
        before asserting the results.

        :param name:
            The name of the VTK field that shall be checked for example
            "Displacement" as a string.
        :param point:
            A tuple with three numeric values defining at which point in space
            to compare the results with the given values.
        :param expect:
            The expected value at *point*.
        :param rel_tol:
            The permitted relative deviation of the result from the specified
            expected value.
        :param abs_tol:
            The permitted absolute deviation of the result from the specified
            expected value.

        :raises TestSetupError:
            If no or more that one results object where found. This can happen
            if the simulation didn't complete properly or it wasn't executed at
            all.
        """
        dataAtPoint = self.doc.addObject(
            "Fem::FemPostDataAtPointFilter")
        self._addFilter(dataAtPoint)
        dataAtPoint.Center = point
        self.doc.recompute()
        dataAtPoint.FieldName = name
        values = dataAtPoint.PointData
        if not values:
            raise AssertionError("no data points found")
        if not all(self._isclose(x, expect, rel_tol, abs_tol) for x in values):
            self.doc.removeObject(dataAtPoint.Name)
            msg = "{} not close enough to {}".format(values, expect)
            raise AssertionError(msg)
        self.doc.removeObject(dataAtPoint.Name)

    def _loadResults(self):
        self.pipe = None
        pipelines = self.doc.findObjects("Fem::FemPostPipeline")
        if len(pipelines) == 0:
            raise TestSetupError("no Result found")
        if len(pipelines) > 1:
            raise TestSetupError("multiple Results found")
        self.pipe = pipelines[0]

    def _convertResults(self):
        results = self.doc.findObjects("Fem::FemResultObject")
        for r in results:
            obj = self.doc.addObject("Fem::FemPostPipeline")
            obj.load(r)

    def _isclose(self, a, b, rel_tol=1e-09, abs_tol=0.0):
        return abs(a - b) <= max(rel_tol * max(abs(a), abs(b)), abs_tol)

    def _addFilter(self, f):
        self.pipe.Filter += [f]
