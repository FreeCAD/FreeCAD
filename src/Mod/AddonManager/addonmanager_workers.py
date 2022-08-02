# -*- coding: utf-8 -*-
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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

import os
import re
import shutil
import stat
import json
import tempfile
import hashlib
import threading
import queue
import io
import time
import subprocess
import sys
import platform
import itertools
from datetime import datetime
from typing import Union, List, Dict
from enum import Enum, auto


from PySide2 import QtCore

import FreeCAD
import addonmanager_utilities as utils
from addonmanager_macro import Macro
from Addon import Addon
import NetworkManager

if FreeCAD.GuiUp:
    import FreeCADGui

have_git = False
try:
    import git

    # some versions of git module have no "Repo" class??  Bug #4072 module
    # 'git' has no attribute 'Repo'
    have_git = hasattr(git, "Repo")
    if not have_git:
        FreeCAD.Console.PrintMessage(
            "'import git' gave strange results (no Repo attribute)... do you have GitPython installed?"
        )
except ImportError:
    pass

have_zip = False
try:
    import zipfile

    have_zip = True
except ImportError:
    pass

have_markdown = False
try:
    import markdown

    have_markdown = True
except ImportError:
    pass

translate = FreeCAD.Qt.translate

#  @package AddonManager_workers
#  \ingroup ADDONMANAGER
#  \brief Multithread workers for the addon manager
#  @{

NOGIT = False  # for debugging purposes, set this to True to always use http downloads
NOMARKDOWN = False  # for debugging purposes, set this to True to disable Markdown lib
"""Multithread workers for the Addon Manager"""


Z
