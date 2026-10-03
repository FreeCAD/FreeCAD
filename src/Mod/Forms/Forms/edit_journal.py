# SPDX-License-Identifier: LGPL-2.1-or-later
"""Document changes owned by a Forms edit session, independent of the GUI."""

import FreeCAD as App


class EditJournal:
    """Restore touched objects, created objects and links on session Cancel.

    Individual tools still use ordinary FreeCAD transactions for Undo/Redo.
    This baseline is deliberately limited to objects affected by the session;
    unrelated document edits are not rolled back.
    """

    def __init__(self, obj):
        self.document = obj.Document
        self.snapshots = {}
        self.created = set()
        self.capture(obj)

    def capture(self, obj):
        if obj.Name in self.created or obj.Name in self.snapshots:
            return
        view = getattr(obj, "ViewObject", None)
        self.snapshots[obj.Name] = (
            obj.TypeId, obj.dumpContent(0),
            view.dumpContent(0) if view is not None else None,
            {name: getattr(view, name) for name in (
                "Visibility", "DisplayMode", "ShapeColor", "LineColor", "PointColor",
                "Transparency", "LineWidth", "PointSize", "ShowControlCage",
            ) if view is not None and name in view.PropertiesList
             and getattr(view, name) is not None},
        )

    def capture_removal(self, obj):
        self.capture(obj)
        # removeObject clears inbound Link/LinkSub/group references. Capture
        # their owners before deletion so recreating the object also reconnects it.
        for dependent in obj.InList:
            self.capture(dependent)

    def record_created(self, objects):
        for obj in objects:
            if obj.Name not in self.snapshots:
                self.created.add(obj.Name)

    def restore(self):
        doc = self.document
        opened = doc.getBookedTransactionID() == 0
        if opened:
            doc.openTransaction(App.Qt.translate("Forms_Edit", "Cancel form editing"))
        try:
            recreated = set()
            for name in self.created:
                if doc.getObject(name) is not None:
                    doc.removeObject(name)
            # All names must exist before restoring serialized links.
            for name, (type_id, _content, _view, _appearance) in self.snapshots.items():
                if doc.getObject(name) is None:
                    doc.addObject(type_id, name)
                    recreated.add(name)
            for name, (_type_id, content, view_content, appearance) in self.snapshots.items():
                obj = doc.getObject(name)
                obj.restoreContent(content)
                view = getattr(obj, "ViewObject", None)
                if view is not None:
                    if name in recreated and view_content is not None:
                        view.restoreContent(view_content)
                        proxy = view.Proxy
                        if proxy is not None and hasattr(proxy, "attach"):
                            proxy.attach(view)
                    else:
                        # Replacing the active view proxy while its unsetEdit is
                        # running leaves detached overlays and callback frames.
                        for property_name, value in appearance.items():
                            setattr(view, property_name, value)
                obj.purgeTouched()
            doc.recompute()
        except Exception:
            if opened and doc.getBookedTransactionID():
                doc.abortTransaction()
            raise
        if opened:
            doc.commitTransaction()

    def clear(self):
        self.snapshots.clear()
        self.created.clear()
        self.document = None
