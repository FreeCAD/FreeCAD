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


class Type(object):
    """
    A type can be defined from its name and scope
    Looking into the scope dict returns the python type class.
    This is the base class for aggregated data types or constructed data types
    """

    def __init__(self, typedef, scope):
        self._scope = scope
        self._typedef = typedef

    def get_scope(self):
        return self._scope

    def get_type(self):
        if type(self._typedef) == str:
            if self._scope == None:
                raise AssertionError("No scope defined for this type")
            elif self._typedef in vars(self._scope):
                return vars(self._scope)[self._typedef]
            else:
                raise TypeError("Type '%s' is not defined in given scope" % self._typedef)
        else:
            return self._typedef


class Aggregate:
    """
    This is an abstract class. ARRAY, LIST, SET and BAG inherit from this class
    """

    pass


if __name__ == "__main__":
    import sys

    scp = sys.modules[__name__]

    class line:
        pass

    new_type = Type("lie", scp)
    print(new_type.get_type())
