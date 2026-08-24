#!/usr/bin/env python3
"""Export a colored STEP from doghouse face prediction JSON."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def _load_prediction(path: str | Path):
    with open(path, encoding="utf-8") as f:
        data = json.load(f)
    dog_faces = set()
    for inst in data.get("doghouse_instances", []):
        for idx in inst.get("faces", []):
            dog_faces.add(int(idx))
    if not dog_faces:
        for row in data.get("face_predictions", []):
            if int(row.get("doghouse", 0)) > 0:
                dog_faces.add(int(row["face_idx"]))
    return data, dog_faces


def _load_step_faces(step_path: str | Path):
    from OCC.Core.STEPControl import STEPControl_Reader
    from OCC.Core.TopAbs import TopAbs_FACE
    from OCC.Core.TopExp import TopExp_Explorer

    reader = STEPControl_Reader()
    if reader.ReadFile(str(step_path)) != 1:
        raise RuntimeError(f"failed to read STEP: {step_path}")
    reader.TransferRoots()
    shape = reader.OneShape()
    faces = []
    exp = TopExp_Explorer(shape, TopAbs_FACE)
    while exp.More():
        faces.append(exp.Current())
        exp.Next()
    return shape, faces


def export_colored_step(
    step_path: str | Path,
    prediction_json: str | Path,
    output_step: str | Path,
) -> None:
    from OCC.Core.Interface import Interface_Static
    from OCC.Core.Quantity import Quantity_Color, Quantity_TOC_RGB
    from OCC.Core.STEPCAFControl import STEPCAFControl_Writer
    from OCC.Core.STEPControl import STEPControl_AsIs
    from OCC.Core.TDocStd import TDocStd_Document
    from OCC.Core.XCAFDoc import XCAFDoc_ColorSurf, XCAFDoc_DocumentTool

    _data, dog_faces = _load_prediction(prediction_json)
    shape, faces = _load_step_faces(step_path)

    doc = TDocStd_Document("XCAF")
    label = doc.Main()
    shape_tool = XCAFDoc_DocumentTool.ShapeTool(label)
    color_tool = XCAFDoc_DocumentTool.ColorTool(label)
    shape_tool.AddShape(shape)

    red = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB)
    gray = Quantity_Color(0.72, 0.72, 0.72, Quantity_TOC_RGB)
    for idx, face in enumerate(faces):
        color_tool.SetColor(face, red if idx in dog_faces else gray, XCAFDoc_ColorSurf)

    output_step = Path(output_step)
    output_step.parent.mkdir(parents=True, exist_ok=True)
    Interface_Static.SetCVal("write.step.unit", "MM")
    Interface_Static.SetCVal("write.step.schema", "AP214CD")
    writer = STEPCAFControl_Writer()
    writer.Transfer(doc, STEPControl_AsIs)
    writer.Write(str(output_step))

    print(f"saved: {output_step}")
    print(f"source_step: {step_path}")
    print(f"prediction_json: {prediction_json}")
    print(f"red_faces: {len(dog_faces)} / {len(faces)}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--step", help="Source STEP. Defaults to source_step in prediction JSON.")
    parser.add_argument("--prediction-json", required=True)
    parser.add_argument("--output-step", required=True)
    args = parser.parse_args()

    data, _ = _load_prediction(args.prediction_json)
    step_path = args.step or data.get("source_step")
    if not step_path:
        raise ValueError("--step is required when prediction JSON has no source_step")
    export_colored_step(step_path, args.prediction_json, args.output_step)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
