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

## @package FemBeamSection
#  \ingroup FEM


class _FemBeamSection:
    "The FemBeamSection object"
    known_beam_types = ['Rectangular', 'Circular', 'Pipe']
    def __init__(self, obj):
        obj.addProperty("App::PropertyLength", "RectWidth", "RectBeamSection", "set width of the rectangular beam elements")
        obj.addProperty("App::PropertyLength", "RectHeight", "RectBeamSection", "set height of therectangular beam elements")
        obj.addProperty("App::PropertyLength", "CircRadius", "CircBeamSection", "set radius of the circular beam elements")
        obj.addProperty("App::PropertyLength", "PipeRadius", "PipeBeamSection", "set height of the pipe beam elements")
        obj.addProperty("App::PropertyLength", "PipeThickness", "PipeBeamSection", "set height of the pipe beam elements")
        obj.addProperty("App::PropertyEnumeration", "SectionType", "BeamSection", "select beam section type")
        obj.addProperty("App::PropertyLinkSubList", "References", "BeamSection", "List of beam section shapes")
        obj.SectionType = _FemBeamSection.known_beam_types
        obj.SectionType = 'Rectangular'
        obj.Proxy = self
        self.Type = "FemBeamSection"

    def execute(self, obj):
        return
