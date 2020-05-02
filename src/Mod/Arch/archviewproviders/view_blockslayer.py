#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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

"""This module provides tools to build Wall objects.  Walls are simple
objects, usually vertical, typically obtained by giving a thickness to a base
line, then extruding it vertically.

Examples
--------
TODO put examples here.

"""

import FreeCAD,Draft,ArchComponent,DraftVecUtils,ArchCommands,math
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import draftguitools.gui_trackers as DraftTrackers
else:
    # \cond
    def translate(ctxt,txt, utf8_decode=False):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchWall
#  \ingroup ARCH
#  \brief The Wall object and tools
#
#  This module provides tools to build Wall objects.  Walls are simple objects,
#  usually vertical, typically obtained by giving a thickness to a base line,
#  then extruding it vertically.

__title__="FreeCAD Wall"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

class ViewProviderBlocksLayer(ArchComponent.ViewProviderComponent):
    """The view provider for the wall object.

    Parameters
    ----------
    vobj: <Gui.ViewProviderDocumentObject>
        The view provider to turn into a wall view provider.
    """

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj) # TODO: check
        vobj.ShapeColor = ArchCommands.getDefaultColor("Wall")

    def getIcon(self):
        """Return the path to the appropriate icon.

        If a clone, return the cloned wall icon path. Otherwise return the
        Arch wall icon.

        Returns
        -------
        str
            Path to the appropriate icon .svg file.
        """

        import Arch_rc
        if hasattr(self,"Object"):
            if self.Object.CloneOf:
                return ":/icons/Arch_Wall_Clone.svg"
            elif (not self.Object.Base) and self.Object.Additions:
                return ":/icons/Arch_Wall_Tree_Assembly.svg"
        return ":/icons/Arch_Wall_Tree.svg"

    def attach(self,vobj):
        """Add display modes' data to the coin scenegraph.

        Add each display mode as a coin node, whose parent is this view
        provider. 

        Each display mode's node includes the data needed to display the object
        in that mode. This might include colors of faces, or the draw style of
        lines. This data is stored as additional coin nodes which are children
        of the display mode node.

        Add the textures used in the Footprint display mode.
        """

        self.Object = vobj.Object
        from pivy import coin
        tex = coin.SoTexture2()
        image = Draft.loadTexture(Draft.svgpatterns()['simple'][1], 128)
        if not image is None:
            tex.image = image
        texcoords = coin.SoTextureCoordinatePlane()
        s = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetFloat("patternScale",0.01)
        texcoords.directionS.setValue(s,0,0)
        texcoords.directionT.setValue(0,s,0)
        self.fcoords = coin.SoCoordinate3()
        self.fset = coin.SoIndexedFaceSet()
        sep = coin.SoSeparator()
        sep.addChild(tex)
        sep.addChild(texcoords)
        sep.addChild(self.fcoords)
        sep.addChild(self.fset)
        vobj.RootNode.addChild(sep)
        ArchComponent.ViewProviderComponent.attach(self,vobj)

    def updateData(self,obj,prop):
        """Method called when the host object has a property changed.

        If the host object's Placement, Shape, or Material has changed, and the
        host object has a Material assigned, give the shape the color and
        transparency of the Material.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The host object that has changed.
        prop: string
            The name of the property that has changed.
        """

        if prop in ["Placement","Shape","Material"]:
            if obj.ViewObject.DisplayMode == "Footprint":
                obj.ViewObject.Proxy.setDisplayMode("Footprint")
            if hasattr(obj,"Material"):
                if obj.Material and obj.Shape:
                    if hasattr(obj.Material,"Materials"):
                        activematerials = [obj.Material.Materials[i] for i in range(len(obj.Material.Materials)) if obj.Material.Thicknesses[i] >= 0]
                        if len(activematerials) == len(obj.Shape.Solids):
                            cols = []
                            for i,mat in enumerate(activematerials):
                                c = obj.ViewObject.ShapeColor
                                c = (c[0],c[1],c[2],obj.ViewObject.Transparency/100.0)
                                if 'DiffuseColor' in mat.Material:
                                    if "(" in mat.Material['DiffuseColor']:
                                        c = tuple([float(f) for f in mat.Material['DiffuseColor'].strip("()").split(",")])
                                if 'Transparency' in mat.Material:
                                    c = (c[0],c[1],c[2],float(mat.Material['Transparency']))
                                cols.extend([c for j in range(len(obj.Shape.Solids[i].Faces))])
                            obj.ViewObject.DiffuseColor = cols
        ArchComponent.ViewProviderComponent.updateData(self,obj,prop)
        if len(obj.ViewObject.DiffuseColor) > 1:
            # force-reset colors if changed
            obj.ViewObject.DiffuseColor = obj.ViewObject.DiffuseColor

    def getDisplayModes(self,vobj):
        """Define the display modes unique to the Arch Wall.

        Define mode Footprint, which only displays the footprint of the wall.
        Also add the display modes of the Arch Component.

        Returns
        -------
        list of str
            List containing the names of the new display modes.
        """

        modes = ArchComponent.ViewProviderComponent.getDisplayModes(self,vobj) + ["Footprint"]
        return modes

    def setDisplayMode(self,mode):
        """Method called when the display mode changes.

        Called when the display mode changes, this method can be used to set
        data that wasn't available when .attach() was called.

        When Footprint is set as display mode, find the faces that make up the
        footprint of the wall, and give them a lined texture. Then display
        the wall as a wireframe.

        Then pass the displaymode onto Arch Component's .setDisplayMode().

        Parameters
        ----------
        mode: str
            The name of the display mode the view provider has switched to.

        Returns
        -------
        str:
            The name of the display mode the view provider has switched to.
        """

        self.fset.coordIndex.deleteValues(0)
        self.fcoords.point.deleteValues(0)
        if mode == "Footprint":
            if hasattr(self,"Object"):
                faces = self.Object.Proxy.getFootprint(self.Object)
                if faces:
                    verts = []
                    fdata = []
                    idx = 0
                    for face in faces:
                        tri = face.tessellate(1)
                        for v in tri[0]:
                            verts.append([v.x,v.y,v.z])
                        for f in tri[1]:
                            fdata.extend([f[0]+idx,f[1]+idx,f[2]+idx,-1])
                        idx += len(tri[0])
                    self.fcoords.point.setValues(verts)
                    self.fset.coordIndex.setValues(0,len(fdata),fdata)
            return "Wireframe"
        return ArchComponent.ViewProviderComponent.setDisplayMode(self,mode)
