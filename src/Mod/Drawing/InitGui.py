# Drawing gui init module
# (c) 2003 Juergen Riegel
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up

#***************************************************************************
#*   (c) Juergen Riegel (juergen.riegel@web.de) 2002                        
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



class DrawingWorkbench ( Workbench ):
	"Drawing workbench object"
	Icon = """
			/* XPM */
			static const char *colors[]={
			"16 16 49 1",
			"Qt c None",
			".	c #B0B0B0",
			"+	c #C8C8C8",
			"@	c #CACACA",
			"#	c #CBCBCB",
			"$	c #CDCDCD",
			"%	c #D0D0D0",
			"&	c #D1D1D1",
			"*	c #D2D2D2",
			"=	c #D3D3D3",
			"-	c #D4D4D4",
			";	c #D7D7D7",
			">	c #D8D8D8",
			",	c #E6E6E6",
			"'	c #E7E7E7",
			")	c #E5E5E5",
			"!	c #E0E0E0",
			"~	c #E4E4E4",
			"{	c #DEDEDE",
			"]	c #E1E1E1",
			"^	c #DADADA",
			"/	c #CCCCCC",
			"(	c #EAEAEA",
			"_	c #EBEBEB",
			":	c #E9E9E9",
			"<	c #E8E8E8",
			"[	c #E2E2E2",
			"}	c #DDDDDD",
			"|	c #ECECEC",
			"1	c #DBDBDB",
			"2	c #EEEEEE",
			"3	c #EDEDED",
			"4	c #E3E3E3",
			"5	c #F0F0F0",
			"6	c #F1F1F1",
			"7	c #EFEFEF",
			"8	c #F2F2F2",
			"9	c #F3F3F3",
			"0	c #D6D6D6",
			"a	c #F4F4F4",
			"b	c #F5F5F5",
			"c	c #F7F7F7",
			"d	c #F6F6F6",
			"e	c #F8F8F8",
			"f	c #F9F9F9",
			"g	c #CFCFCF",
			"h	c #B3B3B3",
			"i	c #CECECE",
			"j	c #BBBBBB",
			"                ",
			"                ",
			" .+@#$%&*=-;>>$ ",
			" #,'')!)~{!]!{^ ",
			" /(___(::<',~[} ",
			" /||||_(:<')~[1 ",
			" /22223|_(<')4^ ",
			" $5665723_(<,~; ",
			" $89986523_:')0 ",
			" /abba9673|(<)- ",
			" $bccda852|(<,* ",
			" $defcb852|(<,% ",
			" /bccda852|(<,g ",
			" hggggiiigiiiij ",
			"                ",
			"                "};
			"""
	MenuText = "Drawing"
	ToolTip = "Drawing workbench"

	def Initialize(self):
		# load the module
		import DrawingGui
	def GetClassName(self):
		return "DrawingGui::Workbench"
        
Gui.addWorkbench(DrawingWorkbench())

# Append the open handler
FreeCAD.addImportType("Drawing (*.svg *.svgz)","DrawingGui")
FreeCAD.addExportType("Drawing (*.svg *.svgz *.dxf)","DrawingGui")
