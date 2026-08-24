"""Clip library scanning for the FreeCAD task panel."""
from __future__ import annotations

import json
from pathlib import Path


STEP_SUFFIXES = (".step", ".stp", ".STEP", ".STP")
IMAGE_SUFFIXES = (".png", ".jpg", ".jpeg", ".bmp", ".webp", ".PNG", ".JPG", ".JPEG", ".BMP", ".WEBP")


def _bolt_cyl_indices(label_json: Path) -> set[int]:
    if not label_json.exists():
        return set()
    data = json.loads(label_json.read_text(encoding="utf-8"))
    out = set()
    for face in data.get("faces") or []:
        if str(face.get("type", "")).upper() == "BOLT_CYL":
            index = face.get("index")
            if index is not None:
                out.add(int(index) - 1)
    return out


def _read_geom_specs(path: Path, label_json: Path | None = None) -> tuple[float, float]:
    if not path.exists():
        return 0.0, 0.0
    data = json.loads(path.read_text(encoding="utf-8"))
    faces = data.get("faces") or []
    bolt_indices = _bolt_cyl_indices(label_json) if label_json else set()
    radii = []
    heights = []
    for face in faces:
        label = str(face.get("label", "")).upper()
        face_idx = face.get("face_idx")
        is_bolt_cyl = "BOLT_CYL" in label or (
            face_idx is not None and int(face_idx) in bolt_indices
        )
        if not is_bolt_cyl:
            continue
        radius = face.get("radius")
        if radius is not None:
            radii.append(float(radius))
        if face.get("v_min") is not None and face.get("v_max") is not None:
            heights.append(abs(float(face["v_max"]) - float(face["v_min"])))
        elif face.get("depth") is not None:
            heights.append(abs(float(face["depth"])))
    radius_mm = sum(radii) / len(radii) if radii else 0.0
    height_mm = sum(heights) / len(heights) if heights else 0.0
    return radius_mm * 2.0, height_mm


def _find_step_for(stem: str, root: Path) -> Path | None:
    for suffix in STEP_SUFFIXES:
        candidate = root / f"{stem}{suffix}"
        if candidate.exists():
            return candidate
    return None


def _find_thumbnail_for(stem: str, root: Path) -> Path | None:
    search_dirs = [
        root,
        root / "thumbnails",
        root / "thumbnail",
        root / "images",
        root / "image",
        root / "缩略图",
        root / "图片",
    ]
    for folder in search_dirs:
        for suffix in IMAGE_SUFFIXES:
            candidate = folder / f"{stem}{suffix}"
            if candidate.exists():
                return candidate
    return None


def scan_clip_library(root: Path) -> list[dict]:
    root = Path(root)
    rows = []
    if not root.exists():
        return rows
    for label_json in sorted(root.glob("*.json")):
        if label_json.name.endswith(".geom.json"):
            continue
        stem = label_json.stem
        step_path = _find_step_for(stem, root)
        thumbnail_path = _find_thumbnail_for(stem, root)
        geom_path = root / f"{stem}.geom.json"
        diameter_mm, height_mm = _read_geom_specs(geom_path, label_json)
        rows.append(
            {
                "name": stem,
                "label_json": str(label_json),
                "geom_json": str(geom_path) if geom_path.exists() else "",
                "step_path": str(step_path) if step_path else "",
                "thumbnail_path": str(thumbnail_path) if thumbnail_path else "",
                "has_step": step_path is not None,
                "bolt_cyl_diameter_mm": round(float(diameter_mm), 6),
                "bolt_cyl_height_mm": round(float(height_mm), 6),
            }
        )
    return rows


def merge_recommendations(library_rows: list[dict], recommendation_payload: dict | None) -> list[dict]:
    by_name = {
        clip.get("name"): clip
        for clip in (recommendation_payload or {}).get("clips", [])
        if clip.get("name")
    }
    merged = []
    for row in library_rows:
        item = dict(row)
        rec = by_name.get(item["name"])
        if rec:
            item.update(
                {
                    "rank": rec.get("rank"),
                    "diameter_gap_mm": rec.get("diameter_gap_mm"),
                    "depth_gap_mm": rec.get("depth_gap_mm"),
                    "diameter_valid": rec.get("diameter_valid"),
                    "score": rec.get("score"),
                }
            )
        merged.append(item)
    return merged
