# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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
import FreeCADGui
import PySide.QtCore as QtCore
import PathScripts.PathPreferences as PathPreferences


class CommandToolBitSelectorOpen:
    '''
    Command to toggle the ToolBitSelector Dock
    '''

    def __init__(self):
        pass

    def GetResources(self):
        return {'Pixmap': 'Path_ToolTable',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathToolBitLibrary", "ToolBit Dock"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathToolBitLibrary", "Toggle the Toolbit Dock"),
                'Accel': "P, T",
                'CmdType': "ForEdit"}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        import PathScripts.PathToolBitLibraryGui as PathToolBitLibraryGui
        dock = PathToolBitLibraryGui.ToolBitSelector()

        lastlib = PathPreferences.lastPathToolLibrary()

        if PathPreferences.toolsOpenLastLibrary():
            dock.open(lastlib)
        else:
            dock.open()


class CommandToolBitLibraryOpen:
    '''
    Command to open ToolBitLibrary editor.
    '''

    def __init__(self):
        pass

    def GetResources(self):
        return {'Pixmap': 'Path_ToolTable',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathToolBitLibrary", "ToolBit Library editor"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathToolBitLibrary", "Open an editor to manage ToolBit libraries"),
                'CmdType': "ForEdit"}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        import PathScripts.PathToolBitLibraryGui as PathToolBitLibraryGui
        library = PathToolBitLibraryGui.ToolBitLibrary()

        lastlib = PathPreferences.lastPathToolLibrary()

        if PathPreferences.toolsOpenLastLibrary():
            library.open(lastlib)
        else:
            library.open()

# class CommandToolBitLibraryLoad:
#     '''
#     Command used to load an entire ToolBitLibrary (or part of it) from a file into a job.
#     '''

#     def __init__(self):
#         pass

#     def GetResources(self):
#         return {'Pixmap': 'Path_ToolTable',
#                 'MenuText': QtCore.QT_TRANSLATE_NOOP("PathToolBitLibrary", "Load ToolBit Library"),
#                 'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathToolBitLibrary", "Load an entire ToolBit library or part of it into a job")}

#     def selectedJob(self):
#         if FreeCAD.ActiveDocument:
#             sel = FreeCADGui.Selection.getSelectionEx()
#             if sel and sel[0].Object.Name[:3] == 'Job':
#                 return sel[0].Object
#         jobs = [o for o in FreeCAD.ActiveDocument.Objects if o.Name[:3] == 'Job']
#         if 1 == len(jobs):
#             return jobs[0]
#         return None

#     def IsActive(self):
#         return not self.selectedJob() is None

#     def Activated(self):
#         job = self.selectedJob()
#         self.Execute(job)

#     @classmethod
#     def Execute(cls, job):
#         import PathScripts.PathToolBitLibraryGui as PathToolBitLibraryGui
#         import PathScripts.PathToolControllerGui as PathToolControllerGui

#         library = PathToolBitLibraryGui.ToolBitLibrary()

#         if 1 == library.open() and job:
#             for nr, tool in library.selectedOrAllTools():
#                 tc = PathToolControllerGui.Create("TC: {}".format(tool.Label), tool, nr)
#                 job.Proxy.addToolController(tc)
#             FreeCAD.ActiveDocument.recompute()
#             return True
#         return False


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Path_ToolBitLibraryOpen', CommandToolBitLibraryOpen())
    FreeCADGui.addCommand('Path_ToolBitDock', CommandToolBitSelectorOpen())

BarList = ['Path_ToolBitDock']
MenuList = ['Path_ToolBitLibraryOpen', 'Path_ToolBitDock']

FreeCAD.Console.PrintLog("Loading PathToolBitLibraryCmd... done\n")
