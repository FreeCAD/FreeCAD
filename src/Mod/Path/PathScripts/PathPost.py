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
from PathScripts.PathPostProcessor import PostProcessor
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

    DefaultOutputFile = "DefaultOutputFile"
    DefaultOutputPolicy = "DefaultOutputPolicy"

    @classmethod
    def saveDefaults(cls, path, policy):
        preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        preferences.SetString(cls.DefaultOutputFile, path)
        preferences.SetString(cls.DefaultOutputPolicy, policy)

    @classmethod
    def defaultOutputFile(cls):
        preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        return preferences.GetString(cls.DefaultOutputFile, "")

    @classmethod
    def defaultOutputPolicy(cls):
        preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        return preferences.GetString(cls.DefaultOutputPolicy, "")

    def resolveFileName(self, job):
        path = "tmp.tap"
        if job.OutputFile:
            path = job.OutputFile
        filename = path
        if '%D' in filename:
            D = FreeCAD.ActiveDocument.FileName
            if D:
                D = os.path.dirname(D)
            else:
                FreeCAD.Console.PrintError("Please save document in order to resolve output path!\n")
                return None
            filename = filename.replace('%D', D)

        if '%d' in filename:
            d = FreeCAD.ActiveDocument.Label
            filename = filename.replace('%d', d)

        if '%j' in filename:
            j = job.Label
            filename = filename.replace('%j', j)

        if '%M' in filename:
            pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro")
            M = pref.GetString("MacroPath", FreeCAD.getUserAppDataDir())
            filename = filename.replace('%M', M)

        policy = job.OutputPolicy
        if not policy or policy == 'Use default':
            policy = self.defaultOutputPolicy()

        openDialog = policy == 'Open File Dialog'
        if os.path.isdir(filename) or not os.path.isdir(os.path.dirname(filename)):
            # Either the entire filename resolves into a directory or the parent directory doesn't exist.
            # Either way I don't know what to do - ask for help
            openDialog = True

        if os.path.isfile(filename) and not openDialog:
            if policy == 'Open File Dialog on conflict':
                openDialog = True
            elif policy == 'Append Unique ID on conflict':
                fn, ext = os.path.splitext(filename)
                nr = fn[-3:]
                n = 1
                if nr.isdigit():
                    n = int(nr)
                while os.path.isfile("%s%03d%s" % (fn, n, ext)):
                    n = n + 1
                filename = "%s%03d%s" % (fn, n, ext)

        if openDialog:
            foo = QtGui.QFileDialog.getSaveFileName(QtGui.qApp.activeWindow(), "Output File", filename)
            if foo:
                filename = foo[0]
            else:
                filename = None

        #print("resolveFileName(%s, %s) -> '%s'" % (path, policy, filename))
        return filename

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
        postArgs = ""

        print "in activated %s" %(obj)

        # check if the user has a project and has set the default post and
        # output filename
        if hasattr(obj[0], "Group") and hasattr(obj[0], "Path"):
        #     # Check for a selected post post processor if it's set
            job = obj[0]

            if hasattr(obj[0], "PostProcessor"):
                postobj = obj[0]

                # need to check for existance of these: obj.PostProcessor,
                # obj.OutputFile
                if postobj and postobj.PostProcessor:
                    sys.path.append(os.path.split(postobj.PostProcessor)[0])
                    lessextn = os.path.splitext(postobj.PostProcessor)[0]
                    postname = os.path.split(lessextn)[1]

                if hasattr(postobj, "PostProcessorArgs"):
                    postArgs = postobj.PostProcessorArgs

        filename = self.resolveFileName(job)
        if filename:
            processor = PostProcessor.load(postname)
            processor.export(obj, filename, postArgs)

            FreeCAD.ActiveDocument.commitTransaction()
        else:
            FreeCAD.ActiveDocument.abortTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Post', CommandPathPost())

FreeCAD.Console.PrintLog("Loading PathPost... done\n")
