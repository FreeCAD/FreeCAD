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

import weakref

global_stacks = {}  # dict of TVStacks. key = document name.


class TVStack(object):
    index_LUT = None  # Key = id(tempovis_instance). Value = index in the stack.
    stack = None  # list of weakrefs to TV instances. Using weakrefs, so that TempoVis can self-destruct if forgotten.
    document = None

    _rewind_tv = None

    def __init__(self, document):
        self.document = None
        self.index_LUT = {}
        self.stack = []
        from . import TVObserver  # to start the observer

    def insert(self, tv, index=None):
        if index is None:
            index = len(self.stack)
        idtv = id(tv)
        ref = weakref.ref(tv, (lambda _, idtv=idtv, self=self: self._destruction(idtv)))
        self.stack.insert(index, ref)

        self.rebuild_index(index)

        tv._inserted(self, index)

    def _destruction(self, idtv):
        # the tempovis itself is destroyed already. Purge it from the stack (it should have withdrawn itself, but just in case).
        try:
            index = self.index_LUT.get(idtv)
        except KeyError:
            # already withdrawn
            pass
        else:
            self.stack.pop(index)
            self.index_LUT.pop(idtv)
            self.rebuild_index(index)

    def withdraw(self, tv):
        idtv = id(tv)
        index = self.index_LUT[idtv]
        ref = self.stack.pop(index)
        self.index_LUT.pop(idtv)

        self.rebuild_index(index)

        tv = ref()
        if tv:
            tv._withdrawn(self, index)

    def value_after(self, tv, detail):
        """value_after(tv, detail): returns tuple (tv1, detail), or None.
        Here, tv1 is the tv that remembers the value, and detail is reference to recorded
        data in tv1. None is returned, if no TVs in the stack after the provided one have
        recorded a change to this detail.

        tv can be None, then, the function returns the original value of the detail, or
        None, if the current value matches the original."""
        from . import mTempoVis

        index = self.index_LUT[id(tv)] if tv is not None else -1
        for tvref in self.stack[index + 1 :]:
            tv = tvref()
            if tv.state == mTempoVis.S_ACTIVE:
                if tv.has(detail):
                    return (tv, tv.data[detail.full_key])
        return None

    def rebuild_index(self, start=0):
        if start == 0:
            self.index_LUT = {}
        for i in range(start, len(self.stack)):
            self.index_LUT[id(self.stack[i]())] = i

    def purge_dead(self):
        """removes dead TV instances from the stack"""
        n = 0
        for i in reversed(range(len(self.stack))):
            if self.stack[i]() is None:
                self.stack.pop(i)
                n += 1
        if n > 0:
            self.rebuild_index()
        return n

    def dissolve(self):
        """silently cleans all TVs, so that they won't restore."""
        for ref in self.stack:
            if ref() is not None:
                ref().forget()

    def unwindForSaving(self):
        from . import mTempoVis

        self.rewindAfterSaving()  # just in case there was a failed save before.

        details = (
            {}
        )  # dict of detail original values. Key = detail key; value = detail instance with data representing the original value
        for ref in self.stack:
            tv = ref()
            for key, detail in tv.data.items():
                if not key in details:
                    if detail.affects_persistence:
                        details[detail.full_key] = detail

        self._rewind_tv = mTempoVis.TempoVis(self.document, None)
        for key, detail in details.items():
            self._rewind_tv.modify(detail)

    def rewindAfterSaving(self):
        if self._rewind_tv is not None:
            self._rewind_tv.restore()
            self._rewind_tv = None

    def getSplitSequence(self, tv):
        """getSplitSequence(tv): returns (list_before, list_after), neither list includes tv."""
        index = self.index_LUT[id(tv)]

        def deref(lst):
            return [ref() for ref in lst]

        return deref(self.stack[0:index]), deref(self.stack[index + 1 :])

    def __getitem__(self, index):
        return self.stack[index]()

    def __len__(self):
        return len(self.stack)

    def __iter__(self):
        for ref in self.stack:
            yield ref()

    def __reversed__(self):
        for ref in reversed(self.stack):
            yield ref()

    def restoreAll(self):
        for ref in reversed(self.stack):
            ref().restore()

    def byTag(self, tag):
        return [ref() for ref in self.stack if ref().tag == tag]


def mainStack(document, create_if_missing=True):
    """mainStack(document, create_if_missing = True):returns the main TVStack instance for provided document"""
    docname = document.Name

    if create_if_missing:
        if not docname in global_stacks:
            global_stacks[docname] = TVStack(document)

    return global_stacks.get(docname, None)


def _slotDeletedDocument(document):
    docname = document.Name
    stk = global_stacks.pop(docname, None)
    if stk is not None:
        stk.dissolve()


def _slotStartSaveDocument(doc):
    stk = mainStack(doc, create_if_missing=False)
    if stk is not None:
        stk.unwindForSaving()


def _slotFinishSaveDocument(doc):
    stk = mainStack(doc, create_if_missing=False)
    if stk is not None:
        stk.rewindAfterSaving()
