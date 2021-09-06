# ***************************************************************************
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides the Fillet class for objects created with a prototype version.

The original Fillet object and Gui Command was introduced
in the development cycle of 0.19, in commit d5ca09c77b, 2019-08-22.

However, when this class was implemented, the reorganization
of the workbench was not advanced.

When the reorganization was on its way it was clear that this tool
also needed to be broken into different modules; however, this was done
only at the end of the reorganization.

In commit 01df7c0a63, 2020-02-10, the Gui Command was removed from
the graphical interface so that the user cannot create this object
graphically any more. The object class was still kept
so that previous objects created between August 2019 and February 2020
would open correctly.

Now in this module the older class is redirected to the new class
in order to migrate the object.

A new Gui Command in `draftguitools` and new make function in `draftmake`
are now used to create `Fillet` objects. Therefore, this module
is only required to migrate old objects created in that time
with the 0.19 development version.

Since this module is only used to migrate older objects, it is only temporary,
and will be removed after one year, that is, in January 2021.

The explanation of the migration methods is in the wiki page:
https://wiki.freecadweb.org/Scripted_objects_migration
"""
## @package DraftFillet
# \ingroup DRAFT
# \brief Provides Fillet class for objects created with a prototype version.
#
# This module is only required to migrate old objects created
# from August 2019 to February 2020. It will be removed definitely
# in January 2021, as the new Fillet object should be available.

import FreeCAD as App
import draftobjects.fillet
import draftobjects.base as base
from draftutils.messages import _wrn

if App.GuiUp:
    import draftviewproviders.view_fillet as view_fillet

# -----------------------------------------------------------------------------
# Removed definitions
# def _extract_edges(objs):

# def makeFillet(objs, radius=100, chamfer=False, delete=False):

# class Fillet(Draft._DraftObject):

# class CommandFillet(DraftTools.Creator):
# -----------------------------------------------------------------------------


class Fillet(base.DraftObject):
    """The old Fillet object. DEPRECATED.

    This class is solely defined to migrate older objects.

    When an old object is opened it will reconstruct the object
    by searching for this class. So we implement `onDocumentRestored`
    to test that it is the old class and we migrate it,
    by assigning the new proxy class, and the new viewprovider.
    """

    def onDocumentRestored(self, obj):
        """Run when the document that is using this class is restored."""
        if hasattr(obj, "Proxy") and obj.Proxy.Type == "Fillet":
            _module = str(obj.Proxy.__class__)
            _module = _module.lstrip("<class '").rstrip("'>")

            if _module == "DraftFillet.Fillet":
                self._migrate(obj, _module)

    def _migrate(self, obj, _module):
        """Migrate the object to the new object."""
        _wrn("v0.19, {0}, '{1}' object ".format(obj.Label, _module)
             + "will be migrated to 'draftobjects.fillet.Fillet'")

        # Save the old properties and delete them
        old_dict = _save_properties0_19_to_0_19(obj)

        # We assign the new class, which could have different properties
        # from the older class. Since we removed the older properties
        # we know the new properties will not collide with the old properties.
        # The new class itself should handle some logic so that it does not
        # add already existing properties of the same name and type.
        draftobjects.fillet.Fillet(obj)

        # Assign the old properties
        obj = _assign_properties0_19_to_0_19(obj, old_dict)

        # The same is done for the viewprovider.
        if App.GuiUp:
            vobj = obj.ViewObject
            old_dict = _save_vproperties0_19_to_0_19(vobj)
            view_fillet.ViewProviderFillet(vobj)
            _assign_vproperties0_19_to_0_19(vobj, old_dict)


def _save_properties0_19_to_0_19(obj):
    """Save the old property values and remove the old properties.

    Since we know the structure of the older Proxy class,
    we can take its old values and store them before
    we remove the property.

    We do not need to save the old properties if these
    can be recalculated from the new data.
    """
    _wrn("Old property values saved, old properties removed.")
    old_dict = dict()
    if hasattr(obj, "Length"):
        old_dict["Length"] = obj.Length
        obj.removeProperty("Length")
    if hasattr(obj, "Start"):
        old_dict["Start"] = obj.Start
        obj.removeProperty("Start")
    if hasattr(obj, "End"):
        old_dict["End"] = obj.End
        obj.removeProperty("End")
    if hasattr(obj, "FilletRadius"):
        old_dict["FilletRadius"] = obj.FilletRadius
        obj.removeProperty("FilletRadius")
    return old_dict


def _assign_properties0_19_to_0_19(obj, old_dict):
    """Assign the new properties from the old properties.

    If new properties are named differently than the older properties
    or if the old values need to be transformed because the class
    now manages differently the data, this can be done here.
    Otherwise simple assigning the old values is possible.
    """
    _wrn("New property values added.")
    if hasattr(obj, "Length"):
        obj.Length = old_dict["Length"]
    if hasattr(obj, "Start"):
        obj.Start = old_dict["Start"]
    if hasattr(obj, "End"):
        obj.End = old_dict["End"]
    if hasattr(obj, "FilletRadius"):
        obj.FilletRadius = old_dict["FilletRadius"]
    return obj


def _save_vproperties0_19_to_0_19(vobj):
    """Save the old property values and remove the old properties.

    The view provider didn't add new properties so this just returns
    an empty element.
    """
    _wrn("Old view property values saved, old view properties removed. NONE.")
    old_dict = dict()
    return old_dict


def _assign_vproperties0_19_to_0_19(vobj, old_dict):
    """Assign the new properties from the old properties.

    The view provider didn't add new properties so this just returns
    the same viewprovider.
    """
    _wrn("New view property values added. NONE.")
    return vobj
