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

""" This module provide string utils"""


def process_nested_parent_str(attr_str, idx=0):
    """
    The first letter should be a parenthesis
    input string: "(1,4,(5,6),7)"
    output: ['1','4',['5','6'],'7']
    """
    params = []
    current_param = ""
    k = 0
    while k < len(attr_str):
        ch = attr_str[k]
        k += 1
        if ch == ",":
            params.append(current_param)
            current_param = ""
        elif ch == "(":
            nv = attr_str[k:]
            current_param, progress = process_nested_parent_str(nv)
            params.append(current_param)
            current_param = ""
            k += progress + 1
        elif ch == ")":
            params.append(current_param)
            return params, k
        else:
            current_param += ch
    params.append(current_param)
    return params, k


if __name__ == "__main__":
    print(process_nested_parent_str2("'A'")[0])
    print(process_nested_parent_str2("30.0,0.0,5.0")[0])
    print(process_nested_parent_str2("1,2,(3,4,5),6,7,8")[0])
    print(process_nested_parent_str2("(#9149,#9166),#9142,.T.")[0])
