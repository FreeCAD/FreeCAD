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
param = FreeCAD.ParamGet(\
    "User parameter:BaseApp/Preferences/Mod/OpenSCAD")
openscadfilename = param.GetString('openscadexecutable')


class OpenSCADWorkbench ( Workbench ):
    "OpenSCAD workbench object"
    Icon = """
/* XPM */
static char * openscadlogo_xpm[] = {
"16 16 33 1",
" 	c None",
".	c #61320B",
"+	c #5D420B",
"@	c #4F4C09",
"#	c #564930",
"$	c #754513",
"%	c #815106",
"&	c #666509",
"*	c #875F55",
"=	c #6E7000",
"-	c #756A53",
";	c #717037",
">	c #946637",
",	c #92710E",
"'	c #797A0A",
")	c #7C7720",
"!	c #8A8603",
"~	c #88886F",
"{	c #AF8181",
"]	c #999908",
"^	c #BB8D8D",
"/	c #AAAA00",
"(	c #A9A880",
"_	c #B5B419",
":	c #C1A9A9",
"<	c #B1B19A",
"[	c #BEBE00",
"}	c #B9B8B4",
"|	c #CACC00",
"1	c #D4D4BC",
"2	c #DBD2D0",
"3	c #EEEEED",
"4	c None",
"4444444444444444",
"4444443113444444",
"4444<;']]!;<^^24",
"444(&@!]]]=&#^{3",
"44<']')@++)!&*{^",
"44)]/[|//[/]'@{{",
"42=/_|||||[]!&*{",
"4(&][|||||[/'@#}",
"3-..,|||||[)&&~4",
"^*$%.!|||[!+/](4",
"^{%%%._[[_&/[_14",
":{>%%.!//])_[_44",
"2{{%%+!]!!)]]344",
"4:{{#@&=&&@#3444",
"44224}~--~}44444",
"4444444444444444"};
"""
    MenuText = "OpenSCAD"
    ToolTip = "OpenSCAD workbench"
    def Initialize(self):
        import OpenSCAD_rc,OpenSCADCommands
        commands=['OpenSCAD_ReplaceObject','OpenSCAD_RemoveSubtree',\
            'OpenSCAD_RefineShapeFeature',"OpenSCAD_Edgestofaces",\
            'OpenSCAD_ExpandPlacements']
        toolbarcommands=['OpenSCAD_ReplaceObject','OpenSCAD_RemoveSubtree',\
            'OpenSCAD_RefineShapeFeature']
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

        self.appendToolbar("OpenSCADTools",toolbarcommands)
        self.appendMenu('OpenSCAD',commands)
        self.appendToolbar('OpenSCAD Part tools',parttoolbarcommands)
        #self.appendMenu('OpenSCAD',["AddOpenSCADElement"])
        ###self.appendCommandbar("&Generic Tools",["ColorCodeShape"])
        FreeCADGui.addIconPath(":/icons")
        FreeCADGui.addLanguagePath(":/translations")
        FreeCADGui.addPreferencePage(":/ui/openscadprefs-base.ui","OpenSCAD")
    def GetClassName(self):
        #return "OpenSCADGui::Workbench"
        return "Gui::PythonWorkbench"


Gui.addWorkbench(OpenSCADWorkbench())
