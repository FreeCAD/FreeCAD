# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************
''' Post Process command that will make use of the Output File and Post Processor entries in PathProject '''
import FreeCAD, FreeCADGui
import Path, PathScripts,PathGui
from PathScripts import PostUtils
from PathScripts import PathProject
import os,sys
from PySide import QtCore,QtGui

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
        return {'Pixmap'  : 'Path-Post',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathPost","Post Process"),
                'Accel': "P, P",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathPost","Post Process the selected Project")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(translate("PathPost","Post Process the Selected path(s)"))
        FreeCADGui.addModule("PathScripts.PathPost")
        #select the PathProject that you want to post output from
        obj = FreeCADGui.Selection.getSelection()

        #default to the dumper post and default .tap file
        postname = "dumper_post"
        filename = "tmp.tap"

        #check if the user has a project and has set the default post and output filename
        if hasattr(obj[0],"Group") and hasattr(obj[0],"Path"):
            #Check for a machine and use the post processor if it's set
            proj = obj[0]
            postobj = None
            for p in obj[0].Group:
                if p.Name == "Machine":
                    postobj = p
                
            #need to check for existance of these: obj.PostProcessor, obj.OutputFile
            if postobj and postobj.PostProcessor:
                sys.path.append(os.path.split(postobj.PostProcessor)[0])
                lessextn = os.path.splitext(postobj.PostProcessor)[0]
                postname = os.path.split(lessextn)[1]
    
            if proj.OutputFile:
                filename = proj.OutputFile
        
        exec "import %s as current_post" % postname
        current_post.export(obj,filename)

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Post',CommandPathPost())

FreeCAD.Console.PrintLog("Loading PathPost... done\n")
