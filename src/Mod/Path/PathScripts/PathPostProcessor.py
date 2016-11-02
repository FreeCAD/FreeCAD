# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD
from PathScripts.PathPreferences import PathPreferences

class PostProcessor:

    @classmethod
    def exists(cls, processor):
        return processor in PathPreferences.allAvailablePostProcessors()

    @classmethod
    def load(cls, processor):
        postname = processor + "_post"

        exec "import %s as current_post" % postname
        # make sure the script is reloaded if it was previously loaded
        # should the script have been imported for the first time above
        # then the initialization code of the script gets executed twice
        # resulting in 2 load messages if the script outputs one of those.
        exec "reload(%s)" % 'current_post'

        instance = PostProcessor(current_post)
        instance.units = None
        if hasattr(current_post, "UNITS"):
            if current_post.UNITS == "G21":
                instance.units = "Metric"
            else:
                instance.units = "Inch"

        instance.machineName = None
        if hasattr(current_post, "MACHINE_NAME"):
            instance.machineName = current_post.MACHINE_NAME

        instance.cornerMax = None
        if hasattr(current_post, "CORNER_MAX"):
            instance.cornerMax = {'x': current_post.CORNER_MAX['x'],
                    'y': current_post.CORNER_MAX['y'],
                    'z': current_post.CORNER_MAX['z']}

        instance.cornerMin = None
        if hasattr(current_post, "CORNER_MIN"):
            instance.cornerMin = {'x': current_post.CORNER_MIN['x'],
                    'y': current_post.CORNER_MIN['y'],
                    'z': current_post.CORNER_MIN['z']}

        instance.tooltip = None
        instance.tooltipArgs = None
        if hasattr(current_post, "TOOLTIP"):
            instance.tooltip = current_post.TOOLTIP
            if hasattr(current_post, "TOOLTIP_ARGS"):
                instance.tooltipArgs = current_post.TOOLTIP_ARGS
        return instance

    def __init__(self, script):
        self.script = script

    def export(self, obj, filename, args):
        self.script.export(obj, filename, args)
