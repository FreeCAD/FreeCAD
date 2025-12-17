#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

"""Script to parse changed files and determine if any new C++ dependencies were added between
FreeCAD modules."""

import argparse
import os
import re
import sys

from utils import (
    init_environment,
    write_file,
    append_file,
    emit_problem_matchers,
    add_common_arguments,
)

KNOWN_MODULES = [
    "app",
    "base",
    "gui",
    "addonmanager",
    "assembly",
    "bim",
    "cam",
    "cloud",
    "draft",
    "fem",
    "help",
    "idf",
    "import",
    "inspection",
    "jtreader",
    "material",
    "measure",
    "mesh",
    "meshpart",
    "openscad",
    "part",
    "partdesign",
    "plot",
    "points",
    "reverseengineering",
    "robot",
    "sandbox",
    "show",
    "sketcher",
    "spreadsheet",
    "start",
    "surface",
    "techdraw",
    "templatepymod",
    "test",
    "tux",
    "web",
]


def parse_diff_by_file(diff_content: str):
    """
    Parse git diff output and return a dictionary mapping filenames to their diffs.

    Returns:
        dict: {filename: diff_chunk}
    """
    file_diffs = {}
    current_file = None
    current_chunk = []

    lines = diff_content.split("\n")

    for line in lines:
        if line.startswith("diff --git"):
            if current_file and current_chunk:
                file_diffs[current_file] = "\n".join(current_chunk)

            match = re.search(r"diff --git a/(.*?) b/", line)
            if match:
                current_file = match.group(1)
                current_chunk = [line]
            else:
                current_file = None
                current_chunk = []
        elif current_file is not None:
            current_chunk.append(line)

    if current_file and current_chunk:
        file_diffs[current_file] = "\n".join(current_chunk)

    return file_diffs


def check_module_compatibility(
    file_module: str, included_file_module: str, intermodule_dependencies: dict[str, list[str]]
) -> bool:
    if file_module == included_file_module:
        return True
    if file_module not in KNOWN_MODULES or included_file_module not in KNOWN_MODULES:
        return True  # We are only checking compatibility between modules *in FreeCAD*
    if file_module in intermodule_dependencies:
        return included_file_module in intermodule_dependencies[file_module]
    else:
        return False


def load_intermodule_dependencies(cmake_file_path: str) -> dict[str, list[str]]:
    """FreeCAD already has a file that defines the known dependencies between modules. The basic rule is that no NEW
    dependencies can be added (without extensive discussion with the core developers). This function loads that file
    and parses it such that we can use it to check if a new dependency was added."""
    dependencies = {
        "base": [],
        "app": ["base"],
        "gui": ["app", "base"]
    }

    if not os.path.exists(cmake_file_path):
        print(f"ERROR: {cmake_file_path} not found", file=sys.stderr)
        exit(1)

    with open(cmake_file_path, "r") as f:
        content = f.read()

    pattern = r"REQUIRES_MODS\(\s*(\w+)((?:\s+\w+)*)\s*\)"
    matches = re.finditer(pattern, content)
    for match in matches:
        dependent = match.group(1)
        prerequisites = match.group(2).split()
        module_name = dependent.replace("BUILD", "").replace("_", "").lower()
        prereq_names = [p.replace("BUILD", "").replace("_", "").lower() for p in prerequisites]
        prereq_names.append("app")  # Everything in this list depends on (or is allowed to depend on) App
        dependencies[module_name] = prereq_names

    for KNOWN_MODULE in KNOWN_MODULES:
        if KNOWN_MODULE not in dependencies:
            dependencies[KNOWN_MODULE] = ["app", "base"]  # NOT Gui, which must be handled separately

    resolve_transitive_dependencies(dependencies)

    #print()
    #print("Recognized intermodule dependencies")
    #print("-----------------------------------")
    #for module, deps in dependencies.items():
    #    print(f"{module} depends on: {', '.join(deps)}")
    #print()

    return dependencies

def resolve_transitive_dependencies(dependencies: dict[str, list[str]]) -> None:
    changed = True
    while changed:
        changed = False
        for module, deps in dependencies.items():
            for dep in list(deps):  # copy to avoid mutation during iteration
                for transitive in dependencies.get(dep, []):
                    if transitive not in deps:
                        deps.append(transitive)
                        changed = True



def check_file_dependencies(
    file: str, intermodule_dependencies: dict[str, list[str]]
) -> bool:
    """Returns true if the dependencies are OK, or false if they are not."""
    file_module = file.split("/")[1]  # src/Gui, etc.
    if file_module == "Mod":
        file_module = file.split("/")[2]  # src/Mod/Part, etc.
    is_gui = "gui" in [x.lower() for x in file.split("/")]
    with open(file, "r", encoding="utf-8") as f:
        content = f.read()
    lines = content.splitlines()
    failed = False
    line_number = 0
    for line in lines:
        line_number += 1
        if file.endswith(".h") or file.endswith(".cpp"):
            include_file = (m := re.search(r'^\s*#include\s*[<"]([^>"]+)[>"]', line)) and m.group(
                1
            )
            if include_file:
                include_file_module = include_file.split("/")[0]
                if include_file_module == "Mod":
                    include_file_module = include_file.split("/")[1]
            else:
                include_file_module = None
        elif file.endswith(".py") or file.endswith(".pyi"):
            include_file_module = (
                m := re.search(
                    r"^\s*(?:from\s+([\w.]+)\s+)?import\s+([\w.]+(?:\s+as\s+\w+)?(?:\s*,\s*[\w.]+(?:\s+as\s+\w+)?)*)",
                    line,
                )
            ) and (m.group(1) or m.group(2))
        else:
            return True
        if not include_file_module:
            continue
        if is_gui and include_file_module.lower() == "gui":
            # Special case: anything with "gui" in its path is allowed to include Gui
            continue
        compatibility = check_module_compatibility(
            file_module.lower(), include_file_module.lower(), intermodule_dependencies
        )

        if not compatibility:
            print(
                f"  --> {file_module} added a new dependency on {include_file_module} in {file} line {line_number}",
                file=sys.stderr,
            )
            failed = True
    return not failed


def main():

    dependencies = load_intermodule_dependencies(
        "cMake/FreeCAD_Helpers/CheckIntermoduleDependencies.cmake"
    )
    failed_files = []
    for file in sys.argv[1:]:
        if not check_file_dependencies(file, dependencies):
            failed_files.append(file)
    if failed_files:
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
   main()
