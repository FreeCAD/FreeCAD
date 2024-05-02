import FreeCAD


class CyclicSelectionObserver:
    def addSelection(self, document, object, element, position):
        import FreeCADGui

        if not FreeCAD.ActiveDocument:
            return
        if not hasattr(FreeCAD, "CyclicSelectionObserver"):
            return
        FreeCADGui.Selection.removeSelection(FreeCAD.ActiveDocument.getObject(object))
        FreeCADGui.Selection.removeObserver(FreeCAD.CyclicSelectionObserver)
        del FreeCAD.CyclicSelectionObserver
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

        if (
            event.getState() != coin.SoMouseButtonEvent.DOWN
            or not self.selectableObjects
        ):
            return

        pos = event.getPosition().getValue()
        element_list = FreeCADGui.ActiveDocument.ActiveView.getObjectsInfo(
            (int(pos[0]), int(pos[1]))
        )

        if not element_list:
            self.selectableObjects = []
            if hasattr(FreeCAD, "CyclicSelectionObserver"):
                FreeCADGui.Selection.removeObserver(FreeCAD.CyclicSelectionObserver)
                del FreeCAD.CyclicSelectionObserver
            return

        FreeCAD.CyclicSelectionObserver = CyclicSelectionObserver()
        FreeCADGui.Selection.addObserver(FreeCAD.CyclicSelectionObserver)

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
    def slotActivateDocument(self, doc):
        from pivy import coin

        cos = CyclicObjectSelector()
        if doc and doc.ActiveView and hasattr(doc.ActiveView, "getSceneGraph"):
            self.callback = doc.ActiveView.addEventCallbackPivy(
                coin.SoMouseButtonEvent.getClassTypeId(), cos.selectObject
            )
            self.callback = doc.ActiveView.addEventCallbackPivy(
                coin.SoKeyboardEvent.getClassTypeId(), cos.cycleSelectableObjects
            )
