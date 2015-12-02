# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "_FemBeamSection"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


class _FemBeamSection:
    "The FemBeamSection object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyLength", "Width", "BeamSection", "set width of the beam elements")
        obj.addProperty("App::PropertyLength", "Height", "BeamSection", "set height of the beam elements")
        obj.addProperty("App::PropertyLinkSubList", "References", "BeamSection", "List of beam section shapes")
        obj.Proxy = self
        self.Type = "FemBeamSection"

    def execute(self, obj):
        return
