# ##### BEGIN GPL LICENSE BLOCK #####
#
#  Author: Jose Luis Cercos Pita <jlcercos@gmail.com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

class ShipWorkbench ( Workbench ):
    """ @brief Workbench of Ship design module. Here toolbars & icons are append. """
    from shipUtils import Paths, Translator
    import ShipGui

    Icon = Paths.iconsPath() + "/Ico.png"
    MenuText = str(Translator.translate("Ship design"))
    ToolTip = str(Translator.translate("Ship design"))

    def Initialize(self):
        # ToolBar
        list = ["Ship_CreateShip", "Ship_OutlineDraw"]
        self.appendToolbar("Ship design",list)
        
        # Menu
        list = ["Ship_CreateShip", "Ship_OutlineDraw"]
        self.appendMenu("Ship design",list)

Gui.addWorkbench(ShipWorkbench())
