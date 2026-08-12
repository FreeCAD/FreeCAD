# SPDX-License-Identifier: LGPL-2.1-or-later

import argparse
import os
import sys
import tempfile

import FreeCAD as App


DEFAULT_BOX_LENGTH = 40.0
DEFAULT_BOX_WIDTH = 40.0
DEFAULT_BOX_HEIGHT = 20.0
DEFAULT_SUPPORT_FACE = "Face6"


def default_output_path():
    return os.path.join(tempfile.gettempdir(), "SketcherSupportFaceReview.FCStd")


def create_review_document(
    box_length=DEFAULT_BOX_LENGTH,
    box_width=DEFAULT_BOX_WIDTH,
    box_height=DEFAULT_BOX_HEIGHT,
    support_face=DEFAULT_SUPPORT_FACE,
):
    doc = App.newDocument("SketcherSupportFaceReview")

    box = doc.addObject("Part::Box", "SupportBox")
    box.Length = box_length
    box.Width = box_width
    box.Height = box_height

    sketch = doc.addObject("Sketcher::SketchObject", "MappedSketch")
    doc.recompute()

    sketch.AttachmentSupport = (box, (support_face,))
    sketch.MapMode = "FlatFace"
    doc.recompute()

    if len(sketch.ExternalGeometry) != 1:
        raise RuntimeError(
            "Expected one automatic support reference, got {}".format(
                len(sketch.ExternalGeometry)
            )
        )

    if len(sketch.ExternalGeo) <= 2:
        raise RuntimeError(
            "Expected automatic support edges to be exposed in ExternalGeo"
        )

    return doc, box, sketch


def summarize_document(doc, box, sketch, output_path):
    return {
        "document": doc.Name,
        "output_path": output_path,
        "support_object": box.Name,
        "support_face": tuple(sketch.AttachmentSupport[0][1])[0],
        "external_geometry_count": len(sketch.ExternalGeometry),
        "external_geo_entries": len(sketch.ExternalGeo),
        "map_mode": sketch.MapMode,
    }


def format_summary(summary):
    return "\n".join(
        [
            "Created Sketcher review scenario:",
            "  document: {}".format(summary["document"]),
            "  output: {}".format(summary["output_path"]),
            "  support: {}.{}".format(summary["support_object"], summary["support_face"]),
            "  map mode: {}".format(summary["map_mode"]),
            "  external refs: {}".format(summary["external_geometry_count"]),
            "  external geo entries: {}".format(summary["external_geo_entries"]),
            "Next GUI checks:",
            "  1. Open MappedSketch and start Sketcher_CreateLine.",
            "  2. Snap directly to the support-face border without manual projection.",
            "  3. Disable mapping or remove the support and verify cleanup.",
        ]
    )


def build_arg_parser():
    parser = argparse.ArgumentParser(
        description="Create a review document for Sketcher automatic support-face borders."
    )
    parser.add_argument(
        "--output",
        default=default_output_path(),
        help="FCStd path to create for manual GUI review.",
    )
    parser.add_argument(
        "--support-face",
        default=DEFAULT_SUPPORT_FACE,
        help="Support face to map the sketch onto.",
    )
    parser.add_argument(
        "--box-length",
        type=float,
        default=DEFAULT_BOX_LENGTH,
        help="Length of the support box.",
    )
    parser.add_argument(
        "--box-width",
        type=float,
        default=DEFAULT_BOX_WIDTH,
        help="Width of the support box.",
    )
    parser.add_argument(
        "--box-height",
        type=float,
        default=DEFAULT_BOX_HEIGHT,
        help="Height of the support box.",
    )
    return parser


def main(argv=None):
    if argv is None and running_under_freecadcmd():
        argv = sys.argv[2:]
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    output_dir = os.path.dirname(os.path.abspath(args.output))
    if output_dir and not os.path.isdir(output_dir):
        os.makedirs(output_dir, exist_ok=True)

    doc, box, sketch = create_review_document(
        box_length=args.box_length,
        box_width=args.box_width,
        box_height=args.box_height,
        support_face=args.support_face,
    )
    doc.saveAs(args.output)

    summary = summarize_document(doc, box, sketch, os.path.abspath(args.output))
    print(format_summary(summary))
    return summary


def running_under_freecadcmd():
    return (
        __package__ == ""
        and len(sys.argv) > 1
        and os.path.abspath(sys.argv[1]) == os.path.abspath(__file__)
    )


if __name__ == "__main__" or running_under_freecadcmd():
    main()
