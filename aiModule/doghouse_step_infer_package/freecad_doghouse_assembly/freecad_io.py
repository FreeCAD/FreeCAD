"""FreeCAD document export/import helpers for preview-only assembly."""
from __future__ import annotations

import shutil
from pathlib import Path


def default_cache_dir(project_root: Path, document_name: str) -> Path:
    safe_name = "".join(ch if ch.isalnum() or ch in ("-", "_") else "_" for ch in document_name)
    return Path(project_root) / "outputs" / "freecad_plugin" / safe_name / "cache"


def _safe_stem(value: str) -> str:
    value = "".join(ch if ch.isalnum() or ch in ("-", "_") else "_" for ch in value)
    return value.strip("_") or "freecad_source"


def export_active_model_to_step(cache_dir: Path) -> Path:
    import FreeCAD
    import FreeCADGui
    import ImportGui

    doc = FreeCAD.ActiveDocument
    if doc is None:
        raise RuntimeError("No active FreeCAD document")
    cache_dir = Path(cache_dir)
    cache_dir.mkdir(parents=True, exist_ok=True)
    selected = FreeCADGui.Selection.getSelection()  # type: ignore[name-defined]
    source_objects = selected or [
        obj for obj in doc.Objects
        if getattr(obj, "ViewObject", None) is None or getattr(obj.ViewObject, "Visibility", True)
    ]
    if not source_objects:
        raise RuntimeError("No visible solid objects to analyze")
    source_name = getattr(source_objects[0], "Label", "") or getattr(source_objects[0], "Name", "") or doc.Name
    step_path = cache_dir / f"{_safe_stem(source_name)}.step"
    ImportGui.export(source_objects, str(step_path))
    return step_path


def _visible_source_objects():
    import FreeCAD
    import FreeCADGui

    doc = FreeCAD.ActiveDocument
    if doc is None:
        return []
    selected = FreeCADGui.Selection.getSelection()
    return selected or [
        obj for obj in doc.Objects
        if hasattr(obj, "Shape")
        and getattr(getattr(obj, "Shape", None), "Faces", None)
        and (getattr(obj, "ViewObject", None) is None or getattr(obj.ViewObject, "Visibility", True))
        and not obj.Name.startswith("Doghouse_Auto_Assembly_Preview")
    ]


def color_analysis_faces(recommendation_payload: dict):
    """Color detected mount faces and hole walls on the active FreeCAD model."""
    import FreeCAD
    import FreeCADGui

    try:
        view = FreeCADGui.ActiveDocument.ActiveView
        for draw_style in ("Flat lines", "Shaded", "As is"):
            try:
                view.setDrawStyle(draw_style)
                break
            except Exception:
                pass
    except Exception:
        pass

    objects = _visible_source_objects()
    if not objects:
        return
    mount_indices = {
        int(hole["mount_face_idx"])
        for hole in recommendation_payload.get("holes", [])
        if hole.get("mount_face_idx") is not None
    }
    hole_indices = {
        int(face_idx)
        for hole in recommendation_payload.get("holes", [])
        for face_idx in hole.get("hole_face_indices", [])
    }
    offset = 0
    for obj in objects:
        shape = getattr(obj, "Shape", None)
        faces = getattr(shape, "Faces", []) if shape is not None else []
        if not faces or getattr(obj, "ViewObject", None) is None:
            offset += len(faces)
            continue
        view = obj.ViewObject
        try:
            view.DisplayMode = "Flat Lines"
        except Exception:
            try:
                view.DisplayMode = "Shaded"
            except Exception:
                pass
        try:
            view.Transparency = 0
            view.ShapeColor = (0.82, 0.82, 0.82)
            view.LineColor = (0.05, 0.05, 0.05)
        except Exception:
            pass
        if not hasattr(view, "DiffuseColor"):
            offset += len(faces)
            continue
        existing = [(0.82, 0.82, 0.82)] * len(faces)
        for local_idx in range(len(faces)):
            global_idx = offset + local_idx
            if global_idx in mount_indices:
                existing[local_idx] = (1.0, 0.55, 0.05)
            elif global_idx in hole_indices:
                existing[local_idx] = (0.05, 0.45, 1.0)
        obj.ViewObject.DiffuseColor = existing
        offset += len(faces)
    FreeCAD.ActiveDocument.recompute()
    try:
        FreeCADGui.ActiveDocument.ActiveView.redraw()
    except Exception:
        pass


def _preview_group_children(group) -> list:
    children = []
    for attr in ("Group", "OutList"):
        for child in getattr(group, attr, []) or []:
            if child not in children:
                children.append(child)
            for nested in _preview_group_children(child):
                if nested not in children:
                    children.append(nested)
    return children


def _is_numbered_clip_label(obj, clip_names: set[str]) -> bool:
    label = str(getattr(obj, "Label", "") or "")
    for clip_name in clip_names:
        prefix = f"{clip_name}_"
        if label.startswith(prefix) and label[len(prefix):].isdigit():
            return True
    return False


def clear_preview_group(group_name: str = "Doghouse_Auto_Assembly_Preview", clip_names=None) -> int:
    import FreeCAD

    doc = FreeCAD.ActiveDocument
    if doc is None:
        return 0
    clip_names = {str(name) for name in (clip_names or []) if name}
    group = doc.getObject(group_name)
    to_remove = []
    if group is not None:
        to_remove.extend(_preview_group_children(group))
        to_remove.append(group)
    for obj in list(doc.Objects):
        if getattr(obj, "Name", "") == group_name:
            if obj not in to_remove:
                to_remove.append(obj)
        elif bool(getattr(obj, "DoghouseAutoPreview", False)):
            if obj not in to_remove:
                to_remove.append(obj)
        elif clip_names and _is_numbered_clip_label(obj, clip_names):
            if obj not in to_remove:
                to_remove.append(obj)
    removed = 0
    for obj in reversed(to_remove):
        name = getattr(obj, "Name", "")
        if not name or doc.getObject(name) is None:
            continue
        doc.removeObject(name)
        removed += 1
    if removed:
        doc.recompute()
    return removed


def _placement_from_matrix(matrix):
    import FreeCAD

    m = FreeCAD.Matrix()
    flat = [float(v) for row in matrix for v in row]
    for idx, value in enumerate(flat):
        setattr(m, f"A{idx // 4 + 1}{idx % 4 + 1}", value)
    return FreeCAD.Placement(m)


def import_clip_previews(placement_payload: dict, group_name: str = "Doghouse_Auto_Assembly_Preview"):
    import FreeCAD
    import ImportGui

    doc = FreeCAD.ActiveDocument
    if doc is None:
        raise RuntimeError("No active FreeCAD document")
    clear_preview_group(group_name)
    group = doc.addObject("App::DocumentObjectGroup", group_name)
    clip_step = Path(placement_payload["clip_step"])
    created = []
    for placement in placement_payload.get("placements", []):
        before = set(obj.Name for obj in doc.Objects)
        ImportGui.insert(str(clip_step), doc.Name)
        added = [obj for obj in doc.Objects if obj.Name not in before]
        for obj in added:
            obj.Label = f"{placement_payload.get('selected_clip', clip_step.stem)}_{placement.get('hole_index')}"
            try:
                obj.addProperty("App::PropertyBool", "DoghouseAutoPreview", "Doghouse")
                obj.DoghouseAutoPreview = True
            except Exception:
                pass
            obj.Placement = _placement_from_matrix(placement["matrix"])
            group.addObject(obj)
            created.append(obj)
    doc.recompute()
    return created


def copy_plugin_to_freecad_mod(source_dir: Path, freecad_mod_dir: Path) -> Path:
    """Optional helper for users who want to install this workbench manually."""
    target = Path(freecad_mod_dir) / "freecad_doghouse_assembly"
    if target.exists():
        shutil.rmtree(target)
    shutil.copytree(source_dir, target)
    return target
