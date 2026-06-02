# SPDX-License-Identifier: LGPL-2.1-or-later

from SketcherTests.TestConstraintPreselectionGui import SketcherGuiTestCases
from SketcherTests.TestOnViewParameterGui import TestOnViewParameterGui
from SketcherTests.TestPlacementUpdate import TestSketchPlacementUpdate

# Use the module so that code checkers don't complain (flake8)
True if SketcherGuiTestCases and TestSketchPlacementUpdate and TestOnViewParameterGui else False
