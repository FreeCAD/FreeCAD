#ifndef COIN_SBSTRINGCONVERT_H
#define COIN_SBSTRINGCONVERT_H

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

#include <Inventor/SbBasic.h>
#include <Inventor/SbString.h>
#include "base/coinString.h"

class SbString;
class SbVec2s;
class SbVec2f;
class SbVec3f;
class SbVec3d;
class SbRotation;

template<size_t numArgs, typename Type, typename ArgumentType>
  struct constructFromArray {
    
  };

template<typename Type, typename ArgumentType>
  struct constructFromArray<2,Type,ArgumentType> {
  static Type constructor(const ArgumentType * inArray)
  {
    return Type(inArray[0],inArray[1]);
  }
};

template<typename Type, typename ArgumentType>
  struct constructFromArray<3,Type,ArgumentType> {
  static Type constructor(const ArgumentType * inArray)
  {
    return Type(inArray[0],inArray[1],inArray[2]);
  }
};

template<typename Type, typename ArgumentType>
  struct constructFromArray<4,Type,ArgumentType> {
  static Type constructor(const ArgumentType * inArray)
  {
    return Type(inArray[0],inArray[1],inArray[2],inArray[3]);
  }
};

template <typename T>
struct ScXMLConvert {
    static
    T fromString(const SbString & str, SbBool * conversionOk = NULL)
    {
      typename SbTypeInfo<T>::PrimitiveType tmpVal[SbTypeInfo<T>::Dimensions];
      SbString substr;
      int start = str.find("(");
      int end = str.find(")");
      if (start == -1 || end == -1) {
        if (conversionOk) *conversionOk = FALSE;
        return T();
      }
      substr = str.getSubString(0, start-1);
      if (substr != SbTypeInfo<T>::getTypeName()) {
        if (conversionOk) *conversionOk = FALSE;
        return T();
      }
      --end;
      ++start;
      assert(end>start);
      substr = str.getSubString(start,end);
      start = 0;
      SbIntList indices;
      substr.findAll(",",indices);
      if (indices.getLength()!= SbTypeInfo<T>::Dimensions-1) {
        if (conversionOk) *conversionOk = FALSE;
        return T();
      }
      for (int i=0;i<SbTypeInfo<T>::Dimensions-1;++i) {
        end = indices[i];
        SbString token = substr.getSubString(start,end);
        start=end+1;
        typename SbTypeInfo<T>::PrimitiveType t = FromString< typename SbTypeInfo<T>::PrimitiveType > (token, conversionOk);
        tmpVal[i]=t;
      }
      SbString token = substr.getSubString(start,-1);
      typename SbTypeInfo<T>::PrimitiveType t = FromString< typename SbTypeInfo<T>::PrimitiveType > (token, conversionOk);
      tmpVal[SbTypeInfo<T>::Dimensions-1]=t;
      if (conversionOk) *conversionOk = TRUE;
      return constructFromArray<SbTypeInfo<T>::Dimensions ,T, typename SbTypeInfo<T>::PrimitiveType >::constructor(tmpVal);
    }

  static SbString toString(const T & in) {
    SbString retVal = SbTypeInfo<T>::getTypeName();
    retVal+="(";
    retVal+=ToString<>(in[0]);
    for (int i=1;i<SbTypeInfo<T>::Dimensions;++i) {
      retVal+=", ";
      retVal+=ToString<>(in[i]);
    }
    retVal+=")";
    return retVal;
  }
};

template<typename T>
struct PrimitiveConvert {
  static T fromString(const SbString & str, SbBool * conversionOk = NULL) {
      return FromString<T>(str, conversionOk);
  }
  static SbString toString(const T & in) {
      return ToString<>(in);
  }
};

class SbStringConvert {
public:
  enum TypeIdentity {
    BOOLEAN,
    NUMERIC,
    SBVEC2S,
    SBVEC2F,
    SBVEC3F,
    SBVEC3D,
    SBROTATION,

    UNKNOWN
  };

  static TypeIdentity typeOf(const SbString & str);
  
  template <typename T>
    static
    T fromString(const SbString & str, SbBool * conversionOk = NULL)
    {
      return
        IF< SbTypeInfo<T>::isPrimitive, PrimitiveConvert<T>, ScXMLConvert<T> >::RET::fromString(str,conversionOk);
    }

  template <typename T>
    static
    SbString toString(const T & in)
    {
      SbString retVal =
        IF< SbTypeInfo<T>::isPrimitive, PrimitiveConvert<T>, ScXMLConvert<T> >::RET::toString(in);

      return retVal;
    }

};

#endif // !COIN_SBSTRINGCONVERT_H
