# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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

import PathScripts
import PathScripts.PathLog as PathLog

LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

Processed = False

def Startup():
    global Processed
    if not Processed:
        PathLog.debug('Initializing PathGui')
        from PathScripts import PathAdaptiveGui
        from PathScripts import PathArray
        from PathScripts import PathComment
        from PathScripts import PathCustom
        from PathScripts import PathDeburrGui
        from PathScripts import PathDressupAxisMap
        from PathScripts import PathDressupDogbone
        from PathScripts import PathDressupDragknife
        from PathScripts import PathDressupRampEntry
        from PathScripts import PathDressupTagGui
        from PathScripts import PathDressupLeadInOut
        from PathScripts import PathDrillingGui
        from PathScripts import PathEngraveGui
        from PathScripts import PathFixture
        from PathScripts import PathHelixGui
        from PathScripts import PathHop
        from PathScripts import PathInspect
        from PathScripts import PathMillFaceGui
        from PathScripts import PathPocketGui
        from PathScripts import PathPocketShapeGui
        from PathScripts import PathPost
        from PathScripts import PathProfileContourGui
        from PathScripts import PathProfileEdgesGui
        from PathScripts import PathProfileFacesGui
        from PathScripts import PathSanity
        from PathScripts import PathSetupSheetGui
        from PathScripts import PathSimpleCopy
        from PathScripts import PathSimulatorGui
        from PathScripts import PathStop
        try:
            import ocl
            from PathScripts import PathSurfaceGui
        except ImportError:
            import FreeCAD
            FreeCAD.Console.PrintError("OpenCamLib is not working!\n")
            pass
        from PathScripts import PathToolController
        from PathScripts import PathToolControllerGui
        from PathScripts import PathToolLibraryManager
        from PathScripts import PathUtilsGui
        Processed = True
    else:
        PathLog.debug('Skipping PathGui initialisation')

