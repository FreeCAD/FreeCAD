# SPDX-License-Identifier: LGPL-2.1-or-later

import glob
import os

import Arch_rc
import PartGui

# populate the list of submodules
modules = glob.glob(os.path.join(os.path.dirname(__file__), "*.py"))
__all__ = [os.path.basename(f)[:-3] for f in modules if os.path.isfile(f) and not f.endswith('__init__.py')]

from . import *
