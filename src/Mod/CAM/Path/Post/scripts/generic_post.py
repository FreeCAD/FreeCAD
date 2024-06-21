# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2024 Ondsel <development@ondsel.com>                    *
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

import os
from Path.Post.Processor import PostProcessor
import Path
import FreeCAD

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate

debug = True
if debug:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class Generic(PostProcessor):
    def __init__(self, job):
        super().__init__(
            job,
            tooltip=translate("CAM", "Generic post processor"),
            tooltipargs=["arg1", "arg2"],
            units="kg",
        )
        Path.Log.debug("Generic post processor initialized")

    def export(self):
        Path.Log.debug("Exporting the job")

        postables = self._buildPostList()
        Path.Log.debug(f"postables count: {len(postables)}")

        g_code_sections = []
        for idx, section in enumerate(postables):
            partname, sublist = section

            # here is where the sections are converted to gcode.
            g_code_sections.append((idx, partname))

        return g_code_sections

    @property
    def tooltip(self):

        tooltip = """
        This is a generic post processor.
        It doesn't do anything yet because we haven't immplemented it.

        Implementing it would be a good idea
        """
        return tooltip

    @property
    def tooltipArgs(self):
        argtooltip = """
        --arg1: This is the first argument
        --arg2: This is the second argument

        """
        return argtooltip

    @property
    def units(self):
        return self._units
