# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM post extractor object task panel"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package task_post_extractor
#  \ingroup FEM
#  \brief universal task dialog for extractor objects.

from PySide import QtCore, QtGui

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from . import base_fempostpanel


class _ExtractorTaskPanel(base_fempostpanel._BasePostTaskPanel):
    """
    The TaskPanel for editing properties extractor objects. The actual UI is
    provided by the viewproviders. This allows using a universal task panel
    """

    def __init__(self, obj):
        super().__init__(obj)

        # form is used to display individual task panels
        self.form = obj.ViewObject.Proxy.get_edit_widgets(self)



