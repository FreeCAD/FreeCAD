# SPDX-License-Identifier: LGPL-2.1-or-later

import argparse
import json
import os
import sys
import tempfile

import FreeCAD as App

THIS_DIR = os.path.dirname(__file__)
ASSEMBLY_DIR = os.path.dirname(THIS_DIR)
if ASSEMBLY_DIR not in sys.path:
    sys.path.insert(0, ASSEMBLY_DIR)

import Preferences


def _allow_partial(assembly_link):
    return "AllowPartial" in assembly_link.getPropertyStatus("LinkedObject")


def _has_joint_group(assembly_link):
    return any(getattr(obj, "TypeId", "") == "Assembly::JointGroup" for obj in assembly_link.Group)


def _default_output_path(threshold):
    filename = "AssemblyLargeReview_threshold_{}.FCStd".format(threshold)
    return os.path.join(tempfile.gettempdir(), filename)


def _default_small_components(threshold):
    if threshold <= 1:
        return 1
    return threshold - 1


def _default_large_components(threshold):
    if threshold <= 0:
        return 25
    return threshold + 5


def _create_subassembly(doc, name, component_count):
    subassembly = doc.addObject("Assembly::AssemblyObject", name)
    for index in range(component_count):
        subassembly.newObject("App::Part", "{}Part{}".format(name, index))
    return subassembly


def _create_link(assembly, name, linked_object, load_mode, rigid=False):
    assembly_link = assembly.newObject("Assembly::AssemblyLink", name)
    assembly_link.LinkedObject = linked_object
    assembly_link.Rigid = rigid
    assembly_link.LoadMode = load_mode
    return assembly_link


def _summarize_link(assembly_link):
    return {
        "name": assembly_link.Name,
        "linked_object": assembly_link.LinkedObject.Name,
        "load_mode": assembly_link.LoadMode,
        "allow_partial": _allow_partial(assembly_link),
        "joint_group_present": _has_joint_group(assembly_link),
    }


def create_review_document(threshold, small_components=None, large_components=None):
    if small_components is None:
        small_components = _default_small_components(threshold)
    if large_components is None:
        large_components = _default_large_components(threshold)
    if large_components < small_components:
        raise ValueError("large_components must be greater than or equal to small_components")

    pref = Preferences.preferences()
    pref.SetInt("LargeAssemblyThreshold", threshold)

    doc = App.newDocument("AssemblyLargeReview")
    assembly = doc.addObject("Assembly::AssemblyObject", "Assembly")
    small_subassembly = _create_subassembly(doc, "SmallSubAssembly", small_components)
    large_subassembly = _create_subassembly(doc, "LargeSubAssembly", large_components)

    links = [
        _create_link(assembly, "SmallAutoLink", small_subassembly, "Auto"),
        _create_link(assembly, "LargeNormalLink", large_subassembly, "Normal"),
        _create_link(assembly, "LargeAutoLink", large_subassembly, "Auto"),
        _create_link(assembly, "LargeLightweightLink", large_subassembly, "Lightweight"),
    ]

    doc.recompute()
    return doc, links


def summarize_document(doc, links, threshold, output_path):
    return {
        "document": doc.Name,
        "output_path": output_path,
        "threshold": threshold,
        "links": [_summarize_link(assembly_link) for assembly_link in links],
    }


def format_summary(summary):
    rows = [
        "Created Assembly review scenario:",
        "  document: {}".format(summary["document"]),
        "  output: {}".format(summary["output_path"]),
        "  threshold: {}".format(summary["threshold"]),
        "  links:",
    ]
    for link in summary["links"]:
        rows.append(
            "    - {name}: mode={load_mode}, partial={allow_partial}, joint-group={joint_group_present}, target={linked_object}".format(
                **link
            )
        )
    rows.extend(
        [
            "Next GUI checks:",
            "  1. Right-click each AssemblyLink and inspect the Load mode submenu.",
            "  2. Verify Auto matches SmallAutoLink below threshold and LargeAutoLink above threshold.",
            "  3. Re-run with --threshold 0 and confirm Auto no longer switches to lightweight behavior.",
        ]
    )
    return "\n".join(rows)


def build_arg_parser():
    parser = argparse.ArgumentParser(
        description="Create a review document for Assembly large-assembly load modes."
    )
    parser.add_argument(
        "--threshold",
        type=int,
        default=Preferences.DEFAULT_LARGE_ASSEMBLY_THRESHOLD,
        help="Large assembly threshold to apply before creating the review document.",
    )
    parser.add_argument(
        "--small-components",
        type=int,
        default=None,
        help="Component count used by the below-threshold Auto scenario.",
    )
    parser.add_argument(
        "--large-components",
        type=int,
        default=None,
        help="Component count used by the above-threshold Auto/Normal/Lightweight scenarios.",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="FCStd path to create for manual GUI review.",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Emit the scenario summary as JSON.",
    )
    parser.add_argument(
        "--restore-threshold",
        action="store_true",
        help="Restore the previous threshold after generating the document.",
    )
    return parser


def main(argv=None):
    if argv is None and running_under_freecadcmd():
        argv = sys.argv[2:]
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    output_path = args.output or _default_output_path(args.threshold)
    output_dir = os.path.dirname(os.path.abspath(output_path))
    if output_dir and not os.path.isdir(output_dir):
        os.makedirs(output_dir, exist_ok=True)

    pref = Preferences.preferences()
    old_threshold = pref.GetInt(
        "LargeAssemblyThreshold", Preferences.DEFAULT_LARGE_ASSEMBLY_THRESHOLD
    )

    doc = None
    try:
        doc, links = create_review_document(
            threshold=args.threshold,
            small_components=args.small_components,
            large_components=args.large_components,
        )
        doc.saveAs(output_path)
        summary = summarize_document(doc, links, args.threshold, os.path.abspath(output_path))
        if args.json:
            print(json.dumps(summary, indent=2))
        else:
            print(format_summary(summary))
        return summary
    finally:
        if args.restore_threshold:
            pref.SetInt("LargeAssemblyThreshold", old_threshold)


def running_under_freecadcmd():
    return (
        __package__ == ""
        and len(sys.argv) > 1
        and os.path.abspath(sys.argv[1]) == os.path.abspath(__file__)
    )


if __name__ == "__main__" or running_under_freecadcmd():
    main()
