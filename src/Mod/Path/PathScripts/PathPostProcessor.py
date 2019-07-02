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

import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import sys

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

class PostProcessor:

    @classmethod
    def exists(cls, processor):
        return processor in PathPreferences.allAvailablePostProcessors()

    @classmethod
    def load(cls, processor):
        PathLog.track(processor)
        syspath = sys.path
        paths = PathPreferences.searchPaths()
        paths.extend(sys.path)
        sys.path = paths

        postname = processor + "_post"
        namespace = {}

        #can't modify function local scope with exec in python3
        exec("import %s as current_post" % postname, namespace) # pylint: disable=exec-used
        current_post = namespace['current_post']

        # make sure the script is reloaded if it was previously loaded
        # should the script have been imported for the first time above
        # then the initialization code of the script gets executed twice
        # resulting in 2 load messages if the script outputs one of those.
        try:
            # Python 2.7
            exec("reload(%s)" % 'current_post') # pylint: disable=exec-used
        except NameError:
            # Python 3.4+
            from importlib import reload        # pylint: disable=redefined-builtin,unused-import
            exec("reload(%s)" % 'current_post') # pylint: disable=exec-used

        sys.path = syspath

        instance = PostProcessor(current_post)
        if hasattr(current_post, "UNITS"):
            if current_post.UNITS == "G21":
                instance.units = "Metric"
            else:
                instance.units = "Inch"

        if hasattr(current_post, "MACHINE_NAME"):
            instance.machineName = current_post.MACHINE_NAME

        if hasattr(current_post, "CORNER_MAX"):
            instance.cornerMax = {'x': current_post.CORNER_MAX['x'],
                    'y': current_post.CORNER_MAX['y'],
                    'z': current_post.CORNER_MAX['z']}

        if hasattr(current_post, "CORNER_MIN"):
            instance.cornerMin = {'x': current_post.CORNER_MIN['x'],
                    'y': current_post.CORNER_MIN['y'],
                    'z': current_post.CORNER_MIN['z']}

        if hasattr(current_post, "TOOLTIP"):
            instance.tooltip = current_post.TOOLTIP
            if hasattr(current_post, "TOOLTIP_ARGS"):
                instance.tooltipArgs = current_post.TOOLTIP_ARGS
        return instance

    def __init__(self, script):
        self.script = script
        self.tooltip = None
        self.tooltipArgs = None
        self.cornerMax = None
        self.cornerMin = None
        self.units = None
        self.machineName = None

    def export(self, obj, filename, args):
        return self.script.export(obj, filename, args)
