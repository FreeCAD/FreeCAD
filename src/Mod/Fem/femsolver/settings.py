# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
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
""" Query FEM specific settings including solver settings.

Query settings from the hierarchically organized settings/parameter system of
FreeCAD related to the FEM module. The collection of independent functions use
the settings system as a backend and expose a easy to use interface for other
modules of the FEM module.

Functions querying solver specific settings always take a solver name as a
string to identify the solver in question. At the moment the following solvers
are supported:

    - Calculix
    - ElmerSolver
    - Mystran
    - Z88

To query settings about those solver the solver name must be given exactly in
the form written in the list above. To make the solver recognize settings for a
new solver have a look at :class:`_SolverDlg`.
"""

__title__ = "FreeCAD FEM solver settings"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "https://www.freecad.org"


import FreeCAD


class DirSetting:
    """ Enum of possible directory setting values.

    Strings used to indicate the solver directory setting set in FreeCADs
    setting system. Returned by :func:`get_dir_setting` for that purpose. There
    are three different possible values:

    :cvar TEMPORARY:
        Let FreeCAD manage (create, delete) the working directories for all
        solver. Use temporary directories.

    :cvar BESIDE:
        Create a directory in the same folder in which the FCStd file of the
        document is located. Use Subfolder for each solver (e.g. for a file
        ./mydoc.FCStd and a solver with the label Elmer002 use
        ./mydoc/Elmer002).

    :cvar CUSTOM:
        Use directory set below. Create own subdirectory for every solver. Name
        directory after the solver label prefixed with the document name.
    """
    TEMPORARY = "temporary"
    BESIDE = "beside"
    CUSTOM = "custom"


# FEM parameter location path
_PARAM_PATH = "User parameter:BaseApp/Preferences/Mod/Fem/"
_GENERAL_PARAM = _PARAM_PATH + "General"


def get_binary(name, silent=False):
    """ Find binary of solver *name* honoring user settings.

    Return the specific path set by the user in FreeCADs settings/parameter
    system if set or the default binary name if no specific path is set. If no
    path was found because the solver *name* is not supported ``None`` is
    returned.
    This method does not check whether the binary actually exists and is callable.
    That check is done in DlgSettingsFem_Solver_Imp.cpp

    :param name: solver id as a ``str`` (see :mod:`femsolver.settings`)
    :param silent: whether to output error if binary not found
    """
    if name in _SOLVER_PARAM:
        binary = _SOLVER_PARAM[name].get_binary(silent)
        return binary
    else:
        if not silent:
            FreeCAD.Console.PrintError(
                "Settings solver name: {} not found in "
                "solver settings modules _SOLVER_PARAM dirctionary.\n"
                .format(name)
            )
        return None


def get_cores(name):
    """ Read number of CPU cores for solver *name* honoring user settings.

    Returns number of CPU cores to be used for the solver run

    :param name: solver id as a ``str`` (see :mod:`femsolver.settings`)
    """
    if name in _SOLVER_PARAM:
        cores = _SOLVER_PARAM[name].get_cores()
        return cores
    else:
        FreeCAD.Console.PrintError(
            "Settings solver name: {} not found in "
            "solver settings modules _SOLVER_PARAM dirctionary.\n"
            .format(name)
        )
        return None


def get_write_comments(name):
    """ Check whether "write_comments" is set for solver.

    Returns ``True`` if the "write_comments" setting/parameter is set for the
    solver with the id *name*. Returns ``False`` otherwise. If the solver is
    not supported ``None`` is returned.

    :param name: solver id as a ``str`` (see :mod:`femsolver.settings`)
    """
    if name in _SOLVER_PARAM:
        return _SOLVER_PARAM[name].get_write_comments()
    else:
        FreeCAD.Console.PrintError(
            "Settings solver name: {} not found in "
            "solver settings modules _SOLVER_PARAM dirctionary.\n"
            .format(name)
        )
        return None


def get_custom_dir():
    """ Get value for :term:`General/CustomDirectoryPath` parameter. """
    param_group = FreeCAD.ParamGet(_GENERAL_PARAM)
    return param_group.GetString("CustomDirectoryPath")


def get_dir_setting():
    """ Return directory setting set by the user.

    Return one of the three possible values of the :class:`DirSetting` enum
    depending on the setting set in FreeCAD parameter system. Result dependes
    on the values of :term:`General/UseTempDirectory`,
    :term:`General/UseBesideDirectory` and :term:`General/UseCustomDirectory`.
    """
    param_group = FreeCAD.ParamGet(_GENERAL_PARAM)
    if param_group.GetBool("UseBesideDirectory"):
        return DirSetting.BESIDE
    elif param_group.GetBool("UseCustomDirectory"):
        return DirSetting.CUSTOM
    return DirSetting.TEMPORARY


def get_default_solver():
    """ Return default solver name.
    """
    solver_map = {0: "None"}
    if get_binary("Calculix", True):
        solver_map[1] = "CalculixCcxTools"
    if get_binary("ElmerSolver", True):
        solver_map[len(solver_map)] = "Elmer"
    if get_binary("Mystran", True):
        solver_map[len(solver_map)] = "Mystran"
    if get_binary("Z88", True):
        solver_map[len(solver_map)] = "Z88"
    param_group = FreeCAD.ParamGet(_GENERAL_PARAM)
    return solver_map[param_group.GetInt("DefaultSolver", 0)]


class _SolverDlg(object):
    """ Internal query logic for solver specific settings.

    Each instance queries settings for one specific solver (e.g. Elmer) common
    among all solvers. To clarify: There are a few settings that are useful
    for every solver (e.g. where to find the solver binary) but the value and
    the FreeCAD parameter path is different for each one. A instance of this
    class contains all the solver specific paths needed. The settings can be
    queried via the methods which use those path members to query the value for
    the specific solver.

    :ivar default:
        Default binary name as a string preferably without a prefix path to
        make it more generic (e.g. "ccx"). This only works if the binary can be
        found via the PATH environment variable on linux or similar mechanisms
        on other operating systems. Used if nothing else is specified by the
        user.

    :ivar param_path:
        Parent param path (FreeCADs settings/parameter system) that contains
        all settings for the specific solver.

    :ivar use_default:
        Param path identifying the "use_default" setting. Only specifie the
        last part as the *param_path* is prepended to this value.

    :ivar custom_path:
        Param path identifying the "custom_path" setting. Only specifie the
        last part as the *param_path* is prepended to this value.
    """

    WRITE_COMMENTS_PARAM = "writeCommentsToInputFile"

    def __init__(self, default, param_path, use_default, custom_path):
        self.default = default
        self.param_path = param_path
        self.use_default = use_default
        self.custom_path = custom_path

        self.param_group = FreeCAD.ParamGet(self.param_path)

    def get_binary(self, silent=False):

        # set the binary path to the FreeCAD defaults
        # ATM pure unix shell commands without path names are used as standard
        # TODO the binaries provided with the FreeCAD distribution should be found
        # without any additional user input
        # see ccxttols, it works for Windows and Linux there
        binary = self.default
        FreeCAD.Console.PrintLog("Solver binary path default: {} \n".format(binary))

        # check if use_default is set to True
        # if True the standard binary path will be overwritten with a user binary path
        if self.param_group.GetBool(self.use_default, True) is False:
            binary = self.param_group.GetString(self.custom_path)
        FreeCAD.Console.PrintLog("Solver binary path user setting: {} \n".format(binary))

        # get the whole binary path name for the given command or binary path and return it
        # None is returned if the binary has not been found
        # The user does not know what exactly has going wrong.
        from distutils.spawn import find_executable as find_bin
        the_found_binary = find_bin(binary)
        if (the_found_binary is None) and (not silent):
            FreeCAD.Console.PrintError(
                "The binary has not been found. Full binary search path: {}\n"
                .format(binary)
            )
        else:
            FreeCAD.Console.PrintLog("Found solver binary path: {}\n".format(the_found_binary))
        return the_found_binary

    def get_cores(self):
        cores = self.param_group.GetInt("UseNumberOfCores")
        return cores

    def get_write_comments(self):
        return self.param_group.GetBool(self.WRITE_COMMENTS_PARAM, True)


_SOLVER_PARAM = {
    "Calculix": _SolverDlg(
        default="ccx",
        param_path=_PARAM_PATH + "Ccx",
        use_default="UseStandardCcxLocation",
        custom_path="ccxBinaryPath"),
    "ElmerSolver": _SolverDlg(
        default="ElmerSolver",
        param_path=_PARAM_PATH + "Elmer",
        use_default="UseStandardElmerLocation",
        custom_path="elmerBinaryPath"),
    "ElmerGrid": _SolverDlg(
        default="ElmerGrid",
        param_path=_PARAM_PATH + "Elmer",
        use_default="UseStandardGridLocation",
        custom_path="gridBinaryPath"),
    "Mystran": _SolverDlg(
        default="mystran",
        param_path=_PARAM_PATH + "Mystran",
        use_default="UseStandardMystranLocation",
        custom_path="mystranBinaryPath"),
    "Z88": _SolverDlg(
        default="z88r",
        param_path=_PARAM_PATH + "Z88",
        use_default="UseStandardZ88Location",
        custom_path="z88BinaryPath"),
}
