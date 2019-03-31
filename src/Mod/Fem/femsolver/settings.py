# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver settings"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

'''
parameter in FreeCAD can be edited in two ways, either in the
Preferences: menu Edit --> Preferences
or
Parameter editor: menu Tools --> Edit parameter
'''

import FreeCAD


# working directory: possible choices
TEMPORARY = "temporary"
BESIDE = "beside"
CUSTOM = "custom"


# FEM parameter location path
_PARAM_PATH = "User parameter:BaseApp/Preferences/Mod/Fem/"
_GENERAL_PARAM = _PARAM_PATH + "General"


# ******** binary parameter **********************************************************************
class _SolverDlg(object):

    def __init__(self, default, param_path, use_default, custom_path):

        # set the parameter identifier
        self.default = default
        self.param_path = param_path
        self.use_default = use_default
        self.custom_path = custom_path
        self.write_comments = "writeCommentsToInputFile"

        # get the parameter object where the paramete are saved in
        self.param_group = FreeCAD.ParamGet(self.param_path)

    def get_binary(self):

        # set the binary path to the FreeCAD defaults, ATM pure unix shell commands without path names are used
        # TODO see todo on use_default later in this module
        binary = self.default
        FreeCAD.Console.PrintLog('Solver binary path: {} \n'.format(binary))

        # check if use_default is set to True
        # if True the standard binary path will be overwritten with a user binary path
        if self.param_group.GetBool(self.use_default, True) is False:
            binary = self.param_group.GetString(self.custom_path)
        FreeCAD.Console.PrintLog('Solver binary path: {} \n'.format(binary))

        # get the whole binary path name for the given command or binary path and return it
        from distutils.spawn import find_executable as find_bin
        return find_bin(binary)

    def get_write_comments(self):
        return self.param_group.GetBool(self.write_comments, True)


'''
default:
    default command to run the binary, this one is taken if the UseStandardXXXLocation is not given or set to True
param:
    path where these settings are saved, in FEM normally one path in one Tab in Preferences GUI
use_default:
    the UseStandardXXXLocation parameter identifier
    if this parameter is set to True FreeCAD standards for the binary are used, or FreeCAD tries to find the binary
    TODO: see method setup_ccx in ccx tools module, which sets up ccx binary for various os
custom_path:
    the xxxBinaryPath parameter identifier
    binary path given by the user
'''
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
    "Z88": _SolverDlg(
        default="z88r",
        param_path=_PARAM_PATH + "Z88",
        use_default="UseStandardZ88Location",
        custom_path="z88BinaryPath"),
}


def get_binary(name):
    if name in _SOLVER_PARAM:
        binary = _SOLVER_PARAM[name].get_binary()
        FreeCAD.Console.PrintMessage('Solver binary path: {} \n'.format(binary))
        return binary
    else:
        FreeCAD.Console.PrintError(
            'Settings solver name: {} not found in solver settings modules _SOLVER_PARAM dirctionary.\n'.format(name)
        )
        return None


def get_write_comments(name):
    if name in _SOLVER_PARAM:
        return _SOLVER_PARAM[name].get_write_comments()
    else:
        FreeCAD.Console.PrintError(
            'Settings solver name: {} not found in solver settings modules _SOLVER_PARAM dirctionary.\n'.format(name)
        )
        return None


# ******** working directory parameter ***********************************************************
def getCustomDir():
    param_group = FreeCAD.ParamGet(_GENERAL_PARAM)
    return param_group.GetString("CustomDirectoryPath")


def getDirSetting():
    param_group = FreeCAD.ParamGet(_GENERAL_PARAM)
    if param_group.GetBool("UseTempDirectory"):
        return TEMPORARY
    elif param_group.GetBool("UseBesideDirectory"):
        return BESIDE
    elif param_group.GetBool("UseCustomDirectory"):
        return CUSTOM


##  @}
