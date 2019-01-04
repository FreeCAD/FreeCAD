# OpenSCAD gui init module
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up

#***************************************************************************
#*   (c) Juergen Riegel (juergen.riegel@web.de) 2002                       *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#*   Juergen Riegel 2002                                                   *
#***************************************************************************/

import FreeCAD
import FreeCADGui

param = FreeCAD.ParamGet(\
    "User parameter:BaseApp/Preferences/Mod/OpenSCAD")
openscadfilename = param.GetString('openscadexecutable')


class OpenSCADWorkbench ( Workbench ):
    "OpenSCAD workbench object"
    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/OpenSCAD/Resources/icons/OpenSCADWorkbench.svg"
        self.__class__.MenuText = "OpenSCAD"
        self.__class__.ToolTip = "OpenSCAD workbench"

    def Initialize(self):
        def QT_TRANSLATE_NOOP(scope, text):
            return text
        import OpenSCAD_rc,OpenSCADCommands
        commands=['OpenSCAD_ReplaceObject','OpenSCAD_RemoveSubtree',\
            'OpenSCAD_RefineShapeFeature',\
            'OpenSCAD_IncreaseToleranceFeature', 'OpenSCAD_Edgestofaces', \
            'OpenSCAD_ExpandPlacements','OpenSCAD_ExplodeGroup']
        toolbarcommands=['OpenSCAD_ReplaceObject','OpenSCAD_RemoveSubtree',\
            'OpenSCAD_ExplodeGroup','OpenSCAD_RefineShapeFeature']
            #'OpenSCAD_IncreaseToleranceFeature' #icon still missing
        import PartGui
        parttoolbarcommands = ['Part_CheckGeometry',"Part_Primitives",\
            "Part_Builder",'Part_Cut','Part_Fuse','Part_Common',\
            'Part_Extrude',"Part_Revolve"]
        import FreeCAD
        param = FreeCAD.ParamGet(\
            "User parameter:BaseApp/Preferences/Mod/OpenSCAD")
        openscadfilename = param.GetString('openscadexecutable')
        if not openscadfilename:

            import OpenSCADUtils
            openscadfilename = OpenSCADUtils.searchforopenscadexe()
            if openscadfilename: #automatic search was succsessful
                FreeCAD.addImportType("OpenSCAD Format (*.scad)","importCSG") 
                param.SetString('openscadexecutable',openscadfilename) #save the result
        if openscadfilename:
            commands.extend(['OpenSCAD_AddOpenSCADElement',
                'OpenSCAD_MeshBoolean','OpenSCAD_Hull','OpenSCAD_Minkowski'])
            toolbarcommands.extend(['OpenSCAD_AddOpenSCADElement',
                'OpenSCAD_MeshBoolean','OpenSCAD_Hull','OpenSCAD_Minkowski'])
        else:
            FreeCAD.Console.PrintWarning('OpenSCAD executable not found\n')

        self.appendToolbar(QT_TRANSLATE_NOOP('Workbench','OpenSCADTools'),toolbarcommands)
        self.appendMenu('OpenSCAD',commands)
        self.appendToolbar(QT_TRANSLATE_NOOP('Workbech','OpenSCAD Part tools'),parttoolbarcommands)
        #self.appendMenu('OpenSCAD',["AddOpenSCADElement"])
        ###self.appendCommandbar("&Generic Tools",["ColorCodeShape"])
        FreeCADGui.addIconPath(":/icons")
        FreeCADGui.addLanguagePath(":/translations")
        FreeCADGui.addPreferencePage(":/ui/openscadprefs-base.ui","OpenSCAD")
    def GetClassName(self):
        #return "OpenSCADGui::Workbench"
        return "Gui::PythonWorkbench"


Gui.addWorkbench(OpenSCADWorkbench())
