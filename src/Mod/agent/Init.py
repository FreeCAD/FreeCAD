# SPDX-License-Identifier: LicenseRef-Parashell-Proprietary

# ********************************************************************************
# *   Copyright (c) 2026 Odin Glynn-Martin <odin.glynn[at]parashell.cloud>  	    *
# *                                                                         	    *
# *   This file is part of the Parashell Mod system.                        	    *
# *                                                                         	    *
# *   PROPRIETARY // NON-CONFIDENTIAL                                           *
# *                                                                         	    *
# *   This file contains proprietary resources used by compiled       		    *
# *   Cython modules within the Parashell Mod system. It is provided in     	    *
# *   uncompiled form solely to support the runtime environment and              *
# *   must not be treated as open-source software.                          	    *
# *                                                                         	    *
# *   Unauthorised copying, distribution, modification, or use of this      	    *
# *   file, in whole or in part, is strictly prohibited without the         	    *
# *   express written permission of the copyright holder.                   	    *
# *                                                                         	    *
# *   All rights reserved.                                                  	    *
# *                                                                         	    *
# ********************************************************************************

"""
Initialise the Agent module
"""

import os

import FreeCAD

import parashell_telemetry

parashell_telemetry.install("Agent")

_chromium_flags = os.environ.get("QTWEBENGINE_CHROMIUM_FLAGS", "").split()
for _flag in (
    "--disable-gpu-compositing",
    "--disable-background-timer-throttling",
    "--disable-renderer-backgrounding",
    "--disable-backgrounding-occluded-windows",
):
    if _flag not in _chromium_flags:
        _chromium_flags.append(_flag)
os.environ["QTWEBENGINE_CHROMIUM_FLAGS"] = " ".join(_chromium_flags).strip()

FreeCAD.Console.PrintMessage("Parashell Agent plugin loaded.\n")
