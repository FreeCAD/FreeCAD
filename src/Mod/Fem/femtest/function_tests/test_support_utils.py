from ..app import support_utils
import unittest


class TestParse_Diff(unittest.TestCase):
    def test_ignore_triple_minuses(self) -> None:
        diff_lines = """---
---
---""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertFalse(bad_lines, f"Triple minuses should be ignored {bad_lines = }")

    def test_ignore_single_character(self) -> None:
        diff_lines = """@
@
?
*""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertFalse(bad_lines, f"Single characters are not ignored {bad_lines = }")

    def test_not_ignore_single_word(self) -> None:
        diff_lines = """---
+++
@@ @@
-12,
+11""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertTrue(bad_lines, f"Single words are ignored {bad_lines = }")

    def test_ignore_double_at_sign(self) -> None:
        diff_lines = """@@
@@
@@""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertFalse(bad_lines, f"Double '@' signs are not ignored {bad_lines = }")

    def test_good_float_rounding(self) -> None:
        diff_lines = """---
+++
@@ -11717 +11717 @@
-2505,2,-9.5670268990152E-01
+2505,2,-9.5670268990153E-01
@@ -12539 +12539 @@
-3327,2,-2.2462134450621E+00
+3327,2,-2.2462134450620E+00""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertFalse(bad_lines, f"Good rounding failed {bad_lines = }")

    def test_ignore_bad_formatting(self) -> None:
        diff_lines = """---
+++
@@ @@
-2, 3, 4, 5, 6
+2,3,4,5,6""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertFalse(bad_lines, f"Formatting is not checked {bad_lines = }")

    def test_good_rounding_consecutive_changes(self) -> None:
        diff_lines = """---
+++
@@ @@
-2505,2,-9.5670268990152E-01
+2505,2,-9.5670268990153E-01
-3327,2,-2.2462134450621E+00
+3327,2,-2.2462134450620E+00""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertFalse(bad_lines, f"Good consecutive rounding changes failed {bad_lines = }")

    def test_good_rounding_space_split(self) -> None:
        diff_lines = """---
+++
@@ @@
-2505 2 -9.5670268990152E-01
+2505 2 -9.5670268990153E-01""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertFalse(
            bad_lines,
            f"Good consecutive rounding changes split by space failed {bad_lines = }",
        )

    def test_bad_rounding_space_split(self) -> None:
        diff_lines = """---
+++
@@ @@
-2505 2 -9.5670268990152E-01
+2505 2 -9.5680268990153E-01""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertTrue(bad_lines, f"Bad rounding split by space shouldn't pass {bad_lines = }")

    def test_good_rounding_space_with_extra_word(self) -> None:
        diff_lines = """---
+++
@@ @@
-EXTRA 2505 2 -9.5670268990152E-01
+EXTRA 2505 2 -9.5670268990153E-01""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertFalse(bad_lines, f"Good rounding with extra word failed {bad_lines = }")

    def test_good_and_bad_rounding(self) -> None:
        diff_lines = """---
+++
@@ @@
-2505, 2, -9.5670268990152E-01
+2505, 2, -9.5670268990153E-01
@@ @@
-2505, 2, -9.5670268990152E-01
+2505, 2, -9.5680268990153E-01""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertTrue(bad_lines, f"Bad rounding split by space shouldn't pass {bad_lines = }")
        self.assertTrue(
            sum(line.startswith("@@") for line in bad_lines) == 1,
            f"Exactly one double '@' lines should be present {bad_lines = }",
        )

    def test_good_and_bad_rounding_consecutive(self) -> None:
        diff_lines = """---
+++
@@ @@
-2505, 2, -9.5670268990152E-01
+2505, 2, -9.5670268990153E-01
-2505, 2, -9.5670268990152E-01
+2505, 2, -9.5680268990153E-01""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertTrue(bad_lines, f"Bad rounding shouldn't pass {bad_lines = }")
        self.assertTrue(
            len(bad_lines) == 3,
            f"Number of bad_lines should be exactly three {bad_lines = }",
        )
        self.assertTrue(
            sum(line.startswith("@@") for line in bad_lines) == 1,
            f"Exactly one double '@' lines should be present {bad_lines = }",
        )

    def test_bad_and_bad_rounding(self) -> None:
        diff_lines = """---
+++
@@ @@
-2505, 2, -9.5670268990152E-01
+2505, 2. -9.5671268990153E-01
@@ @@
-2505, 2, -9.5670268990152E-01
+2505, 2, -9.5680268990153E-01""".splitlines()
        bad_lines = support_utils.parse_diff(diff_lines=iter(diff_lines))
        self.assertTrue(bad_lines, f"Bad rounding split by space shouldn't pass {bad_lines = }")
        self.assertTrue(
            sum(line.startswith("@@") for line in bad_lines) == 2,
            f"Exactly two double '@' lines should be present {bad_lines = }",
        )
