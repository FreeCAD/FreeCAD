# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
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

"""Methods to verify if the python VTK module is the correct one

FreeCAD is linked with VTK libraries during its build process. To use the VTK
python module and pass objects between python and c++ the compiled module library
needs to be linked to the exact same vtk library as FreeCAD is. This is ensured by
installing VTK via linux app managers: All known distros install the python side
packages together with vtk libs. Libpack and other OS systems ensure this too.

However, if a vtk python package is installed manually, e.g. by "pip install vtk",
it could be found instead of the system module. This python API brings its own
set of VTK libs, and hence object passing in FreeCAD fails. (Note: import and
pure vtk python code still works, only passing to c++ fails)

This file provides functions that detect this situation and inform the user.
Additionally we try to find the correct module in the path and offer to use
it instead.

Note that this problem occurs with all "compiled binary" python APIs, also
with PySide. It is the users responsibility to handle his python path and keep
it clean/working. The functions provided here are a workaround only.
"""

__title__ = "FEM GUI vtk python module check"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"


# Note: This file is imported from FreeCAD App files. Do not import any FreeCADGui
#       directly to support cmd line use.

__user_input_received = False


def vtk_module_compatible():
    # checks if the VTK library FreeCAD is build against is the one used by
    # the python module

    # make sure we do not contaminate the modules with vtk to not trick
    # the check later
    unload = not _vtk_is_loaded()

    import Fem
    from vtkmodules.vtkCommonCore import vtkVersion, vtkBitArray

    # simple version check
    if Fem.getVtkVersion() != vtkVersion.GetVTKVersion():
        return False

    # check binary compatibility
    result = Fem.isVtkCompatible(vtkBitArray())

    if unload:
        # cleanup our own import
        _unload_vtk_modules()

    return result


def _vtk_is_loaded():
    import sys

    return any("vtkmodules" in module for module in sys.modules)


def _unload_vtk_modules():
    # unloads all loaded vtk modules
    # NOTE: does not remove any stored references in objects

    import sys

    for module in sys.modules.copy():
        if "vtkmodules" in module:
            del sys.modules[module]


def _find_compatible_module():
    # Check all python path folders if they contain a vtk module

    import Fem
    import sys

    # remove module from runtime
    _unload_vtk_modules()

    path = sys.path.copy()

    for folder in reversed(path):
        try:
            # use a single folder as path and try to load vtk
            sys.path = [folder]
            if vtk_module_compatible():
                # we do still unload, to let the user decide if they want to use it
                _unload_vtk_modules()
                sys.path = path
                return folder

        except:
            continue

    # reset the correct path and indicate that we failed
    sys.path = path
    return None


def _load_vtk_from(folder):

    import sys

    path = sys.path
    try:
        sys.path = [folder]
        import vtkmodules
    finally:
        sys.path = path


# If FreeCAD is build with VTK python support this function checks if the
# used python module is compatible with the c++ lib. Does inform the user
# if not so and offers the correct module, if available
#
# Note: Call this also from Python feature module, as on document load
#       this can be loaded before initializing FEM workbench.
def vtk_module_handling():

    import sys
    import FreeCAD

    if not "BUILD_FEM_VTK_PYTHON" in FreeCAD.__cmake__:
        # no VTK python api support in FreeCAD
        return

    # only ask user once per session
    global __user_input_received
    if __user_input_received:
        return
    __user_input_received = True

    loaded = _vtk_is_loaded()

    # check if we are compatible
    if not vtk_module_compatible():

        if not FreeCAD.GuiUp:
            FreeCAD.Console.PrintError(
                "FEM: vtk python module is not compatible with internal vtk library"
            )
            return

        import FreeCAD, Fem
        from vtkmodules.vtkCommonCore import vtkVersion
        import inspect
        from PySide import QtGui

        translate = FreeCAD.Qt.translate

        path = inspect.getfile(vtkVersion)
        path = path[: path.find("vtkmodules")]

        message = translate(
            "FEM",
            (
                "FreeCAD is linked to a different VTK library than the imported "
                "VTK python module. This is incompatible and will lead to errors."
                "\n\nWrong python module is imported from: \n{}"
            ),
        ).format(path)

        buttons = QtGui.QMessageBox.Discard

        # check if there is any compatible vtk module
        compatible_module = _find_compatible_module()

        if compatible_module:
            # there is a compatible module of VTK available.
            message += translate("FEM", "\n\nCorrect module found in: \n{}").format(
                compatible_module
            )

            if not loaded:
                # vtk was not loaded beforehand, therefore we can realistically reload
                message += translate("FEM", "\n\nShould this module be loaded instead?")

                buttons = QtGui.QMessageBox.Yes | QtGui.QMessageBox.No

            else:
                message += translate(
                    "FEM",
                    (
                        "\n\nAs the wrong module was already loaded, a reload is not possible. "
                        "Restart FreeCAD to get the option for loading this module."
                    ),
                )

        else:
            message += translate(
                "FEM", "\n\nNo matching module was found in the current python path."
            )

        # raise a dialog to the user
        import FreeCADGui

        button = QtGui.QMessageBox.critical(
            FreeCADGui.getMainWindow(),
            translate("FEM", "VTK module conflict"),
            message,
            buttons=buttons,
        )

        if button == QtGui.QMessageBox.Yes:
            # try to reload the correct vtk module
            _load_vtk_from(compatible_module)


# Returns if vtk python is incompatible and hence operations need to be aborted.
# If inform=True the user gets informed by dialog about incompatibilities
def vtk_compatibility_abort(inform=True):

    if not vtk_module_compatible():

        if inform:
            # raise a dialog to the user that this functionality is not available
            import FreeCAD
            import FreeCADGui
            from PySide import QtGui

            translate = FreeCAD.Qt.translate

            button = QtGui.QMessageBox.critical(
                FreeCADGui.getMainWindow(),
                translate("FEM", "VTK module conflict"),
                translate(
                    "FEM", "This functionality is not available due to VTK python module conflict"
                ),
                buttons=QtGui.QMessageBox.Discard,
            )

        return True

    return False
