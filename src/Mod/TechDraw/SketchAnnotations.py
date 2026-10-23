# SPDX-License-Identifier: LGPL-2.1-or-later
"""Reconcile Sketcher visual annotations with native TechDraw objects.

The adapter belongs to TechDraw; neither Sketcher nor its solver imports it.
Source identities are persisted on the native objects. Reconciliation happens at
transaction/recompute boundaries, or on the next GUI event, never from execute().
"""

import math
import os
import tempfile
import FreeCAD as App

_TYPES = {
    "Text": "TechDraw::DrawRichAnno",
    "Leader": "TechDraw::DrawLeaderLine",
    "Hatch": "TechDraw::DrawGeomHatch",
}
_GROUP = "Sketch annotation"
_busy = False
# The adapter stays dormant until a document actually holds a drawing page. Until then
# it must not cost anything: a document observer with a slotChangedObject is called for
# every property change of every object in every document.
_active = False
# A view whose projection is still running is retried, but never forever.
_MAX_PENDING_RETRIES = 40
_RETRY_INTERVAL = 100
# Sketch properties whose changes can alter the linked page annotations.
_SKETCH_PROPERTIES = frozenset(
    ("Annotations", "Geometry", "ExternalGeo", "Placement", "Visibility", "Label")
)


def _set(obj, name, value):
    previous = getattr(obj, name)
    if name == "Source" and isinstance(value, tuple):
        previous = (previous[0], tuple(previous[1])) if previous else None
        value = (value[0], tuple(value[1]))
    if obj.getTypeIdOfProperty(name) == "App::PropertyColor":
        previous = tuple(previous)[:3]
        value = tuple(value)[:3]
    if previous != value:
        setattr(obj, name, value)


def _property(obj, kind, name, description, hidden=False):
    if name not in obj.PropertiesList:
        obj.addProperty("App::Property" + kind, name, _GROUP, description)
    obj.setEditorMode(name, 2 if hidden else 1)


def _create(doc, view, sketch, annotation, page):
    obj = doc.addObject(_TYPES[annotation["Type"]], "SketchAnnotation")
    _property(obj, "Link", "SourceSketch", "Sketch containing the annotation")
    _property(obj, "Integer", "SourceAnnotationId", "Stable annotation identifier")
    _property(obj, "Link", "AnnotationView", "Destination view")
    _property(obj, "String", "AnnotationStatus", "Status of the linked annotation")
    obj.addProperty(
        "App::PropertyVector",
        "PageOffset",
        _GROUP,
        "Additional page offset in millimetres, independent of the source",
    )
    obj.SourceSketch = sketch
    obj.SourceAnnotationId = annotation["Id"]
    obj.AnnotationView = view
    if annotation["Type"] != "Hatch":
        page.addView(obj)
        obj.LockPosition = True
        for name in ("X", "Y", "Rotation", "LockPosition"):
            obj.setEditorMode(name, 1)
    return obj


def _point(view, sketch, point, transformed=True):
    world = sketch.getGlobalPlacement().multVec(point)
    p = view.projectPoint(world - view.getGeometricCenter(), False)
    if transformed:
        p *= float(view.Scale)
        p = App.Rotation(App.Vector(0, 0, 1), float(view.Rotation)).multVec(p)
    return p


def _visible(sketch, annotation):
    if annotation["Construction"] or annotation["Id"] in getattr(
        sketch.ViewObject, "HiddenAnnotations", ()
    ):
        return False
    return True


def _view_faces(view, cache):
    """Extracting every projected face is expensive; do it once per reconciliation."""
    faces = cache.get(view.Name)
    if faces is None:
        faces = view.getFaces()
        cache[view.Name] = faces
    return faces


def _hatch_faces(view, sketch, annotation, cache):
    """Match the complete projected stable boundary, including every hole.

    No face index is retained across projections and no nearest-face fallback is
    permitted. An ambiguous, partial, or edge-on projection stays unresolved.
    """
    face = sketch.getAnnotationFace(annotation["Id"])
    origin = _point(view, sketch, App.Vector())
    x = _point(view, sketch, App.Vector(1, 0, 0)) - origin
    y = _point(view, sketch, App.Vector(0, 1, 0)) - origin
    if x.cross(y).Length < 1.0e-9:
        raise ValueError("Hatch plane is edge-on to this view")
    matrix = App.Matrix(x.x, y.x, 0, origin.x, x.y, y.y, 0, origin.y, 0, 0, 1, 0, 0, 0, 0, 1)
    projected = face.transformGeometry(matrix)
    candidates = _view_faces(view, cache)
    matches = []
    for source_face in projected.Faces:
        found = []
        tolerance = max(1.0e-7, source_face.Area * 1.0e-7)
        for index, target in enumerate(candidates):
            if target.isNull():
                continue
            if abs(target.Area - source_face.Area) > tolerance:
                continue
            common = source_face.common(target)
            if abs(common.Area - source_face.Area) <= tolerance:
                found.append(index)
        if len(found) != 1:
            raise ValueError("Hatch boundary has no unique corresponding projected face")
        matches.append("Face" + str(found[0]))
    if len(matches) != len(set(matches)):
        raise ValueError("Hatch boundaries overlap in this projection")
    return matches


def _update(obj, view, sketch, a, page, cache):
    kind = a["Type"]
    _set(obj, "Label", a["Label"])
    visible = _visible(sketch, a)
    error = ""
    if kind == "Text":
        _set(obj, "AnnoParent", view)
        _set(obj, "AnnoText", a["Html"] if visible else "")
        _set(obj, "ShowFrame", False)
        _set(obj, "OriginCentered", False)
        _set(obj, "TextHeight", a["TextSize"] * float(view.Scale))
        _set(obj, "MaxWidth", a["TextWidth"] * float(view.Scale) if a["TextWidth"] else -1.0)
        p = _point(view, sketch, a["Position"]) + obj.PageOffset
        _set(obj, "X", p.x)
        _set(obj, "Y", p.y)
        angle = math.radians(a["Rotation"])
        origin = _point(view, sketch, App.Vector())
        direction = _point(view, sketch, App.Vector(math.cos(angle), math.sin(angle), 0)) - origin
        _set(obj, "Rotation", math.degrees(math.atan2(direction.y, direction.x)))
        for name in (
            "AnnoText",
            "AnnoParent",
            "ShowFrame",
            "OriginCentered",
            "MaxWidth",
            "TextHeight",
        ):
            obj.setEditorMode(name, 1)
    elif kind == "Leader":
        _set(obj, "LeaderParent", view)
        _set(obj, "Scalable", True)
        _set(obj, "RotatesWithParent", True)
        _set(obj, "AutoHorizontal", False)
        _set(obj, "ArrowSize", a["ArrowSize"] * float(view.Scale))
        points = [_point(view, sketch, p, False) for p in a["Points"]]
        start = points[0]
        _set(
            obj,
            "WayPoints",
            [App.Vector(p.x - start.x, start.y - p.y, 0) for p in points] if visible else [],
        )
        offset = App.Rotation(App.Vector(0, 0, 1), -float(view.Rotation)).multVec(
            obj.PageOffset
        ) / float(view.Scale)
        p = _point(view, sketch, a["Points"][0], False) + offset
        _set(obj, "X", p.x)
        _set(obj, "Y", p.y)
        _set(obj, "StartSymbol", a["ArrowStyle"])
        _set(obj, "EndSymbol", "None")
        for name in (
            "WayPoints",
            "LeaderParent",
            "Scalable",
            "RotatesWithParent",
            "AutoHorizontal",
            "StartSymbol",
            "EndSymbol",
            "ArrowSize",
        ):
            obj.setEditorMode(name, 1)
    else:
        try:
            names = _hatch_faces(view, sketch, a, cache) if visible else []
            _set(obj, "Source", (view, names))
            origin = _point(view, sketch, App.Vector())
            anchor = _point(view, sketch, a["Position"])
            specs = []
            # The sketch draws exactly these PAT families; map each one through the view.
            for family in sketch.getAnnotationPattern(a["Id"]):
                direction = _point(view, sketch, family["Direction"]) - origin
                length = direction.Length
                if length < 1.0e-12:
                    raise ValueError("Hatch pattern collapses in this projection")
                unit = direction * (1.0 / length)
                normal = App.Vector(-unit.y, unit.x, 0)
                offset = _point(view, sketch, family["Offset"]) - origin
                start = _point(view, sketch, family["Origin"]) - anchor
                if abs(offset.dot(normal)) < 1.0e-9:
                    raise ValueError("Hatch pattern collapses in this projection")
                values = [
                    math.degrees(math.atan2(unit.y, unit.x)),
                    start.x,
                    start.y,
                    offset.dot(unit),
                    offset.dot(normal),
                ] + [dash * length for dash in family["Dashes"]]
                specs.append(",".join(f"{value:.12g}" for value in values))
            pattern = "*SKETCH_ANNOTATION,Linked Sketcher hatch\n" + "\n".join(specs) + "\n"
            _property(obj, "String", "AnnotationPattern", "Embedded projected pattern", True)
            if obj.AnnotationPattern != pattern or not obj.PatIncluded:
                # DrawGeomHatch copies this into its included-file property immediately.
                with tempfile.NamedTemporaryFile(
                    mode="w", suffix=".pat", delete=False, encoding="ascii"
                ) as stream:
                    stream.write(pattern)
                    path = stream.name
                try:
                    obj.FilePattern = path
                    obj.NamePattern = "SKETCH_ANNOTATION"
                    obj.recompute()
                    obj.AnnotationPattern = pattern
                finally:
                    os.unlink(path)
            _set(obj, "ScalePattern", 1.0)
            _set(obj, "PatternRotation", 0.0)
            _set(obj, "PatternOffset", _point(view, sketch, a["Position"]) + obj.PageOffset)
        except Exception as exc:
            # One unresolvable hatch must not abandon the rest of the document.
            error = str(exc)
            visible = False
            _set(obj, "Source", (view, []))
        for name in (
            "Source",
            "FilePattern",
            "NamePattern",
            "ScalePattern",
            "PatternRotation",
            "PatternOffset",
        ):
            obj.setEditorMode(name, 1)
    _set(obj, "AnnotationStatus", error or ("Linked" if visible else "Hidden by source"))
    if App.GuiUp:
        _set(obj.ViewObject, "Visibility", visible)
        # Cosmetics use black, the default page color.
        rgb = (0.0, 0.0, 0.0)
        if kind == "Text":
            _property(obj, "Color", "AnnotationColor", "Linked color", True)
            _set(obj, "AnnotationColor", rgb)
        elif kind == "Leader":
            _set(obj.ViewObject, "Color", rgb)
        else:
            _set(obj.ViewObject, "ColorPattern", rgb)

    if obj.State and "Touched" in obj.State:
        obj.recompute()


def synchronize(doc):
    """Update the native counterparts in a document; also available headlessly."""
    global _busy
    if _busy or doc.Restoring:
        return
    _activate()
    _busy = True
    try:
        if App.GuiUp:
            installGuiObserver()
        faces = {}
        existing = {
            (
                o.AnnotationView.Name if o.AnnotationView else "",
                o.SourceSketch.Name if o.SourceSketch else "",
                o.SourceAnnotationId,
            ): o
            for o in doc.Objects
            if "SourceAnnotationId" in o.PropertiesList
            and "AnnotationView" in o.PropertiesList
            and "SourceSketch" in o.PropertiesList
        }
        wanted = set()
        pending = False
        pages = [o for o in doc.Objects if o.isDerivedFrom("TechDraw::DrawPage")]
        for view in list(doc.Objects):
            if not view.isDerivedFrom("TechDraw::DrawViewPart") or not hasattr(view, "Source"):
                continue
            page = next((page for page in pages if view in page.Views), None)
            if page is None:
                continue
            if not view.isProjectionReady():
                pending = True
                wanted.update(key for key in existing if key[0] == view.Name)
                continue
            for sketch in view.Source:
                if not sketch.isDerivedFrom("Sketcher::SketchObject") or not hasattr(
                    sketch, "Annotations"
                ):
                    continue
                for annotation in sketch.Annotations:
                    key = (view.Name, sketch.Name, annotation["Id"])
                    wanted.add(key)
                    obj = existing.get(key)
                    if obj is None:
                        if annotation["Construction"]:
                            continue
                        obj = _create(doc, view, sketch, annotation, page)
                    try:
                        _update(obj, view, sketch, annotation, page, faces)
                    except Exception as exc:
                        App.Console.PrintError(
                            "Sketch annotation '{}': {}\n".format(obj.Name, exc)
                        )
                        try:
                            _set(obj, "AnnotationStatus", str(exc))
                        except Exception:
                            pass
        # Retain counterparts and identity through source deletion/undo. Suppress content.
        for key, obj in existing.items():
            if key in wanted:
                continue
            _set(obj, "AnnotationStatus", "Source annotation unavailable")
            if hasattr(obj, "AnnoText"):
                _set(obj, "AnnoText", "")
            if hasattr(obj, "WayPoints"):
                _set(obj, "WayPoints", [])
            if obj.isDerivedFrom("TechDraw::DrawGeomHatch"):
                _set(obj, "Source", (obj.AnnotationView, []) if obj.AnnotationView else None)
            if App.GuiUp:
                _set(obj.ViewObject, "Visibility", False)
        return pending
    finally:
        _busy = False


class _Observer:
    def __init__(self):
        self.pending = set()
        self.queued = False
        self.retries = {}

    def queue(self, doc):
        if _busy or doc.Restoring:
            return
        self.pending.add(doc.Name)
        # A real change restarts the budget for waiting on an asynchronous projection.
        self.retries.pop(doc.Name, None)
        if App.GuiUp and not self.queued:
            from PySide import QtCore

            self.queued = True
            QtCore.QTimer.singleShot(0, self.flush)

    def flush(self):
        self.queued = False
        names, self.pending = self.pending, set()
        for name in names:
            doc = App.listDocuments().get(name)
            if doc is None:
                self.retries.pop(name, None)
                continue
            try:
                if not synchronize(doc):
                    self.retries.pop(name, None)
                    continue
            except Exception as exc:
                self.retries.pop(name, None)
                App.Console.PrintError("Sketch annotation synchronization: " + str(exc) + "\n")
                continue
            # A projection that never becomes ready must not spin a timer forever.
            attempts = self.retries.get(name, 0) + 1
            if attempts > _MAX_PENDING_RETRIES:
                self.retries.pop(name, None)
                App.Console.PrintWarning(
                    "Sketch annotations: the projection of '{}' is not ready; linked "
                    "annotations will update on the next change.\n".format(name)
                )
                continue
            self.retries[name] = attempts
            self.pending.add(name)
        if self.pending and App.GuiUp and not self.queued:
            from PySide import QtCore

            self.queued = True
            QtCore.QTimer.singleShot(_RETRY_INTERVAL, self.flush)

    def slotChangedObject(self, obj, prop):
        # Only sketch properties that change what the page shows: a solver step while
        # geometry is dragged must not re-match every hatch face.
        if (
            (hasattr(obj, "Annotations") and prop in _SKETCH_PROPERTIES)
            or obj.isDerivedFrom("TechDraw::DrawViewPart")
            or prop == "PageOffset"
        ):
            self.queue(obj.Document)

    def slotRecomputedDocument(self, doc):
        if not _busy:
            self.queue(doc)
            if not App.GuiUp:
                self.flush()

    def slotDeletedObject(self, obj):
        self.queue(obj.Document)

    def slotCommitTransaction(self, doc):
        self.queue(doc)

    def slotUndoDocument(self, doc):
        self.queue(doc)

    def slotRedoDocument(self, doc):
        self.queue(doc)

    def slotFinishRestoreDocument(self, doc):
        self.queue(doc)


_observer = _Observer()


def _hasPage(doc):
    return any(o.isDerivedFrom("TechDraw::DrawPage") for o in doc.Objects)


def _activate():
    """Attach the reconciling observer. Idempotent, and never called on import."""
    global _active
    if _active:
        return
    _active = True
    App.addDocumentObserver(_observer)
    if App.GuiUp:
        installGuiObserver()


class _Sentinel:
    """Watches only for a document that has a drawing page.

    Deliberately has no slotChangedObject: FreeCAD connects only the slots an observer
    actually defines, so this costs nothing on the property-change path.
    """

    def slotCreatedObject(self, obj):
        try:
            if obj.isDerivedFrom("TechDraw::DrawPage"):
                _activate()
                _observer.queue(obj.Document)
        except Exception:
            pass

    def slotFinishRestoreDocument(self, doc):
        try:
            if _hasPage(doc):
                _activate()
                _observer.queue(doc)
        except Exception:
            pass


_sentinel = _Sentinel()
App.addDocumentObserver(_sentinel)


class _GuiObserver:
    def slotChangedObject(self, provider, prop):
        if prop == "HiddenAnnotations":
            obj = provider.Object
            if hasattr(obj, "Annotations"):
                _observer.queue(obj.Document)


_guiObserver = None


def installGuiObserver():
    global _guiObserver
    if _guiObserver is None:
        import FreeCADGui as Gui

        _guiObserver = _GuiObserver()
        Gui.addDocumentObserver(_guiObserver)
