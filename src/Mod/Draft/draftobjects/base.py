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
"""Provides the object code for the base Draft object."""
## @package base
# \ingroup draftobjects
# \brief Provides the object code for the base Draft object.

## \addtogroup draftobjects
# @{


class DraftObject(object):
    """The base class for Draft objects.

    Parameters
    ----------
    obj : a base C++ object
        The base object instantiated during creation,
        which commonly may be of types `Part::Part2DObjectPython`,
        `Part::FeaturePython`, or `App::FeaturePython`.

            >>> obj = App.ActiveDocument.addObject('Part::Part2DObjectPython')
            >>> DraftObject(obj)

        This class instance is stored in the `Proxy` attribute
        of the base object.
        ::
            obj.Proxy = self

    tp : str, optional
        It defaults to `'Unknown'`.
        It indicates the type of this scripted object,
        which will be assigned to the Proxy's `Type` attribute.

        This is useful to distinguish different types of scripted objects
        that may be derived from the same C++ class.

    Attributes
    ----------
    Type : str
        It indicates the type of scripted object.
        Normally `Type = tp`.

        All objects have a `TypeId` attribute, but this attribute
        refers to the C++ class only. Using the `Type` attribute
        allows distinguishing among various types of objects
        derived from the same C++ class.

            >>> print(A.TypeId, "->", A.Proxy.Type)
            Part::Part2DObjectPython -> Wire
            >>> print(B.TypeId, "->", B.Proxy.Type)
            Part::Part2DObjectPython -> Circle

    This class attribute is accessible through the `Proxy` object:
    `obj.Proxy.Type`.
    """

    def __init__(self, obj, tp="Unknown"):
        # This class is assigned to the Proxy attribute
        if obj:
            obj.Proxy = self
        self.Type = tp

    def onDocumentRestored(self,obj):
        # Object properties are updated when the document is opened.
        self.props_changed_clear()

    def dumps(self):
        """Return a tuple of all serializable objects or None.

        When saving the document this object gets stored
        using Python's `json` module.

        Override this method to define the serializable objects to return.

        By default it returns the `Type` attribute.

        Returns
        -------
        str
            Returns the value of `Type`
        """
        return self.Type

    def loads(self, state):
        """Set some internal properties for all restored objects.

        When a document is restored this method is used to set some properties
        for the objects stored with `json`.

        Override this method to define the properties to change for the
        restored serialized objects.

        By default the `Type` was serialized, so `state` holds this value,
        which is re-assigned to the `Type` attribute.
        ::
            self.Type = state

        Parameters
        ----------
        state : state
            A serialized object.
        """
        if state:
            self.Type = state

    def execute(self, obj):
        """Run this method when the object is created or recomputed.

        Override this method to produce effects when the object
        is newly created, and whenever the document is recomputed.

        By default it does nothing.

        Parameters
        ----------
        obj : the scripted object.
            This commonly may be of types `Part::Part2DObjectPython`,
            `Part::FeaturePython`, or `App::FeaturePython`.
        """
        pass

    def onChanged(self, obj, prop):
        """Run this method when a property is changed.

        Override this method to handle the behavior
        of the object depending on changes that occur to its properties.

        By default it does nothing.

        Parameters
        ----------
        obj : the scripted object.
            This commonly may be of types `Part::Part2DObjectPython`,
            `Part::FeaturePython`, or `App::FeaturePython`.

        prop : str
            Name of the property that was modified.
        """
        pass

    def props_changed_store(self, prop):
        """Store the name of the property that will be changed in the
        self.props_changed list.

        The function should be called at the start of onChanged or onBeforeChange.

        The list is used to detect Placement-only changes. In such cases the
        Shape of an object does not need to be updated. Not updating the Shape
        avoids Uno/Redo issues for the Windows version and is also more efficient.
        """
        if not hasattr(self, "props_changed"):
            self.props_changed = [prop]
        else:
            self.props_changed.append(prop)

    def props_changed_clear(self):
        """Remove the self.props_changed attribute if it exists.

        The function should be called just before execute returns.
        """
        if hasattr(self, "props_changed"):
            delattr(self, "props_changed")

    def props_changed_placement_only(self, obj=None):
        """Return `True` if the self.props_changed list, after removing `Shape`
        and `_LinkTouched` items, only contains `Placement` items.

        Parameters
        ----------
        obj : the scripted object. Need only be supplied if the Shape of obj
            is, or can be, derived from other objects.

        """
        if not hasattr(self, "props_changed"):
            return False

        # For an attached object whose Shape depends on one or more source
        # objects the situation can occur that when those source objects
        # change its Placement is also changed, but for no apparent reason:
        # the new Placement is identical to the old. For such an object a
        # `placement_only` change cannot be detected reliably and therefore
        # this function should return `False`.
        # https://github.com/FreeCAD/FreeCAD/issues/8771
        if obj is not None \
                and obj.OutList \
                and hasattr(obj, "Support") \
                and hasattr(obj, "MapMode") \
                and obj.Support \
                and obj.MapMode != "Deactivated":
            return False

        props = set(self.props_changed)
        if "Shape" in props:
            props.remove("Shape")
        if "_LinkTouched" in props:
            props.remove("_LinkTouched")
        return props == {"Placement"}


# Alias for compatibility with v0.18 and earlier
_DraftObject = DraftObject

## @}
