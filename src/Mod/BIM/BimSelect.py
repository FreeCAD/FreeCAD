# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import FreeCAD

WORKBENCH_NAME = "BIMWorkbench"
CYCLIC_SELECTION_OBSERVER_KEY = "cyclic_selection_observer"
SETUP_RUNTIME_KEY = "cyclic_selection_setup"


def _get_bim_runtime(create=False):
    import FreeCADGui

    try:
        if create:
            return FreeCADGui.workbenchRuntime(WORKBENCH_NAME)
        return FreeCADGui.findWorkbenchRuntime(WORKBENCH_NAME)
    except RuntimeError:
        return None


def _release_cyclic_selection_observer():
    runtime = _get_bim_runtime(create=False)
    if runtime is None:
        return False
    return runtime.release(CYCLIC_SELECTION_OBSERVER_KEY)


class CyclicSelectionObserver:
    def addSelection(self, document, object, element, position):
        import FreeCADGui

        if not FreeCAD.ActiveDocument:
            return
        runtime = _get_bim_runtime(create=False)
        if runtime is None or not runtime.owns(CYCLIC_SELECTION_OBSERVER_KEY):
            return
        FreeCADGui.Selection.removeSelection(FreeCAD.ActiveDocument.getObject(object))
        _release_cyclic_selection_observer()
        preselection = FreeCADGui.Selection.getPreselection()
        FreeCADGui.Selection.addSelection(
            FreeCAD.ActiveDocument.getObject(preselection.Object.Name),
            preselection.SubElementNames[0],
        )
        FreeCAD.ActiveDocument.recompute()


class CyclicObjectSelector:
    def __init__(self):
        self.selectableObjects = []
        self.objectIndex = 0

    def selectObject(self, event_callback):
        import FreeCADGui
        from pivy import coin

        if not FreeCAD.ActiveDocument:
            return
        event = event_callback.getEvent()

        if event.getState() != coin.SoMouseButtonEvent.DOWN or not self.selectableObjects:
            return

        pos = event.getPosition().getValue()
        element_list = FreeCADGui.ActiveDocument.ActiveView.getObjectsInfo(
            (int(pos[0]), int(pos[1]))
        )

        if not element_list:
            self.selectableObjects = []
            _release_cyclic_selection_observer()
            return

        runtime = _get_bim_runtime(create=True)
        if runtime is None:
            return
        if not runtime.owns(CYCLIC_SELECTION_OBSERVER_KEY):
            runtime.addSelectionObserver(
                CyclicSelectionObserver(),
                key=CYCLIC_SELECTION_OBSERVER_KEY,
            )

    def cycleSelectableObjects(self, event_callback):
        import FreeCADGui

        if not FreeCAD.ActiveDocument:
            return
        event = event_callback.getEvent()

        if not event.isKeyPressEvent(event, event.TAB):
            return

        pos = event.getPosition().getValue()
        selectableObjects = FreeCADGui.ActiveDocument.ActiveView.getObjectsInfo(
            (int(pos[0]), int(pos[1]))
        )

        if not selectableObjects:
            return

        if self.selectableObjects != selectableObjects:
            self.selectableObjects = selectableObjects
            self.objectIndex = 0
        elif self.objectIndex < len(self.selectableObjects) - 1:
            self.objectIndex += 1
        else:
            self.objectIndex = 0
        object_name = self.selectableObjects[self.objectIndex]["Object"]
        subelement_name = self.selectableObjects[self.objectIndex]["Component"]
        FreeCADGui.getMainWindow().showMessage(
            "Cycle preselected (TAB): {} - {}".format(object_name, subelement_name), 0
        )
        FreeCADGui.Selection.setPreselection(
            FreeCAD.ActiveDocument.getObject(object_name), subelement_name
        )


class Setup:
    def __init__(self):
        self._view_callbacks = []

    def _attach_view(self, view):
        from pivy import coin

        if not view or not hasattr(view, "getSceneGraph"):
            return
        if any(registered_view is view for registered_view, _callbacks in self._view_callbacks):
            return

        cos = CyclicObjectSelector()
        callbacks = [
            (
                coin.SoMouseButtonEvent.getClassTypeId(),
                view.addEventCallbackPivy(
                    coin.SoMouseButtonEvent.getClassTypeId(),
                    cos.selectObject,
                ),
            ),
            (
                coin.SoKeyboardEvent.getClassTypeId(),
                view.addEventCallbackPivy(
                    coin.SoKeyboardEvent.getClassTypeId(),
                    cos.cycleSelectableObjects,
                ),
            ),
        ]
        self._view_callbacks.append((view, callbacks))

    def _detach_view(self, view, callbacks):
        for event_type, callback in callbacks:
            try:
                view.removeEventCallbackPivy(event_type, callback)
            except Exception:
                pass

    def dispose(self):
        while self._view_callbacks:
            view, callbacks = self._view_callbacks.pop()
            self._detach_view(view, callbacks)
        _release_cyclic_selection_observer()

    def activeCallbackCount(self):
        return sum(len(callbacks) for _view, callbacks in self._view_callbacks)

    def slotActivateDocument(self, doc):
        self._attach_view(getattr(doc, "ActiveView", None))
