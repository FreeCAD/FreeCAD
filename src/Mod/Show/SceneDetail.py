# /***************************************************************************
# *   Copyright (c) 2019 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************/


class SceneDetail(object):
    """SceneDetail class: abstract class for tempovis scene save/restore plug-in. An implementation must provide:
    * data storage (as "data" attribute of the object)
    * constructor (preferably, with value for stored data as optional argument)
    * methods to apply values to actual scene (apply_data),
    * ...and to read out the state of the detail in the actual scene (scene_value)
    * keying, for identifying two detail instances that affect the exact same thing
    * class_id string, which is required for keying
    * copying
    * test for equality, that checks if .data attributes of two SceneDetail instances are equal
    * info on if the modification affects what is saved to disk, and thus should be undone temporarily for file writing.
    """

    class_id = ""

    data = None
    doc = None

    ## Storage field for TV. Mild restore means that the saved state won't be restored
    # if the current state differs from the state requested via last TempoVis.modify(...) call.
    # This is a default, it may be changed per-instance.
    mild_restore = False

    def set_doc(self, doc):
        self.doc = doc

    # <interface>
    key = None  # a string or something alike to use to store/find the entry in TempoVis. For example, a string "{object_name}.{property_name}".
    affects_persistence = False  # True indicate that the changes will be recorded if the doc is saved, and that this detail should be restored for saving

    def scene_value(self):
        """scene_value(): returns the value from the scene"""
        raise NotImplementedError()

    def apply_data(self, val):
        """apply a value to scene"""
        raise NotImplementedError()

    ## Equality test. Override if data attributes can't be directly compared
    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.data == other.data and self.data is not None
        else:
            raise TypeError(
                "{self} can't be compared with {other}".format(self=repr(self), other=repr(other))
            )

    # </interface>

    # <utility>
    @property
    def full_key(self):
        return (self.class_id, self.doc.Name if self.doc else None, self.key)

    def __ne__(self, other):
        return not self.__eq__(other)
