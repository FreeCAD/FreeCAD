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

class SurfWorkbench ( Workbench ):
    """ @brief Workbench of Ship design module. Here toolbars & icons are append. """
    from surfUtils import Paths, Translator
    import SurfGui

    Icon     = Paths.iconsPath() + "/Ico.png"
    MenuText = str(Translator.translate("Surface tools"))
    ToolTip  = str(Translator.translate("Surface tools"))

    def Initialize(self):
        # ToolBar
        list = ["Surf_IsoCurve", "Surf_SliceCurve", "Surf_Border", "Surf_Convert"]
        self.appendToolbar("Surface tools",list)
        
        # Menu
        list = ["Surf_IsoCurve", "Surf_SliceCurve", "Surf_Border", "Surf_Convert"]
        self.appendMenu("Surface tools",list)
Gui.addWorkbench(SurfWorkbench())
