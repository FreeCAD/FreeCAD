# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import distutils.spawn
import FreeCAD as App


TEMPORARY = "temporary"
BESIDE = "beside"
CUSTOM = "custom"


_PARAM_PATH = "User parameter:BaseApp/Preferences/Mod/Fem/"
_GENERAL_PARAM = _PARAM_PATH + "General"
_CCX_PARAM = _PARAM_PATH + "Ccx"
_ELMER_PARAM = _PARAM_PATH + "Elmer"
_Z88_PARAM = _PARAM_PATH + "Z88"


class _BinaryDlg(object):

    def __init__(self, default, param, useDefault, customPath):
        self.default = default
        self.param = param
        self.useDefault = useDefault
        self.customPath = customPath

    def getBinary(self):
        paramObj = App.ParamGet(self.param)
        binary = self.default
        if not paramObj.GetBool(self.useDefault):
            binary = paramObj.GetString(self.customPath)
        return distutils.spawn.find_executable(binary)


_BINARIES = {
    "Calculix": _BinaryDlg(
        default="ccx",
        param=_CCX_PARAM,
        useDefault="UseStandardCcxLocation",
        customPath="ccxBinaryPath"),
    "ElmerSolver": _BinaryDlg(
        default="ElmerSolver",
        param=_ELMER_PARAM,
        useDefault="UseStandardElmerLocation",
        customPath="elmerBinaryPath"),
    "ElmerGrid": _BinaryDlg(
        default="ElmerGrid",
        param=_ELMER_PARAM,
        useDefault="UseStandardGridLocation",
        customPath="gridBinaryPath"),
    "Z88": _BinaryDlg(
        default="z88r",
        param=_Z88_PARAM,
        useDefault="UseStandardZ88Location",
        customPath="z88BinaryPath"),
}


def getBinary(name):
    if name in _BINARIES:
        return _BINARIES[name].getBinary()
    return None


def getCustomDir():
    param = App.ParamGet(_GENERAL_PARAM)
    return param.GetString("CustomDirectoryPath")


def getDirSetting():
    param = App.ParamGet(_GENERAL_PARAM)
    if param.GetBool("UseTempDirectory"):
        return TEMPORARY
    elif param.GetBool("UseBesideDirectory"):
        return BESIDE
    elif param.GetBool("UseCustomDirectory"):
        return CUSTOM

##  @}
