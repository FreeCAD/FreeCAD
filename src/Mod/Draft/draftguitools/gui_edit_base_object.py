# ***************************************************************************
# *   Copyright (c) 2019, 2020 Carlo Pavan <carlopav@gmail.com>             *
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
"""Provides support functions to edit Arch objects."""
## @package gui_edit_arch_objects
# \ingroup draftguitools
# \brief Provides support functions to edit Arch objects.

__title__ = "FreeCAD Draft Edit Tool"
__author__ = ("Carlo Pavan")
__url__ = "https://www.freecadweb.org"

## \addtogroup draftguitools
# @{


class GuiTools:
    """ Base class for object editing tools
    """

    def __init__(self):
        pass

    def get_edit_points(self, obj):
        """Return to Draft_Edit a list of vectors for the given object.
        Remember to use object local coordinates system.

        Parameters:
        obj: the object
        """
        pass

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        """Update the object from modified Draft_Edit point.
        No need to recompute at the end.

        Parameters:
        obj: the object
        node_idx: number of the edited node
        v: target vector of the node in object local coordinates system
        alt_edit_mode: alternative edit mode to perform different operations
                       (usually can be selected from the Draft_Edit context menu)
                       default = 0
        """
        pass

    def get_edit_point_context_menu(self, obj, node_idx):
        """ Return a list of Draft_Edit context menu actions.
        """
        pass
    

    def evaluate_context_menu_action(self, edit_command, obj, node_idx, action):
        """ Do something when a Draft_Edit context menu action is triggered over a node.
        """
        pass

    def get_object_style(self, obj):
        pass

    def set_object_editing_style(self, obj):
        pass

    def restore_object_style(self, obj, modes):
        pass

    def init_preview_object(self, obj):
        pass

    def update_preview_object(self, edit_command, obj, node_idx, v):
        pass

## @}
