# *************************************************************************
#  Copyright (c) 2024 lyphrowny                                           *
#                                                                         *
#  This file is part of the FreeCAD development system.                   *
#                                                                         *
#  This program is free software; you can redistribute it and/or modify   *
#  it under the terms of the GNU Lesser General Public License (LGPL)     *
#  as published by the Free Software Foundation; either version 2 of      *
#  the License, or (at your option) any later version.                    *
#  for detail see the LICENCE text file.                                  *
#                                                                         *
#  This program is distributed in the hope that it will be useful,        *
#  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
#  GNU Library General Public License for more details.                   *
#                                                                         *
#  You should have received a copy of the GNU General Public License      *
#  along with this program. If not, see <https://www.gnu.org/licenses/>.  *
# *************************************************************************

import unittest

from femtest.app import support_utils


class TestParseDiff(unittest.TestCase):
    def test_good_rounding_one_block(self):
        diff_lines = """---
+++
@@ -11717 +11717 @@
-2505,2,-9.5670268990152E-01
+2505,2,-9.5670268990153E-01""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert not bad_lines

    def test_bad_rounding_one_block(self):
        diff_lines = """---
+++
@@ -11717 +11717 @@
-2505,2,-9.5670268990152E-01
+2505,2,-9.5680268990152E-01""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert len(bad_lines) == 3

    def test_good_rounding_two_blocks(self):
        diff_lines = """---
+++
@@ -11717 +11717 @@
-2505,2,-9.5670268990152E-01
+2505,2,-9.5670268990153E-01
@@ -12539 +12539 @@
-3327,2,-2.2462134450621E+00
+3327,2,-2.2462134450620E+00""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert not bad_lines

    def test_bad_and_good_rounding_two_blocks(self):
        diff_lines = """---
+++
@@ -11717 +11717 @@
-2505,2,-9.5680268990152E-01
+2505,2,-9.5670268990152E-01
@@ -12539 +12539 @@
-3327,2,-2.2462134450621E+00
+3327,2,-2.2462134450620E+00""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert len(bad_lines) == 3
        assert bad_lines == diff_lines[2:5]

    def test_good_rounding_big_block(self):
        diff_lines = """---
+++
@@ -11717,2 +11717,2 @@
-2505,2,-9.5670268990152E-01
-3327,2,-2.2462134450621E+00
+2505,2,-9.5670268990153E-01
+3327,2,-2.2462134450620E+00""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert not bad_lines

    def test_bad_and_good_rounding_big_block(self):
        diff_lines = """---
+++
@@ -11717,2 +11717,2 @@
-2505,2,-9.5680268990153E-01
-3327,2,-2.2462134450621E+00
+2505,2,-9.5670268990153E-01
+3327,2,-2.2462134450620E+00""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert len(bad_lines) == 5

    def test_fail_single_characters(self):
        diff_lines = """---
+++
s
i
n""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert len(bad_lines) == 3

    def test_not_ignored_added_newlines(self):
        diff_lines = """---
+++
@@ -1 +1,3 @@
-ccc
+
+
+cccd
""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert len(bad_lines) == 5

    def test_not_ignored_removed_newlines(self):
        diff_lines = """---
+++
@@ -1,3 +1 @@
-
-
-cccd
+ccc
""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert len(bad_lines) == 5

    def test_not_ignored_single_word(self):
        diff_lines = """---
+++
@@ -1 +1 @@
-11
+12""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert len(bad_lines) == 3

    def test_ignore_bad_formatting(self) -> None:
        diff_lines = """---
+++
@@ -1 +1 @@
-2, 3, 4, 5, 6
+2,3,4,5,6""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert not bad_lines

    def test_good_rounding_space_split(self) -> None:
        diff_lines = """---
+++
@@ -1 +1 @@
-2505 2 -9.5670268990152E-01
+2505 2 -9.5670268990153E-01""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert not bad_lines

    def test_good_rounding_space_with_extra_word(self) -> None:
        diff_lines = """---
+++
@@ -1 +1 @@
-EXTRA 2505 2 -9.5670268990152E-01
+EXTRA 2505 2 -9.5670268990153E-01""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        assert not bad_lines
