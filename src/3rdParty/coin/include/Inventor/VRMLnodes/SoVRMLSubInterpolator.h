/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#ifndef COIN_SOVRMLSUBINTERPOLATOR_H
#define COIN_SOVRMLSUBINTERPOLATOR_H

#include <Inventor/nodes/SoSubNode.h>

#define SO_INTERPOLATOR_HEADER(classname) \
  SO_NODE_HEADER(classname)
#define SO_INTERPOLATOR_ABSTRACT_HEADER(classname) \
  SO_NODE_ABSTRACT_HEADER(classname)

#define SO_INTERPOLATOR_SOURCE(classname) \
  SO_NODE_SOURCE(classname)

#define SO_INTERPOLATOR_ABSTRACT_SOURCE(classname) \
  SO_NODE_ABSTRACT_SOURCE(classname)

#define SO_INTERPOLATOR_INIT_CLASS(classname, printname, parentclass) \
  SO_NODE_INIT_CLASS(classname, printname, parentclass)

#define SO_INTERPOLATOR_INIT_ABSTRACT_CLASS(classname,printname,parent) \
  SO_NODE_INIT_ABSTRACT_CLASS(classname, printname, parent)

#define SO_INTERPOLATOR_CONSTRUCTOR(classname) \
  SO_NODE_CONSTRUCTOR(classname)

#define SO_INTERPOLATOR_ADD_INPUT(inputName, defaultValue) \
  SO_NODE_ADD_FIELD(inputName, defaultValue)

#define SO_INTERPOLATOR_ADD_OUTPUT(outputName, outputType)
#define SO_INTERPOLATOR_DEFINE_ENUM_VALUE(enumType, enumValue) \
  SO_NODE_DEFINE_ENUM_VALUE(enumType, enumValue)
#define SO_INTERPOLATOR_IS_FIRST_INSTANCE() \
  SO_NODE_IS_FIRST_INSTANCE()

#define SO_INTERPOLATOR_SET_SF_ENUM_TYPE(fieldName, enumType) \
  SO_NODE_SET_SF_ENUM_TYPE(fieldName, enumType)

#define SO_INTERPOLATOR_SET_MF_ENUM_TYPE(fieldName, enumType) \
  SO_NODE_SET_MF_ENUM_TYPE(fieldName, enumType)

#define SO_INTERPOLATOR_OUTPUT(outputName, outputType, method)

#endif // ! COIN_SOVRMLSUBINTERPOLATOR_H
