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
import os
import glob

class PathPreferences:
    PostProcessorDefault     = "PostProcessorDefault"
    PostProcessorDefaultArgs = "PostProcessorDefaultArgs"
    PostProcessorBlacklist   = "PostProcessorBlacklist"

    @classmethod
    def allAvailablePostProcessors(cls):
        path = FreeCAD.getHomePath() + ("Mod/Path/PathScripts/")
        posts = glob.glob(path + '/*_post.py')
        allposts = [ str(os.path.split(os.path.splitext(p)[0])[1][:-5]) for p in posts]

        grp = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro")
        path = grp.GetString("MacroPath", FreeCAD.getUserAppDataDir())
        posts = glob.glob(path + '/*_post.py')

        allposts.extend([ str(os.path.split(os.path.splitext(p)[0])[1][:-5]) for p in posts])
        allposts.sort()
        return allposts

    @classmethod
    def allEnabledPostProcessors(cls, include = None):
        blacklist = cls.postProcessorBlacklist()
        enabled = [processor for processor in cls.allAvailablePostProcessors() if not processor in blacklist]
        if include:
            l = list(set(include + enabled))
            l.sort()
            return l
        return enabled


    @classmethod
    def defaultPostProcessor(cls):
        preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        return preferences.GetString(cls.PostProcessorDefault, "")

    @classmethod
    def defaultPostProcessorArgs(cls):
        preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        return preferences.GetString(cls.PostProcessorDefaultArgs, "")

    @classmethod
    def postProcessorBlacklist(cls):
        preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        blacklist = preferences.GetString(cls.PostProcessorBlacklist, "")
        if not blacklist:
            return []
        return eval(blacklist)

    @classmethod
    def savePostProcessorDefaults(cls, processor, args, blacklist):
        preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        preferences.SetString(cls.PostProcessorDefault, processor)
        preferences.SetString(cls.PostProcessorDefaultArgs, args)
        preferences.SetString(cls.PostProcessorBlacklist, "%s" % (blacklist))


    DefaultOutputFile = "DefaultOutputFile"
    DefaultOutputPolicy = "DefaultOutputPolicy"

    @classmethod
    def saveOutputFileDefaults(cls, file, policy):
        preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        preferences.SetString(cls.DefaultOutputFile, file)
        preferences.SetString(cls.DefaultOutputPolicy, policy)

    @classmethod
    def defaultOutputFile(cls):
        preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        return preferences.GetString(cls.DefaultOutputFile, "")

    @classmethod
    def defaultOutputPolicy(cls):
        preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        return preferences.GetString(cls.DefaultOutputPolicy, "")
