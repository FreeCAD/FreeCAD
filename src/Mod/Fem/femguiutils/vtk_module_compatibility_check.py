# SPDX-License-Identifier: LGPL-2.1-or-later

"""Exits with non-zero return code if the VTK Python package
at FREECAD_FEM_VTK_PYTHON_PATH is not compatible with the loaded VTK library"""

import os
import sys

import Fem

try:
    folder = os.environ.get("FREECAD_FEM_VTK_PYTHON_PATH")
    if folder:
        sys.path = [folder]

    from vtkmodules.vtkCommonCore import vtkBitArray, vtkVersion

    compatible = Fem.getVtkVersion() == vtkVersion.GetVTKVersion()
    compatible = compatible and Fem.isVtkCompatible(vtkBitArray())
except Exception:
    compatible = False

os._exit(0 if compatible else 1)
