# SPDX-License-Identifier: LGPL-2.1-or-later

from AssemblyTests.TestCommandInsertLink import TestCommandInsertLink
from AssemblyTests.TestCommandCreateView import TestCommandCreateView
from AssemblyTests.TestSimulationExport import TestSimulationExport

# Use the modules so that code checkers don't complain (flake8)
True if TestCommandInsertLink else False
True if TestCommandCreateView else False
True if TestSimulationExport else False
