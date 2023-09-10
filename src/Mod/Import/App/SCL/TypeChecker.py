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

from .ConstructedDataTypes import ENUMERATION, SELECT
from . import BaseType

RAISE_EXCEPTION_IF_TYPE_DOES_NOT_MATCH = True
DEBUG = False


def cast_python_object_to_aggregate(obj, aggregate):
    """This function casts a python object to an aggregate type. For instance:
    [1.,2.,3.]-> ARRAY(1,3,REAL)"""
    aggregate_lower_bound = aggregate.bound_1()
    aggregate_upper_bound = aggregate.bound_2()
    if type(obj) == list:
        for idx in range(aggregate_lower_bound, aggregate_upper_bound + 1):
            aggregate[idx] = obj[idx - aggregate_lower_bound]
    return aggregate


def check_type(instance, expected_type):
    """This function checks whether an object is an instance of a given class
    returns False or True
    """
    type_match = False  # by default, will be set to True if any match
    if DEBUG:
        print("===")
        print("Instance passed: ", instance)
        print("Expected type: ", expected_type)
    # in the case of an enumeration, we have to check if the instance is in the list
    if isinstance(expected_type, ENUMERATION):
        allowed_ids = expected_type.get_enum_ids()
        if instance in allowed_ids:
            type_match = True
        else:
            raise TypeError(
                "Enumeration ids must be %s ( passed %s)" % (allowed_ids, type(instance))
            )
    elif isinstance(expected_type, SELECT):
        # we check if the instance is of the type of any of the types that are in the SELECT
        allowed_types = expected_type.get_allowed_basic_types()
        for allowed_type in allowed_types:
            if isinstance(instance, allowed_type):
                type_match = True
        if not type_match:
            if RAISE_EXCEPTION_IF_TYPE_DOES_NOT_MATCH:
                raise TypeError(
                    "Argument type must be %s (you passed %s)" % (allowed_types, type(instance))
                )
            else:
                print(
                    "WARNING: expected '%s' but passed a '%s', casting from python value to EXPRESS type"
                    % (allowed_types, type(instance))
                )
                return False
    elif isinstance(expected_type, BaseType.Aggregate):
        # first check that they are instance of the same class
        if not (type(instance) == type(expected_type)):
            raise TypeError("Expected %s but passed %s" % (type(expected_type), type(instance)))
        # then check that the base type is the same
        elif not (instance.get_type() == expected_type.get_type()):
            # print instance.get_type()
            # print expected_type.get_type()
            raise TypeError(
                "Expected %s:%s base type but passed %s:%s base type"
                % (
                    type(expected_type),
                    expected_type.get_type(),
                    type(instance),
                    instance.get_type(),
                )
            )
        # check optional and unique attributes
        # elif not (instance._unique == expected_type._unique):
        #    raise TypeError('Aggregate expects UNIQUE:%s property but passed UNIQUE:%s'%(expected_type._unique, instance._unique))
        # elif not (instance._optional == expected_type._optional):
        #    raise TypeError('Aggregate expects OPTIONAL:%s property but passed OPTIONAL:%s'%(expected_type._optional, instance._optional))
        # @TODO: check aggregate bounds
        else:
            type_match = True
    else:  # simple data types
        type_match = isinstance(instance, expected_type)
        if not type_match:
            if RAISE_EXCEPTION_IF_TYPE_DOES_NOT_MATCH:
                raise TypeError(
                    "Argument type must be %s (you passed %s)" % (expected_type, type(instance))
                )
            else:
                print(
                    "WARNING: expected '%s' but passed a '%s', casting from python value to EXPRESS type"
                    % (expected_type, type(instance))
                )
                return False
    return True
