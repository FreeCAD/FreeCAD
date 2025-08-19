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
import pathlib
from collections import defaultdict
from typing import Optional


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate

PreferencesGroup = "User parameter:BaseApp/Preferences/Mod/CAM"

DefaultFilePath = "DefaultFilePath"
DefaultJobTemplate = "DefaultJobTemplate"
DefaultStockTemplate = "DefaultStockTemplate"
DefaultTaskPanelLayout = "DefaultTaskPanelLayout"

PostProcessorDefault = "PostProcessorDefault"
PostProcessorDefaultArgs = "PostProcessorDefaultArgs"
PostProcessorBlacklist = "PostProcessorBlacklist"
PostProcessorOutputFile = "PostProcessorOutputFile"
PostProcessorOutputPolicy = "PostProcessorOutputPolicy"

ToolGroup = PreferencesGroup + "/Tools"
ToolPath = "ToolPath"
LastToolLibrary = "LastToolLibrary"

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


_observers = defaultdict(list)  # maps group name to callback functions


def _add_group_observer(group, callback):
    """Add an observer for any changes on the given parameter group"""
    _observers[group].append(callback)


def _emit_change(group, *args):
    for cb in _observers[group]:
        cb(group, *args)


def preferences():
    return FreeCAD.ParamGet(PreferencesGroup)


def tool_preferences():
    return FreeCAD.ParamGet(ToolGroup)


def addToolPreferenceObserver(callback):
    _add_group_observer(ToolGroup, callback)


def pathPostSourcePath():
    return os.path.join(FreeCAD.getHomePath(), "Mod/CAM/Path/Post/")


def getBuiltinAssetPath() -> pathlib.Path:
    home = pathlib.Path(FreeCAD.getHomePath())
    return home / "Mod" / "CAM" / "Tools"


def getBuiltinLibraryPath() -> pathlib.Path:
    return getBuiltinAssetPath() / "Library"


def getBuiltinShapePath() -> pathlib.Path:
    return getBuiltinAssetPath() / "Shape"


def getBuiltinToolBitPath() -> pathlib.Path:
    return getBuiltinAssetPath() / "Bit"


def getDefaultAssetPath():
    config = pathlib.Path(FreeCAD.ConfigGet("UserConfigPath"))
    return config / "CamAssets"


def getAssetPath() -> pathlib.Path:
    pref = tool_preferences()
    default = getDefaultAssetPath()
    path = pref.GetString(ToolPath, str(default))
    return pathlib.Path(path or default)


def setAssetPath(path: pathlib.Path):
    assert path.is_dir(), f"Cannot put a non-initialized asset directory into preferences: {path}"
    if str(path) == str(getAssetPath()):
        return
    pref = tool_preferences()
    pref.SetString(ToolPath, str(path))
    _emit_change(ToolGroup, ToolPath, path)


def getToolBitPath() -> pathlib.Path:
    return getAssetPath() / "Bit"


def getLastToolLibrary() -> Optional[str]:
    pref = tool_preferences()
    return pref.GetString(LastToolLibrary) or None


def setLastToolLibrary(name: str):
    assert isinstance(name, str), f"Library name '{name}' is not a string"
    pref = tool_preferences()
    pref.SetString(LastToolLibrary, name)


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
        processor for processor in allAvailablePostProcessors() if processor not in blacklist
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


def setPreferencesAdvanced(ocl, warnSpeeds, warnRapids, warnModes, warnOCL, warnVelocity):
    preferences().SetBool(EnableAdvancedOCLFeatures, ocl)
    preferences().SetBool(WarningSuppressAllSpeeds, warnSpeeds)
    preferences().SetBool(WarningSuppressRapidSpeeds, warnRapids)
    preferences().SetBool(WarningSuppressSelectionMode, warnModes)
    preferences().SetBool(WarningSuppressOpenCamLib, warnOCL)
    preferences().SetBool(WarningSuppressVelocity, warnVelocity)
