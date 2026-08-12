# SPDX-License-Identifier: LGPL-2.1-or-later

import argparse
import json
import time

import FreeCAD as App

import Preferences


def _has_joint_group(assembly_link):
    return any(getattr(obj, "TypeId", "") == "Assembly::JointGroup" for obj in assembly_link.Group)


def create_benchmark_document(component_count, link_count, rigid=False):
    doc = App.newDocument("AssemblyBenchmark")
    assembly = doc.addObject("Assembly::AssemblyObject", "Assembly")
    subassembly = doc.addObject("Assembly::AssemblyObject", "BenchmarkSubAssembly")
    for index in range(component_count):
        subassembly.newObject("App::Part", f"Part{index}")

    assembly_links = []
    for index in range(link_count):
        assembly_link = assembly.newObject("Assembly::AssemblyLink", f"SubAssemblyLink{index}")
        assembly_link.LinkedObject = subassembly
        assembly_link.Rigid = rigid
        assembly_links.append(assembly_link)

    doc.recompute()
    return doc, assembly_links


def measure_load_mode(component_count, load_mode, threshold, link_count=1, rigid=False):
    pref = Preferences.preferences()
    old_threshold = pref.GetInt(
        "LargeAssemblyThreshold", Preferences.DEFAULT_LARGE_ASSEMBLY_THRESHOLD
    )

    doc = None
    try:
        pref.SetInt("LargeAssemblyThreshold", threshold)
        doc, assembly_links = create_benchmark_document(component_count, link_count, rigid=rigid)

        start = time.perf_counter()
        for assembly_link in assembly_links:
            assembly_link.LoadMode = load_mode
        doc.recompute()
        elapsed = time.perf_counter() - start

        assembly_link = assembly_links[0]
        return {
            "component_count": component_count,
            "link_count": link_count,
            "load_mode": load_mode,
            "threshold": threshold,
            "elapsed_seconds": elapsed,
            "allow_partial": "AllowPartial" in assembly_link.getPropertyStatus("LinkedObject"),
            "joint_group_present": _has_joint_group(assembly_link),
        }
    finally:
        pref.SetInt("LargeAssemblyThreshold", old_threshold)
        if doc is not None:
            App.closeDocument(doc.Name)


def run_benchmark(component_counts, load_modes, threshold, link_count=1, rigid=False):
    results = []
    for component_count in component_counts:
        for load_mode in load_modes:
            results.append(
                measure_load_mode(
                    component_count=component_count,
                    load_mode=load_mode,
                    threshold=threshold,
                    link_count=link_count,
                    rigid=rigid,
                )
            )
    return results


def build_arg_parser():
    parser = argparse.ArgumentParser(
        description="Benchmark helper for Assembly large-assembly load modes."
    )
    parser.add_argument(
        "--components",
        type=int,
        nargs="+",
        default=[50, Preferences.DEFAULT_LARGE_ASSEMBLY_THRESHOLD, 500],
        help="Component counts to generate in the synthetic sub-assembly.",
    )
    parser.add_argument(
        "--load-modes",
        nargs="+",
        default=["Normal", "Auto", "Lightweight"],
        help="AssemblyLink load modes to benchmark.",
    )
    parser.add_argument(
        "--threshold",
        type=int,
        default=Preferences.DEFAULT_LARGE_ASSEMBLY_THRESHOLD,
        help="Large assembly threshold used while benchmarking Auto mode.",
    )
    parser.add_argument(
        "--links",
        type=int,
        default=1,
        help="Number of AssemblyLink instances that reference the generated sub-assembly.",
    )
    parser.add_argument(
        "--rigid",
        action="store_true",
        help="Benchmark rigid links instead of the default flexible sub-assembly behavior.",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Emit results as JSON instead of a human-readable table.",
    )
    return parser


def format_results(results):
    header = "components | links | mode        | threshold | seconds   | partial | joint-group"
    rows = [header, "-" * len(header)]
    for result in results:
        rows.append(
            "{component_count:10d} | {link_count:5d} | {load_mode:11s} | {threshold:9d} | "
            "{elapsed_seconds:9.6f} | {allow_partial!s:7s} | {joint_group_present!s}".format(
                **result
            )
        )
    return "\n".join(rows)


def main(argv=None):
    parser = build_arg_parser()
    args = parser.parse_args(argv)
    results = run_benchmark(
        component_counts=args.components,
        load_modes=args.load_modes,
        threshold=args.threshold,
        link_count=args.links,
        rigid=args.rigid,
    )
    if args.json:
        print(json.dumps(results, indent=2))
    else:
        print(format_results(results))
    return results


if __name__ == "__main__":
    main()
