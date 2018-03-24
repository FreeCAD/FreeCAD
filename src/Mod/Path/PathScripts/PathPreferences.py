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
import glob
import os
import PathScripts.PathLog as PathLog

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule()

class PathPreferences:
    DefaultFilePath           = "DefaultFilePath"
    DefaultJobTemplate        = "DefaultJobTemplate"
    DefaultStockTemplate      = "DefaultStockTemplate"

    PostProcessorDefault      = "PostProcessorDefault"
    PostProcessorDefaultArgs  = "PostProcessorDefaultArgs"
    PostProcessorBlacklist    = "PostProcessorBlacklist"
    PostProcessorOutputFile   = "PostProcessorOutputFile"
    PostProcessorOutputPolicy = "PostProcessorOutputPolicy"

    # Linear tolerance to use when generating Paths, eg when tessellating geometry
    GeometryTolerance       = "GeometryTolerance"
    LibAreaCurveAccuracy    = "LibAreaCurveAccuarcy"

    EnableExperimentalFeatures = "EnableExperimentalFeatures"


    @classmethod
    def preferences(cls):
        return FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")

    @classmethod
    def pathScriptsSourcePath(cls):
        return FreeCAD.getHomePath() + ("Mod/Path/PathScripts/")

    @classmethod
    def pathScriptsPostSourcePath(cls):
        return cls.pathScriptsSourcePath() + ("/post/")

    @classmethod
    def allAvailablePostProcessors(cls):
        allposts = []
        for path in cls.searchPaths():
            posts = [ str(os.path.split(os.path.splitext(p)[0])[1][:-5]) for p in glob.glob(path + '/*_post.py')]
            allposts.extend(posts)
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
        pref = cls.preferences()
        return pref.GetString(cls.PostProcessorDefault, "")

    @classmethod
    def defaultPostProcessorArgs(cls):
        pref = cls.preferences()
        return pref.GetString(cls.PostProcessorDefaultArgs, "")

    @classmethod
    def defaultGeometryTolerance(cls):
        return cls.preferences().GetFloat(cls.GeometryTolerance, 0.01)

    @classmethod
    def defaultLibAreaCurveAccuracy(cls):
        return cls.preferences().GetFloat(cls.LibAreaCurveAccuracy, 0.01)

    @classmethod
    def defaultFilePath(cls):
        return cls.preferences().GetString(cls.DefaultFilePath)

    @classmethod
    def filePath(cls):
        path = cls.defaultFilePath()
        if not path:
            path = cls.macroFilePath()
        return path

    @classmethod
    def macroFilePath(cls):
        grp = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro")
        return grp.GetString("MacroPath", FreeCAD.getUserMacroDir())

    @classmethod
    def searchPaths(cls):
        paths = []
        p = cls.defaultFilePath()
        if p:
            paths.append(p)
        paths.append(cls.macroFilePath())
        paths.append(cls.pathScriptsPostSourcePath())
        paths.append(cls.pathScriptsSourcePath())
        return paths

    @classmethod
    def defaultJobTemplate(cls):
        template = cls.preferences().GetString(cls.DefaultJobTemplate)
        if 'xml' not in template:
            return template
        return ''

    @classmethod
    def setJobDefaults(cls, filePath, jobTemplate, geometryTolerance, curveAccuracy):
        PathLog.track("(%s='%s', %s, %s, %s)" % (cls.DefaultFilePath, filePath, jobTemplate, geometryTolerance, curveAccuracy))
        pref = cls.preferences()
        pref.SetString(cls.DefaultFilePath, filePath)
        pref.SetString(cls.DefaultJobTemplate, jobTemplate)
        pref.SetFloat(cls.GeometryTolerance, geometryTolerance)
        pref.SetFloat(cls.LibAreaCurveAccuracy, curveAccuracy)

    @classmethod
    def postProcessorBlacklist(cls):
        pref = cls.preferences()
        blacklist = pref.GetString(cls.PostProcessorBlacklist, "")
        if not blacklist:
            return []
        return eval(blacklist)

    @classmethod
    def setPostProcessorDefaults(cls, processor, args, blacklist):
        pref = cls.preferences()
        pref.SetString(cls.PostProcessorDefault, processor)
        pref.SetString(cls.PostProcessorDefaultArgs, args)
        pref.SetString(cls.PostProcessorBlacklist, "%s" % (blacklist))


    @classmethod
    def setOutputFileDefaults(cls, file, policy):
        pref = cls.preferences()
        pref.SetString(cls.PostProcessorOutputFile, file)
        pref.SetString(cls.PostProcessorOutputPolicy, policy)

    @classmethod
    def defaultOutputFile(cls):
        pref = cls.preferences()
        return pref.GetString(cls.PostProcessorOutputFile, "")

    @classmethod
    def defaultOutputPolicy(cls):
        pref = cls.preferences()
        return pref.GetString(cls.PostProcessorOutputPolicy, "")

    @classmethod
    def defaultStockTemplate(cls):
        return cls.preferences().GetString(cls.DefaultStockTemplate, "")
    @classmethod
    def setDefaultStockTemplate(cls, template):
        cls.preferences().SetString(cls.DefaultStockTemplate, template)


    @classmethod
    def experimentalFeaturesEnabled(cls):
        return cls.preferences().GetBool(cls.EnableExperimentalFeatures, False)

