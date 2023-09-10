# Copyright (c) 2011, Thomas Paviot (tpaviot@gmail.com)
# All rights reserved.

# This file is part of the StepClassLibrary (SCL).
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
#   Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
#   Neither the name of the <ORGANIZATION> nor the names of its contributors may
#   be used to endorse or promote products derived from this software without
#   specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""
Docstrings are courtesy of ISO 10303-11:1994(E)
"""


class NUMBER:
    """
    EXPRESS definition:
    ===================
    The number data type has as its domain all numeric values in the language. The number data
    type shall be used when a more specific numeric representation is not important.
    Syntax:
    248 number_type = NUMBER .
    EXAMPLE 15 - Since we may not know the context of size we do not know how to correctly
    represent it, e.g. the size of the crowd at a football game would be an integer, whereas the area
    of the pitch would be a real.
    size : NUMBER ;

    Python definition:
    ==================
    class NUMBER is an abstract class, aimed at being specialized.
    """

    pass


class REAL(float, NUMBER):
    """
    EXPRESS definition:
    ===================
    The real data type has as its domain all rational, irrational and scientific real numbers. It is
    a specialization of the number data type.
    Syntax:
    265 real_type = REAL [ '(' precision_spec ')' ] .
    255 precision_spec = numeric_expression .
    Rational and irrational numbers have infnite resolution and are exact. Scientific numbers rep-
    resent quantities which are known only to a specified precision. The precision_spec is stated
    in terms of significant digits.
    A real number literal is represented by a mantissa and optional exponent. The number of digits
    making up the mantissa when all leading zeros have been removed is the number of significant
    digits. The known precision of a value is the number of leading digits that are necessary to the
    application.
    Rules and restrictions:
    a) The precision_spec gives the minimum number of digits of resolution that are re-
    quired. This expression shall evaluate to a positive integer value.
    b) When no resolution specification is given the precision of the real number is uncon-
    strained.

    Note 9.2.6:
    integer and real are both specializations of number;

    Python definition:
    ==================
    REAL both inherits from float and NUMBER
    """

    pass


class INTEGER(int, NUMBER):
    """
    EXPRESS definition:
    ===================
    The integer data type has as its domain all integer numbers. It is a specialization of the real
    data type.
    Syntax:
    227 integer_type = INTEGER .
    EXAMPLE 16 - This example uses an integer data type to represent an attribute named nodes.
    The domain of this attribute is all integers, with no further constraint.
    ENTITY foo;
    nodes : INTEGER;
    END_ENTITY;

    Note 9.2.6: integer and real are both specializations of number;

    Python definition:
    ==================
    INTEGER both inherits from int and NUMBER

    @TODO: note 9.2.6 tells that integer is a specialization of real
    """

    pass


class STRING(str):
    """
    The string data type has as its domain sequences of characters. The characters which are
    permitted to form part of a string value are defined in ISO 10646.
    Syntax:
    293 string_type = STRING [ width_spec ] .
    318 width_spec = '(' width ')' [ FIXED ] .
    317 width = numeric_expression .
    A string data type may be defined as either fixed or varying width (number of characters). If
    it is not specfically defined as fixed width (by using the fixed reserved word in the dfinition)
    the string has varying width.

    The domain of a fixed width string data type is the set of all character sequences of exactly
    the width specified in the type definition.
    The domain of a varying width string data type is the set of all character sequences of width
    less than or equal to the maximum width specified in the type definition.
    If no width is specified, the domain is the set of all character sequences, with no constraint on
    the width of these sequences.
    Substrings and individual characters may be addressed using subscripts as described in 12.5.
    The case (upper or lower) of letters within a string is significant.

    Python mapping: INTEGER is mapped the 'str' type. An additional width_spec parameter can be passed
    to handle the FIXED length constraint
    """

    pass


class LOGICAL:
    """
    The logical data type has as its domain the three literals true, false and unknown.
    Syntax:
    243 logical_type = LOGICAL .
    The following ordering holds for the values of the logical data type: false < unknown <
    true. The logical data type is compatible with the boolean data type, except that the value
    unknown cannot be assigned to a boolean variable.
    """

    pass


Unknown = LOGICAL()

#
# The boolean data type has as its domain the two literals true and false. The boolean data
# type is a specialization of the logical data type.
#
# Python mapping: BOOLEAN is mapped to 'bool' type
#
# The bool data type can't however be subclassed in Python (see
# See http://mail.python.org/pipermail/python-dev/2002-March/020822.html)
# so it is just set to bool
BOOLEAN = bool


class BINARY(str):
    """
    The binary data type has as its domain sequences of bits, each bit being represented by 0 or 1.
    Syntax:
    172 binary_type = BINARY [ width_spec ] .
    318 width_spec = '(' width ')' [ FIXED ] .
    317 width = numeric_expression .
    A binary data type may be defined as either fixed or varying width (number of bits). If it is
    not specifically defined as fixed width (by using the fixed reserved word in the definition) the
    binary data type has varying width.
    The domain of a fixed width binary data type is the set of all bit sequences of exactly the width
    specified in the type definition.
    The domain of a varying width binary data type is the set of all bit sequences of width less
    than or equal to the maximum width specified in the type definition. If no width is specified,
    the domain is the set of all bit sequences, with no constraint on the width of these sequences.
    Subbinaries and individual bits may be addressed using subscripts as described in 12.3.

    Python mapping: BINARY is mapped to the 'str' type. A check is performed to validate it is a binary
    string representing a number.
    """

    def __new__(self, value, width=-1, fixed=False):
        return str.__new__(self, value)

    def __init__(self, value, width=-1, fixed=False):
        """By default, length is set to None"""
        self._specified_width = width
        self._fixed = fixed
        # Check implicit width
        if (width != -1) and not fixed:
            raise ValueError(
                "The 'width' parameter is passed but 'fixed' is still false. Please explicitly set 'fixed' to True to avoid implicit declaration"
            )
        # First check the string length if 'fixed' is set to True
        if fixed:
            if len(value) != width:
                raise ValueError(
                    "The BINARY width %i is not consistent with the 'width' declaration(%i)"
                    % (len(value), width)
                )
        # Check that the value passed is actually a binary
        try:
            int(value, 2)
        except ValueError:
            raise ValueError("%s is not a binary" % value)


if __name__ == "__main__":
    print("Creating REAL from float value")
    a = REAL(1.5)
    print(a * 2)
    print("Creating REAL from string value")
    a = REAL("1.2")
    print(a * 3)
    print("Creating INTEGER from int value")
    b = INTEGER(2)
    c = INTEGER(3)
    print(b + c)
    print("Creating INTEGER from string value")
    e = INTEGER("5")
    f = INTEGER("8")
    print(e * f)
