# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2021 FreeCAD Developers                                 *
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
"""Provides the viewprovider code for the Layer object."""
## @package view_layer
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for the Layer object.

## \addtogroup draftviewproviders
# @{
import pivy.coin as coin
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
from draftobjects.layer import Layer
from draftutils import params
from draftutils import utils
from draftutils.translate import translate


class ViewProviderLayer:
    """The viewprovider for the Layer object."""

    def __init__(self, vobj):
        self.Object = vobj.Object
        self.set_properties(vobj)

        vobj.Proxy = self

    def set_properties(self, vobj):
        """Set the properties only if they don't already exist."""
        properties = vobj.PropertiesList
        self.set_override_options(vobj, properties)
        self.set_visual_properties(vobj, properties)

    def set_override_options(self, vobj, properties):
        """Set property options only if they don't already exist."""
        if "OverrideLineColorChildren" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "If it is true, the objects contained "
                                     "within this layer will adopt "
                                     "the line color of the layer")
            vobj.addProperty("App::PropertyBool",
                             "OverrideLineColorChildren",
                             "Layer",
                             _tip)
            vobj.OverrideLineColorChildren = True

        if "OverrideShapeAppearanceChildren" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "If it is true, the objects contained "
                                     "within this layer will adopt "
                                     "the shape appearance of the layer")
            vobj.addProperty("App::PropertyBool",
                             "OverrideShapeAppearanceChildren",
                             "Layer",
                             _tip)
            vobj.OverrideShapeAppearanceChildren = True

        if "UsePrintColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "If it is true, the print color "
                                     "will be used when objects in this "
                                     "layer are placed on a TechDraw page")
            vobj.addProperty("App::PropertyBool",
                             "UsePrintColor",
                             "Print",
                             _tip)

    def set_visual_properties(self, vobj, properties):
        """Set visual properties only if they don't already exist."""
        if "LineColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The line color of the objects "
                                     "contained within this layer")
            vobj.addProperty("App::PropertyColor",
                             "LineColor",
                             "Layer",
                             _tip)
            vobj.LineColor = params.get_param_view("DefaultShapeLineColor") & 0xFFFFFF00

        if "ShapeColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The shape color of the objects "
                                     "contained within this layer")
            vobj.addProperty("App::PropertyColor",
                             "ShapeColor",
                             "Layer",
                             _tip,
                             4)  # Hidden
            vobj.ShapeColor = params.get_param_view("DefaultShapeColor") & 0xFFFFFF00

        if "ShapeAppearance" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The shape appearance of the objects "
                                     "contained within this layer")
            vobj.addProperty("App::PropertyMaterialList",
                             "ShapeAppearance",
                             "Layer",
                             _tip)
            material = App.Material()
            material.DiffuseColor = params.get_param_view("DefaultShapeColor") & 0xFFFFFF00
            vobj.ShapeAppearance = (material, )

        if "LineWidth" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The line width of the objects contained "
                                     "within this layer")
            vobj.addProperty("App::PropertyFloat",
                             "LineWidth",
                             "Layer",
                             _tip)
            vobj.LineWidth = params.get_param_view("DefaultShapeLineWidth")

        if "DrawStyle" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The draw style of the objects contained "
                                     "within this layer")
            vobj.addProperty("App::PropertyEnumeration",
                             "DrawStyle",
                             "Layer",
                             _tip)
            vobj.DrawStyle = utils.DRAW_STYLES
            vobj.DrawStyle = params.get_param("DefaultDrawStyle")

        if "Transparency" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The transparency of the objects "
                                     "contained within this layer")
            vobj.addProperty("App::PropertyPercent",
                             "Transparency",
                             "Layer",
                             _tip)
            vobj.Transparency = params.get_param_view("DefaultShapeTransparency")

        if "LinePrintColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The line color of the objects "
                                     "contained within this layer, "
                                     "when used on a TechDraw page")
            vobj.addProperty("App::PropertyColor",
                             "LinePrintColor",
                             "Print",
                             _tip)
            vobj.LinePrintColor = params.get_param("DefaultPrintColor")

    def getIcon(self):
        """Return the path to the icon used by the viewprovider.

        Normally it returns the basic Layer icon, but if `icondata` exists
        it is the modified icon with the line and shape colors of the layer.
        """
        if hasattr(self, "icondata"):
            return self.icondata
        return ":/icons/Draft_Layer.svg"

    def attach(self, vobj):
        """Set up the scene sub-graph of the viewprovider."""
        self.Object = vobj.Object
        sep = coin.SoGroup()
        vobj.addDisplayMode(sep, "Default")

    def claimChildren(self):
        """Return objects that will be placed under it in the tree view.

        These are the elements of the `Group` property of the Proxy object.
        """
        if hasattr(self, "Object") and hasattr(self.Object, "Group"):
            if getattr(self.Object.ViewObject, "HideChildren", False):
                return []
            return self.Object.Group

    def getDisplayModes(self, vobj):
        """Return the display modes that this viewprovider supports."""
        return ["Default"]

    def getDefaultDisplayMode(self):
        """Return the default display mode."""
        return "Default"

    def setDisplayMode(self, mode):
        """Return the saved display mode."""
        return mode

    def dumps(self):
        """Return a tuple of objects to save or None."""
        return None

    def loads(self, state):
        """Set the internal properties from the restored state."""
        return None

    def updateData(self, obj, prop):
        """Execute when a property from the Proxy class is changed."""
        if prop == "Group":
            for _prop in ("LineColor", "ShapeAppearance", "LineWidth",
                          "DrawStyle", "Visibility"):
                self.onChanged(obj.ViewObject, _prop)

    def change_view_properties(self, vobj, prop):
        """Iterate over the contents and change the properties."""
        obj = vobj.Object

        # Return if the property does not exist
        if not hasattr(vobj, prop):
            return
        # If the override properties are not set return without change
        if prop == "LineColor" and not vobj.OverrideLineColorChildren:
            return
        elif prop == "ShapeAppearance" and not vobj.OverrideShapeAppearanceChildren:
            return

        for target_obj in obj.Group:
            target_vobj = target_obj.ViewObject

            # This checks that the property exists in the target object,
            # and then sets the target property accordingly
            if hasattr(target_vobj, prop):
                setattr(target_vobj, prop, getattr(vobj, prop))

            # Use the line color for the point color and text color
            if prop == "LineColor":
                if hasattr(target_vobj, "PointColor"):
                    target_vobj.PointColor = vobj.LineColor
                if hasattr(target_vobj, "TextColor"):
                    target_vobj.TextColor = vobj.LineColor
            # Use the line width for the point size
            elif prop == "LineWidth":
                if hasattr(target_vobj, "PointSize"):
                    target_vobj.PointSize = vobj.LineWidth

    def onChanged(self, vobj, prop):
        """Execute when a view property is changed."""

        if not hasattr(vobj, prop):
            return

        if prop == "ShapeColor":
            if hasattr(vobj, "ShapeAppearance"):
                material = vobj.ShapeAppearance[0]
                if material.DiffuseColor != vobj.ShapeColor:
                    material.DiffuseColor = vobj.ShapeColor
                    vobj.ShapeAppearance = (material, )
            # The changed ShapeAppearance will do the rest:
            return

        if prop == "Transparency":
            if hasattr(vobj, "ShapeAppearance"):
                material = vobj.ShapeAppearance[0]
                if material.Transparency != vobj.Transparency / 100:
                    material.Transparency = vobj.Transparency / 100
                    vobj.ShapeAppearance = (material, )
            # The changed ShapeAppearance will do the rest:
            return

        if (prop == "ShapeAppearance"
                and hasattr(vobj, "ShapeColor")
                and hasattr(vobj, "Transparency")):
            material = vobj.ShapeAppearance[0]
            if material.DiffuseColor != vobj.ShapeColor:
                vobj.ShapeColor = material.DiffuseColor
            if material.Transparency != vobj.Transparency / 100:
                vobj.Transparency = int(material.Transparency * 100)

        if (prop in ("LineColor", "ShapeAppearance", "LineWidth",
                     "DrawStyle", "Visibility")
                and hasattr(vobj, "OverrideLineColorChildren")
                and hasattr(vobj, "OverrideShapeAppearanceChildren")):
            self.change_view_properties(vobj, prop)

        # Paint the layer icon in the tree view:
        if (prop in ("LineColor", "ShapeAppearance")
                and hasattr(vobj, "LineColor")
                and hasattr(vobj, "ShapeAppearance")):
            l_color = vobj.LineColor
            s_color = vobj.ShapeAppearance[0].DiffuseColor

            l_color = QtGui.QColor(int(l_color[0] * 255),
                                   int(l_color[1] * 255),
                                   int(l_color[2] * 255))
            s_color = QtGui.QColor(int(s_color[0] * 255),
                                   int(s_color[1] * 255),
                                   int(s_color[2] * 255))
            p1 = QtCore.QPointF(2, 17)
            p2 = QtCore.QPointF(13, 8)
            p3 = QtCore.QPointF(30, 15)
            p4 = QtCore.QPointF(20, 25)

            image = QtGui.QImage(32, 32, QtGui.QImage.Format_ARGB32)
            image.fill(QtCore.Qt.transparent)

            pt = QtGui.QPainter(image)
            pt.setBrush(QtGui.QBrush(s_color, QtCore.Qt.SolidPattern))
            pt.drawPolygon([p1, p2, p3, p4])
            pt.setPen(QtGui.QPen(l_color, 2,
                                 QtCore.Qt.SolidLine, QtCore.Qt.FlatCap))
            pt.drawPolygon([p1, p2, p3, p4])
            pt.end()

            byte_array = QtCore.QByteArray()
            buffer = QtCore.QBuffer(byte_array)
            buffer.open(QtCore.QIODevice.WriteOnly)
            image.save(buffer, "XPM")

            self.icondata = byte_array.data().decode("latin1")
            vobj.signalChangeIcon()

    def _get_layer(self, obj):
        """Get the layer the object belongs to.
        """
        from draftmake.make_layer import get_layer_container
        # First look in the LayerContainer:
        for layer in get_layer_container().Group:
            if utils.get_type(layer) == "Layer" and obj in layer.Group:
                return layer
        # If not found, look through all App::FeaturePython objects (not just layers):
        for find in obj.Document.findObjects(Type="App::FeaturePython"):
            if utils.get_type(find) == "Layer" and obj in find.Group:
                return find
        return None

    def canDragObject(self, obj):
        """Return True to allow dragging one object from the Layer.

        Also store parent group data for update_groups_after_drag_drop and
        trigger that function.
        """
        if not hasattr(self, "old_parent_data"):
            self.old_parent_data = {}
        old_data = []
        for parent in obj.InList:
            if hasattr(parent, "Group"):
                old_data.append([parent, parent.Group])
        # Layers are not in the Inlist because a layer's Group is App::PropertyLinkListHidden:
        layer = self._get_layer(obj)
        if layer is not None:
            old_data.append([layer, layer.Group])
        if old_data:
            self.old_parent_data.setdefault(obj, old_data)
            QtCore.QTimer.singleShot(0, self.update_groups_after_drag_drop)

        return True

    def canDragObjects(self):
        """Return True to allow dragging many objects from the Layer."""
        return True

    def dragObject(self, vobj, otherobj):
        """Remove the object that was dragged from the layer."""
        layer = vobj.Object
        if otherobj in layer.Group:
            group = layer.Group
            group.remove(otherobj)
            layer.Group = group
            App.ActiveDocument.recompute()

    def canDropObject(self, obj):
        """Return true to allow dropping one object.

        If the object being dropped is itself a `'Layer'`, return `False`
        to prevent dropping a layer inside a layer, at least for now.

        Also store parent group data for update_groups_after_drag_drop and
        trigger that function.
        """
        if utils.get_type(obj) == "Layer":
            return False
        return self.canDragObject(obj)

    def canDropObjects(self):
        """Return true to allow dropping many objects."""
        return True

    def dropObject(self, vobj, otherobj):
        """Add object that was dropped into the Layer to the group.

        If the object being dropped is itself a `'Layer'`,
        return immediately to prevent dropping a layer inside a layer,
        at least for now.
        """
        if utils.get_type(otherobj) == "Layer":
            return

        # We assume a single old layer...

        old_layer = self._get_layer(otherobj)
        if old_layer is not None:
            group = old_layer.Group
            group.remove(otherobj)
            old_layer.Group = group

        new_layer = vobj.Object
        if otherobj not in new_layer.Group:
            group = new_layer.Group
            group.append(otherobj)
            new_layer.Group = group

        App.ActiveDocument.recompute()

    def update_groups_after_drag_drop(self):
        """Workaround function to improve the drag and drop behavior of Layer
        objects.

        The function processes the parent group data stored in the
        old_parent_data dictionary by canDragObject and canDropObject.
        """

        # The function can be called multiple times, old_parent_data will be
        # empty after the first call.
        if (not hasattr(self, "old_parent_data")) or (not self.old_parent_data):
            return

        # List to collect parents whose Group must be updated.
        # This has to happen later in a separate loop as we need the unmodified
        # InList properties of the children in the main loop.
        parents_to_update = []

        # Main loop:
        for child, old_data in self.old_parent_data.items():

            # We assume a single old and a single new layer...

            old_layer = None
            for old_parent, old_parent_group in old_data:
                if utils.get_type(old_parent) == "Layer":
                    old_layer = old_parent
                    break

            new_layer = self._get_layer(child)
            if new_layer == old_layer:
                continue

            elif new_layer is None:
                # An object was dragged out of a layer.
                # We need to check if it was put in a new group. If that is
                # the case the content of old_layer should be restored.
                # If the object was not put in a new group it was dropped on
                # the document node, in that case we do nothing.
                old_parents = [sub[0] for sub in old_data]
                for new_parent in child.InList:
                    if (hasattr(new_parent, "Group")
                            and new_parent not in old_parents): # New group check.
                        for old_parent, old_parent_group in old_data:
                            if old_parent == old_layer:
                                parents_to_update.append([old_parent, old_parent_group])
                                break
                        break

            else:
                # A new layer was assigned.
                # The content of all `non-layer` groups should be restored.
                for old_parent, old_parent_group in old_data:
                    if utils.get_type(old_parent) != "Layer":
                        parents_to_update.append([old_parent, old_parent_group])

        # Update parents:
        if parents_to_update:
            for old_parent, old_parent_group in parents_to_update:
                old_parent.Group = old_parent_group
            App.ActiveDocument.recompute()

        self.old_parent_data = {}

    def replaceObject(self, old_obj, new_obj):
        """Return immediately to prevent replacement of children."""
        return

    def setupContextMenu(self, vobj, menu):
        """Set up actions to perform in the context menu."""
        action_activate = QtGui.QAction(QtGui.QIcon(":/icons/button_right.svg"),
                                        translate("draft", "Activate this layer"),
                                        menu)
        action_activate.triggered.connect(self.activate)
        menu.addAction(action_activate)

        action_select = QtGui.QAction(QtGui.QIcon(":/icons/Draft_SelectGroup.svg"),
                                      translate("draft", "Select layer contents"),
                                      menu)
        action_select.triggered.connect(self.select_contents)
        menu.addAction(action_select)

    def activate(self):
        """Activate the selected layer, it becomes the Autogroup."""
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(self.Object)
        if not "Draft_AutoGroup" in Gui.listCommands():
            Gui.activateWorkbench("DraftWorkbench")
        Gui.runCommand("Draft_AutoGroup")

    def select_contents(self):
        """Select the contents of the layer."""
        Gui.Selection.clearSelection()
        for layer_obj in self.Object.Group:
            Gui.Selection.addSelection(layer_obj)


class ViewProviderLayerContainer:
    """The viewprovider for the LayerContainer object."""

    def __init__(self, vobj):
        self.Object = vobj.Object
        vobj.Proxy = self

    def getIcon(self):
        """Return the path to the icon used by the viewprovider."""
        return ":/icons/Draft_Layer.svg"

    def attach(self, vobj):
        """Set up the scene sub-graph of the viewprovider."""
        self.Object = vobj.Object

    def setupContextMenu(self, vobj, menu):
        """Set up actions to perform in the context menu."""
        action_merge = QtGui.QAction(QtGui.QIcon(":/icons/Draft_Layer.svg"),
                                     translate("draft", "Merge layer duplicates"),
                                     menu)
        action_merge.triggered.connect(self.merge_by_name)
        menu.addAction(action_merge)

        action_add = QtGui.QAction(QtGui.QIcon(":/icons/Draft_NewLayer.svg"),
                                   translate("draft", "Add new layer"),
                                   menu)
        action_add.triggered.connect(self.add_layer)
        menu.addAction(action_add)

    def merge_by_name(self):
        """Merge the layers that have the same base label."""
        doc = App.ActiveDocument
        doc.openTransaction(translate("draft", "Merge layer duplicates"))

        layer_container = self.Object
        layers = []
        for obj in layer_container.Group:
            if utils.get_type(obj) == "Layer":
                layers.append(obj)

        to_delete = []
        for layer in layers:
            # Remove trailing digits (usually 3 but there might be more) and
            # trailing spaces from Label before comparing:
            base_label = layer.Label.rstrip("0123456789 ")

            # Try to find the `'base'` layer:
            base = None
            for other_layer in layers:
                if ((not other_layer in to_delete) # Required if there are duplicate labels.
                        and other_layer != layer
                        and other_layer.Label.upper() == base_label.upper()):
                    base = other_layer
                    break

            if base:
                if layer.Group:
                    base_group = base.Group
                    for obj in layer.Group:
                        if not obj in base_group:
                            base_group.append(obj)
                    base.Group = base_group
                to_delete.append(layer)
            elif layer.Label != base_label:
                layer.Label = base_label

        for layer in to_delete:
            doc.removeObject(layer.Name)

        doc.recompute()
        doc.commitTransaction()

    def add_layer(self):
        """Creates a new layer"""
        import Draft

        doc = App.ActiveDocument
        doc.openTransaction(translate("draft", "Add new layer"))

        Draft.make_layer(name=None, line_color=None, shape_color=None,
                         line_width=None, draw_style=None, transparency=None)

        doc.recompute()
        doc.commitTransaction()

    def dumps(self):
        """Return a tuple of objects to save or None."""
        return None

    def loads(self, state):
        """Set the internal properties from the restored state."""
        return None

    def replaceObject(self, old_obj, new_obj):
        """Return immediately to prevent replacement of children."""
        return


# Alias for compatibility with v0.18 and earlier
_ViewProviderVisGroup = ViewProviderLayer

## @}
