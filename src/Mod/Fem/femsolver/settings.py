# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
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


__title__ = "settings"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


import distutils.spawn
import FreeCAD as App


TEMPORARY = "temporary"
BESIDE = "beside"
CUSTOM = "custom"

_ELMER_PARAM = "User parameter:BaseApp/Preferences/Mod/Fem/Elmer"
_GRID_PARAM = "User parameter:BaseApp/Preferences/Mod/Fem/Grid"
_CCX_PARAM = "User parameter:BaseApp/Preferences/Mod/Fem/Ccx"
_Z88_PARAM = "User parameter:BaseApp/Preferences/Mod/Fem/Z88"


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
    "ElmerSolver": _BinaryDlg(
        default="ElmerSolver",
        param=_ELMER_PARAM,
        useDefault="UseStandardElmerLocation",
        customPath="elmerBinaryPath"),
    "ElmerGrid": _BinaryDlg(
        default="ElmerGrid",
        param=_GRID_PARAM,
        useDefault="UseStandardGridLocation",
        customPath="gridBinaryPath"),
    "Calculix": _BinaryDlg(
        default="ccx",
        param=_CCX_PARAM,
        useDefault="UseStandardCcxLocation",
        customPath="ccxBinaryPath"),
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
    param = App.ParamGet(_ELMER_PARAM)
    return param.GetString("CustomDirectoryPath")


def getDirSetting():
    param = App.ParamGet(_ELMER_PARAM)
    if param.GetBool("UseTempDirectory"):
        return TEMPORARY
    elif param.GetBool("UseBesideDirectory"):
        return BESIDE
    elif param.GetBool("UseCustomDirectory"):
        return CUSTOM
