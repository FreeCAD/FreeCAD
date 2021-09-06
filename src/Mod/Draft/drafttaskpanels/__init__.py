# ***************************************************************************
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Modules that define classes that handle task panels of the GuiCommands.

These classes load `.ui` files that will be used in the task panels
of the graphical commands defined in the `draftguitools` submodules.
These classes also define the callbacks and interactions of the widgets
in the `.ui` files.

The task panel interface should be, in the possible, not manually written
in Python. Rather a `.ui` file should be built using Qt Designer,
and should be placed in the `Resources/ui/` directory so that it is loaded
by the respective task panel class.

::

    class CommandTaskPanel:
        def __init__(self):
            ui_file = ":/ui/TaskPanel_OrthoArray.ui"
            self.form = Gui.PySideUic.loadUi(ui_file)

At the moment, most GuiCommands don't have their own `.ui` file,
nor task panel class. Instead their task panels are completely defined
by the `DraftGui` module.

This should be changed in the future because `DraftGui`
is a very large module, and creates many different widgets
and handles many callbacks, making it difficult to extend and maintain.
This module selectively chooses the widgets to show and to hide
depending on the command that is activated.

Individual task panel classes and `.ui` files are more maintainable
because changes can be done to a single tool without affecting the rest,
and the module size is kept small.
"""
## \defgroup drafttaskpanels drafttaskpanels
# \ingroup DRAFT
# \brief Modules with classes that handle task panels of the GuiCommands.
