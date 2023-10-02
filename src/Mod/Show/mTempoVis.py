# /***************************************************************************
# *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

# module is named mTempoVis, because Show.TimpoVis exposes the class as its member, and hides the module TempoVis.py

from . import Containers

from . import TVStack

import FreeCAD as App

if App.GuiUp:
    import FreeCADGui as Gui
Wrn = lambda msg: App.Console.PrintWarning(msg + "\n")
Err = lambda msg: App.Console.PrintError(msg + "\n")
Log = lambda msg: App.Console.PrintLog(msg + "\n")

from copy import copy

S_EMPTY = 0  # TV is initialized, but no changes were done through it
S_ACTIVE = 1  # TV has something to be undone
S_RESTORED = 2  # TV has been restored
S_INTERNAL = 3  # TV instance is being used by another TV instance as a redo data storage


def _printTraceback(err):
    import sys

    if err is sys.exc_info()[1]:
        import traceback

        tb = traceback.format_exc()
        Log(tb)


class MAINSTACK(object):
    """it's just a default value definition for TV constructor"""

    pass


class JUST_SAVE(object):
    '''it's just a default value meaning "save current scene value but don't modify anything"'''

    pass


class TempoVis(object):
    """TempoVis - helper object to save visibilities of objects before doing
    some GUI editing, hiding or showing relevant stuff during edit, and
    then restoring all visibilities after editing.

    Constructors:
    TempoVis(document, stack = MAINSTACK, **kwargs): creates a new TempoVis.

    document: required. Objects not belonging to the document can't be modified via TempoVis.

    stack: optional. Which stack to insert this new TV into. Can be:
    a TVStack instance (then, the new TV is added to the top of the stack),
    MAINSTACK special value (a global stack for the document will be used), or
    None (then, the TV is not in any stack, and can be manually instertd into one if desired).

    Any additional keyword args are assigned as attributes. You can use it to immediately set a tag, for example."""

    document = None
    stack = None  # reference to stack this TV is in

    data = None  # dict. key = ("class_id","key"), value = instance of SceneDetail
    data_requested = None  # same as data, but stores (wanted) values passed to modify()

    state = S_EMPTY

    tag = ""  # stores any user-defined string for identification purposes

    def _init_attrs(self):
        """initialize member variables to empty values (needed because we can't use mutable initial values when initializing member variables in class definition)"""
        self.data = {}
        self.data_requested = {}

    # <core interface>
    def __init__(self, document, stack=MAINSTACK, **kwargs):
        self._init_attrs()
        self.document = document

        if stack is MAINSTACK:
            stack = TVStack.mainStack(document)

        if stack is None:
            pass
        else:
            stack.insert(self)

        for key, val in kwargs.items():
            setattr(self, key, val)

    def __del__(self):
        if self.state == S_ACTIVE:
            self.restore(ultimate=True)

    def has(self, detail):
        """has(self, detail): returns True if this TV has this detail value saved.
        example: tv.has(VProperty(obj, "Visibility"))"""
        return detail.full_key in self.data

    def stored_val(self, detail):
        """stored_val(self, detail): returns value of detail remembered by this TV. If not, raises KeyError."""
        return self.data[detail.full_key].data

    def save(self, detail, mild_restore=False):
        """save(detail, mild_restore = False):saves the scene detail to be restored.
        The detail is saved only once; repeated calls are ignored.
        mild_restore: internal, do not use."""
        self._change()
        if not detail.full_key in self.data:
            # not saved yet
            tv1, curr = self._value_after(detail, query_scene=True)
            self.data[detail.full_key] = copy(curr)
            self.data[detail.full_key].mild_restore = mild_restore
        else:
            # saved already. Change restore policy, if necessary.
            stored_dt = self.data[detail.full_key]
            if not mild_restore:
                stored_dt.mild_restore = False

    def modify(self, detail, mild_restore=None):
        """modify(detail, mild_restore = True): modifies scene detail through this TV.
        The value is provided as an instance of SceneDetail implementation.
        The procedure takes care to account for the stack - that is, if in a TV applied
        later than this one this detail was changed too, the value saved therein is altered,
        rather than applied to the scene.

        mild_restore: if True, when restoring later, checks if the value was changed
        by user after last call to modify(), and doesn't restore if it was changed.

        Example: tv.modify(VProperty(obj, "Visibility", True))"""

        self._change()

        if mild_restore is not None:
            detail.mild_restore = mild_restore

        # save current
        self.save(detail, detail.mild_restore)

        # apply
        tv1, curr = self._value_after(detail)
        if tv1 is not None:
            tv1.data[detail.full_key].data = detail.data
        else:
            detail.apply_data(detail.data)

        # and record.
        if detail.mild_restore:
            self.data_requested[detail.full_key] = copy(detail)

    def restoreDetail(self, detail, ultimate=False):
        """restoreDetail(detail, ultimate = False): restores a specific scene detail.
        ultimate: if true, the saved value is cleaned out.
        If the detail is not found, nothing is done.
        """
        if not self.has(detail):
            return
        self._restore_detail(detail)
        if ultimate:
            self.forgetDetail(detail)

    def forgetDetail(self, detail):
        """forgetDetail(detail): ditches a saved detail value, making the change done through this TV permanent."""
        self.data.pop(detail.full_key, None)
        self.data_requested.pop(detail.full_key, None)

    def forget(self):
        """forget(self): clears this TV, making all changes done through it permanent.
        Also, withdraws the TV from the stack."""
        self.state = S_EMPTY
        self.data = {}
        if self.is_in_stack:
            self.stack.withdraw(self)

    def restore(self, ultimate=True):
        """restore(ultimate = True): undoes all changes done through this tempovis / restores saved scene details.
        ultimate: if true, the saved values are cleaned out, and the TV is withdrawn from
        the stack. If false, the TV will still remember stuff, and restore can be called again.
        """
        if self.state == S_RESTORED:
            return

        if self.state != S_INTERNAL and ultimate:
            self.state = S_RESTORED

        for key, detail in self.data.items():
            try:
                self._restoreDetail(detail)
            except Exception as err:
                Err(
                    "TempoVis.restore: failed to restore detail {key}: {err}".format(
                        key=key, err=str(err)
                    )
                )
                _printTraceback(err)
        if ultimate:
            self.data = {}
            if self.is_in_stack:
                self.stack.withdraw(self)

    # </core interface>

    # <stack interface>
    def _inserted(self, stack, index):
        """calles when this tv is inserted into a stack"""
        self.stack = stack

    def _withdrawn(self, stack, index):
        """calles when this tv is withdrawn from a stack"""
        self.stack = None

    @property
    def is_in_stack(self):
        return self.stack is not None

    # </stack interface>

    # <convenience functions>
    def modifyVPProperty(self, doc_obj_or_list, prop_names, new_value=JUST_SAVE, mild_restore=None):
        """modifyVPProperty(doc_obj_or_list, prop_names, new_value = JUST_SAVE, mild_restore = None): modifies
        prop_name property of ViewProvider of doc_obj_or_list, and remembers
        original value of the property. Original values will be restored upon
        TempoVis deletion, or call to restore().

        mild_restore: test if user changed the value manually when restoring the TV."""

        if self.state == S_RESTORED:
            Wrn("Attempting to use a TV that has been restored. There must be a problem with code.")
            return

        if not hasattr(doc_obj_or_list, "__iter__"):
            doc_obj_or_list = [doc_obj_or_list]
        if not isinstance(prop_names, (list, tuple)):
            prop_names = [prop_names]
        for doc_obj in doc_obj_or_list:
            for prop_name in prop_names:
                if not hasattr(doc_obj.ViewObject, prop_name):
                    Wrn(
                        "TempoVis: object {obj} has no attribute {attr}. Skipped.".format(
                            obj=doc_obj.Name, attr=prop_name
                        )
                    )
                    continue

                # Because the introduction of external objects, we shall now
                # accept objects from all opened documents.
                #
                #  if doc_obj.Document is not self.document:  #ignore objects from other documents
                #      raise ValueError("Document object to be modified does not belong to document TempoVis was made for.")
                from .SceneDetails.VProperty import VProperty

                if new_value is JUST_SAVE:
                    if mild_restore:
                        Wrn(
                            "TempoVis: can't just save a value for mild restore. Saving for hard restore."
                        )
                    self.save(VProperty(doc_obj, prop_name, new_value))
                else:
                    self.modify(VProperty(doc_obj, prop_name, new_value), mild_restore)

    def restoreVPProperty(self, doc_obj_or_list, prop_names):
        """restoreVPProperty(doc_obj_or_list, prop_name, new_value): restores specific property changes."""
        from .SceneDetails.VProperty import VProperty

        if not hasattr(doc_obj_or_list, "__iter__"):
            doc_obj_or_list = [doc_obj_or_list]
        if not isinstance(prop_names, (tuple, list)):
            prop_names = [prop_names]
        for doc_obj in doc_obj_or_list:
            for prop_name in prop_names:
                try:
                    self.restoreDetail(VProperty(doc_obj, prop_name))
                except Exception as err:
                    Err(
                        "TempoVis.restore: failed to restore detail {key}: {err}".format(
                            key=key, err=str(err)
                        )
                    )
                    _printTraceback(err)

    def saveBodyVisibleFeature(self, doc_obj_or_list):
        """saveBodyVisibleFeature(self, doc_obj_or_list): saves Visibility of currently
        visible feature, for every body of PartDesign features in the provided list."""
        if not hasattr(doc_obj_or_list, "__iter__"):
            doc_obj_or_list = [doc_obj_or_list]
        objs = []
        bodies = set()
        for obj in doc_obj_or_list:
            body = getattr(obj, "_Body", None)
            if not body or body in bodies:
                continue
            bodies.add(body)
            feature = getattr(body, "VisibleFeature", None)
            if feature:
                objs.append(feature)
        self.modifyVPProperty(objs, "Visibility", JUST_SAVE)
        return objs

    def show(self, doc_obj_or_list, links_too=True, mild_restore=None):
        """show(doc_obj_or_list, links_too = True): shows objects (sets their Visibility to True).
        doc_obj_or_list can be a document object, or a list of document objects.
        If links_too is True, all Links of the objects are also hidden, by setting LinkVisibility attribute of each object."""
        doc_obj_or_list = self._3D_objects(doc_obj_or_list)
        self.saveBodyVisibleFeature(
            doc_obj_or_list
        )  # fix implicit hiding of other features by PartDesign not being recorded to TV
        self.modifyVPProperty(doc_obj_or_list, "Visibility", True, mild_restore)
        if links_too:
            self.modifyVPProperty(doc_obj_or_list, "LinkVisibility", True, mild_restore)

    def hide(self, doc_obj_or_list, links_too=True, mild_restore=None):
        """hide(doc_obj_or_list): hides objects (sets their Visibility to False). doc_obj_or_list can be a document object, or a list of document objects"""
        doc_obj_or_list = self._3D_objects(doc_obj_or_list)
        # no need to saveBodyVisibleFeature here, as no implicit showing will happen
        self.modifyVPProperty(doc_obj_or_list, "Visibility", False, mild_restore)
        if links_too:
            self.modifyVPProperty(doc_obj_or_list, "LinkVisibility", False, mild_restore)

    def get_all_dependent(self, doc_obj, subname=None):
        """get_all_dependent(doc_obj, subname = None): gets all objects that depend on doc_obj. Containers and Links (if subname) required for visibility of the object are excluded from the list."""
        from . import Containers
        from .Containers import isAContainer
        from .DepGraphTools import getAllDependencies, getAllDependent

        if subname:
            # a link-path was provided. doc_obj has nothing to do with the object we want
            # to collect dependencies from. So, replace it with the one pointed by link-path.
            cnt_chain = doc_obj.getSubObjectList(subname)
            doc_obj = cnt_chain[-1].getLinkedObject()
            # cnt_chain can either end with the object (e.g. if a sketch is in a part, and
            # a link is to a part), or it may be a Link object (if we have a straight or
            # even nested Link to the sketch).
            #
            # I don't know why do we need that isAContainer check here, but I'm leaving it,
            # realthunder must be knowing his business --DeepSOIC
            cnt_chain = [
                o for o in cnt_chain if o == cnt_chain[-1] or isAContainer(o, links_too=True)
            ]
        else:
            cnt_chain = Containers.ContainerChain(doc_obj)
        return [o for o in getAllDependent(doc_obj) if not o in cnt_chain]

    def hide_all_dependent(self, doc_obj):
        """hide_all_dependent(doc_obj): hides all objects that depend on doc_obj. Groups, Parts and Bodies are not hidden by this."""
        self.hide(self._3D_objects(self.get_all_dependent(doc_obj)))

    def show_all_dependent(self, doc_obj):
        """show_all_dependent(doc_obj): shows all objects that depend on doc_obj. This method is probably useless."""
        from .DepGraphTools import getAllDependencies, getAllDependent

        self.show(self._3D_objects(getAllDependent(doc_obj)))

    def restore_all_dependent(self, doc_obj):
        """show_all_dependent(doc_obj): restores original visibilities of all dependent objects."""
        from .DepGraphTools import getAllDependencies, getAllDependent

        self.restoreVPProperty(getAllDependent(doc_obj), ("Visibility", "LinkVisibility"))

    def hide_all_dependencies(self, doc_obj):
        """hide_all_dependencies(doc_obj): hides all objects that doc_obj depends on (directly and indirectly)."""
        from .DepGraphTools import getAllDependencies, getAllDependent

        self.hide(self._3D_objects(getAllDependencies(doc_obj)))

    def show_all_dependencies(self, doc_obj):
        """show_all_dependencies(doc_obj): shows all objects that doc_obj depends on (directly and indirectly). This method is probably useless."""
        from .DepGraphTools import getAllDependencies, getAllDependent

        self.show(self._3D_objects(getAllDependencies(doc_obj)))

    def saveCamera(self, vw=None):
        self._change()
        from .SceneDetails.Camera import Camera

        self.save(Camera(self.document))

    def restoreCamera(self, ultimate=False):
        from .SceneDetails.Camera import Camera

        dt = Camera(self.document)
        self.restoreDetail(dt, ultimate)

    def setUnpickable(
        self, doc_obj_or_list, actual_pick_style=2
    ):  # 2 is coin.SoPickStyle.UNPICKABLE
        """setUnpickable(doc_obj_or_list, actual_pick_style = 2): sets object unpickable (transparent to clicks).
        doc_obj_or_list: object or list of objects to alter (App)
        actual_pick_style: optional parameter, specifying the actual pick style:
        0 = regular, 1 = bounding box, 2 (default) = unpickable.

        Implementation detail: uses SoPickStyle node. If viewprovider already has a node
        of this type as direct child, one is used. Otherwise, new one is created and
        inserted as the very first node, and remains there even after restore()/deleting
        tempovis."""

        from .SceneDetails.Pickability import Pickability
        from .ShowUtils import is3DObject

        if not hasattr(doc_obj_or_list, "__iter__"):
            doc_obj_or_list = [doc_obj_or_list]
        for doc_obj in doc_obj_or_list:
            if not is3DObject(doc_obj):
                continue
            dt = Pickability(doc_obj, actual_pick_style)
            self.modify(dt)

    def clipPlane(self, doc_obj_or_list, enable, placement, offset=0.02):
        """clipPlane(doc_obj_or_list, enable, placement, offset): slices off the object with a clipping plane.
        doc_obj_or_list: object or list of objects to alter (App)
        enable: True if you want clipping, False if you want to remove clipping:
        placement: XY plane of local coordinates of the placement is the clipping plane. The placement must be in document's global coordinate system.
        offset: shifts the plane. Positive offset reveals more of the object.

        Implementation detail: uses SoClipPlane node. If viewprovider already has a node
        of this type as direct child, one is used. Otherwise, new one is created and
        inserted as the very first node. The node is left, but disabled when tempovis is restoring."""

        from .SceneDetails.ObjectClipPlane import ObjectClipPlane
        from .ShowUtils import is3DObject

        if not hasattr(doc_obj_or_list, "__iter__"):
            doc_obj_or_list = [doc_obj_or_list]
        for doc_obj in doc_obj_or_list:
            if not is3DObject(doc_obj):
                continue
            dt = ObjectClipPlane(doc_obj, enable, placement, offset)
            self.modify(dt)

    @staticmethod
    def allVisibleObjects(aroundObject):
        """allVisibleObjects(aroundObject): returns list of objects that have to be toggled invisible for only aroundObject to remain.
        If a whole container can be made invisible, it is returned, instead of its child objects."""
        from .ShowUtils import is3DObject
        from . import Containers

        chain = Containers.VisGroupChain(aroundObject)
        result = []
        for i in range(len(chain)):
            cnt = chain[i]
            cnt_next = chain[i + 1] if i + 1 < len(chain) else aroundObject
            container = Containers.Container(cnt)
            for obj in container.getVisGroupChildren():
                if not is3DObject(obj):
                    continue
                if obj is not cnt_next:
                    if container.isChildVisible(obj):
                        result.append(obj)
        return result

    def sketchClipPlane(self, sketch, enable=None, reverted=False):
        """sketchClipPlane(sketch, enable = None): Clips all objects by plane of sketch.
        If enable argument is omitted, calling the routine repeatedly will toggle clipping plane."""

        from .SceneDetails.ClipPlane import ClipPlane

        editDoc = Gui.editDocument()
        if editDoc is None:
            doc = sketch.Document
            pla = sketch.getGlobalPlacement()
        else:
            doc = editDoc.Document
            pla = App.Placement(editDoc.EditingTransform)
        toggle = {False: 0, True: 1, None: -1}[enable]

        if reverted:
            pla = pla * App.Rotation(0, 1, 0, 0)

        if enable:  # clip plane shall be disabled so new placement can be applied
            self.modify(ClipPlane(doc, 0))

        self.modify(ClipPlane(doc, toggle, pla, 0.001))
        sketch.ViewObject.SectionView = (
            enable if enable is not None else not sketch.ViewObject.SectionView
        )

    def activateWorkbench(self, wb_name):
        from .SceneDetails.Workbench import Workbench

        self.modify(Workbench(wb_name))

    # </convenience functions>

    # <internals>
    def _restoreDetail(self, detail):
        p = self.data[detail.full_key]
        tv1, curr = self._value_after(detail, query_scene=p.mild_restore)
        if p.mild_restore:
            if self.data_requested[detail.full_key] != curr:
                # the value on the scene doesn't match what was requested through TV. User probably changed it. We don't want to mess it up.
                self._purge_milds(detail)
                return
        if tv1 is None:
            # no other TV has changed this detail later, apply to the scene
            detail.apply_data(p.data)
        else:
            # modify saved detail of higher TV
            tv1.data[detail.full_key].data = p.data

    def _purge_milds(self, detail):
        """_purge_milds(detail): wipes out detail from earlier TVs if the detail is mild-restore."""
        if not self.is_in_stack:
            return
        seq_before, seq_after = self.stack.getSplitSequence(self)
        for tv in reversed(seq_before):
            if tv.has(detail):
                if tv.data[detail.full_key].mild_restore:
                    tv.forgetDetail(detail)
                else:
                    # hard-restoring value encountered, stop
                    break

    def _change(self):
        """to be called whenever anything is done that is to be restored later."""
        if self.state == S_EMPTY:
            self.state = S_ACTIVE
        if self.state == S_RESTORED:
            Wrn("Attempting to use a TV that has been restored. There must be a problem with code.")
        self.tv_redo = None

    def _value_after(self, detail, query_scene=False):
        """_value_current(detail): returns (tv, detail1). SceneDetail instance holds "current" value of
        scene detail (current from the context of this TV; i.e. either the current scene
        status, or the saved state from upper TVs).
        If no upper TV has saved the detail value, returns either (None, None), or
        (None, detail1) if query_scene is True, where detail1 holds value from the scene."""

        def scene_value():
            if query_scene:
                cpy = copy(detail)
                cpy.data = cpy.scene_value()
                return (None, cpy)
            else:
                return (None, None)

        if self.is_in_stack:
            va = self.stack.value_after(self, detail)
            if va is None:
                return scene_value()
            else:
                return va
        else:
            return scene_value()

    def _3D_objects(self, doc_obj_or_list):
        """_3D_objects(doc_obj_or_list): returns list of objects that are in 3d view."""
        from .ShowUtils import is3DObject

        if not hasattr(doc_obj_or_list, "__iter__"):
            doc_obj_or_list = [doc_obj_or_list]

        return [obj for obj in doc_obj_or_list if is3DObject(obj)]

    def dumps(self):
        return None

    def loads(self, state):
        self._init_attrs()
