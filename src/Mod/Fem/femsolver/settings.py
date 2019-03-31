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
class _BinaryDlg(object):

    def __init__(self, default, param, useDefault, customPath):
        self.default = default
        self.param = param
        self.useDefault = useDefault
        self.customPath = customPath

    def getBinary(self):

        # get the parameter object where the paramete are saved in
        paramObj = FreeCAD.ParamGet(self.param)

        # set the binary path to the FreeCAD defaults, ATM pure unix shell commands without path names are used
        # TODO see todo on useDefault later in this module
        binary = self.default
        FreeCAD.Console.PrintLog('Solver binary path: {} \n'.format(binary))

        # check if useDefault is set to True
        # if True the standard binary path will be overwritten with a user binary path
        if not paramObj.GetBool(self.useDefault, True):
            binary = paramObj.GetString(self.customPath)
        FreeCAD.Console.PrintLog('Solver binary path: {} \n'.format(binary))

        # get the whole binary path name for the given command or binary path and return it
        from distutils.spawn import find_executable as find_bin
        return find_bin(binary)


'''
default:
    default command to run the binary, this one is taken if the UseStandardXXXLocation is not given or set to True
param:
    path where these settings are saved, in FEM normally one path in one Tab in Preferences GUI
useDefault:
    the UseStandardXXXLocation parameter identifier
    if this parameter is set to True FreeCAD standards for the binary are used, or FreeCAD tries to find the binary
    TODO: see method setup_ccx in ccx tools module, which sets up ccx binary for various os
customPath:
    the xxxBinaryPath parameter identifier
    binary path given by the user
'''
_BINARIES = {
    "Calculix": _BinaryDlg(
        default="ccx",
        param=_PARAM_PATH + "Ccx",
        useDefault="UseStandardCcxLocation",
        customPath="ccxBinaryPath"),
    "ElmerSolver": _BinaryDlg(
        default="ElmerSolver",
        param=_PARAM_PATH + "Elmer",
        useDefault="UseStandardElmerLocation",
        customPath="elmerBinaryPath"),
    "ElmerGrid": _BinaryDlg(
        default="ElmerGrid",
        param=_PARAM_PATH + "Elmer",
        useDefault="UseStandardGridLocation",
        customPath="gridBinaryPath"),
    "Z88": _BinaryDlg(
        default="z88r",
        param=_PARAM_PATH + "Z88",
        useDefault="UseStandardZ88Location",
        customPath="z88BinaryPath"),
}


def getBinary(name):
    if name in _BINARIES:
        binary = _BINARIES[name].getBinary()
        FreeCAD.Console.PrintMessage('Solver binary path: {} \n'.format(binary))
        return binary
    else:
        FreeCAD.Console.PrintError(
            'Settings solver name: {} not found in solver settings modules _BINARIES dirctionary.\n'.format(name)
        )
        return None


# ******** working directory parameter ***********************************************************
def getCustomDir():
    param = FreeCAD.ParamGet(_GENERAL_PARAM)
    return param.GetString("CustomDirectoryPath")


def getDirSetting():
    param = FreeCAD.ParamGet(_GENERAL_PARAM)
    if param.GetBool("UseTempDirectory"):
        return TEMPORARY
    elif param.GetBool("UseBesideDirectory"):
        return BESIDE
    elif param.GetBool("UseCustomDirectory"):
        return CUSTOM

##  @}
