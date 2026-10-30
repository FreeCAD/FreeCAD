# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 The FreeCAD Project
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""This script generates a standalone Python parser from the ArchSql.lark grammar."""

import sys
import os

try:
    from lark import Lark
    from lark.tools.standalone import gen_standalone
except ImportError:
    print("Error: The 'lark' Python package is required to generate the parser.")
    print("Please install it using: pip install lark")
    sys.exit(1)


def main():
    if len(sys.argv) != 3:
        print("Usage: python ArchSqlParserGenerator.py <input_grammar.lark> <output_parser.py>")
        return 1

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    if not os.path.exists(input_file):
        print(f"Error: Input grammar file not found at '{input_file}'")
        return 1

    print(
        f"Generating standalone parser from '{os.path.basename(input_file)}' to '{os.path.basename(output_file)}'..."
    )

    # 1. Read the grammar file content.
    with open(input_file, "r", encoding="utf8") as f:
        grammar_text = f.read()

    # 2. Create an instance of the Lark parser.
    #    The 'lalr' parser is recommended for performance.
    lark_instance = Lark(grammar_text, parser="lalr")

    # 3. Open the output file and call the gen_standalone() API function.
    with open(output_file, "w", encoding="utf8") as f:
        gen_standalone(lark_instance, out=f)

    print("Parser generation complete.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
