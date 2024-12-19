# -*- coding: utf8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

"""The BIM UnClone command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_Unclone:

    def GetResources(self):
        return {
            "Pixmap": "BIM_Unclone",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Unclone", "Unclone"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Unclone",
                "Makes a selected clone object independent from its original",
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        import Draft

        # get selected object and face
        sel = FreeCADGui.Selection.getSelection()

        if len(sel) == 1:
            # make this undoable
            FreeCAD.ActiveDocument.openTransaction("Reextrude")
            obj = sel[0]

            # check that types are identical
            if hasattr(obj, "CloneOf") and obj.CloneOf:
                cloned = obj.CloneOf
                placement = FreeCAD.Placement(obj.Placement)
                if Draft.getType(obj) != Draft.getType(cloned):
                    # wrong type - we need to create a new object
                    newobj = getattr(Arch, "make" + Draft.getType(cloned))()
                else:
                    newobj = obj
                    newobj.CloneOf = None

                # copy properties over, except special ones
                for prop in cloned.PropertiesList:
                    if not prop in [
                        "Objects",
                        "CloneOf",
                        "ExpressionEngine",
                        "HorizontalArea",
                        "Area",
                        "VerticalArea",
                        "PerimeterLength",
                        "Proxy",
                        "Shape",
                    ]:
                        setattr(newobj, prop, getattr(cloned, prop))
                        FreeCAD.ActiveDocument.recompute()
                        newobj.Placement = cloned.Placement.multiply(placement)
                # update/reset view properties too? no i think...
                # for prop in cloned.ViewObject.PropertiesList:
                #    if not prop in ["Proxy"]:
                #        setattr(newobj.ViewObject,prop,getattr(cloned.ViewObject,prop))

                # update objects relating to this one
                for parent in obj.InList:
                    for prop in parent.PropertiesList:
                        if getattr(parent, prop) == obj:
                            setattr(parent, prop, newobj)
                            FreeCAD.Console.PrintMessage(
                                "Object "
                                + parent.Label
                                + "'s reference to this object has been updated\n"
                            )
                        elif isinstance(getattr(parent, prop), list) and (
                            obj in getattr(parent, prop)
                        ):
                            if (prop == "Group") and hasattr(parent, "addObject"):
                                parent.addObject(newobj)
                            else:
                                g = getattr(parent, prop)
                                g.append(newobj)
                                setattr(parent, prop, g)
                            FreeCAD.Console.PrintMessage(
                                "Object "
                                + parent.Label
                                + "'s reference to this object has been updated\n"
                            )
                        # TODO treat PropertyLinkSub / PropertyLinkSubList DANGEROUS - toponaming

                # remove old object if needed, and relabel new object
                if newobj != obj:
                    name = obj.Name
                    label = obj.Label

                    FreeCAD.ActiveDocument.removeObject(name)
                    newobj.Label = label

                # commit changes
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()

            elif Draft.getType(obj) == "Clone":
                FreeCAD.Console.PrintError(
                    translate("BIM", "Draft Clones are not supported yet!") + "\n"
                )
            else:
                FreeCAD.Console.PrintError(
                    translate("BIM", "The selected object is not a clone") + "\n"
                )
        else:
            FreeCAD.Console.PrintError(
                translate("BIM", "Please select exactly one object") + "\n"
            )


FreeCADGui.addCommand("BIM_Unclone", BIM_Unclone())
