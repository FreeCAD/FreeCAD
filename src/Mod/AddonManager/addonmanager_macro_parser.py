# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""Contains the parser class for extracting metadata from a FreeCAD macro"""

# pylint: disable=too-few-public-methods

import io
import re
from typing import Any, Tuple

try:
    from PySide import QtCore
except ImportError:
    QtCore = None

try:
    import FreeCAD
except ImportError:
    FreeCAD = None


class DummyThread:
    @classmethod
    def isInterruptionRequested(cls):
        return False


class MacroParser:
    """Extracts metadata information from a FreeCAD macro"""

    MAX_LINES_TO_SEARCH = 200  # To speed up parsing: some files are VERY large

    def __init__(self, name: str, code: str = ""):
        """Create a parser for the macro named "name". Note that the name is only
        used as the context for error messages, it is not otherwise important."""
        self.name = name
        self.parse_results = {
            "comment": "",
            "url": "",
            "wiki": "",
            "version": "",
            "other_files": [""],
            "author": "",
            "date": "",
            "icon": "",
            "xpm": "",
        }
        self.remaining_item_map = {}
        self.console = None if FreeCAD is None else FreeCAD.Console
        self.current_thread = DummyThread() if QtCore is None else QtCore.QThread.currentThread()
        if code:
            self.fill_details_from_code(code)

    def _reset_map(self):
        """This map tracks which items we've already read. If the same parser is used
        twice, it has to be reset."""
        self.remaining_item_map = {
            "__comment__": "comment",
            "__web__": "url",
            "__wiki__": "wiki",
            "__version__": "version",
            "__files__": "other_files",
            "__author__": "author",
            "__date__": "date",
            "__icon__": "icon",
            "__xpm__": "xpm",
        }

    def fill_details_from_code(self, code: str) -> None:
        """Reads in the macro code from the given string and parses it for its
        metadata."""

        self._reset_map()
        line_counter = 0
        content_lines = io.StringIO(code)
        while content_lines and line_counter < self.MAX_LINES_TO_SEARCH:
            line = content_lines.readline()
            if not line:
                break
            if self.current_thread.isInterruptionRequested():
                return
            line_counter += 1
            if not line.startswith("__"):
                # Speed things up a bit... this comparison is very cheap
                continue
            try:
                self._process_line(line, content_lines)
            except SyntaxError as e:
                err_string = f"Syntax error when parsing macro {self.name}:\n{str(e)}"
                if self.console:
                    self.console.PrintWarning(err_string)
                else:
                    print(err_string)

    def _process_line(self, line: str, content_lines: io.StringIO):
        """Given a single line of the macro file, see if it matches one of our items,
        and if so, extract the data."""

        lowercase_line = line.lower()
        for key in self.remaining_item_map:
            if lowercase_line.startswith(key):
                self._process_key(key, line, content_lines)
                break

    def _process_key(self, key: str, line: str, content_lines: io.StringIO):
        """Given a line that starts with a known key, extract the data for that key,
        possibly reading in additional lines (if it contains a line continuation
        character, or is a triple-quoted string)."""

        line = self._handle_backslash_continuation(line, content_lines)
        line, was_triple_quoted = self._handle_triple_quoted_string(line, content_lines)

        _, _, line = line.partition("=")
        if not was_triple_quoted:
            line, _, _ = line.partition("#")
        self._detect_illegal_content(line)
        final_content_line = line.strip()

        stripped_of_quotes = self._strip_quotes(final_content_line)
        if stripped_of_quotes is not None:
            self._standard_extraction(self.remaining_item_map[key], stripped_of_quotes)
            self.remaining_item_map.pop(key)
        else:
            self._apply_special_handling(key, line)

    @staticmethod
    def _handle_backslash_continuation(line, content_lines) -> str:
        while line.strip().endswith("\\"):
            line = line.strip()[:-1]
            concat_line = content_lines.readline()
            line += concat_line.strip()
        return line

    @staticmethod
    def _handle_triple_quoted_string(line, content_lines) -> Tuple[str, bool]:
        result = line
        was_triple_quoted = False
        if '"""' in result:
            was_triple_quoted = True
            while True:
                new_line = content_lines.readline()
                if not new_line:
                    raise SyntaxError("Syntax error while reading macro")
                if '"""' in new_line:
                    last_line, _, _ = new_line.partition('"""')
                    result += last_line + '"""'
                    break
                result += new_line
        return result, was_triple_quoted

    @staticmethod
    def _strip_quotes(line) -> str:
        line = line.strip()
        stripped_of_quotes = None
        if line.startswith('"""') and line.endswith('"""'):
            stripped_of_quotes = line[3:-3]
        elif (line[0] == '"' and line[-1] == '"') or (line[0] == "'" and line[-1] == "'"):
            stripped_of_quotes = line[1:-1]
        return stripped_of_quotes

    def _standard_extraction(self, value: str, match_group: str):
        """For most macro metadata values, this extracts the required data"""
        if isinstance(self.parse_results[value], str):
            self.parse_results[value] = match_group
            if value == "comment":
                self._cleanup_comment()
        elif isinstance(self.parse_results[value], list):
            self.parse_results[value] = [of.strip() for of in match_group.split(",")]
        else:
            raise SyntaxError(f"Conflicting data type for {value}")

    def _cleanup_comment(self):
        """Remove HTML from the comment line, and truncate it at 512 characters."""

        self.parse_results["comment"] = re.sub("<.*?>", "", self.parse_results["comment"])
        if len(self.parse_results["comment"]) > 512:
            self.parse_results["comment"] = self.parse_results["comment"][:511] + "…"

    def _apply_special_handling(self, key: str, line: str):
        # Macro authors are supposed to be providing strings here, but in some
        # cases they are not doing so. If this is the "__version__" tag, try
        # to apply some special handling to accept numbers, and "__date__"
        if key == "__version__":
            self._process_noncompliant_version(line)
            self.remaining_item_map.pop(key)
            return

        raise SyntaxError(f"Failed to process {key} from {line}")

    def _process_noncompliant_version(self, after_equals):
        if is_float(after_equals):
            self.parse_results["version"] = str(after_equals).strip()
        elif "__date__" in after_equals.lower() and self.parse_results["date"]:
            self.parse_results["version"] = self.parse_results["date"]
        else:
            self.parse_results["version"] = "(Unknown)"
            raise SyntaxError(f"Unrecognized version string {after_equals}")

    @staticmethod
    def _detect_illegal_content(line: str):
        """Raise a syntax error if this line contains something we can't handle"""

        lower_line = line.strip().lower()
        if lower_line.startswith("'") and lower_line.endswith("'"):
            return
        if lower_line.startswith('"') and lower_line.endswith('"'):
            return
        if is_float(lower_line):
            return
        if lower_line == "__date__":
            return
        raise SyntaxError(f"Metadata is expected to be a static string, but got {line}")


# Borrowed from Stack Overflow:
# https://stackoverflow.com/questions/736043/checking-if-a-string-can-be-converted-to-float
def is_float(element: Any) -> bool:
    """Determine whether a given item can be converted to a floating-point number"""
    try:
        float(element)
        return True
    except ValueError:
        return False
