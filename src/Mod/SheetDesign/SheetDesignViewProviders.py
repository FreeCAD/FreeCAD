# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""View provider logic of the SheetDesign module.

The features use a Python-capable view provider. The proxy class here gives each feature its
icon and opens its task panel for an edit.
"""

import FreeCADGui

# The icons and the .ui files are in the resource of this library. A document can open when
# a different workbench is active, thus this module loads the library itself.
import SheetDesignGui  # noqa: F401

ICONS = {
    "SheetDesign::Convert": "SheetDesign_Convert",
    "SheetDesign::Form": "SheetDesign_Form",
    "SheetDesign::Perforation": "SheetDesign_Perforation",
    "SheetDesign::Extend": "SheetDesign_Extend",
    "SheetDesign::Fold": "SheetDesign_Fold",
    "SheetDesign::Hem": "SheetDesign_Hem",
    "SheetDesign::CornerRelief": "SheetDesign_CornerRelief",
    "SheetDesign::BaseShape": "SheetDesign_BaseShape",
    "SheetDesign::BasePanel": "SheetDesign_Base",
    "SheetDesign::Flange": "SheetDesign_Flange",
    "SheetDesign::Cut": "SheetDesign_Cut",
    "SheetDesign::Refold": "SheetDesign_Refold",
    "SheetDesign::Unfold": "SheetDesign_Unfold",
    "SheetDesign::FlatPattern": "SheetDesign_FlatPattern",
}


class ViewProvider:
    """Proxy for the view provider of a Sheet Design feature."""

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.vobj = vobj

    def getIcon(self):
        name = ICONS.get(self.vobj.Object.TypeId)
        return f":/icons/{name}.svg" if name else ""

    def setEdit(self, vobj, mode=0):
        if mode != 0:
            raise NotImplementedError  # The C++ view provider does the other edit modes.
        import SheetDesignTaskPanels

        if FreeCADGui.Control.activeDialog():
            return False
        panel = SheetDesignTaskPanels.panel_for(vobj.Object)
        if panel is None:
            return False
        global ACTIVE_PANEL
        ACTIVE_PANEL = panel
        FreeCADGui.Control.showDialog(panel)
        return True

    def unsetEdit(self, _vobj, mode=0):
        if mode != 0:
            raise NotImplementedError
        FreeCADGui.Control.closeDialog()
        return True

    def onDelete(self, vobj, _subelements):
        """A flat pattern of the deleted feature goes back to the feature before it."""
        obj = vobj.Object
        source = source_of(obj)
        if source is not None:
            for other in obj.Document.Objects:
                if other.isDerivedFrom("SheetDesign::FlatPattern") and other.Source == obj:
                    other.Source = source
            users = [o for o in obj.InList if o.isDerivedFrom("SheetDesign::Feature")]
            if not users and source.ViewObject is not None:
                source.ViewObject.show()
        return True

    def doubleClicked(self, vobj):
        FreeCADGui.ActiveDocument.setEdit(vobj.Object.Name, 0)
        return True

    def dumps(self):
        return None

    def loads(self, _state):
        return None


def attach(obj):
    """Give a feature its proxy. Returns the feature."""
    if obj.ViewObject is not None and obj.TypeId in ICONS:
        ViewProvider(obj.ViewObject)
    return obj


#: The task panel that the last edit opened.
ACTIVE_PANEL = None

#: Features that a command made and that wait for their task panel: name -> names of the objects
#: that the command hid. The transaction of the command is open for these features.
PENDING = {}


def edit(obj, created=False, hidden=()):
    """Open the task panel of a feature.

    With created, the transaction of the command that made the feature is open. The panel
    takes it: OK commits it, and Cancel aborts it, thus Cancel removes the new feature.
    """
    doc = obj.Document
    if created:
        PENDING[obj.Name] = [item.Name for item in hidden]
    FreeCADGui.ActiveDocument.setEdit(obj.Name, 0)
    if created and PENDING.pop(obj.Name, None) is not None:
        # No panel took the transaction.
        doc.commitTransaction()


def source_of(obj):
    """The sheet design feature that a feature continues, or None."""
    source = getattr(obj, "Source", None)
    if source is None and getattr(obj, "Edge", None):
        source = obj.Edge[0]
    return source
