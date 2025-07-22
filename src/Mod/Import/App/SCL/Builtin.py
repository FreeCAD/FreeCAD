# Copyright (c) 2011-2012, Thomas Paviot (tpaviot@gmail.com)
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

__doc__ = "This module defines EXPRESS built in constants and functions"
import math

from SimpleDataTypes import *
from BaseType import Aggregate
from AggregationDataTypes import *

SCL_float_epsilon = 1e-7
# Builtin constants

# EXPRESS definition:
# ===================
# 14.1 CONST_E is a REAL constant representing the mathematical value e, the base of the natural
# logarithm function (ln).
CONST_E = REAL(math.pi)

# EXPRESS definition:
# ===================
# 14.2 Indeterminate
# The indeterminate symbol (?) stands for an ambiguous value. It is compatible with all data
# types.
# NOTE - The most common use of indeterminate (?) is as the upper bound specification of a bag,
# list or set. This usage represents the notion that the size of the aggregate value defined by the
# aggregation data type is unbounded.
# python note: indeterminate value is mapped to None in aggregate bounds

# EXPRESS definition:
# ===================
# 14.3 False
# false is a logical constant representing the logical notion of falsehood. It is compatible with
# the boolean and logical data types.
FALSE = False

# EXPRESS definition:
# ===================
# 14.4 Pi
# PI is a REAL constant representing the mathematical value π, the ratio of a circle's circumference
# to its diameter.
PI = REAL(math.pi)

# EXPRESS definition:
# ===================
# 14.5 Self
# SELF refers to the current entity instance or type value. self may appear within an entity
# declaration, a type declaration or an entity constructor.
# NOTE - SELF is not a constant, but behaves as one in every context in which it can appear.
# python note: SELF is not mapped to any constant, but is mapper to self

# EXPRESS definition:
# ===================
# 14.6 True
# true is a logical constant representing the logical notion of truth. It is compatible with the
# boolean and logical data types.
TRUE = True

# EXPRESS definition:
# ===================
# 14.7 Unknown
# unknown is a logical constant representing that there is insufficient information available to
# be able to evaluate a logical condition. It is compatible with the logical data type, but not
# with the boolean data type.
# @TODO: define UNKNOWN in python

#
# Builtin Functions
# 15 Built-in functions
# All functions (and mathematical operations in general) are assumed to evaluate to exact results.
# The prototype for each of the built-in functions is given to show the type of the formal parameters
# and the result.
#

# EXPRESS definition:
# ===================
# 15.1 Abs - arithmetic function
# FUNCTION ABS ( V:NUMBER ) : NUMBER;
# The abs function returns the absolute value of a number.
# Parameters : V is a number.
# Result : The absolute value of V. The returned data type is identical to the data type of V.
# EXAMPLE 125 { ABS ( -10 ) --> 10
# Python definition:
# ==================
# ABS is mapped to python abs builtin function
def ABS(V):
    if not isinstance(V, NUMBER):
        raise TypeError("ABS function takes a NUMBER parameter")
    return type(V)(abs(V))


# EXPRESS definition:
# ===================
# 15.2 ACos - arithmetic function
# FUNCTION ACOS ( V:NUMBER ) : REAL;
# The acos function returns the angle given a cosine value.
# Parameters : V is a number which is the cosine of an angle.
# Result : The angle in radians (0 <= result <= pi) whose cosine is V.
# Conditions : -1.0=<V<=1.0
# EXAMPLE 126 { ACOS ( 0.3 ) --> 1.266103...
# Python definition:
# ==================
# ACOS is mapped to python math.acos builtin function
def ACOS(V):
    if not isinstance(V, NUMBER):
        raise TypeError("ACOS function takes a NUMBER parameter")
    return REAL(math.acos(V))


# it's the same for ASIN and ATAN
def ASIN(V):
    if not isinstance(V, NUMBER):
        raise TypeError("ASIN function takes a NUMBER parameter")
    return REAL(math.asin(V))


# EXPRESS definition:
# ===================
# 15.3 ATan - arithmetic function
# FUNCTION ATAN ( V1:NUMBER; V2:NUMBER ) : REAL;
# The atan function returns the angle given a tangent value of V , where V is given by the
# expression V = V1/V2.
# Parameters :
# a) V1 is a number.
# b) V2 is a number.
# Result : The angle in radians (-pi/2<=result<=pi/2) whose tangent is V. If V2 is zero, the result
# is pi/2 or -pi/2 depending on the sign of V1.
# Conditions : Both V1 and V2 shall not be zero.
# EXAMPLE 128 { ATAN ( -5.5, 3.0 ) --> -1.071449...
def ATAN(V1, V2):
    if not isinstance(V1, NUMBER) and not isinstance(V2, NUMBER):
        raise TypeError("ATAN function takes 2 NUMBER parameters")
    if V2 == 0:
        if V1 > 0:
            return REAL(math.pi / 2)
        elif V1 < 0:
            return REAL(-math.pi / 2)
        else:
            raise ValueError("ATAN parameters can be both equal to zero")
    else:
        return REAL(math.atan(float(V1) / float(V2)))


# EXPRESS definition:
# ===================
# 15.5 BLength - binary function
# FUNCTION BLENGTH ( V:BINARY ) : INTEGER;
# The blength function returns the number of bits in a binary.
# Parameters : V is a binary value.
# Result : The returned value is the actual number of bits in the binary value passed.
# EXAMPLE 129
# LOCAL
# n : NUMBER;
# x : BINARY := %01010010 ;
# END_LOCAL;
# ...
# n := BLENGTH ( x ); -- n is assigned the value 8
def BLENGTH(V):
    if not isinstance(V, BINARY):
        raise TypeError("BLENGTH function takes one BINARY parameter")
    return INTEGER(len(V))


# EXPRESS definition:
# ===================
# 15.6 Cos - arithmetic function
# FUNCTION COS ( V:NUMBER ) : REAL;
# The cos function returns the cosine of an angle.
# Parameters : V is a number which is an angle in radians.
# Result : The cosine of V (-1.0<=result<=1.0).
# EXAMPLE 130 { COS ( 0.5 ) --> 8.77582...E-1
#
# 15.21 Sin - arithmetic function
# FUNCTION SIN ( V:NUMBER ) : REAL;
# The sin function returns the sine of an angle.
# Parameters : V is a number representing an angle expressed in radians.
# Result : The sine of V (-1.0 <= result <= 1.0).
# EXAMPLE 144 { SIN ( PI ) --> 0.0
#
def COS(V):
    if not isinstance(V, NUMBER):
        raise TypeError("COS function takes a NUMBER parameter")
    return REAL(math.cos(V))


def SIN(V):
    if not isinstance(V, NUMBER):
        raise TypeError("SIN function takes a NUMBER parameter")
    return REAL(math.sin(V))


# EXPRESS definition:
# ===================
# 15.7 Exists - general function
# FUNCTION EXISTS ( V:GENERIC ) : BOOLEAN;
# The exists function returns true if a value exists for the input parameter, or false if no value
# exists for it. The exists function is useful for checking if values have been given to optional
# attributes, or if variables have been initialized.
# Parameters : V is an expression which results in any type.
# Result : true or false depending on whether V has an actual or indeterminate (?) value.
# EXAMPLE 131 { IF EXISTS ( a ) THEN ...
def EXISTS(V):
    if V == None:
        return False
    else:
        return True


# EXPRESS definition:
# ===================
# 15.8 Exp - arithmetic function
# FUNCTION EXP ( V:NUMBER ) : REAL;
# The exp function returns e (the base of the natural logarithm system) raised to the power V.
# Parameters : V is a number.
# Result : The value eV .
# EXAMPLE 132 { EXP ( 10 ) --> 2.202646...E+4
def EXP(V):
    if not isinstance(V, NUMBER):
        raise TypeError("EXP function takes a NUMBER parameter")
    return REAL(math.exp(V))


# EXPRESS definition:
# ===================
# 15.9 Format - general function
# FUNCTION FORMAT(N:NUMBER; F:STRING):STRING;
# The format returns a formatted string representation of a number.
# Parameters :
# a) N is a number (integer or real).
# b) F is a string containing formatting commands.
# Result : A string representation of N formatted according to F. Rounding is applied to the
# string representation if necessary.
# The formatting string contains special characters to indicate the appearance of the result. The
# formatting string can be written in three ways:
# a) The formatting string can give a symbolic description of the output representation.
# b) The formatting string can give a picture description of the output representation.
# c) When the formatting string is empty, a standard output representation is produced.
# Table 20:
# Number Format Display Comment
# 10 +7I ' +10' Zero suppression
# 10 +07I '+000010' Zeros not suppressed
# 10 10.3E ' 1.000E+01'
# 123.456789 8.2F ' 123.46'
# 123.456789 8.2E '1.23E+02'
# 123.456789 08.2E '0.12E+02' Preceding zero forced
# 9.876E123 8.2E '9.88E+123' Exponent part is 3 characters
# and width ignored
# 32.777 6I ' 33' Rounded
# Python definition
# =================
# python string formatting is obtained from the val function
# @TODO: implement a safe eval or provide another implementation
# that avoids unsafe eval python builtin function.
def FORMAT(N, F):
    if not isinstance(N, NUMBER):
        raise TypeError("FORMAT function takes a NUMBER parameter")
    if not isinstance(F, STRING):
        raise TypeError("FORMAT function takes a NUMBER parameter")
    py_formatting = F.lower()
    string_to_evaluate = "'%"
    string_to_evaluate += "%s'" % py_formatting
    string_to_evaluate += "%"
    string_to_evaluate += "%s" % N
    result = eval(string_to_evaluate).upper()
    return STRING(result)


# EXPRESS definition:
# ===================
# 15.10 HiBound - arithmetic function
# FUNCTION HIBOUND ( V:AGGREGATE OF GENERIC ) : INTEGER;
# The hibound function returns the declared upper index of an array or the declared upper
# bound of a bag, list or set.
# Parameters : V is an aggregate value.
# Result :
# a) When V is an array the returned value is the declared upper index.
# b) When V is a bag, list or set the returned value is the declared upper bound; if there
# are no bounds declared or the upper bound is declared to be indeterminate (?) indeterminate
# (?) is returned.
# EXAMPLE 133 { Usage of hibound function on nested aggregate values.
# LOCAL
# a : ARRAY[-3:19] OF SET[2:4] OF LIST[0:?] OF INTEGER;
# h1, h2, h3 : INTEGER;
# END_LOCAL;
# ...
# a[-3][1][1] := 2; -- places a value in the list
# ...
# h1 := HIBOUND(a); -- =19 (upper bound of array)
# h2 := HIBOUND(a[-3]); -- = 4 (upper bound of set)
# h3 := HIBOUND(a[-3][1]); -- = ? (upper bound of list (unbounded))
def HIBOUND(V):
    if not isinstance(V, Aggregate):
        raise TypeError("HIBOUND takes an aggregate of generic")
    return V.get_hibound()


# EXPRESS definition:
# ===================
# 15.11 HiIndex - arithmetic function
# FUNCTION HIINDEX ( V:AGGREGATE OF GENERIC ) : INTEGER;
# The hiindex function returns the upper index of an array or the number of elements in a bag,
# list or set
# Parameters : V is an aggregate value.
# Result :
# a) When V is an array, the returned value is the declared upper index.
# b) When V is a bag, list or set, the returned value is the actual number of elements in
# the aggregate value.
# EXAMPLE 134 { Usage of hiindex function on nested aggregate values.
# LOCAL
# a : ARRAY[-3:19] OF SET[2:4] OF LIST[0:?] OF INTEGER;
# h1, h2, h3 : INTEGER;
# END_LOCAL;
# a[-3][1][1] := 2; -- places a value in the list
# h1 := HIINDEX(a); -- = 19 (upper bound of array)
# h2 := HIINDEX(a[-3]); -- = 1 (size of set) -- this is invalid with respect
# -- to the bounds on the SET
# h3 := HIINDEX(a[-3][1]); -- = 1 (size of list)
def HIINDEX(V):
    if not isinstance(V, Aggregate):
        raise TypeError("HIINDEX takes an aggregate of generic")
    return V.get_hiindex()


# EXPRESS definition:
# ===================
# 15.12 Length - string function
# FUNCTION LENGTH ( V:STRING ) : INTEGER;
# The length function returns the number of characters in a string.
# Parameters : V is a string value.
# Result : The returned value is the number of characters in the string and shall be greater than
# or equal to zero.
# EXAMPLE 135 - Usage of the length function.
# LOCAL
# n : NUMBER;
# x1 : STRING := 'abc';
# x2 : STRING := "000025FF000101B5;
# END_LOCAL;
# ...
# n := LENGTH ( x1 ); -- n is assigned the value 3
# n := LENGTH ( x2 ); -- n is assigned the value 2
def LENGTH(V):
    if not isinstance(V, STRING):
        raise TypeError("LENGTH take a STRING parameter")
    return INTEGER(len(V))


# EXPRESS definition:
# ===================
# 15.13 LoBound - arithmetic function
# FUNCTION LOBOUND ( V:AGGREGATE OF GENERIC ) : INTEGER;
# The lobound function returns the declared lower index of an array, or the declared lower
# bound of a bag, list or set.
# Parameters : V is an aggregate value.
# Result :
# a) When V is an array the returned value is the declared lower index.
# b) When V is a bag, list or set the returned value is the declared lower bound; if no
# lower bound is declared, zero (0) is returned.
# EXAMPLE 136 { Usage of lobound function on nested aggregate values.
# LOCAL
# a : ARRAY[-3:19] OF SET[2:4] OF LIST[0:?] OF INTEGER;
# h1, h2, h3 : INTEGER;
# END_LOCAL;
# ...
# h1 := LOBOUND(a); -- =-3 (lower index of array)
# h2 := LOBOUND(a[-3]); -- = 2 (lower bound of set)
# h3 := LOBOUND(a[-3][1]); -- = 0 (lower bound of list)
def LOBOUND(V):
    if not isinstance(V, Aggregate):
        raise TypeError("HIBOUND takes an aggregate of generic")
    return V.get_lobound()


# EXPRESS definition:
# ===================
# 15.14 Log - arithmetic function
# FUNCTION LOG ( V:NUMBER ) : REAL;
# The log function returns the natural logarithm of a number.
# Parameters : V is a number.
# Result : A real number which is the natural logarithm of V.
# Conditions : V > 0:0
# EXAMPLE 137 { LOG ( 4.5 ) --> 1.504077...E0
# 15.15 Log2 - arithmetic function
# FUNCTION LOG2 ( V:NUMBER ) : REAL;
# The log2 function returns the base two logarithm of a number.
# Parameters : V is a number.
# Result : A real number which is the base two logarithm of V.
# Conditions : V > 0:0
# EXAMPLE 138 { LOG2 ( 8 ) --> 3.00...E0
# 15.16 Log10 - arithmetic function
# FUNCTION LOG10 ( V:NUMBER ) : REAL;
# The log10 function returns the base ten logarithm of a number.
# Parameters : V is a number.
# Result : A real number which is the base ten logarithm of V.
# Conditions : V > 0:0
# EXAMPLE 139 { LOG10 ( 10 ) --> 1.00...E0
def LOG(V):
    if not isinstance(V, NUMBER):
        raise TypeError("LOG function takes a NUMBER parameter")
    return REAL(math.log(V))


def LOG2(V):
    if not isinstance(V, NUMBER):
        raise TypeError("LOG2 function takes a NUMBER parameter")
    return REAL(math.log(V, 2))


def LOG10(V):
    if not isinstance(V, NUMBER):
        raise TypeError("LOG10 function takes a NUMBER parameter")
    return REAL(math.log10(V))


# EXPRESS definition:
# ===================
# 15.17 LoIndex - arithmetic function
# FUNCTION LOINDEX ( V:AGGREGATE OF GENERIC ) : INTEGER;
# The loindex function returns the lower index of an aggregate value.
# Parameters : V is an aggregate value.
# Result :
# a) When V is an array the returned value is the declared lower index.
# b) When V is a bag, list or set, the returned value is 1 (one).
# EXAMPLE 140 { Usage of loindex function on nested aggregate values.
# LOCAL
# a : ARRAY[-3:19] OF SET[2:4] OF LIST[0:?] OF INTEGER;
# h1, h2, h3 : INTEGER;
# END_LOCAL;
# ...
# h1 := LOINDEX(a); -- =-3 (lower bound of array)
# h2 := LOINDEX(a[-3]); -- = 1 (for set)
# h3 := LOINDEX(a[-3][1]); -- = 1 (for list)
def LOINDEX(V):
    if not isinstance(V, Aggregate):
        raise TypeError("LOINDEX takes an aggregate of generic")
    return V.get_loindex()


# EXPRESS definition:
# ===================
# 15.18 NVL - null value function
# FUNCTION NVL(V:GENERIC:GEN1; SUBSTITUTE:GENERIC:GEN1):GENERIC:GEN1;
# The nvl function returns either the input value or an alternate value in the case where the input
# has a indeterminate (?) value.
# Parameters :
# a) V is an expression which is of any type.
# b) SUBSTITUTE is an expression which shall not evaluate to indeterminate (?).
# Result : When V is not indeterminate (?) that value is returned. Otherwise, SUBSTITUTE is
# returned.
# EXAMPLE 141 { ENTITY unit_vector;
# x, y : REAL;
# z : OPTIONAL REAL;
# WHERE
# x**2 + y**2 + NVL(z, 0.0)**2 = 1.0;
# END_ENTITY;
# The nvl function is used to supply zero (0.0) as the value of Z when Z is indeterminate (?).
def NVL(V, SUBSTITUTE):
    if V is not None:
        return V
    else:
        return SUBSTITUTE


# EXPRESS definition:
# ===================
# 15.19 Odd - arithmetic function
# FUNCTION ODD ( V:INTEGER ) : LOGICAL;
# The odd function returns true or false depending on whether a number is odd or even.
# Parameters : V is an integer number.
# Result : When V MOD 2 = 1 true is returned; otherwise false is returned.
# Conditions : Zero is not odd.
# EXAMPLE 142 { ODD ( 121 ) --> TRUE
def ODD(V):
    if not isinstance(V, INTEGER):
        raise TypeError("ODD takes an INTEGER")
    if V % 2 == 0:
        return False
    else:
        return True


# EXPRESS definition:
# ===================
# 15.20 RolesOf - general function
# FUNCTION ROLESOF ( V:GENERIC ) : SET OF STRING;
# The rolesof function returns a set of strings containing the fully qualified names of the roles
# played by the specified entity instance. A fully qualified name is defined to be the name of the
# attribute qualified by the name of the schema and entity in which this attribute is declared (i.e.
#'SCHEMA.ENTITY.ATTRIBUTE').
# Parameters : V is any instance of an entity data type.
# Result : A set of string values (in upper case) containing the fully qualified names of the
# attributes of the entity instances which use the instance V.
# When a named data type is used or referenced, the schema and the name in that schema,
# if renamed, are also returned. Since use statements may be chained, all the chained schema
# names and the name in each schema are returned.
# EXAMPLE 143 { This example shows that a point might be used as the centre of a circle. The
# rolesof function determines what roles an entity instance actually plays.
# SCHEMA that_schema;
# ENTITY point;
# x, y, z : REAL;
# END_ENTITY;
# ENTITY line;
# start,
# end : point;
# END_ENTITY;
# END_SCHEMA;
# SCHEMA this_schema;
# USE FROM that_schema (point,line);
# CONSTANT
# origin : point := point(0.0, 0.0, 0.0);
# END_CONSTANT;
# ENTITY circle;
# centre : point;
# axis : vector;
# radius : REAL;
# END_ENTITY;
# ...
# LOCAL
# p : point := point(1.0, 0.0, 0.0);
# c : circle := circle(p, vector(1,1,1), 1.0);
# l : line := line(p, origin);
# END_LOCAL;
# ...
# IF 'THIS_SCHEMA.CIRCLE.CENTRE' IN ROLESOF(p) THEN -- true
# ...
# IF 'THIS_SCHEMA.LINE.START' IN ROLESOF(p) THEN -- true
# ...
# IF 'THAT_SCHEMA.LINE.START' IN ROLESOF(p) THEN -- true
# ...
# IF 'THIS_SCHEMA.LINE.END' IN ROLESOF(p) THEN -- false
#
# Python note:
# @TODO: implement the ROLESOF function
def ROLESOF(V):
    raise NotImplemented("Function ROLESOF not implemented")


# EXPRESS definition:
# ===================
# 15.22 SizeOf - aggregate function
# FUNCTION SIZEOF ( V:AGGREGATE OF GENERIC ) : INTEGER;
# The sizeof function returns the number of elements in an aggregate value.
# Parameters : V is an aggregate value.
# Result :
# a) When V is an array the returned value is its declared number of elements in the
# aggregation data type.
# b) When V is a bag, list or set, the returned value is the actual number of elements in
# the aggregate value.
# EXAMPLE 145 { LOCAL
# n : NUMBER;
# y : ARRAY[2:5] OF b;
# END_LOCAL;
# ...
# n := SIZEOF (y); -- n is assigned the value 4
def SIZEOF(V):
    if not isinstance(V, Aggregate):
        raise TypeError("SIZEOF takes an aggregate of generic")
    return V.get_size()


# EXPRESS definition:
# ===================
# 15.23 Sqrt - arithmetic function
# FUNCTION SQRT ( V:NUMBER ) : REAL;
# The sqrt function returns the non-negative square root of a number.
# Parameters : V is any non-negative number.
# Result : The non-negative square root of V.
# Conditions : V >= 0:0
# EXAMPLE 146 - SQRT ( 121 ) --> 11.0
def SQRT(V):
    if not isinstance(V, NUMBER):
        raise TypeError("SQRT function takes a NUMBER parameter")
    if V < 0.0:
        raise ValueError("SQRT takes a non-negative parameter")
    return REAL(math.sqrt(V))


# EXPRESS definition:
# ===================
# 15.24 Tan - arithmetic function
# FUNCTION TAN ( V:NUMBER ) : REAL;
# The tan function returns the tangent of an angle.
# Parameters : V is a number representing an angle expressed in radians.
# Result : The tangent of the angle. If the angle is npi/2, where n is an odd integer, indeterminate
# (?) is returned.
# EXAMPLE 147 - TAN ( 0.0 ) --> 0.0
def TAN(V):
    if not isinstance(V, NUMBER):
        raise TypeError("TAN function takes a NUMBER parameter")
    # check if angle is npi/2 where n is an odd integer
    a = V / (PI / 2)
    if abs(a % 2 - 1.0) < SCL_float_epsilon:
        return None
    else:
        return REAL(math.tan(V))


# EXPRESS definition:
# ===================
# 15.25 TypeOf - general function
# FUNCTION TYPEOF ( V:GENERIC ) : SET OF STRING;
# The typeof function returns a set of strings that contains the names of all the data types
# of which the parameter is a member. Except for the simple data types (binary, boolean,
# integer, logical, number, real, and string) and the aggregation data types (array, bag,
# list, set) these names are qualified by the name of the schema which contains the definition of
# the type.
# NOTE 1 { The primary purpose of this function is to check whether a given value (variable, at-
# tribute value) can be used for a certain purpose, e.g. to ensure assignment compatibility between
# two values. It may also be used if different subtypes or specializations of a given type have to be
# treated differently in some context.
# Parameters : V is a value of any type.
# Result : The contents of the returned set of string values are the names (in upper case) of all
# types the value V is a member of. Such names are qualified by the name of the schema which
# contains the definition of the type ('SCHEMA.TYPE') if it is neither a simple data type nor an
# aggregation data type. It may be derived by the following algorithm (which is given here for
# specification purposes rather than to prescribe any particular type of implementation)
def TYPEOF(V):
    # Create the set to return
    v_types = set()
    # append the type of V to the set
    try:  # it's a class
        to_add = V.__name__.upper()
    except AttributeError:  # it's an instance, first retrieve the type
        to_add = type(V).__name__.upper()
    if not to_add in ["FLOAT", "INT", "AGGREGATE"]:
        v_types.add(to_add)
    # recursively adds the base class names
    for base_type in type(V).__bases__:
        # print base_type
        if not base_type == object:
            v_types = v_types.union(TYPEOF(base_type))
    # finally, converts the v_types set to SET
    return v_types


# EXPRESS definition:
# ===================
# 15.26 UsedIn - general function
# FUNCTION USEDIN ( T:GENERIC; R:STRING) : BAG OF GENERIC;
# The usedin function returns each entity instance that uses a specified entity instance in a
# specified role.
def USEDIN(T, R):
    raise NotImplemented("USEDIN function not yet implemented.")


# EXPRESS definition:
# ===================
# 15.27 Value - arithmetic function
# FUNCTION VALUE ( V:STRING ) : NUMBER;
# The value function returns the numeric representation of a string.
# Parameters : V is a string containing either a real or integer literal.
# Result : A number corresponding to the string representation. If it is not possible to interpret
# the string as either a real or integer literal, indeterminate (?) is returned.
# EXAMPLE 151 { VALUE ( '1.234' ) --> 1.234 (REAL)
# VALUE ( '20' ) --> 20 (INTEGER)
# VALUE ( 'abc' ) --> ? null
def VALUE(V):
    if not isinstance(V, STRING):
        raise TypeError("VALUE function takes a NUMBER parameter")
    # first try to instantiate an INTEGER from the string:
    try:
        return INTEGER(V)
    except Exception:
        pass  # not possible, try to cast to REAL
    try:
        return REAL(V)
    except Exception:
        pass
    # else return None
    return None


# EXPRESS definition:
# ===================
# 15.28 Value in - membership function
# FUNCTION VALUE_IN ( C:AGGREGATE OF GENERIC:GEN; V:GENERIC:GEN ) : LOGICAL;
# The value in function returns a logical value depending on whether or not a particular value
# is a member of an aggregation.
# Parameters :
# a) C is an aggregation of any type.
# b) V is an expression which is assignment compatible with the base type of C.
# Result :
# a) If either V or C is indeterminate (?), unknown is returned.
# b) If any element of C has a value equal to the value of V, true is returned.
# c) If any element of C is indeterminate (?), unknown is returned.
# d) Otherwise false is returned.
# EXAMPLE 152 { The following test ensures that there is at least one point which is positioned at
# the origin.
# LOCAL
# points : SET OF point;
# END_LOCAL;
# ...
# IF VALUE_IN(points, point(0.0, 0.0, 0.0)) THEN ...
def VALUE_IN(C, V):
    if not isinstance(C, Aggregate):
        raise TypeError("VALUE_IN method takes an aggregate as first parameter")
    raise NotImplemented("VALUE_IN function not yet implemented")


# EXPRESS definition:
# ===================
# 15.29 Value unique - uniqueness function
# FUNCTION VALUE UNIQUE ( V:AGGREGATE OF GENERIC) : LOGICAL;
# The value unique function returns a logical value depending on whether or not the elements
# of an aggregation are value unique.
# Parameters : V is an aggregation of any type.
# Result :
# a) If V is indeterminate (?), unknown is returned.
# b) If any any two elements of V are value equal, false is returned.
# c) If any element of V is indeterminate (?), unknown is returned.
# d) Otherwise true is returned.
# EXAMPLE 153 { The following test ensures that each point is placed at a different position, (by
# definition they are distinct, i.e., instance unique).
# IF VALUE_UNIQUE(points) THEN ...
def VALUE_UNIQUE(V):
    if not isinstance(V, Aggregate):
        raise TypeError("VALUE_UNIQUE method takes an aggregate as first parameter")
    return V.get_value_unique()
