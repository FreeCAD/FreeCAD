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

class ShipWorkbench ( Workbench ):
    """ @brief Workbench of Ship design module. Here toolbars & icons are append. """
    from shipUtils import Paths, Translator
    import ShipGui

    Icon = Paths.iconsPath() + "/Ico.png"
    MenuText = str(Translator.translate("Ship design"))
    ToolTip = str(Translator.translate("Ship design"))

    def Initialize(self):
        # ToolBar
        list = ["Ship_LoadExample", "Ship_CreateShip", "Ship_OutlineDraw", "Ship_AreasCurve", "Ship_Hydrostatics"]
        self.appendToolbar("Ship design",list)
        list = ["Ship_Weights", "Ship_CreateTank", "Ship_GZ"]
        self.appendToolbar("Weights",list)
        # Simulation stuff only if pyOpenCL & numpy are present
        hasOpenCL = True
        hasNumpy  = True
        try:
            import pyopencl
        except ImportError:
            hasOpenCL = False
            msg = Translator.translate("pyOpenCL not installed, ship simulations disabled\n")
            App.Console.PrintWarning(msg)
        try:
            import numpy
        except ImportError:
            hasNumpy = False
            msg = Translator.translate("numpy not installed, ship simulations disabled\n")
            App.Console.PrintWarning(msg)
        if hasOpenCL and hasNumpy:
            list = ["Ship_CreateSim", "Ship_RunSim", "Ship_StopSim"]
            self.appendToolbar("Simulation",list)
        
        # Menu
        list = ["Ship_LoadExample", "Ship_CreateShip", "Ship_OutlineDraw", "Ship_AreasCurve", "Ship_Hydrostatics"]
        self.appendMenu("Ship design",list)
        list = ["Ship_Weights", "Ship_CreateTank", "Ship_GZ"]
        self.appendMenu("Weights",list)
        if hasOpenCL and hasNumpy:
            list = ["Ship_CreateSim", "Ship_RunSim", "Ship_StopSim"]
            self.appendMenu("Simulation",list)

Gui.addWorkbench(ShipWorkbench())
