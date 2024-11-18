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

from SimpleDataTypes import *
from TypeChecker import check_type
import BaseType


class BaseAggregate(object):
    """A class that define common properties to ARRAY, LIST, SET and BAG."""

    def __init__(self, bound1, bound2, base_type):
        # check that bound1<bound2
        if bound1 != None and bound2 != None:
            if bound1 > bound2:
                raise AssertionError("bound1 shall be less than or equal to bound2")
        self._bound1 = bound1
        self._bound2 = bound2
        self._base_type = base_type

    def __getitem__(self, index):
        if index < self._bound1:
            raise IndexError(
                "ARRAY index out of bound (lower bound is %i, passed %i)" % (self._bound1, index)
            )
        elif self._bound2 != None and index > self._bound2:
            raise IndexError(
                "ARRAY index out of bound (upper bound is %i, passed %i)" % (self._bound2, index)
            )
        else:
            return list.__getitem__(self, index)

    def __setitem__(self, index, value):
        if index < self._bound1:
            raise IndexError(
                "ARRAY index out of bound (lower bound is %i, passed %i)" % (self._bound1, index)
            )
        elif self._bound2 != None and index > self._bound2:
            raise IndexError(
                "ARRAY index out of bound (upper bound is %i, passed %i)" % (self._bound2, index)
            )
        elif not isinstance(value, self._base_type):
            raise TypeError("%s type expected, passed %s." % (self._base_type, type(value)))
        else:
            # first find the length of the list, and extend it if ever
            # the index is
            list.__setitem__(self, index, value)


class ARRAY(BaseType.Type, BaseType.Aggregate):
    """
    EXPRESS definition:
    ==================
    An array data type has as its domain indexed, fixed-size collections of like elements. The lower
    and upper bounds, which are integer-valued expressions, define the range of index values, and
    thus the size of each array collection.
    An array data type definition may optionally specify
    that an array value cannot contain duplicate elements.
    It may also specify that an array value
    need not contain an element at every index position.

    Given that m is the lower bound and n is the upper bound, there are exactly n-m+1 elements
    in the array. These elements are indexed by subscripts from m to n, inclusive (see 12.6.1).
    NOTE 1 { The bounds may be positive, negative or zero, but may not be indeterminate (?) (see
    14.2).

    Syntax:
    165 array_type = ARRAY bound_spec OF [ OPTIONAL ] [ UNIQUE ] base_type .
    176 bound_spec = '[' bound_1 ':' bound_2 ']' .
    174 bound_1 = numeric_expression .
    175 bound_2 = numeric_expression .
    171 base_type = aggregation_types | simple_types | named_types .
    Given that m is the lower bound and n is the upper bound, there are exactly n-m+1 elements
    in the array. These elements are indexed by subscripts from m to n, inclusive (see 12.6.1).
    NOTE 1 { The bounds may be positive, negative or zero, but may not be indeterminate (?) (see
    14.2).
    Rules and restrictions:
    a) Both expressions in the bound specification, bound_1 and bound_2, shall evaluate to
    integer values. Neither shall evaluate to the indeterminate (?) value.
    b) bound_1 gives the lower bound of the array. This shall be the lowest index which is
    valid for an array value of this data type.
    c) bound_2 gives the upper bound of the array. This shall be the highest index which is
    valid for an array value of this data type.
    d) bound_1 shall be less than or equal to bound_2.
    e) If the optional keyword is specified, an array value of this data type may have the
    indeterminate (?) value at one or more index positions.
    f) If the optional keyword is not specified, an array value of this data type shall not
    contain an indeterminate (?) value at any index position.
    g) If the unique keyword is specified, each element in an array value of this data type
    shall be different from (i.e., not instance equal to) every other element in the same array
    value.
    NOTE 2 : Both optional and unique may be specified in the same array data type definition.
    This does not preclude multiple indeterminate (?) values from occurring in a single array value.
    This is because comparisons between indeterminate (?) values result in unknown so the uniqueness
    constraint is not violated.
    EXAMPLE 27 : This example shows how a multi-dimensioned array is declared.
    sectors : ARRAY [ 1 : 10 ] OF -- first dimension
    ARRAY [ 11 : 14 ] OF -- second dimension
    UNIQUE something;
    The first array has 10 elements of data type ARRAY[11:14] OF UNIQUE something. There is
    a total of 40 elements of data type something in the attribute named sectors. Within each
    ARRAY[11:14], no duplicates may occur; however, the same something instance may occur in two
    different ARRAY[11:14] values within a single value for the attribute named sectors.

    Python definition:
    ==================
    @TODO
    """

    def __init__(self, bound_1, bound_2, base_type, UNIQUE=False, OPTIONAL=False, scope=None):
        BaseType.Type.__init__(self, base_type, scope)
        if not type(bound_1) == int:
            raise TypeError("ARRAY lower bound must be an integer")
        if not type(bound_2) == int:
            raise TypeError("ARRAY upper bound must be an integer")
        if not (bound_1 <= bound_2):
            raise AssertionError("ARRAY lower bound must be less than or equal to upper bound")
        # set up class attributes
        self._bound_1 = bound_1
        self._bound_2 = bound_2
        self._unique = UNIQUE
        self._optional = OPTIONAL
        # preallocate list elements
        list_size = bound_2 - bound_1 + 1
        self._container = list_size * [None]

    def bound_1(self):
        return self._bound_1

    def bound_2(self):
        return self._bound_2

    def get_hiindex(self):
        return INTEGER(self._bound_2)

    def get_loindex(self):
        return INTEGER(self._bound_1)

    def get_hibound(self):
        return INTEGER(self._bound_2)

    def get_lobound(self):
        return INTEGER(self._bound_1)

    def get_size(self):
        return INTEGER(self._bound_2 - self._bound_1 + 1)

    def get_value_unique(self):
        """Return True if all items are different in the container, UNKNOWN if some items are
        indeterminate, or False otherwise"""
        if None in self._container:
            return Unknown
        if self.get_size() - len(set(self._container)) > 0:  # some items are repeated
            return False
        else:
            return True

    def __getitem__(self, index):
        if index < self._bound_1:
            raise IndexError(
                "ARRAY index out of bound (lower bound is %i, passed %i)" % (self._bound_1, index)
            )
        elif index > self._bound_2:
            raise IndexError(
                "ARRAY index out of bound (upper bound is %i, passed %i)" % (self._bound_2, index)
            )
        else:
            value = self._container[index - self._bound_1]
            if not self._optional and value == None:
                raise AssertionError(
                    "Not OPTIONAL prevent the value with index %i from being None (default). Please set the value first."
                    % index
                )
            return value

    def __setitem__(self, index, value):
        if index < self._bound_1:
            raise IndexError(
                "ARRAY index out of bound (lower bound is %i, passed %i)" % (self._bound_1, index)
            )
        elif index > self._bound_2:
            raise IndexError(
                "ARRAY index out of bound (upper bound is %i, passed %i)" % (self._bound_2, index)
            )
        else:
            # first check the type of the value
            check_type(value, self.get_type())
            # then check if the value is already in the array
            if self._unique:
                if value in self._container:
                    raise AssertionError("UNIQUE keyword prevents inserting this instance.")
            self._container[index - self._bound_1] = value


class LIST(BaseType.Type, BaseType.Aggregate):
    """
    EXPRESS definition:
    ==================
    A list data type has as its domain sequences of like elements. The optional lower and upper
    bounds, which are integer-valued expressions, define the minimum and maximum number of
    elements that can be held in the collection defined by a list data type.
    A list data type
    definition may optionally specify that a list value cannot contain duplicate elements.

    Syntax:
    237 list_type = LIST [ bound_spec ] OF [ UNIQUE ] base_type .
    176 bound_spec = '[' bound_1 ':' bound_2 ']' .
    174 bound_1 = numeric_expression .
    175 bound_2 = numeric_expression .
    171 base_type = aggregation_types | simple_types | named_types .
    Rules and restrictions:
    a) The bound_1 expression shall evaluate to an integer value greater than or equal to
    zero. It gives the lower bound, which is the minimum number of elements that can be in a
    list value of this data type. bound_1 shall not produce the indeterminate (?) value.
    b) The bound_2 expression shall evaluate to an integer value greater than or equal to
    bound_1, or an indeterminate (?) value. It gives the upper bound, which is the maximum
    number of elements that can be in a list value of this data type.
    If this value is indeterminate (?) the number of elements in a list value of this data type is
    not bounded from above.
    c) If the bound_spec is omitted, the limits are [0:?].
    d) If the unique keyword is specified, each element in a list value of this data type shall
    be different from (i.e., not instance equal to) every other element in the same list value.
    EXAMPLE 28 { This example defines a list of arrays. The list can contain zero to ten arrays. Each
    array of ten integers shall be different from all other arrays in a particular list.
    complex_list : LIST[0:10] OF UNIQUE ARRAY[1:10] OF INTEGER;

    Python definition:
    ==================
    @TODO
    """

    def __init__(self, bound_1, bound_2, base_type, UNIQUE=False, scope=None):
        BaseType.Type.__init__(self, base_type, scope)
        if not type(bound_1) == int:
            raise TypeError("LIST lower bound must be an integer")
        # bound_2 can be set to None
        self._unbounded = False
        if bound_2 == None:
            self._unbounded = True
        elif not type(bound_2) == int:
            raise TypeError("LIST upper bound must be an integer")
        if not bound_1 >= 0:
            raise AssertionError("LIST lower bound must be greater of equal to 0")
        if type(bound_2) == int and not (bound_1 <= bound_2):
            raise AssertionError("ARRAY lower bound must be less than or equal to upper bound")
        # set up class attributes
        self._bound_1 = bound_1
        self._bound_2 = bound_2
        self._unique = UNIQUE
        # preallocate list elements if bounds are both integers
        if not self._unbounded:
            list_size = bound_2 - bound_1 + 1
            self._container = list_size * [None]
        # for unbounded list, this will come after
        else:
            self._container = [None]

    def bound_1(self):
        return self._bound_1

    def bound_2(self):
        return self._bound_2

    def get_size(self):
        number_of_indeterminates = self._container.count(None)
        hiindex = len(self._container) - number_of_indeterminates
        return INTEGER(hiindex)

    def get_hiindex(self):
        """When V is a bag, list or set, the returned value is the actual number of elements in
        the aggregate value."""
        number_of_indeterminates = self._container.count(None)
        hiindex = len(self._container) - number_of_indeterminates
        return INTEGER(hiindex)

    def get_loindex(self):
        return INTEGER(1)

    def get_hibound(self):
        hibound = self._bound_2
        if type(hibound) == int:
            return INTEGER(hibound)
        else:
            return hibound

    def get_lobound(self):
        lobound = self._bound_1
        if type(lobound) == int:
            return INTEGER(lobound)
        else:
            return lobound

    def get_value_unique(self):
        """Return True if all items are different in the container, UNKNOWN if some items are
        indeterminate, or False otherwise"""
        if None in self._container:
            return Unknown
        if self.get_size() - len(set(self._container)) > 0:  # some items are repeated
            return False
        else:
            return True

    def __getitem__(self, index):
        # case bounded
        if not self._unbounded:
            if index < self._bound_1:
                raise IndexError(
                    "ARRAY index out of bound (lower bound is %i, passed %i)"
                    % (self._bound_1, index)
                )
            elif index > self._bound_2:
                raise IndexError(
                    "ARRAY index out of bound (upper bound is %i, passed %i)"
                    % (self._bound_2, index)
                )
            else:
                value = self._container[index - self._bound_1]
                if value == None:
                    raise AssertionError(
                        "Value with index %i not defined. Please set the value first." % index
                    )
                return value
        # case unbounded
        else:
            if index - self._bound_1 > len(self._container):
                raise AssertionError(
                    "Value with index %i not defined. Please set the value first." % index
                )
            else:
                value = self._container[index - self._bound_1]
                if value == None:
                    raise AssertionError(
                        "Value with index %i not defined. Please set the value first." % index
                    )
                return value

    def __setitem__(self, index, value):
        # case bounded
        if not self._unbounded:
            if index < self._bound_1:
                raise IndexError(
                    "ARRAY index out of bound (lower bound is %i, passed %i)"
                    % (self._bound_1, index)
                )
            elif index > self._bound_2:
                raise IndexError(
                    "ARRAY index out of bound (upper bound is %i, passed %i)"
                    % (self._bound_2, index)
                )
            else:
                # first check the type of the value
                check_type(value, self.get_type())
                # then check if the value is already in the array
                if self._unique:
                    if value in self._container:
                        raise AssertionError("UNIQUE keyword prevent inserting this instance.")
                self._container[index - self._bound_1] = value
        # case unbounded
        else:
            if index < self._bound_1:
                raise IndexError(
                    "ARRAY index out of bound (lower bound is %i, passed %i)"
                    % (self._bound_1, index)
                )
            # if the _container list is of good size, just do like the bounded case
            if index - self._bound_1 < len(self._container):
                # first check the type of the value
                check_type(value, self.get_type)
                # then check if the value is already in the array
                if self._unique:
                    if value in self._container:
                        raise AssertionError("UNIQUE keyword prevent inserting this instance.")
                self._container[index - self._bound_1] = value
            # in the other case, we have to extend the base _container list
            else:
                delta_size = (index - self._bound_1) - len(self._container) + 1
                # create a list of None, and extend the list
                list_extension = delta_size * [None]
                self._container.extend(list_extension)
                # first check the type of the value
                check_type(value, self.get_type())
                # then check if the value is already in the array
                if self._unique:
                    if value in self._container:
                        raise AssertionError("UNIQUE keyword prevent inserting this instance.")
                self._container[index - self._bound_1] = value


class BAG(BaseType.Type, BaseType.Aggregate):
    """
    EXPRESS definition:
    ==================
    A bag data type has as its domain unordered collections of like elements. The optional lower
    and upper bounds, which are integer-valued expressions, define the minimum and maximum
    number of elements that can be held in the collection defined by a bag data type.

    Syntax:
    170 bag_type = BAG [ bound_spec ] OF base_type .
    176 bound_spec = '[' bound_1 ':' bound_2 ']' .
    174 bound_1 = numeric_expression .
    175 bound_2 = numeric_expression .
    171 base_type = aggregation_types | simple_types | named_types .

    Rules and restrictions:
    a) The bound_1 expression shall evaluate to an integer value greater than or equal to
    zero. It gives the lower bound, which is the minimum number of elements that can be in a
    bag value of this data type. bound_1 shall not produce the indeterminate (?) value.
    b) The bound_2 expression shall evaluate to an integer value greater than or equal to
    bound_1, or an indeterminate (?) value. It gives the upper bound, which is the maximum
    number of elements that can be in a bag value of this data type.
    If this value is indeterminate (?) the number of elements in a bag value of this data type is
    not be bounded from above.
    c) If the bound_spec is omitted, the limits are [0:?].
    EXAMPLE 29 (This example defines an attribute as a bag of point (where point is a named data
    type assumed to have been declared elsewhere).
    a_bag_of_points : BAG OF point;
    The value of the attribute named a_bag_of_points can contain zero or more points. The same
    point instance may appear more than once in the value of a_bag_of_points.
    If the value is required to contain at least one element, the specification can provide a lower bound,
    as in:
    a_bag_of_points : BAG [1:?] OF point;
    The value of the attribute named a_bag_of_points now must contain at least one point.

    Python definition:
    ==================
    @TODO
    """

    def __init__(self, bound_1, bound_2, base_type, scope=None):
        BaseType.Type.__init__(self, base_type, scope)
        if not type(bound_1) == int:
            raise TypeError("LIST lower bound must be an integer")
        # bound_2 can be set to None
        self._unbounded = False
        if bound_2 == None:
            self._unbounded = True
        elif not type(bound_2) == int:
            raise TypeError("LIST upper bound must be an integer")
        if not bound_1 >= 0:
            raise AssertionError("LIST lower bound must be greater of equal to 0")
        if type(bound_2) == int and not (bound_1 <= bound_2):
            raise AssertionError("ARRAY lower bound must be less than or equal to upper bound")
        # set up class attributes
        self._bound_1 = bound_1
        self._bound_2 = bound_2
        self._container = []

    def bound_1(self):
        return self._bound_1

    def bound_2(self):
        return self._bound_2

    def add(self, value):
        """
        Adds a value to the bag
        """
        if self._unbounded:
            check_type(value, self.get_type())
            self._container.append(value)
        else:
            # first ensure that the bag is not full
            if len(self._container) == self._bound_2 - self._bound_1 + 1:
                raise AssertionError("BAG is full. Impossible to add any more item")
            else:
                check_type(value, self.get_type())
                self._container.append(value)

    def get_size(self):
        """When V is a bag, list or set, the returned value is the actual number of elements in
        the aggregate value."""
        return INTEGER(len(self._container))

    def get_hiindex(self):
        """When V is a bag, list or set, the returned value is the actual number of elements in
        the aggregate value."""
        return INTEGER(len(self._container))

    def get_loindex(self):
        return INTEGER(1)

    def get_hibound(self):
        hibound = self._bound_2
        if type(hibound) == int:
            return INTEGER(hibound)
        else:
            return hibound

    def get_lobound(self):
        lobound = self._bound_1
        if type(lobound) == int:
            return INTEGER(lobound)
        else:
            return lobound

    def get_value_unique(self):
        """Return True if all items are different in the container, UNKNOWN if some items are
        indeterminate, or False otherwise"""
        if None in self._container:
            return Unknown
        if self.get_size() - len(set(self._container)) > 0:  # some items are repeated
            return False
        else:
            return True


class SET(BaseType.Type, BaseType.Aggregate):
    """
    EXPRESS definition:
    ==================
    A set data type has as its domain unordered collections of like elements. The set data type is
    a specialization of the bag data type. The optional lower and upper bounds, which are integer-
    valued expressions, define the minimum and maximum number of elements that can be held in
    the collection defined by a set data type. The collection defined by set data type shall not
    contain two or more elements which are instance equal.
    Syntax:
    285 set_type = SET [ bound_spec ] OF base_type .
    176 bound_spec = '[' bound_1 ':' bound_2 ']' .
    174 bound_1 = numeric_expression .
    175 bound_2 = numeric_expression .
    171 base_type = aggregation_types | simple_types | named_types .
    Rules and restrictions:
    a) The bound_1 expression shall evaluate to an integer value greater than or equal to
    zero. It gives the lower bound, which is the minimum number of elements that can be in a
    set value of this data type. bound_1 shall not produce the indeterminate (?) value.
    b) The bound_2 expression shall evaluate to an integer value greater than or equal to
    bound_1, or an indeterminate (?) value. It gives the upper bound, which is the maximum
    number of elements that can be in a set value of this data type.
    If this value is indeterminate (?) the number of elements in a set value of this data type is
    not be bounded from above.
    c) If the bound_spec is omitted, the limits are [0:?].
    d) Each element in an occurrence of a set data type shall be different from (i.e., not
    instance equal to) every other element in the same set value.
    EXAMPLE 30 { This example defines an attribute as a set of points (a named data type assumed
    to have been declared elsewhere).
    a_set_of_points : SET OF point;
    The attribute named a_set_of_points can contain zero or more points. Each point instance (in
    the set value) is required to be different from every other point in the set.
    If the value is required to have no more than 15 points, the specification can provide an upper bound,
    as in:
    a_set_of_points : SET [0:15] OF point;
    The value of the attribute named a_set_of_points now may contain no more than 15 points.

    Python definition:
    ==================
    The difference with the BAG class is that the base container for SET is a set object.
    """

    def __init__(self, bound_1, bound_2, base_type, scope=None):
        BaseType.Type.__init__(self, base_type, scope)
        if not type(bound_1) == int:
            raise TypeError("LIST lower bound must be an integer")
        # bound_2 can be set to None
        self._unbounded = False
        if bound_2 == None:
            self._unbounded = True
        elif not type(bound_2) == int:
            raise TypeError("LIST upper bound must be an integer")
        if not bound_1 >= 0:
            raise AssertionError("LIST lower bound must be greater of equal to 0")
        if type(bound_2) == int and not (bound_1 <= bound_2):
            raise AssertionError("ARRAY lower bound must be less than or equal to upper bound")
        # set up class attributes
        self._bound_1 = bound_1
        self._bound_2 = bound_2
        self._container = set()

    def bound_1(self):
        return self._bound_1

    def bound_2(self):
        return self._bound_2

    def add(self, value):
        """
        Adds a value to the bag
        """
        if self._unbounded:
            check_type(value, self.get_type())
            self._container.add(value)
        else:
            # first ensure that the bag is not full
            if len(self._container) == self._bound_2 - self._bound_1 + 1:
                if not value in self._container:
                    raise AssertionError("SET is full. Impossible to add any more item")
            else:
                check_type(value, self.get_type())
                self._container.add(value)

    def get_size(self):
        """When V is a bag, list or set, the returned value is the actual number of elements in
        the aggregate value."""
        return INTEGER(len(self._container))

    def get_hiindex(self):
        """When V is a bag, list or set, the returned value is the actual number of elements in
        the aggregate value."""
        return INTEGER(len(self._container))

    def get_loindex(self):
        return INTEGER(1)

    def get_hibound(self):
        hibound = self._bound_2
        if type(hibound) == int:
            return INTEGER(hibound)
        else:
            return hibound

    def get_lobound(self):
        lobound = self._bound_1
        if type(lobound) == int:
            return INTEGER(lobound)
        else:
            return lobound

    def get_value_unique(self):
        """Return True if all items are different in the container, UNKNOWN if some items are
        indeterminate, or False otherwise"""
        if None in self._container:
            return Unknown
        else:
            return True
