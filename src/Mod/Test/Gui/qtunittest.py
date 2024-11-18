# ***************************************************************************
# *   Copyright (c) 2006 Werner Mayer <werner.wm.mayer@gmx.de>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# ***************************************************************************/

# Qt Unit test module

__author__ = "Werner Mayer (werner.wm.mayer@gmx.de)"

import string
import traceback

# Cannot import this file in case Python is not prepared for Tk.
# Copy the needed classes instead.
# import unittestgui

import unittest
import sys
import time
import traceback
import string


##############################################################################
# GUI framework classes
# The classes BaseGUITestRunner, GUITestResult and RollbackImporter are
# taken from the PyUnit framework written by Steve Purcell.
# For more details see also http://pyunit.sourceforge.net.
##############################################################################


class BaseGUITestRunner:
    """Subclass this class to create a GUI TestRunner that uses a specific
    windowing toolkit. The class takes care of running tests in the correct
    manner, and making callbacks to the derived class to obtain information
    or signal that events have occurred.
    """

    def __init__(self, *args, **kwargs):
        self.currentResult = None
        self.running = 0
        self.__rollbackImporter = None
        self.initGUI(args, kwargs)

    def getSelectedTestName(self):
        "Override to return the name of the test selected to be run"
        pass

    def errorDialog(self, title, message):
        "Override to display an error arising from GUI usage"
        pass

    def runClicked(self):
        "To be called in response to user choosing to run a test"
        if self.running:
            return
        testName = self.getSelectedTestName()
        if not testName:
            self.errorDialog("Test name entry", "You must enter a test name")
            return
        if self.__rollbackImporter:
            self.__rollbackImporter.rollbackImports()
        self.__rollbackImporter = RollbackImporter()
        try:
            test = unittest.defaultTestLoader.loadTestsFromName(testName)
        except Exception:
            exc_type, exc_value, exc_tb = sys.exc_info()
            # apply(traceback.print_exception,sys.exc_info())
            traceback.print_exception(*sys.exc_info())
            self.errorDialog(
                "Unable to run test '%s'" % testName,
                "Error loading specified test: %s, %s" % (exc_type, exc_value),
            )
            return
        self.currentResult = GUITestResult(self)
        self.totalTests = test.countTestCases()
        self.running = 1
        self.notifyRunning()
        startTime = time.time()
        result = test.run(self.currentResult)
        stopTime = time.time()
        timeTaken = stopTime - startTime
        self.running = 0
        self.notifyStopped()

        expectedFails = unexpectedSuccesses = skipped = 0
        try:
            results = map(
                len, (result.expectedFailures, result.unexpectedSuccesses, result.skipped)
            )
        except AttributeError:
            pass
        else:
            expectedFails, unexpectedSuccesses, skipped = results

        self.stream.write(
            "\n{}\nRan {} tests in {:.3}s\n\n".format("-" * 70, result.testsRun, timeTaken)
        )
        infos = []
        if not result.wasSuccessful():
            failed, errored = len(result.failures), len(result.errors)
            if failed:
                infos.append("failures=%d" % failed)
            if errored:
                infos.append("errors=%d" % errored)
            if skipped:
                infos.append("skipped=%d" % skipped)
            if expectedFails:
                infos.append("expected failures=%d" % expectedFails)
            if unexpectedSuccesses:
                infos.append("unexpected successes=%d" % unexpectedSuccesses)

            self.stream.write("FAILED (%s)\n" % (", ".join(infos),))
        else:
            self.stream.write("OK\n")

    def stopClicked(self):
        "To be called in response to user stopping the running of a test"
        if self.currentResult:
            self.currentResult.stop()

    # Required callbacks

    def notifyRunning(self):
        "Override to set GUI in 'running' mode, enabling 'stop' button etc."
        pass

    def notifyStopped(self):
        "Override to set GUI in 'stopped' mode, enabling 'run' button etc."
        pass

    def notifyTestFailed(self, test, err):
        "Override to indicate that a test has just failed"
        pass

    def notifyTestErrored(self, test, err):
        "Override to indicate that a test has just errored"
        pass

    def notifyTestStarted(self, test):
        "Override to indicate that a test is about to run"
        pass

    def notifyTestFinished(self, test):
        """Override to indicate that a test has finished (it may already have
        failed or errored)"""
        pass


class GUITestResult(unittest.TestResult):
    """A TestResult that makes callbacks to its associated GUI TestRunner.
    Used by BaseGUITestRunner. Need not be created directly.
    """

    def __init__(self, callback):
        unittest.TestResult.__init__(self)
        self.callback = callback

    def addError(self, test, err):
        unittest.TestResult.addError(self, test, err)
        self.callback.notifyTestErrored(test, err)

    def addFailure(self, test, err):
        unittest.TestResult.addFailure(self, test, err)
        self.callback.notifyTestFailed(test, err)

    def stopTest(self, test):
        unittest.TestResult.stopTest(self, test)
        self.callback.notifyTestFinished(test)

    def startTest(self, test):
        unittest.TestResult.startTest(self, test)
        self.callback.notifyTestStarted(test)


class RollbackImporter:
    """This tricky little class is used to make sure that modules under test
    will be reloaded the next time they are imported.
    """

    def __init__(self):
        self.previousModules = sys.modules.copy()

    def rollbackImports(self):
        for modname in sys.modules.keys():
            if modname not in self.previousModules:
                # Force reload when modname next imported
                del sys.modules[modname]


# ---------------------------------------------------------------------------
# Subclass of BaseGUITestRunner using Qt dialog
# ---------------------------------------------------------------------------


class QtTestRunner(BaseGUITestRunner):
    """An implementation of BaseGUITestRunner using Qt."""

    def initGUI(self, root, initialTestName):
        """Set up the GUI inside the given root window. The test name entry
        field will be pre-filled with the given initialTestName.
        """
        self.root = root
        # Set up values that will be tied to widgets
        import QtUnitGui

        self.gui = QtUnitGui.UnitTest()
        self.gui.setStatusText("Idle")
        self.runCountVar = 0
        self.failCountVar = 0
        self.errorCountVar = 0
        self.remainingCountVar = 0
        self.stream = sys.stdout

    def getSelectedTestName(self):
        return self.gui.getUnitTest()

    def errorDialog(self, title, message):
        return self.gui.errorDialog(title, message)

    def notifyRunning(self):
        self.runCountVar = 0
        self.gui.setRunCount(0)
        self.failCountVar = 0
        self.gui.setFailCount(0)
        self.errorCountVar = 0
        self.gui.setErrorCount(0)
        self.remainingCountVar = self.totalTests
        self.gui.setRemainCount(self.totalTests)
        self.errorInfo = []
        self.gui.clearErrorList()
        self.gui.setProgressFraction(0.0)
        self.gui.updateGUI()

    def notifyStopped(self):
        self.gui.setStatusText("Idle")

    def notifyTestStarted(self, test):
        self.gui.setStatusText(str(test))
        self.gui.updateGUI()

    def notifyTestFailed(self, test, err):
        self.failCountVar = self.failCountVar + 1
        self.gui.setFailCount(self.failCountVar)
        # tracebackLines = apply(traceback.format_exception, err + (10,))
        tracebackLines = traceback.format_exception(*err + (10,))
        # tracebackText = string.join(tracebackLines,'')
        tracebackText = "".join(tracebackLines)
        self.gui.insertError("Failure: %s" % test, tracebackText)
        self.errorInfo.append((test, err))
        self.stream.write("FAILED: {}\n{}\n".format(test, tracebackText))

    def notifyTestErrored(self, test, err):
        self.errorCountVar = self.errorCountVar + 1
        self.gui.setErrorCount(self.errorCountVar)
        # tracebackLines = apply(traceback.format_exception, err + (10,))
        tracebackLines = traceback.format_exception(*err + (10,))
        # tracebackText = string.join(tracebackLines,'')
        tracebackText = "".join(tracebackLines)
        self.gui.insertError("Error: %s" % test, tracebackText)
        self.errorInfo.append((test, err))
        self.stream.write("{}\nERROR: {}\n{}\n{}\n".format("=" * 70, test, "-" * 70, tracebackText))

    def notifyTestFinished(self, test):
        self.remainingCountVar = self.remainingCountVar - 1
        self.gui.setRemainCount(self.remainingCountVar)
        self.runCountVar = self.runCountVar + 1
        self.gui.setRunCount(self.runCountVar)
        fractionDone = float(self.runCountVar) / float(self.totalTests)
        fillColor = len(self.errorInfo) and "red" or "green"
        self.gui.setProgressFraction(fractionDone, fillColor)
