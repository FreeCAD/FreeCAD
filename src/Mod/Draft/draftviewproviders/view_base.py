# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides the viewprovider code for the base Draft object.

Many viewprovider classes may inherit this class in order to have
the same basic behavior."""
## @package view_base
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for the base Draft object.

## \addtogroup draftviewproviders
# @{
import PySide.QtCore as QtCore
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

if App.GuiUp:
    from pivy import coin
    import FreeCADGui as Gui
    import Draft_rc
    # The module is used to prevent complaints from code checkers (flake8)
    bool(Draft_rc.__name__)


class ViewProviderDraft(object):
    """The base class for Draft view providers.

    Parameters
    ----------
    vobj : a base C++ view provider
        The view provider of the scripted object (`obj.ViewObject`),
        which commonly may be of types `PartGui::ViewProvider2DObjectPython`,
        `PartGui::ViewProviderPython`, or `Gui::ViewProviderPythonFeature`.

        A basic view provider is instantiated during the creation
        of the base C++ object, for example,
        `Part::Part2DObjectPython`, `Part::FeaturePython`,
        or `App::FeaturePython`.

            >>> obj = App.ActiveDocument.addObject('Part::Part2DObjectPython')
            >>> vobj = obj.ViewObject
            >>> ViewProviderDraft(vobj)

        This view provider class instance is stored in the `Proxy` attribute
        of the base view provider.
        ::
            vobj.Proxy = self

    Attributes
    ----------
    Object : the base C++ object
        The scripted document object that is associated
        with this view provider, which commonly may be of types
        `Part::Part2DObjectPython`, `Part::FeaturePython`,
        or `App::FeaturePython`.

    texture : coin.SoTexture2
        A texture that could be added to this object.

    texcoords : coin.SoTextureCoordinatePlane
        The coordinates defining a plane to use for aligning the texture.

    These class attributes are accessible through the `Proxy` object:
    `vobj.Proxy.Object`, `vobj.Proxy.texture`, etc.
    """

    def __init__(self, vobj):
        self.Object = vobj.Object
        self.texture = None
        self.texcoords = None

        self._set_properties(vobj)
        # This class is assigned to the Proxy attribute
        vobj.Proxy = self

    def _set_properties(self, vobj):
        """Set the properties of objects if they don't exist."""
        if not hasattr(vobj, "Pattern"):
            _tip = "Defines a hatch pattern."
            vobj.addProperty("App::PropertyEnumeration",
                             "Pattern",
                             "Draft",
                             QT_TRANSLATE_NOOP("App::Property", _tip))
            vobj.Pattern = ["None"] + list(utils.svg_patterns().keys())

        if not hasattr(vobj, "PatternSize"):
            _tip = "Defines the size of the hatch pattern."
            vobj.addProperty("App::PropertyFloat",
                             "PatternSize",
                             "Draft",
                             QT_TRANSLATE_NOOP("App::Property", _tip))
            vobj.PatternSize = utils.get_param("HatchPatternSize", 1)

    def __getstate__(self):
        """Return a tuple of all serializable objects or None.

        When saving the document this view provider object gets stored
        using Python's `json` module.

        Since we have some un-serializable objects (Coin objects) in here
        we must define this method to return a tuple of all serializable
        objects or `None`.

        Override this method to define the serializable objects to return.

        By default it returns `None`.

        Returns
        -------
        None
        """
        return None

    def __setstate__(self, state):
        """Set some internal properties for all restored objects.

        When a document is restored this method is used to set some properties
        for the objects stored with `json`.

        Override this method to define the properties to change for the
        restored serialized objects.

        By default no objects were serialized with `__getstate__`,
        so nothing needs to be done here, and it returns `None`.

        Parameters
        ----------
        state : state
            A serialized object.

        Returns
        -------
        None
        """
        return None

    def attach(self, vobj):
        """Set up the scene sub-graph of the view provider.

        This method should always be defined, even if it does nothing.

        Override this method to set up a custom scene.

        Parameters
        ----------
        vobj : the view provider of the scripted object.
            This is `obj.ViewObject`.
        """
        self.texture = None
        self.texcoords = None
        self.Object = vobj.Object
        self.onChanged(vobj, "Pattern")
        return

    def updateData(self, obj, prop):
        """Run when an object property is changed.

        Override this method to handle the behavior of the view provider
        depending on changes that occur to the real object's properties.

        By default, no property is tested, and it does nothing.

        Parameters
        ----------
        obj : the base C++ object
            The scripted document object that is associated
            with this view provider, which commonly may be of types
            `Part::Part2DObjectPython`, `Part::FeaturePython`,
            or `App::FeaturePython`.

        prop : str
            Name of the property that was modified.
        """
        return

    def getDisplayModes(self, vobj):
        """Return a list of display modes.

        Override this method to return a list of strings with
        display mode styles, such as `'Flat Lines'`, `'Shaded'`,
        `'Wireframe'`, `'Points'`.

        By default it returns an empty list.

        Parameters
        ----------
        vobj : the view provider of the scripted object.
            This is `obj.ViewObject`.

        Returns
        -------
        list
            Empty list `[ ]`
        """
        modes = []
        return modes

    def getDefaultDisplayMode(self):
        """Return the default mode defined in getDisplayModes.

        Override this method to return a string with the default display mode.

        By default it returns `'Flat Lines'`.

        Returns
        -------
        str
            `'Flat Lines'`

        """
        return "Flat Lines"

    def setDisplayMode(self, mode):
        """Map the modes defined in attach with those in getDisplayModes.

        This method is optional.

        By default since they have the same names nothing needs to be done,
        and it just returns the input `mode`.

        Parameters
        ----------
        str
            A string defining a display mode such as
            `'Flat Lines'`, `'Shaded'`, `'Wireframe'`, `'Points'`.
        """
        return mode

    def onChanged(self, vobj, prop):
        """Run when a view property is changed.

        Override this method to handle the behavior
        of the view provider depending on changes that occur to its properties
        such as line color, line width, point color, point size,
        draw style, shape color, transparency, and others.

        This method  updates the texture and pattern if
        the properties `TextureImage`, `Pattern`, `DiffuseColor`,
        and `PatternSize` change.

        Parameters
        ----------
        vobj : the view provider of the scripted object.
            This is `obj.ViewObject`.

        prop : str
            Name of the property that was modified.
        """
        # treatment of patterns and image textures
        if prop in ("TextureImage", "Pattern", "DiffuseColor"):
            if hasattr(self.Object, "Shape"):
                if self.Object.Shape.Faces:
                    path = None
                    if hasattr(vobj, "TextureImage"):
                        if vobj.TextureImage:
                            path = vobj.TextureImage
                    if not path:
                        if hasattr(vobj, "Pattern"):
                            if str(vobj.Pattern) in list(utils.svg_patterns().keys()):
                                path = utils.svg_patterns()[vobj.Pattern][1]
                            else:
                                path = "None"
                    if path and vobj.RootNode:
                        if vobj.RootNode.getChildren().getLength() > 2:
                            if vobj.RootNode.getChild(2).getChildren().getLength() > 0:
                                innodes = vobj.RootNode.getChild(2).getChild(0).getChildren().getLength()
                                if  innodes > 2:
                                    r = vobj.RootNode.getChild(2).getChild(0).getChild(innodes-1)
                                    i = QtCore.QFileInfo(path)
                                    if self.texture:
                                        r.removeChild(self.texture)
                                        self.texture = None
                                    if self.texcoords:
                                        r.removeChild(self.texcoords)
                                        self.texcoords = None
                                    if i.exists():
                                        size = None
                                        if ".SVG" in path.upper():
                                            size = utils.get_param("HatchPatternResolution", 128)
                                            if not size:
                                                size = 128
                                        im = gui_utils.load_texture(path, size)
                                        if im:
                                            self.texture = coin.SoTexture2()
                                            self.texture.image = im
                                            r.insertChild(self.texture, 1)
                                            if size:
                                                s = 1
                                                if hasattr(vobj, "PatternSize"):
                                                    if vobj.PatternSize:
                                                        s = vobj.PatternSize
                                                self.texcoords = coin.SoTextureCoordinatePlane()
                                                self.texcoords.directionS.setValue(s, 0, 0)
                                                self.texcoords.directionT.setValue(0, s, 0)
                                                r.insertChild(self.texcoords, 2)
        elif prop == "PatternSize":
            if hasattr(self, "texcoords"):
                if self.texcoords:
                    s = 1
                    if vobj.PatternSize:
                        s = vobj.PatternSize
                    vS = App.Vector(self.texcoords.directionS.getValue().getValue())
                    vT = App.Vector(self.texcoords.directionT.getValue().getValue())
                    vS.Length = s
                    vT.Length = s
                    self.texcoords.directionS.setValue(vS.x, vS.y, vS.z)
                    self.texcoords.directionT.setValue(vT.x, vT.y, vT.z)
        return

    def _update_pattern_size(self, vobj):
        """Update the pattern size. Helper method in onChanged."""
        if hasattr(self, "texcoords") and self.texcoords:
            s = 1
            if vobj.PatternSize:
                s = vobj.PatternSize
            vS = App.Vector(self.texcoords.directionS.getValue().getValue())
            vT = App.Vector(self.texcoords.directionT.getValue().getValue())
            vS.Length = s
            vT.Length = s
            self.texcoords.directionS.setValue(vS.x, vS.y, vS.z)
            self.texcoords.directionT.setValue(vT.x, vT.y, vT.z)

    def execute(self, vobj):
        """Run when the object is created or recomputed.

        Override this method to produce effects when the object
        is newly created, and whenever the document is recomputed.

        By default it does nothing.

        Parameters
        ----------
        vobj : the view provider of the scripted object.
            This is `obj.ViewObject`.
        """
        return

    def setEdit(self, vobj, mode=0):
        """Enter edit mode of the object.

        Override this method to define a custom command to run when entering
        the edit mode of the object in the tree view.
        It must return `True` to successfully enter edit mode.
        If the conditions to edit are not met, it should return `False`,
        in which case the edit mode is not started.

        By default it runs the `Draft_Edit` GuiCommand.
        ::
            Gui.runCommand('Draft_Edit')

        Parameters
        ----------
        vobj : the view provider of the scripted object.
            This is `obj.ViewObject`.

        mode : int, optional
            It defaults to 0, in which case
            it runs the `Draft_Edit` GuiCommand.
            It indicates the type of edit in the underlying C++ code.

        Returns
        -------
        bool
            It is `True` if `mode` is 0, and `Draft_Edit` ran successfully.
            It is `False` otherwise.
        """
        if mode == 0 and App.GuiUp: #remove guard after splitting every viewprovider
            Gui.runCommand("Draft_Edit")
            return True
        return False

    def unsetEdit(self, vobj, mode=0):
        """Terminate the edit mode of the object.

        Override this method to define a custom command to run when
        terminating the edit mode of the object in the tree view.

        It should return `True` to indicate that the method already
        cleaned up everything and there is no need to call
        the `usetEdit` method of the base class.
        It should return `False` to indicate that cleanup
        is still required, so the `unsetEdit` method of the base class
        is invoked to do the rest.

        By default it runs the `finish` method of the active
        Draft GuiCommand, and closes the task panel.
        ::
            App.activeDraftCommand.finish()
            Gui.Control.closeDialog()

        Parameters
        ----------
        vobj : the view provider of the scripted object.
            This is `obj.ViewObject`.

        mode : int, optional
            It defaults to 0. It is not used.
            It indicates the type of edit in the underlying C++ code.

        Returns
        -------
        bool
            This method always returns `False` so it passes
            control to the base class to finish the edit mode.
        """
        if App.activeDraftCommand:
            App.activeDraftCommand.finish()
        if App.GuiUp: # remove guard after splitting every viewprovider
            Gui.Control.closeDialog()
        return False

    def getIcon(self):
        """Return the path to the icon used by the view provider.

        The path can be a full path in the system, or a relative path
        inside the compiled resource file.
        It can also be a string that defines the icon in XPM format.

        Override this method to provide a specific icon
        for the object in the tree view.

        By default it returns the path to the `Draft_Draft.svg` icon.

        Returns
        -------
        str
            `':/icons/Draft_Draft.svg'`
        """
        if hasattr(self.Object,"Proxy") and hasattr(self.Object.Proxy,"Type"):
            tp = self.Object.Proxy.Type
            if tp in ('Line', 'Wire', 'Polyline'):
                return ":/icons/Draft_N-Linear.svg"
            elif tp in ('Rectangle', 'Polygon'):
                return ":/icons/Draft_N-Polygon.svg"
            elif tp in ('Circle', 'Ellipse', 'BSpline', 'BezCurve', 'Fillet'):
                return ":/icons/Draft_N-Curve.svg"
            elif tp in ("ShapeString"):
                return ":/icons/Draft_ShapeString_tree.svg"
        return ":/icons/Draft_Draft.svg"

    def claimChildren(self):
        """Return objects that will be placed under it in the tree view.

        Override this method to return a list with objects
        that will appear under this object in the tree view.
        That is, this object becomes the `parent`,
        and all those under it are the `children`.

        By default the returned list is composed of objects from
        `Object.Base`, `Object.Objects`, `Object.Components`,
        and `Object.Group`, if they exist.

        Returns
        -------
        list
            List of objects.
        """
        objs = []
        if hasattr(self.Object, "Base"):
            objs.append(self.Object.Base)
        if hasattr(self.Object, "Objects"):
            objs.extend(self.Object.Objects)
        if hasattr(self.Object, "Components"):
            objs.extend(self.Object.Components)
        if hasattr(self.Object, "Group"):
            objs.extend(self.Object.Group)
        return objs


# Alias for compatibility with v0.18 and earlier
_ViewProviderDraft = ViewProviderDraft


class ViewProviderDraftAlt(ViewProviderDraft):
    """A view provider that doesn't absorb its base object in the tree view.

    The `claimChildren` method is overridden to return an empty list.
    """

    def __init__(self, vobj):
        super(ViewProviderDraftAlt, self).__init__(vobj)

    def claimChildren(self):
        objs = []
        return objs


# Alias for compatibility with v0.18 and earlier
_ViewProviderDraftAlt = ViewProviderDraftAlt


class ViewProviderDraftPart(ViewProviderDraftAlt):
    """A view provider that displays a Part icon instead of a Draft icon.

    The `getIcon` method is overridden to provide `Tree_Part.svg`.
    """

    def __init__(self, vobj):
        super(ViewProviderDraftPart, self).__init__(vobj)

    def getIcon(self):
        return ":/icons/Tree_Part.svg"


# Alias for compatibility with v0.18 and earlier
_ViewProviderDraftPart = ViewProviderDraftPart

## @}
