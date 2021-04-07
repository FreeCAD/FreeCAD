# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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

from draftutils.messages import _msg
from draftutils.translate import translate
from draftobjects.layer import Layer


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

        if "OverrideShapeColorChildren" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "If it is true, the objects contained "
                                     "within this layer will adopt "
                                     "the line color of the layer")
            vobj.addProperty("App::PropertyBool",
                             "OverrideShapeColorChildren",
                             "Layer",
                             _tip)
            vobj.OverrideShapeColorChildren = True

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
        view_group = App.ParamGet("User parameter:BaseApp/Preferences/View")

        if "LineColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The line color of the objects "
                                     "contained within this layer")
            vobj.addProperty("App::PropertyColor",
                             "LineColor",
                             "Layer",
                             _tip)

            c = view_group.GetUnsigned("DefaultShapeLineColor", 255)
            vobj.LineColor = (((c >> 24) & 0xFF) / 255,
                              ((c >> 16) & 0xFF) / 255,
                              ((c >> 8) & 0xFF) / 255)

        if "ShapeColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The shape color of the objects "
                                     "contained within this layer")
            vobj.addProperty("App::PropertyColor",
                             "ShapeColor",
                             "Layer",
                             _tip)

            c = view_group.GetUnsigned("DefaultShapeColor", 4294967295)
            vobj.ShapeColor = (((c >> 24) & 0xFF) / 255,
                               ((c >> 16) & 0xFF) / 255,
                               ((c >> 8) & 0xFF) / 255)

        if "LineWidth" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The line width of the objects contained "
                                     "within this layer")
            vobj.addProperty("App::PropertyFloat",
                             "LineWidth",
                             "Layer",
                             _tip)

            width = view_group.GetInt("DefaultShapeLineWidth", 2)
            vobj.LineWidth = width

        if "DrawStyle" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The draw style of the objects contained "
                                     "within this layer")
            vobj.addProperty("App::PropertyEnumeration",
                             "DrawStyle",
                             "Layer",
                             _tip)
            vobj.DrawStyle = ["Solid", "Dashed", "Dotted", "Dashdot"]
            vobj.DrawStyle = "Solid"

        if "Transparency" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The transparency of the objects "
                                     "contained within this layer")
            vobj.addProperty("App::PropertyInteger",
                             "Transparency",
                             "Layer",
                             _tip)
            vobj.Transparency = 0

        if "LinePrintColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The line color of the objects "
                                     "contained within this layer, "
                                     "when used on a TechDraw page")
            vobj.addProperty("App::PropertyColor",
                             "LinePrintColor",
                             "Print",
                             _tip)

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

    def __getstate__(self):
        """Return a tuple of objects to save or None."""
        return None

    def __setstate__(self, state):
        """Set the internal properties from the restored state."""
        return None

    def updateData(self, obj, prop):
        """Execute when a property from the Proxy class is changed."""
        if prop == "Group":
            for _prop in ("LineColor", "ShapeColor", "LineWidth",
                          "DrawStyle", "Transparency", "Visibility"):
                self.onChanged(obj.ViewObject, _prop)

    def change_view_properties(self, vobj, prop):
        """Iterate over the contents and change the properties."""
        obj = vobj.Object

        # Return if the property does not exist
        if not hasattr(vobj, prop):
            return

        for target_obj in obj.Group:
            target_vobj = target_obj.ViewObject

            # If the override properties are not set return without change
            if prop == "LineColor" and not vobj.OverrideLineColorChildren:
                return
            elif prop == "ShapeColor" and not vobj.OverrideShapeColorChildren:
                return

            # This checks that the property exists in the target object,
            # and then sets the target property accordingly
            if hasattr(target_vobj, prop):
                setattr(target_vobj, prop, getattr(vobj, prop))
            else:
                continue

            # Use the line color for the text color if it exists
            if prop == "LineColor":
                if hasattr(target_vobj, "TextColor"):
                    target_vobj.TextColor = vobj.LineColor
                if hasattr(target_vobj, "FontColor"):
                    target_vobj.FontColor = vobj.LineColor

    def onChanged(self, vobj, prop):
        """Execute when a view property is changed."""
        if prop in ("LineColor", "ShapeColor", "LineWidth",
                    "DrawStyle", "Transparency", "Visibility"):
            self.change_view_properties(vobj, prop)

        if (prop in ("LineColor", "ShapeColor")
                and hasattr(vobj, "LineColor")
                and hasattr(vobj, "ShapeColor")):
            # This doesn't do anything to the objects inside the layer,
            # it just uses the defined Line and Shape colors
            # to paint the layer icon accordingly in the tree view
            l_color = vobj.LineColor
            s_color = vobj.ShapeColor

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

    def canDragObject(self, obj):
        """Return True to allow dragging one object from the Layer."""
        return True

    def canDragObjects(self):
        """Return True to allow dragging many objects from the Layer."""
        return True

    def dragObject(self, vobj, otherobj):
        """Remove the object that was dragged from the layer."""
        if hasattr(vobj.Object, "Group") and otherobj in vobj.Object.Group:
            group = vobj.Object.Group
            group.remove(otherobj)
            vobj.Object.Group = group
            App.ActiveDocument.recompute()

    def canDropObject(self, obj):
        """Return true to allow dropping one object.

        If the object being dropped is itself a `'Layer'`, return `False`
        to prevent dropping a layer inside a layer, at least for now.
        """
        if hasattr(obj, "Proxy") and isinstance(obj.Proxy, Layer):
            return False
        return True

    def canDropObjects(self):
        """Return true to allow dropping many objects."""
        return True

    def dropObject(self, vobj, otherobj):
        """Add object that was dropped into the Layer to the group.

        If the object being dropped is itself a `'Layer'`,
        return immediately to prevent dropping a layer inside a layer,
        at least for now.
        """
        if hasattr(otherobj, "Proxy") and isinstance(otherobj.Proxy, Layer):
            return

        obj = vobj.Object

        if hasattr(obj, "Group") and otherobj not in obj.Group:
            group = obj.Group
            group.append(otherobj)
            obj.Group = group

            # Remove from all other layers (not automatic)
            for parent in otherobj.InList:
                if (hasattr(parent, "Proxy")
                        and isinstance(parent.Proxy, Layer)
                        and otherobj in parent.Group
                        and parent != obj):
                    p_group = parent.Group
                    p_group.remove(otherobj)
                    parent.Group = p_group

            App.ActiveDocument.recompute()

    def setupContextMenu(self, vobj, menu):
        """Set up actions to perform in the context menu."""
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/button_right.svg"),
                                translate("draft", "Activate this layer"),
                                menu)
        action1.triggered.connect(self.activate)
        menu.addAction(action1)

        action2 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_SelectGroup.svg"),
                                translate("draft", "Select layer contents"),
                                menu)
        action2.triggered.connect(self.select_contents)
        menu.addAction(action2)

    def activate(self):
        """Activate the selected layer, it becomes the Autogroup."""
        if hasattr(self, "Object"):
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(self.Object)
            Gui.runCommand("Draft_AutoGroup")

    def select_contents(self):
        """Select the contents of the layer."""
        if hasattr(self, "Object"):
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
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_Layer.svg"),
                                translate("Draft", "Merge layer duplicates"),
                                menu)
        action1.triggered.connect(self.merge_by_name)
        menu.addAction(action1)
        action2 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_NewLayer.svg"),
                                translate("Draft", "Add new layer"),
                                menu)
        action2.triggered.connect(self.add_layer)
        menu.addAction(action2)

    def merge_by_name(self):
        """Merge the layers that have the same name."""
        if not hasattr(self, "Object") or not hasattr(self.Object, "Group"):
            return

        obj = self.Object

        layers = list()
        for iobj in obj.Group:
            if hasattr(iobj, "Proxy") and isinstance(iobj.Proxy, Layer):
                layers.append(iobj)

        to_delete = list()
        for layer in layers:
            # Test the last three characters of the layer's Label to see
            # if it's a number, like `'Layer017'`
            if (layer.Label[-1].isdigit()
                    and layer.Label[-2].isdigit()
                    and layer.Label[-3].isdigit()):
                # If the object inside the layer has the same Label
                # as the layer, save this object
                orig = None
                for ol in layer.OutList:
                    if ol.Label == layer.Label[:-3].strip():
                        orig = ol
                        break

                # Go into the objects that reference this layer object
                # and set the layer property with the previous `orig`
                # object found
                # Editor: when is this possible? Maybe if a layer is inside
                # another layer? Currently the code doesn't allow this
                # so maybe this was a previous behavior that was disabled
                # in `ViewProviderLayer`.
                if orig:
                    for par in layer.InList:
                        for prop in par.PropertiesList:
                            if getattr(par, prop) == layer:
                                _msg("Changed property '" + prop
                                     + "' of object " + par.Label
                                     + " from " + layer.Label
                                     + " to " + orig.Label)
                                setattr(par, prop, orig)
                    to_delete.append(layer)

        for layer in to_delete:
            if not layer.InList:
                _msg("Merging duplicate layer: " + layer.Label)
                App.ActiveDocument.removeObject(layer.Name)
            elif len(layer.InList) == 1:
                first = layer.InList[0]

                if first.isDerivedFrom("App::DocumentObjectGroup"):
                    _msg("Merging duplicate layer: " + layer.Label)
                    App.ActiveDocument.removeObject(layer.Name)
            else:
                _msg("InList not empty. "
                     "Unable to delete layer: " + layer.Label)

    def add_layer(self):
        """Creates a new layer"""
        import Draft
        Draft.make_layer()
        App.ActiveDocument.recompute()

    def __getstate__(self):
        """Return a tuple of objects to save or None."""
        return None

    def __setstate__(self, state):
        """Set the internal properties from the restored state."""
        return None


# Alias for compatibility with v0.18 and earlier
_ViewProviderVisGroup = ViewProviderLayer

## @}
