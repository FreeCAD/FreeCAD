#***************************************************************************
#*   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/
""" Script to run FreeCADs python unit tests.

This script is designed to run in internal mode inside of FreeCAD. It purpose
is to execute FreeCADs unit tests easily accessible directly from the command
line and it is also capable of collecting and reporting test coverage
information. This script is necessary because FreeCADs unit tests must be
executed inside the embedded python iterpreter. The standard way of invoking
python unit tests directly via the unittest module cannot provide this.

This script is executed after FreeCAD has started and than executes the tests.
To use the script use the -t and optionally the --cov arguments when invoking
the FreeCAD or FreeCADGui binary. The -t <test> option specifies the unit
test(s) to execute. It can be a module, test class, test function or a
specifically prepared package representing an abitrary collection of tests.

To get a test coverage report use the --cov [<source>] option. The optional
[<source>] option can be used to specify which source files shall be measured
and must be a valid python path to a package or a module if used. The
previously mentioned specifically prepared packages define their sources
themselfs so the <source> option doesn't have to be used to optain a clean
report. With that said it still can be used to override the source definition
of the package.

A package defining a test collection must define two methods:

.. code-block:: python

   def load_tests(loader, tests, pattern):
       return <unittest.TestSuite>

   def cover_tests():
       return <[string]>

The first method (load_tests) returns a test suite in accordence to the
load_test protocoll of the unittest module. cover_tests is a FreeCAD specific
extension defining which modules or packages the collection of tests is
supposted to cover/test. This information is used to produce a meaningful
test coverage report.
"""

import sys
import importlib

try:
    import coverage
except ImportError:
    coverage = None

import FreeCAD
import TestApp


def configureCoverage(cov):
    """ Set configuration options for coverage package.

    Setting configuration options at runtime via the API is only possible sice
    version 4.0 of coverage. This method sets the option for newer versions and
    does nothing for older versions which don't support the configuration
    interface.
    """
    if coverage.__version__[0] >= "4":
        cov.set_option(
                "run:disable_warnings",
                "no-data-collected module-not-imported")


def loadCoverageInfo(path):
    """ Extract source information from test packages.

    If the a test package using the load_tests protocol is used check if it
    also defines the source files the test collection is suposed to cover and
    load it if it does. After that check if the user specified his own source
    definitions via the command line. The command line overrides the source
    definition of a test package. See module documentation for more info.

    :returns:
        A list of modules and packages as strings. This are the modules and
        packages that shall be measured for test coverage.
    """
    source = []
    spec = None
    try: spec = importlib.util.find_spec(path)
    except ModuleNotFoundError: pass
    if spec is not None and spec.origin.endswith("__init__.py"):
        package = importlib.import_module(path)
        if hasattr(package, "cover_tests"):
            source = package.cover_tests()
    if FreeCAD.ConfigGet("TestSource") != "":
        source = [FreeCAD.ConfigGet("TestSource")]
    return source


def report(cov):
    """ Print a coverage summery and generate html report.
    
    The report summery is printed to stdout, the html report generated inside
    the coverage/ subdirectory. Open coverage/index.html to view it.
    """
    try:
        cov.html_report(directory="coverage")
        cov.report()
    except coverage.CoverageException as e:
        print("Coverage Error: " + str(e))


def doCoverage():
    """ Return True if coverage enabled and possible.

    The --cov falg sets the TestCoverage configuration value to true. If it is
    set to true and the coverage module is avaliable the function returns True,
    otherwise False is returned.
    """
    module_avaliable = coverage is not None
    user_setting = FreeCAD.ConfigGet("TestCoverage")
    return module_avaliable and user_setting


def createCoverageObj(*args, **kwargs):
    """ Version independed coverage object creation (wrapper).

    The central coverage class was renamed with the 4.0 version of coverage.
    This function uses the old name if a pre 4.0 version is detected and the
    new name if a new version is used.
    """
    if coverage.__version__[0] < "4":
        return coverage.coverage(*args, **kwargs)
    else:
        return coverage.Coverage(*args, **kwargs)


path = FreeCAD.ConfigGet("TestCase")
cov = None
if doCoverage():
    source = loadCoverageInfo(path)
    cov = createCoverageObj(source=source)
    configureCoverage(cov)
    cov.start()
success = TestApp.TestText(path)
if doCoverage():
    cov.stop()
    report(cov)
sys.exit(0 if success else 1)
