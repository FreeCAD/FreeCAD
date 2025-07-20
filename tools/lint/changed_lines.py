#!/usr/bin/env python3

# Modified to generate output compatible with `clang-tidy`'s `--line-filter` option
#
# Based on https://github.com/hestonhoffman/changed-lines/blob/main/main.py
#
# Original License
#
# The MIT License (MIT)
#
# Copyright (c) 2023 Heston Hoffman
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

"""
Uses GitHub API to grab patch data for a PR and calculate changed lines
"""

import argparse
import json
import os
import re
import requests


class MissingPatchData(Exception):
    """Raised when the patch data is missing"""


def fetch_patch(args):
    """Grabs the patch data from the GitHub API."""
    git_session = requests.Session()
    headers = {
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }
    if args.token:
        headers["Authorization"] = f"Bearer {args.token}"

    git_request = git_session.get(
        f"{args.api_url}/repos/{args.repo}/pulls/{args.pr}/files", headers=headers
    )
    return git_request.json()


def parse_patch_file(entry):
    """Parses the individual file changes within a patch"""
    line_array = []
    sublist = []

    patch_array = re.split("\n", entry["patch"])
    # clean patch array
    patch_array = [i for i in patch_array if i]

    for item in patch_array:
        # Grabs hunk annotation and strips out added lines
        if item.startswith("@@ -"):
            if sublist:
                line_array.append(sublist)
            sublist = [re.sub(r"\s@@(.*)", "", item.split("+")[1])]
        # We don't need removed lines ('-')
        elif not item.startswith("-") and not item == "\\ No newline at end of file":
            sublist.append(item)
    if sublist:
        line_array.append(sublist)
    return line_array


def parse_patch_data(patch_data):
    """Takes the patch data and returns a dictionary of files and the lines"""

    final_dict = {}
    for entry in patch_data:
        # We don't need removed files
        if entry["status"] == "removed":
            continue

        # We can only operate on files with additions and a patch key
        # Some really big files don't have a patch key because GitHub
        # returns a message in the PR that the file is too large to display
        if entry["additions"] != 0 and "patch" in entry:
            line_array = parse_patch_file(entry)
            final_dict[entry["filename"]] = line_array
    return final_dict


def get_lines(line_dict):
    """Takes the dictionary of files and lines and returns a dictionary of files and line numbers"""
    final_dict = {}
    for file_name, sublist in line_dict.items():
        line_array = []
        for array in sublist:
            line_number = 0
            if "," not in array[0]:
                line_number = int(array[0]) - 1
            else:
                line_number = int(array[0].split(",")[0]) - 1

            start = -1
            end = -1
            for line in array:
                if line.startswith("+"):
                    if start < 0:
                        start = line_number
                    end = line_number
                    # line_array.append(line_number)
                line_number += 1
            line_array.append([start, end])

        # Remove deleted/renamed files (which appear as empty arrays)
        if line_array:
            final_dict[file_name] = line_array
    return final_dict


def main():
    """main()"""
    parser = argparse.ArgumentParser(
        prog="changed_lines.py",
        description="Identifies the changed files and lines in a GitHub PR.",
    )
    parser.add_argument("--token")
    parser.add_argument("--api-url", default="https://api.github.com")
    parser.add_argument("--repo", default="FreeCAD/FreeCAD")
    parser.add_argument("--ref", required=True)
    parser.add_argument("--pr", required=True)
    parser.add_argument("--file-filter", default="")
    args = parser.parse_args()

    data = fetch_patch(args)
    added_line_data = parse_patch_data(data)
    added_lines = get_lines(added_line_data)

    if args.file_filter:
        args.file_filter = set(args.file_filter.replace(" ", "").split(","))

    filename_list = []
    line_filter = []
    for filename, _ in added_lines.items():
        if (not args.file_filter) or (
            os.path.splitext(filename)[1] in args.file_filter
        ):
            filename_list.append(filename)
            lines_modified = {}
            lines_modified["name"] = filename
            lines_modified["lines"] = added_lines[filename]
            line_filter.append(lines_modified)

    print(f"{json.dumps(line_filter)}")


if __name__ == "__main__":
    main()
