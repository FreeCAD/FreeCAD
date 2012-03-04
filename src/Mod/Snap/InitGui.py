#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *  
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

class SnapWorkbench ( Workbench ):
    """ @brief Workbench of Ship design module. Here toolbars & icons are append. """
    from snapUtils import Paths, Translator
    import SnapGui, Tools

    Icon     = Paths.iconsPath() + "/Ico.png"
    MenuText = str(Translator.translate("Snap tools"))
    ToolTip  = str(Translator.translate("Snap tools"))

    def Initialize(self):
        # ToolBar
        list = ["Snap_Toolbar", "Snap_Test"]
        self.appendToolbar("Snap tools",list)
        
        # Menu
        list = ["Snap_Toolbar", "Snap_Test"]
        self.appendMenu("Snap tools",list)
Gui.addWorkbench(SnapWorkbench())
