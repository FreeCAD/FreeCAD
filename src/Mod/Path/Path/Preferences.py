# -*- coding: utf-8 -*-
# ***************************************************************************
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
import Path
import glob
import os

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate

DefaultFilePath = "DefaultFilePath"
DefaultJobTemplate = "DefaultJobTemplate"
DefaultStockTemplate = "DefaultStockTemplate"
DefaultTaskPanelLayout = "DefaultTaskPanelLayout"

PostProcessorDefault = "PostProcessorDefault"
PostProcessorDefaultArgs = "PostProcessorDefaultArgs"
PostProcessorBlacklist = "PostProcessorBlacklist"
PostProcessorOutputFile = "PostProcessorOutputFile"
PostProcessorOutputPolicy = "PostProcessorOutputPolicy"

LastPathToolBit = "LastPathToolBit"
LastPathToolLibrary = "LastPathToolLibrary"
LastPathToolShape = "LastPathToolShape"
LastPathToolTable = "LastPathToolTable"

LastFileToolBit = "LastFileToolBit"
LastFileToolLibrary = "LastFileToolLibrary"
LastFileToolShape = "LastFileToolShape"

UseAbsoluteToolPaths = "UseAbsoluteToolPaths"
# OpenLastLibrary                 = "OpenLastLibrary"

# Linear tolerance to use when generating Paths, eg when tessellating geometry
GeometryTolerance = "GeometryTolerance"
LibAreaCurveAccuracy = "LibAreaCurveAccuracy"

WarningSuppressRapidSpeeds = "WarningSuppressRapidSpeeds"
WarningSuppressAllSpeeds = "WarningSuppressAllSpeeds"
WarningSuppressSelectionMode = "WarningSuppressSelectionMode"
WarningSuppressOpenCamLib = "WarningSuppressOpenCamLib"
WarningSuppressVelocity = "WarningSuppressVelocity"
EnableExperimentalFeatures = "EnableExperimentalFeatures"
EnableAdvancedOCLFeatures = "EnableAdvancedOCLFeatures"


def preferences():
    return FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")


def pathPostSourcePath():
    return os.path.join(FreeCAD.getHomePath(), "Mod/Path/Path/Post/")


def pathDefaultToolsPath(sub=None):
    if sub:
        return os.path.join(FreeCAD.getHomePath(), "Mod/Path/Tools/", sub)
    return os.path.join(FreeCAD.getHomePath(), "Mod/Path/Tools/")


def allAvailablePostProcessors():
    allposts = []
    for path in searchPathsPost():
        posts = [
            str(os.path.split(os.path.splitext(p)[0])[1][:-5])
            for p in glob.glob(path + "/*_post.py")
        ]
        allposts.extend(posts)
    allposts.sort()
    return allposts


def allEnabledPostProcessors(include=None):
    blacklist = postProcessorBlacklist()
    enabled = [
        processor
        for processor in allAvailablePostProcessors()
        if processor not in blacklist
    ]
    if include:
        postlist = list(set(include + enabled))
        postlist.sort()
        return postlist
    return enabled


def defaultPostProcessor():
    pref = preferences()
    return pref.GetString(PostProcessorDefault, "")


def defaultPostProcessorArgs():
    pref = preferences()
    return pref.GetString(PostProcessorDefaultArgs, "")


def defaultGeometryTolerance():
    return preferences().GetFloat(GeometryTolerance, 0.01)


def defaultLibAreaCurveAccuracy():
    return preferences().GetFloat(LibAreaCurveAccuracy, 0.01)


def defaultFilePath():
    return preferences().GetString(DefaultFilePath)


def filePath():
    path = defaultFilePath()
    if not path:
        path = macroFilePath()
    return path


def macroFilePath():
    grp = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro")
    return grp.GetString("MacroPath", FreeCAD.getUserMacroDir())


def searchPaths():
    paths = []
    p = defaultFilePath()
    if p:
        paths.append(p)
    paths.append(macroFilePath())
    return paths


def searchPathsPost():
    paths = []
    p = defaultFilePath()
    if p:
        paths.append(p)
    paths.append(macroFilePath())
    paths.append(os.path.join(pathPostSourcePath(), "scripts/"))
    paths.append(pathPostSourcePath())
    return paths


def searchPathsTool(sub):
    paths = []
    paths.append(os.path.join(FreeCAD.getHomePath(), "Mod", "Path", "Tools", sub))
    return paths


def toolsStoreAbsolutePaths():
    return preferences().GetBool(UseAbsoluteToolPaths, False)


# def toolsOpenLastLibrary():
#     return preferences().GetBool(OpenLastLibrary, False)


def setToolsSettings(relative):
    pref = preferences()
    pref.SetBool(UseAbsoluteToolPaths, relative)
    # pref.SetBool(OpenLastLibrary, lastlibrary)


def defaultJobTemplate():
    template = preferences().GetString(DefaultJobTemplate)
    if "xml" not in template:
        return template
    return ""


def setJobDefaults(fileName, jobTemplate, geometryTolerance, curveAccuracy):
    Path.Log.track(
        "(%s='%s', %s, %s, %s)"
        % (DefaultFilePath, fileName, jobTemplate, geometryTolerance, curveAccuracy)
    )
    pref = preferences()
    pref.SetString(DefaultFilePath, fileName)
    pref.SetString(DefaultJobTemplate, jobTemplate)
    pref.SetFloat(GeometryTolerance, geometryTolerance)
    pref.SetFloat(LibAreaCurveAccuracy, curveAccuracy)


def postProcessorBlacklist():
    pref = preferences()
    blacklist = pref.GetString(PostProcessorBlacklist, "")
    if not blacklist:
        return []
    return eval(blacklist)


def setPostProcessorDefaults(processor, args, blacklist):
    pref = preferences()
    pref.SetString(PostProcessorDefault, processor)
    pref.SetString(PostProcessorDefaultArgs, args)
    pref.SetString(PostProcessorBlacklist, "%s" % (blacklist))


def setOutputFileDefaults(fileName, policy):
    pref = preferences()
    pref.SetString(PostProcessorOutputFile, fileName)
    pref.SetString(PostProcessorOutputPolicy, policy)


def defaultOutputFile():
    pref = preferences()
    return pref.GetString(PostProcessorOutputFile, "")


def defaultOutputPolicy():
    pref = preferences()
    return pref.GetString(PostProcessorOutputPolicy, "")


def defaultStockTemplate():
    return preferences().GetString(DefaultStockTemplate, "")


def setDefaultStockTemplate(template):
    preferences().SetString(DefaultStockTemplate, template)


def defaultTaskPanelLayout():
    return preferences().GetInt(DefaultTaskPanelLayout, 0)


def setDefaultTaskPanelLayout(style):
    preferences().SetInt(DefaultTaskPanelLayout, style)


def advancedOCLFeaturesEnabled():
    return preferences().GetBool(EnableAdvancedOCLFeatures, False)


def experimentalFeaturesEnabled():
    return preferences().GetBool(EnableExperimentalFeatures, False)


def suppressAllSpeedsWarning():
    return preferences().GetBool(WarningSuppressAllSpeeds, True)


def suppressRapidSpeedsWarning(user=True):
    return (user and suppressAllSpeedsWarning()) or preferences().GetBool(
        WarningSuppressRapidSpeeds, True
    )


def suppressSelectionModeWarning():
    return preferences().GetBool(WarningSuppressSelectionMode, True)


def suppressOpenCamLibWarning():
    return preferences().GetBool(WarningSuppressOpenCamLib, True)


def suppressVelocity():
    return preferences().GetBool(WarningSuppressVelocity, False)


def setPreferencesAdvanced(
    ocl, warnSpeeds, warnRapids, warnModes, warnOCL, warnVelocity
):
    preferences().SetBool(EnableAdvancedOCLFeatures, ocl)
    preferences().SetBool(WarningSuppressAllSpeeds, warnSpeeds)
    preferences().SetBool(WarningSuppressRapidSpeeds, warnRapids)
    preferences().SetBool(WarningSuppressSelectionMode, warnModes)
    preferences().SetBool(WarningSuppressOpenCamLib, warnOCL)
    preferences().SetBool(WarningSuppressVelocity, warnVelocity)


def lastFileToolLibrary():
    filename = preferences().GetString(LastFileToolLibrary)
    if filename.endswith(".fctl") and os.path.isfile(filename):
        return filename

    libpath = preferences().GetString(
        LastPathToolLibrary, pathDefaultToolsPath("Library")
    )
    libFiles = [f for f in glob.glob(libpath + "/*.fctl")]
    libFiles.sort()
    if len(libFiles) >= 1:
        filename = libFiles[0]
        setLastFileToolLibrary(filename)
        Path.Log.track(filename)
        return filename
    else:
        return None


def setLastFileToolLibrary(path):
    Path.Log.track(path)
    if os.path.isfile(path):  # keep the path and file in sync
        preferences().SetString(LastPathToolLibrary, os.path.split(path)[0])
    return preferences().SetString(LastFileToolLibrary, path)


def lastPathToolBit():
    return preferences().GetString(LastPathToolBit, pathDefaultToolsPath("Bit"))


def setLastPathToolBit(path):
    return preferences().SetString(LastPathToolBit, path)


def lastPathToolLibrary():
    Path.Log.track()
    return preferences().GetString(LastPathToolLibrary, pathDefaultToolsPath("Library"))


def setLastPathToolLibrary(path):
    Path.Log.track(path)
    curLib = lastFileToolLibrary()
    Path.Log.debug("curLib: {}".format(curLib))
    if curLib and os.path.split(curLib)[0] != path:
        setLastFileToolLibrary("")  # a path is known but not specific file
    return preferences().SetString(LastPathToolLibrary, path)


def lastPathToolShape():
    return preferences().GetString(LastPathToolShape, pathDefaultToolsPath("Shape"))


def setLastPathToolShape(path):
    return preferences().SetString(LastPathToolShape, path)


def lastPathToolTable():
    return preferences().GetString(LastPathToolTable, "")


def setLastPathToolTable(table):
    return preferences().SetString(LastPathToolTable, table)
