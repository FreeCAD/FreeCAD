# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
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
''' Post Process command that will make use of the Output File and Post Processor entries in PathJob '''
import FreeCAD
import FreeCADGui
import PathScripts
import os
import sys
from PySide import QtCore, QtGui

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class CommandPathPost:

    def GetResources(self):
        return {'Pixmap': 'Path-Post',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Post", "Post Process"),
                'Accel': "P, P",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Post", "Post Process the selected Job")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(
            translate("Path_Post", "Post Process the Selected path(s)"))
        FreeCADGui.addModule("PathScripts.PathPost")
        # select the Path Job that you want to post output from
        obj = FreeCADGui.Selection.getCompleteSelection()

        # default to the dumper post and default .tap file
        postname = "dumper"
        filename = "tmp.tap"
        postArgs = ""

        print "in activated %s" %(obj)

        # check if the user has a project and has set the default post and
        # output filename
        if hasattr(obj[0], "Group") and hasattr(obj[0], "Path"):
        #     # Check for a selected post post processor if it's set
            proj = obj[0]
        #     postobj = None
        #     for p in obj[0].Group:
        #         if p.Name == "Machine":
        #             postobj = p

            if hasattr(obj[0], "PostProcessor"):
                postobj = obj[0]

                # need to check for existance of these: obj.PostProcessor,
                # obj.OutputFile
                if postobj and postobj.PostProcessor:
                    sys.path.append(os.path.split(postobj.PostProcessor)[0])
                    lessextn = os.path.splitext(postobj.PostProcessor)[0]
                    postname = os.path.split(lessextn)[1]

                if proj.OutputFile:
                    filename = proj.OutputFile
                if hasattr(postobj, "PostProcessorArgs"):
                    postArgs = postobj.PostProcessorArgs

        postname += "_post"
        exec "import %s as current_post" % postname
        reload(current_post)
        current_post.export(obj, filename, postArgs)

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Post', CommandPathPost())

FreeCAD.Console.PrintLog("Loading PathPost... done\n")
