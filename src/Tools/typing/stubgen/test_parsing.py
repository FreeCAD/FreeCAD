# SPDX-License-Identifier: LGPL-2.1-or-later

from pathlib import Path
import sys
import unittest

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.parsing import strip_comments


class StripCommentsTest(unittest.TestCase):
    def test_preserves_layout_and_literals(self) -> None:
        source = (
            'auto url = "https://example.test/a/*b*/"; // trailing\r\n'
            "auto slash = '/'; /* first\nsecond */ int value;\n"
        )
        expected = (
            'auto url = "https://example.test/a/*b*/";             \n'
            "auto slash = '/';         \n          int value;\n"
        )

        stripped = strip_comments(source)

        self.assertEqual(stripped, expected)
        self.assertEqual(len(stripped), len(source))

    def test_preserves_escaped_quotes(self) -> None:
        source = r'''const char* text = "escaped \" // text"; // comment
char quote = '\''; /* comment */
'''
        expected = r'''const char* text = "escaped \" // text";           
char quote = '\'';              
'''

        self.assertEqual(strip_comments(source), expected)

    def test_handles_unterminated_comments_and_literals(self) -> None:
        self.assertEqual(strip_comments("value /* comment\ncontinues"), "value           \n         ")
        self.assertEqual(strip_comments('"// not a comment'), '"// not a comment')


if __name__ == "__main__":
    unittest.main()
