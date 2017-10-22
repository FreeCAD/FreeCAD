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


__title__ = "FemMisc"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


import FreeCAD as App


TEMPORARY = "temporary"
BESIDE = "beside"
CUSTOM = "custom"

_ELMER_PARAM = "User parameter:BaseApp/Preferences/Mod/Fem/Elmer"
_GRID_PARAM = "User parameter:BaseApp/Preferences/Mod/Fem/Grid"


class _BinaryDlg(object):

    def __init__(self, default, param, useDefault, customPath):
        self.default = default
        self.param = param
        self.useDefault = useDefault
        self.customPath = customPath

    def getBinary(self):
        paramObj = App.ParamGet(self.param)
        if paramObj.GetBool(self.useDefault):
            return self.default
        return paramObj.GetString(self.customPath)


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
