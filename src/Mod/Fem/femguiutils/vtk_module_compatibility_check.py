# SPDX-License-Identifier: LGPL-2.1-or-later

"""Exits with non-zero return code if the VTK Python package
at FREECAD_FEM_VTK_PYTHON_PATH is not compatible with the loaded VTK library

Meant to be run as a standalone script in a subprocess (see
vtk_module_handling.vtk_module_compatible), not imported. It only runs and
exits when FREECAD_FEM_VTK_PYTHON_PATH is set. an empty value means "use the
default VTK", so importing should be harmless.
"""

import os
import sys


def main():
    try:
        import Fem

        folder = os.environ.get("FREECAD_FEM_VTK_PYTHON_PATH")
        if folder:
            sys.path = [folder]

        from vtkmodules.vtkCommonCore import vtkBitArray, vtkVersion

        compatible = Fem.getVtkVersion() == vtkVersion.GetVTKVersion()
        compatible = compatible and Fem.isVtkCompatible(vtkBitArray())
    except Exception:
        compatible = False
    return 0 if compatible else 1


# FreeCAD's file runner sets __name__ to the file stem, not "__main__",
# so gate on an env var set by the caller instead.
if os.environ.get("FREECAD_FEM_VTK_PYTHON_PATH") is not None:
    os._exit(main())
