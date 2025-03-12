# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""Misc Arch util commands"""


import os
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")




class Arch_Add:
    "the Arch Add command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_Add',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Add","Add component"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Add","Adds the selected components to the active object")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v and len(FreeCADGui.Selection.getSelection()) > 1

    def Activated(self):
        import Draft
        import Arch
        sel = FreeCADGui.Selection.getSelection()
        if Draft.getType(sel[-1]) == "Space":
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Add space boundary"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("Arch.addSpaceBoundaries( FreeCAD.ActiveDocument."+sel[-1].Name+", FreeCADGui.Selection.getSelectionEx() )")
        elif Draft.getType(sel[-1]).startswith("Ifc"):
            FreeCADGui.addModule("nativeifc.ifc_tools")
            for s in sel[:-1]:
                FreeCADGui.doCommand("nativeifc.ifc_tools.aggregate(FreeCAD.ActiveDocument."+s.Name+",FreeCAD.ActiveDocument."+sel[-1].Name+")")
        else:
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Grouping"))
            if not Arch.mergeCells(sel):
                host = sel.pop()
                ss = "["
                for o in sel:
                    if len(ss) > 1:
                        ss += ","
                    ss += "FreeCAD.ActiveDocument."+o.Name
                ss += "]"
                FreeCADGui.addModule("Arch")
                FreeCADGui.doCommand("Arch.addComponents("+ss+",FreeCAD.ActiveDocument."+host.Name+")")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class Arch_Remove:
    "the Arch Add command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_Remove',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Remove","Remove component"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Remove","Remove the selected components from their parents, or create a hole in a component")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v and bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        import Draft
        sel = FreeCADGui.Selection.getSelection()
        if Draft.getType(sel[-1]) == "Space":
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Remove space boundary"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("Arch.removeSpaceBoundaries( FreeCAD.ActiveDocument."+sel[-1].Name+", FreeCADGui.Selection.getSelection() )")
        elif Draft.getType(sel[-1]).startswith("Ifc"):
            FreeCADGui.addModule("nativeifc.ifc_tools")
            for s in sel[:-1]:
                FreeCADGui.doCommand("nativeifc.ifc_tools.aggregate(FreeCAD.ActiveDocument."+s.Name+",FreeCAD.ActiveDocument."+sel[-1].Name+",mode='opening')")
        else:
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Ungrouping"))
            if len(sel) > 1:
                host = sel.pop()
                ss = "["
                for o in sel:
                    if len(ss) > 1:
                        ss += ","
                    ss += "FreeCAD.ActiveDocument."+o.Name
                ss += "]"
                FreeCADGui.addModule("Arch")
                FreeCADGui.doCommand("Arch.removeComponents("+ss+",FreeCAD.ActiveDocument."+host.Name+")")
            else:
                FreeCADGui.addModule("Arch")
                FreeCADGui.doCommand("Arch.removeComponents(FreeCAD.ActiveDocument."+sel[0].Name+")")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class Arch_SplitMesh:
    "the Arch SplitMesh command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_SplitMesh',
                'MenuText': QT_TRANSLATE_NOOP("Arch_SplitMesh","Split Mesh"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_SplitMesh","Splits selected meshes into independent components")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v and bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        import Arch
        if FreeCADGui.Selection.getSelection():
            sel = FreeCADGui.Selection.getSelection()
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Split Mesh"))
            for obj in sel:
                n = obj.Name
                nobjs = Arch.splitMesh(obj)
                if len(nobjs) > 1:
                    g = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup",n)
                    for o in nobjs:
                        g.addObject(o)
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


class Arch_MeshToShape:
    "the Arch MeshToShape command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_MeshToShape',
                'MenuText': QT_TRANSLATE_NOOP("Arch_MeshToShape","Mesh to Shape"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_MeshToShape","Turns selected meshes into Part Shape objects")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v and bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        import Arch
        from draftutils import params
        if FreeCADGui.Selection.getSelection():
            f = FreeCADGui.Selection.getSelection()[0]
            g = None
            if f.isDerivedFrom("App::DocumentObjectGroup"):
                g = f
                FreeCADGui.Selection.clearSelection()
                for o in f.OutList:
                    FreeCADGui.Selection.addSelection(o)
            else:
                if f.InList:
                    if f.InList[0].isDerivedFrom("App::DocumentObjectGroup"):
                        g = f.InList[0]
            fast = params.get_param_arch("ConversionFast")
            tol = params.get_param_arch("ConversionTolerance")
            flat = params.get_param_arch("ConversionFlat")
            cut = params.get_param_arch("ConversionCut")
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Mesh to Shape"))
            for obj in FreeCADGui.Selection.getSelection():
                newobj = Arch.meshToShape(obj,True,fast,tol,flat,cut)
                if g and newobj:
                    g.addObject(newobj)
            FreeCAD.ActiveDocument.commitTransaction()

class Arch_SelectNonSolidMeshes:
    "the Arch SelectNonSolidMeshes command definition"

    def GetResources(self):
        return {'Pixmap': 'Arch_SelectNonManifold.svg',
                'MenuText': QT_TRANSLATE_NOOP("Arch_SelectNonSolidMeshes","Select non-manifold meshes"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_SelectNonSolidMeshes","Selects all non-manifold meshes from the document or from the selected groups")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        msel = []
        if FreeCADGui.Selection.getSelection():
            for o in FreeCADGui.Selection.getSelection():
                if o.isDerivedFrom("App::DocumentObjectGroup"):
                    msel.extend(o.OutList)
        if not msel:
            msel = FreeCAD.ActiveDocument.Objects
        sel = []
        for o in msel:
            if o.isDerivedFrom("Mesh::Feature"):
                if (not o.Mesh.isSolid()) or o.Mesh.hasNonManifolds():
                    sel.append(o)
        if sel:
            FreeCADGui.Selection.clearSelection()
            for o in sel:
                FreeCADGui.Selection.addSelection(o)


class Arch_RemoveShape:
    "the Arch RemoveShape command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_RemoveShape',
                'MenuText': QT_TRANSLATE_NOOP("Arch_RemoveShape","Remove Shape from BIM"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_RemoveShape","Removes cubic shapes from BIM components")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v and bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        import Arch
        sel = FreeCADGui.Selection.getSelection()
        Arch.removeShape(sel)


class Arch_CloseHoles:
    "the Arch CloseHoles command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_CloseHoles',
                'MenuText': QT_TRANSLATE_NOOP("Arch_CloseHoles","Close holes"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_CloseHoles","Closes holes in open shapes, turning them solids")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v and bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        import Arch
        for o in FreeCADGui.Selection.getSelection():
            s = Arch.closeHole(o.Shape)
            if s:
                o.Shape = s


class Arch_Check:
    "the Arch Check command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Check',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Check","Check"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Check","Checks the selected objects for problems")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v and bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        import Arch
        result = Arch.check(FreeCADGui.Selection.getSelection())
        if not result:
            FreeCAD.Console.PrintMessage(str(translate("Arch","All good! No problems found")))
        else:
            FreeCADGui.Selection.clearSelection()
            for i in result:
                FreeCAD.Console.PrintWarning("Object "+i[0].Name+" ("+i[0].Label+") "+i[1])
                FreeCADGui.Selection.addSelection(i[0])


class Arch_Survey:
    "the Arch Survey command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_Survey',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Survey","Survey"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Survey","Starts survey")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommandGui("Arch.survey()")


class Arch_ToggleIfcBrepFlag:
    "the Toggle IFC B-rep flag command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_ToggleIfcBrepFlag',
                'MenuText': QT_TRANSLATE_NOOP("Arch_ToggleIfcBrepFlag","Toggle IFC B-rep flag"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_ToggleIfcBrepFlag","Force an object to be exported as B-rep or not")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v and bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        import Arch
        for o in FreeCADGui.Selection.getSelection():
            Arch.toggleIfcBrepFlag(o)


class Arch_Component:
    "the Arch Component command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_Component',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Component","Component"),
                'Accel': "C, M",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Component","Creates an undefined architectural component")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Component"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.addModule("Draft")
            FreeCADGui.Control.closeDialog()
            for o in sel:
                FreeCADGui.doCommand("obj = Arch.makeComponent(FreeCAD.ActiveDocument."+o.Name+")")
                FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


class Arch_CloneComponent:
    "the Arch Clone Component command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_Component_Clone',
                'MenuText': QT_TRANSLATE_NOOP("Arch_CloneComponent","Clone component"),
                'Accel': "C, C",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_CloneComponent","Clones an object as an undefined architectural component")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Component"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.addModule("Draft")
            FreeCADGui.Control.closeDialog()
            for o in sel:
                FreeCADGui.doCommand("obj = Arch.cloneComponent(FreeCAD.ActiveDocument."+o.Name+")")
                FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


class Arch_IfcSpreadsheet:
    "the Arch Schedule command definition"

    def GetResources(self):
        return {'Pixmap': 'Arch_Schedule',
                'MenuText': QT_TRANSLATE_NOOP("Arch_IfcSpreadsheet","Create IFC spreadsheet..."),
                'Accel': "I, P",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_IfcSpreadsheet","Creates a spreadsheet to store IFC properties of an object.")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create IFC properties spreadsheet"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.Control.closeDialog()
        if sel:
            for o in sel:
                FreeCADGui.doCommand("Arch.makeIfcSpreadsheet(FreeCAD.ActiveDocument."+o.Name+")")
        else :
            FreeCADGui.doCommand("Arch.makeIfcSpreadsheet()")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class Arch_ToggleSubs:
    "the ToggleSubs command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_ToggleSubs',
                'Accel'   : 'Ctrl+Space',
                'MenuText': QT_TRANSLATE_NOOP("Arch_ToggleSubs","Toggle subcomponents"),
                'ToolTip' : QT_TRANSLATE_NOOP("Arch_ToggleSubs","Shows or hides the subcomponents of this object")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v and bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        import Draft
        mode = None
        for obj in FreeCADGui.Selection.getSelection():
            if hasattr(obj, "Subtractions"):
                for sub in obj.Subtractions:
                    if not (Draft.getType(sub) in ["Window","Roof"]):
                        if mode is None:
                            # take the first sub as base
                            mode = sub.ViewObject.isVisible()
                        if mode:
                            sub.ViewObject.hide()
                        else:
                            sub.ViewObject.show()


class Arch_MergeWalls:
    """The command definition for the Arch workbench's gui tool, Arch MergeWalls.

    A tool for merging walls.

    Join two or more walls by using the ArchWall.joinWalls() function.

    Find documentation on the end user usage of Arch Wall here:
    https://wiki.freecad.org/Arch_MergeWalls
    """

    def GetResources(self):
        """Returns a dictionary with the visual aspects of the Arch MergeWalls tool."""

        return {'Pixmap'  : 'Arch_MergeWalls',
                'MenuText': QT_TRANSLATE_NOOP("Arch_MergeWalls","Merge Walls"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_MergeWalls","Merges the selected walls, if possible")}

    def IsActive(self):
        """Determines whether or not the Arch MergeWalls tool is active.

        Inactive commands are indicated by a greyed-out icon in the menus and
        toolbars.
        """

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v and bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        """Executed when Arch MergeWalls is called.

        Call ArchWall.joinWalls() on walls selected by the user, with the
        delete option enabled. If the user has selected a single wall, check to
        see if the wall has any Additions that are walls. If so, merges these
        additions to the wall, deleting the additions.
        """

        import Draft
        walls = FreeCADGui.Selection.getSelection()
        if len(walls) == 1:
            if Draft.getType(walls[0]) == "Wall":
                ostr = "FreeCAD.ActiveDocument."+ walls[0].Name
                ok = False
                for o in walls[0].Additions:
                    if Draft.getType(o) == "Wall":
                        ostr += ",FreeCAD.ActiveDocument." + o.Name
                        ok = True
                if ok:
                    FreeCAD.ActiveDocument.openTransaction(translate("Arch","Merge Walls"))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("Arch.joinWalls(["+ostr+"],delete=True)")
                    FreeCAD.ActiveDocument.commitTransaction()
                    return
                else:
                    FreeCAD.Console.PrintWarning(translate("Arch","The selected wall contains no subwall to merge"))
                    return
            else:
                FreeCAD.Console.PrintWarning(translate("Arch","Please select only wall objects"))
                return
        for w in walls:
            if Draft.getType(w) != "Wall":
                FreeCAD.Console.PrintMessage(translate("Arch","Please select only wall objects"))
                return
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Merge Walls"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("Arch.joinWalls(FreeCADGui.Selection.getSelection(),delete=True)")
        FreeCAD.ActiveDocument.commitTransaction()


FreeCADGui.addCommand('Arch_Add',Arch_Add())
FreeCADGui.addCommand('Arch_Remove',Arch_Remove())
FreeCADGui.addCommand('Arch_SplitMesh',Arch_SplitMesh())
FreeCADGui.addCommand('Arch_MeshToShape',Arch_MeshToShape())
FreeCADGui.addCommand('Arch_SelectNonSolidMeshes',Arch_SelectNonSolidMeshes())
FreeCADGui.addCommand('Arch_RemoveShape',Arch_RemoveShape())
FreeCADGui.addCommand('Arch_CloseHoles',Arch_CloseHoles())
FreeCADGui.addCommand('Arch_Check',Arch_Check())
FreeCADGui.addCommand('Arch_Survey',Arch_Survey())
FreeCADGui.addCommand('Arch_ToggleIfcBrepFlag',Arch_ToggleIfcBrepFlag())
FreeCADGui.addCommand('Arch_Component',Arch_Component())
FreeCADGui.addCommand('Arch_CloneComponent',Arch_CloneComponent())
FreeCADGui.addCommand('Arch_IfcSpreadsheet',Arch_IfcSpreadsheet())
FreeCADGui.addCommand('Arch_ToggleSubs',Arch_ToggleSubs())
FreeCADGui.addCommand('Arch_MergeWalls',Arch_MergeWalls())
