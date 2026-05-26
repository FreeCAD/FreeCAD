# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Max Wilfinger
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Provides a viewport-only node-editing session.

The session reuses the editTracker and GuiTools machinery from Draft_Edit
but does not own a Draft toolbar / task panel and does not toggle the
document's edit state. This lets a host view provider (e.g. an Arch wall)
keep its own task panel open while still exposing draggable node handles
in the 3D view.

Compared to Draft_Edit, the session intentionally omits:
    - the numeric input panel (Gui.draftToolBar)
    - alternative edit modes / per-node context menus
    - selection mode toggling beyond what is needed for snapping

Snapper integration and the optional ghost preview (init_preview_object /
update_preview_object on the GuiTools) are preserved.
"""

## @package gui_node_edit_session
# \ingroup draftguitools
# \brief Viewport-only node-editing session for use alongside host task panels.

__title__ = "FreeCAD NodeEditSession"
__author__ = "FreeCAD Project Association"
__url__ = "https://www.freecad.org"

import pivy.coin as coin

import FreeCAD as App
import FreeCADGui as Gui
from draftutils import gui_utils
from draftutils import params
from draftguitools import gui_tool_utils
from draftguitools import gui_trackers as trackers

_DEFAULT_COLOR = (0.8, 0.8, 0.8)
_HOVER_COLOR = (1.0, 0.0, 0.0)


class NodeEditSession:
    """Lightweight node-editing session that can run alongside a host task panel.

    Parameters
    ----------
    targets : list of (obj, gui_tools)
        Each target pairs a document object with a GuiTools instance from
        draftguitools.gui_edit_arch_objects / gui_edit_draft_objects / ...
        The GuiTools must implement at least get_edit_points and
        update_object_from_edit_points.
    """

    def __init__(self, targets):
        self._targets = [
            (obj, tools) for obj, tools in targets if obj is not None and tools is not None
        ]
        self._tools_by_name = {obj.Name: tools for obj, tools in self._targets}
        self._view = None
        self._render_manager = None
        self._trackers = {}  # obj.Name -> [editTracker]
        self._objs_formats = {}  # obj.Name -> style snapshot
        self._editing_obj = None  # currently dragged obj
        self._editing_idx = None  # index of currently dragged tracker
        self._drag_start = None  # App.Vector of node at drag start
        self._ghost = None
        self._over_node = None
        self._cb_mouse_pressed = None
        self._cb_mouse_moved = None
        self._cb_key_pressed = None
        self._pick_radius = params.get_param("DraftEditPickRadius")
        self._running = False

    # ------------------------------------------------------------------ lifecycle

    def start(self):
        """Build trackers and register viewport callbacks."""
        if self._running:
            return
        self._view = gui_utils.get_3d_view()
        if self._view is None:
            return
        self._render_manager = self._view.getViewer().getSoRenderManager()

        self._format_objects()
        for obj, tools in self._targets:
            self._build_trackers(obj, tools)

        self._cb_key_pressed = self._view.addEventCallbackPivy(
            coin.SoKeyboardEvent.getClassTypeId(), self._on_key
        )
        self._cb_mouse_moved = self._view.addEventCallbackPivy(
            coin.SoLocation2Event.getClassTypeId(), self._on_mouse_moved
        )
        self._cb_mouse_pressed = self._view.addEventCallbackPivy(
            coin.SoMouseButtonEvent.getClassTypeId(), self._on_mouse_pressed
        )
        self._running = True

    def stop(self):
        """Tear down trackers and unregister viewport callbacks."""
        if not self._running:
            return
        self._cancel_drag()
        self._unregister_callbacks()
        self._remove_all_trackers()
        self._deformat_objects()
        self._view = None
        self._render_manager = None
        self._running = False

    def is_running(self):
        return self._running

    # ------------------------------------------------------------------ callbacks

    def _unregister_callbacks(self):
        if self._view is None:
            return
        try:
            if self._cb_key_pressed is not None:
                self._view.removeEventCallbackSWIG(
                    coin.SoKeyboardEvent.getClassTypeId(), self._cb_key_pressed
                )
            if self._cb_mouse_moved is not None:
                self._view.removeEventCallbackSWIG(
                    coin.SoLocation2Event.getClassTypeId(), self._cb_mouse_moved
                )
            if self._cb_mouse_pressed is not None:
                self._view.removeEventCallbackSWIG(
                    coin.SoMouseButtonEvent.getClassTypeId(), self._cb_mouse_pressed
                )
        except RuntimeError:
            # view was destroyed before us
            pass
        self._cb_key_pressed = None
        self._cb_mouse_moved = None
        self._cb_mouse_pressed = None

    def _on_key(self, event_callback):
        event = event_callback.getEvent()
        if event.getState() != coin.SoKeyboardEvent.DOWN:
            return
        if event.getKey() == coin.SoKeyboardEvent.ESCAPE and self._editing_obj is not None:
            self._cancel_drag()

    def _on_mouse_moved(self, event_callback):
        event = event_callback.getEvent()
        if self._editing_obj is not None:
            self._update_drag(event)
        else:
            self._update_hover(event)

    def _on_mouse_pressed(self, event_callback):
        event = event_callback.getEvent()
        if event.getButton() != event.BUTTON1 or event.getState() != coin.SoMouseButtonEvent.DOWN:
            return
        if event.wasAltDown():
            return  # leave Alt+click for host menus
        if self._editing_obj is None:
            self._begin_drag(event)
        else:
            self._commit_drag()

    # ------------------------------------------------------------------ drag flow

    def _begin_drag(self, event):
        node = self._pick_edit_node(event.getPosition())
        if node is None:
            return
        obj_name = str(node.objectName.getValue())
        if obj_name not in self._trackers:
            return
        idx = self._node_index(node)
        if idx is None:
            return
        obj = App.ActiveDocument.getObject(obj_name) if App.ActiveDocument else None
        if obj is None:
            return

        tracker = self._trackers[obj_name][idx]
        self._editing_obj = obj
        self._editing_idx = idx
        self._drag_start = tracker.get()
        tracker.off()

        Gui.Snapper.setSelectMode(False)
        self._init_ghost(obj)

    def _update_drag(self, event):
        pos = event.getPosition().getValue()
        constrain = bool(event.wasShiftDown())
        snapped = Gui.Snapper.snap((pos[0], pos[1]), self._drag_start, constrain=constrain)
        self._trackers[self._editing_obj.Name][self._editing_idx].set(snapped)
        if self._ghost is not None:
            self._update_ghost(snapped)

    def _commit_drag(self):
        obj = self._editing_obj
        idx = self._editing_idx
        v = self._trackers[obj.Name][idx].get()
        self._finalize_ghost()
        self._trackers[obj.Name][idx].on()
        Gui.Snapper.setSelectMode(True)
        self._editing_obj = None
        self._editing_idx = None
        self._drag_start = None

        self._apply_update(obj, idx, v)

    def _cancel_drag(self):
        if self._editing_obj is None:
            return
        tracker = self._trackers[self._editing_obj.Name][self._editing_idx]
        if self._drag_start is not None:
            tracker.set(self._drag_start)
        tracker.on()
        self._finalize_ghost()
        Gui.Snapper.setSelectMode(True)
        self._editing_obj = None
        self._editing_idx = None
        self._drag_start = None

    def _update_hover(self, event):
        node = self._pick_edit_node(event.getPosition())
        if node is None:
            if self._over_node is not None:
                self._over_node.setColor(_DEFAULT_COLOR)
                self._over_node = None
            return
        obj_name = str(node.objectName.getValue())
        if obj_name not in self._trackers:
            return
        idx = self._node_index(node)
        if idx is None:
            return
        if self._over_node is not None:
            self._over_node.setColor(_DEFAULT_COLOR)
        self._over_node = self._trackers[obj_name][idx]
        self._over_node.setColor(_HOVER_COLOR)

    # ------------------------------------------------------------------ trackers

    def _build_trackers(self, obj, tools):
        pts = tools.get_edit_points(obj)
        if not pts:
            return
        pts = [self._globalize(obj, p) for p in pts]
        self._trackers[obj.Name] = [
            trackers.editTracker(pos=pts[i], name=obj.Name, idx=i) for i in range(len(pts))
        ]

    def _rebuild_trackers(self, obj):
        if obj.Name in self._trackers:
            for t in self._trackers[obj.Name]:
                t.finalize()
            del self._trackers[obj.Name]
        tools = self._tools_by_name.get(obj.Name)
        if tools is not None:
            self._build_trackers(obj, tools)

    def _remove_all_trackers(self):
        for tracker_list in self._trackers.values():
            for t in tracker_list:
                t.finalize()
        self._trackers = {}
        self._over_node = None

    # ------------------------------------------------------------------ object update

    def _apply_update(self, obj, idx, v):
        tools = self._tools_by_name.get(obj.Name)
        if tools is None:
            return
        v_local = self._localize(obj, v)
        App.ActiveDocument.openTransaction("Edit node")
        try:
            tools.update_object_from_edit_points(obj, idx, v_local, 0)
            obj.recompute()
            App.ActiveDocument.commitTransaction()
        except Exception:
            App.ActiveDocument.abortTransaction()
            raise
        self._rebuild_trackers(obj)
        try:
            gui_tool_utils.redraw_3d_view()
        except AttributeError:
            pass

    # ------------------------------------------------------------------ ghost

    def _init_ghost(self, obj):
        tools = self._tools_by_name.get(obj.Name)
        if tools is None:
            return
        try:
            self._ghost = tools.init_preview_object(obj)
        except Exception:
            self._ghost = None

    def _update_ghost(self, v):
        tools = self._tools_by_name.get(self._editing_obj.Name)
        if tools is None or self._ghost is None:
            return
        try:
            self._ghost.on()
            tools.update_preview_object(self, self._editing_obj, self._editing_idx, v)
        except Exception:
            pass

    def _finalize_ghost(self):
        if self._ghost is None:
            return
        try:
            self._ghost.finalize()
        except Exception:
            pass
        self._ghost = None

    # ------------------------------------------------------------------ formatting

    def _format_objects(self):
        for obj, tools in self._targets:
            try:
                self._objs_formats[obj.Name] = tools.get_object_style(obj)
                tools.set_object_editing_style(obj)
            except Exception:
                pass

    def _deformat_objects(self):
        for obj, tools in self._targets:
            if obj.Name not in self._objs_formats:
                continue
            try:
                tools.restore_object_style(obj, self._objs_formats[obj.Name])
            except Exception:
                pass
        self._objs_formats = {}

    # ------------------------------------------------------------------ picking

    def _pick_edit_node(self, screen_pos):
        if self._render_manager is None:
            return None
        ray_pick = coin.SoRayPickAction(self._render_manager.getViewportRegion())
        ray_pick.setPoint(coin.SbVec2s(*screen_pos))
        ray_pick.setRadius(self._pick_radius)
        ray_pick.setPickAll(True)
        ray_pick.apply(self._render_manager.getSceneGraph())
        for picked in ray_pick.getPickedPointList():
            path = picked.getPath()
            node = path.getNode(path.getLength() - 2)
            if hasattr(node, "subElementName") and "EditNode" in str(
                node.subElementName.getValue()
            ):
                return node
        return None

    @staticmethod
    def _node_index(node):
        try:
            return int(str(node.subElementName.getValue())[8:])
        except (ValueError, AttributeError):
            return None

    # ------------------------------------------------------------------ coords

    @staticmethod
    def _globalize(obj, v):
        if hasattr(obj, "getGlobalPlacement"):
            return obj.getGlobalPlacement().multVec(v)
        return v

    @staticmethod
    def _localize(obj, v):
        if hasattr(obj, "getGlobalPlacement"):
            return obj.getGlobalPlacement().inverse().multVec(v)
        return v
