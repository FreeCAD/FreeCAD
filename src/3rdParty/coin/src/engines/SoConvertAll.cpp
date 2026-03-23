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

// SoConvertAll is an internal class used for converting values
// between all built-in field types.
//
// Note that this class is not supposed to be used by the application
// programmer -- so the API class definition header file is not
// installed.

// *************************************************************************

#include "engines/SoConvertAll.h"

#include "SbBasicP.h"

#include <cassert>

#include <Inventor/SoDB.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/fields/SoFields.h>
#include <Inventor/lists/SoEngineOutputList.h>
#include <Inventor/lists/SoFieldList.h>
#include <Inventor/lists/SoTypeList.h>

#include "tidbitsp.h"
#include "engines/SoSubEngineP.h"
#include "misc/SbHash.h"

// *************************************************************************

// FIXME: should perhaps use SbTime::parseDate() for So[SM]FString ->
// So[SM]FTime conversion? 20000331 mortene.

// *************************************************************************

typedef void convert_func(SoField * from, SoField * to);
typedef SbHash<uint32_t, convert_func *> UInt32ToConverterFuncMap;

static UInt32ToConverterFuncMap * convertfunc_dict = NULL;

// *************************************************************************

// SoConvertAll uses a dynamic list for each instance with information
// about input fields and engine outputs, not like the other engines
// (which have a single static list for each class).
//
// Because of this, we can't use the SO_ENGINE_ABSTRACT_SOURCE macro.

PRIVATE_ENGINE_TYPESYSTEM_SOURCE(SoConvertAll);
unsigned int SoConvertAll::classinstances = 0;
const SoFieldData ** SoConvertAll::parentinputdata = NULL;
const SoEngineOutputData ** SoConvertAll::parentoutputdata = NULL;

const SoFieldData *
SoConvertAll::getFieldData(void) const
{
  return this->inputdata_instance;
}

const SoEngineOutputData *
SoConvertAll::getOutputData(void) const
{
  return this->outputdata_instance;
}

// These are unused, but we list them here since they are part of the
// SO_ENGINE_ABSTRACT_HEADER macro, which we are using for
// convenience.
SoFieldData * SoConvertAll::inputdata = reinterpret_cast<SoFieldData *>(0x1);
SoEngineOutputData * SoConvertAll::outputdata = reinterpret_cast<SoEngineOutputData *>(0x1);
const SoFieldData ** SoConvertAll::getInputDataPtr(void) { return NULL; }
const SoEngineOutputData ** SoConvertAll::getOutputDataPtr(void) { return NULL; }

void
SoConvertAll::atexit_cleanup(void)
{
  SoConvertAll::parentinputdata = NULL;
  SoConvertAll::parentoutputdata = NULL;
  SoConvertAll::classTypeId STATIC_SOTYPE_INIT;
  SoConvertAll::classinstances = 0;
}

// Defines functions for converting between vec2*-fields with different primitive types
#define SOCONVERTALL_SINGLE2SINGLE_VEC2(_fromto_, _from_, _to_, _fromtype_, _totype_, _toprim_) \
static void _fromto_(SoField * from, SoField * to) { \
  _fromtype_ val(coin_assert_cast<_from_ *>(from)->getValue()); \
  coin_assert_cast<_to_ *>(to)->setValue(_totype_(static_cast<_toprim_>(val[0]), static_cast<_toprim_>(val[1]))); \
}

#define SOCONVERTALL_SINGLE2MULTI_VEC2(_fromto_, _from_, _to_, _fromtype_, _totype_, _toprim_) \
static void _fromto_(SoField * from, SoField * to) { \
  _fromtype_ val(coin_assert_cast<_from_ *>(from)->getValue()); \
  coin_assert_cast<_to_ *>(to)->setValue(_totype_(static_cast<_toprim_>(val[0]), static_cast<_toprim_>(val[1]))); \
}

#define SOCONVERTALL_MULTI2SINGLE_VEC2(_fromto_, _from_, _to_, _fromtype_, _totype_, _toprim_) \
static void _fromto_(SoField * from, SoField * to) { \
  _fromtype_ val(coin_assert_cast<_from_ *>(from)->operator[](0)); \
  coin_assert_cast<_to_ *>(to)->setValue(_totype_(static_cast<_toprim_>(val[0]), static_cast<_toprim_>(val[1]))); \
}

#define SOCONVERTALL_MULTI2MULTI_VEC2(_fromto_, _from_, _to_, _fromtype_, _totype_, _toprim_) \
static void _fromto_(SoField * from, SoField * to) { \
  const int num = coin_assert_cast<_from_ *>(from)->getNum(); \
  for ( int i = 0; i < num; i++ ) { \
    _fromtype_ val(coin_assert_cast<_from_ *>(from)->operator[](i)); \
    coin_assert_cast<_to_ *>(to)->set1Value(i, _totype_(static_cast<_toprim_>(val[0]), static_cast<_toprim_>(val[1]))); \
  } \
}

// Defines functions for converting between vec3*-fields with different primitive types
#define SOCONVERTALL_SINGLE2SINGLE_VEC3(_fromto_, _from_, _to_, _fromtype_, _totype_, _toprim_) \
static void _fromto_(SoField * from, SoField * to) { \
  _fromtype_ val(coin_assert_cast<_from_ *>(from)->getValue()); \
  coin_assert_cast<_to_ *>(to)->setValue(_totype_(static_cast<_toprim_>(val[0]), static_cast<_toprim_>(val[1]), static_cast<_toprim_>(val[2]))); \
}

#define SOCONVERTALL_SINGLE2MULTI_VEC3(_fromto_, _from_, _to_, _fromtype_, _totype_, _toprim_) \
static void _fromto_(SoField * from, SoField * to) { \
  _fromtype_ val(coin_assert_cast<_from_ *>(from)->getValue()); \
  coin_assert_cast<_to_ *>(to)->setValue(_totype_(static_cast<_toprim_>(val[0]), static_cast<_toprim_>(val[1]), static_cast<_toprim_>(val[2]))); \
}

#define SOCONVERTALL_MULTI2SINGLE_VEC3(_fromto_, _from_, _to_, _fromtype_, _totype_, _toprim_) \
static void _fromto_(SoField * from, SoField * to) { \
  _fromtype_ val(coin_assert_cast<_from_ *>(from)->operator[](0)); \
  coin_assert_cast<_to_ *>(to)->setValue(_totype_(static_cast<_toprim_>(val[0]), static_cast<_toprim_>(val[1]), static_cast<_toprim_>(val[2]))); \
}

#define SOCONVERTALL_MULTI2MULTI_VEC3(_fromto_, _from_, _to_, _fromtype_, _totype_, _toprim_) \
static void _fromto_(SoField * from, SoField * to) { \
  const int num = coin_assert_cast<_from_ *>(from)->getNum(); \
  for ( int i = 0; i < num; i++ ) { \
    _fromtype_ val(coin_assert_cast<_from_ *>(from)->operator[](i)); \
    coin_assert_cast<_to_ *>(to)->set1Value(i, _totype_(static_cast<_toprim_>(val[0]), static_cast<_toprim_>(val[1]), static_cast<_toprim_>(val[2]))); \
  } \
}

// vec4 is only Vec4f - no other types yet

// short-to-float (no SoMFVec2s exists yet)
SOCONVERTALL_SINGLE2SINGLE_VEC2(SoSFVec2f_to_SoSFVec2s, SoSFVec2f, SoSFVec2s, SbVec2f, SbVec2s, short);
SOCONVERTALL_SINGLE2SINGLE_VEC2(SoSFVec2s_to_SoSFVec2f, SoSFVec2s, SoSFVec2f, SbVec2s, SbVec2f, float);
// SOCONVERTALL_SINGLE2MULTI_VEC2(SoSFVec2f_to_SoMFVec2s, SoSFVec2f, SoMFVec2s, SbVec2f, SbVec2s, short);
SOCONVERTALL_SINGLE2MULTI_VEC2(SoSFVec2s_to_SoMFVec2f, SoSFVec2s, SoMFVec2f, SbVec2s, SbVec2f, float);
SOCONVERTALL_MULTI2SINGLE_VEC2(SoMFVec2f_to_SoSFVec2s, SoMFVec2f, SoSFVec2s, SbVec2f, SbVec2s, short);
// SOCONVERTALL_MULTI2SINGLE_VEC2(SoMFVec2s_to_SoSFVec2f, SoMFVec2s, SoSFVec2f, SbVec2s, SbVec2f, float);
// SOCONVERTALL_MULTI2MULTI_VEC2(SoMFVec2f_to_SoMFVec2s, SoMFVec2f, SoMFVec2s, SbVec2f, SbVec2s, short);
// SOCONVERTALL_MULTI2MULTI_VEC2(SoMFVec2s_to_SoMFVec2f, SoMFVec2s, SoMFVec2f, SbVec2s, SbVec2f, float);

// float-to-double
SOCONVERTALL_SINGLE2SINGLE_VEC3(SoSFVec3f_to_SoSFVec3d, SoSFVec3f, SoSFVec3d, SbVec3f, SbVec3d, double);
SOCONVERTALL_SINGLE2SINGLE_VEC3(SoSFVec3d_to_SoSFVec3f, SoSFVec3d, SoSFVec3f, SbVec3d, SbVec3f, float);
SOCONVERTALL_SINGLE2MULTI_VEC3(SoSFVec3f_to_SoMFVec3d, SoSFVec3f, SoMFVec3d, SbVec3f, SbVec3d, double);
SOCONVERTALL_SINGLE2MULTI_VEC3(SoSFVec3d_to_SoMFVec3f, SoSFVec3d, SoMFVec3f, SbVec3d, SbVec3f, float);
SOCONVERTALL_MULTI2SINGLE_VEC3(SoMFVec3f_to_SoSFVec3d, SoMFVec3f, SoSFVec3d, SbVec3f, SbVec3d, double);
SOCONVERTALL_MULTI2SINGLE_VEC3(SoMFVec3d_to_SoSFVec3f, SoMFVec3d, SoSFVec3f, SbVec3d, SbVec3f, float);
SOCONVERTALL_MULTI2MULTI_VEC3(SoMFVec3f_to_SoMFVec3d, SoMFVec3f, SoMFVec3d, SbVec3f, SbVec3d, double);
SOCONVERTALL_MULTI2MULTI_VEC3(SoMFVec3d_to_SoMFVec3f, SoMFVec3d, SoMFVec3f, SbVec3d, SbVec3f, float);

// short-to-float (no SoMFVec3s exists yet)
SOCONVERTALL_SINGLE2SINGLE_VEC3(SoSFVec3s_to_SoSFVec3f, SoSFVec3s, SoSFVec3f, SbVec3s, SbVec3f, float);
SOCONVERTALL_SINGLE2SINGLE_VEC3(SoSFVec3f_to_SoSFVec3s, SoSFVec3f, SoSFVec3s, SbVec3f, SbVec3s, short);
SOCONVERTALL_SINGLE2MULTI_VEC3(SoSFVec3s_to_SoMFVec3f, SoSFVec3s, SoMFVec3f, SbVec3s, SbVec3f, float);
// SOCONVERTALL_SINGLE2MULTI_VEC3(SoSFVec3f_to_SoMFVec3s, SoSFVec3f, SoMFVec3s, SbVec3f, SbVec3s, short);
// SOCONVERTALL_MULTI2SINGLE_VEC3(SoMFVec3s_to_SoSFVec3f, SoMFVec3s, SoSFVec3f, SbVec3s, SbVec3f, float);
SOCONVERTALL_MULTI2SINGLE_VEC3(SoMFVec3f_to_SoSFVec3s, SoMFVec3f, SoSFVec3s, SbVec3f, SbVec3s, short);
// SOCONVERTALL_MULTI2MULTI_VEC3(SoMFVec3s_to_SoMFVec3f, SoMFVec3s, SoMFVec3f, SbVec3s, SbVec3f, float);
// SOCONVERTALL_MULTI2MULTI_VEC3(SoMFVec3f_to_SoMFVec3s, SoMFVec3f, SoMFVec3s, SbVec3f, SbVec3s, short);

// short-to-double (no SoMFVec3s exists yet)
SOCONVERTALL_SINGLE2SINGLE_VEC3(SoSFVec3s_to_SoSFVec3d, SoSFVec3s, SoSFVec3d, SbVec3s, SbVec3d, double);
SOCONVERTALL_SINGLE2SINGLE_VEC3(SoSFVec3d_to_SoSFVec3s, SoSFVec3d, SoSFVec3s, SbVec3d, SbVec3s, short);
SOCONVERTALL_SINGLE2MULTI_VEC3(SoSFVec3s_to_SoMFVec3d, SoSFVec3s, SoMFVec3d, SbVec3s, SbVec3d, double);
// SOCONVERTALL_SINGLE2MULTI_VEC3(SoSFVec3d_to_SoMFVec3s, SoSFVec3d, SoMFVec3s, SbVec3d, SbVec3s, short);
// SOCONVERTALL_MULTI2SINGLE_VEC3(SoMFVec3s_to_SoSFVec3d, SoMFVec3s, SoSFVec3d, SbVec3s, SbVec3d, double);
SOCONVERTALL_MULTI2SINGLE_VEC3(SoMFVec3d_to_SoSFVec3s, SoMFVec3d, SoSFVec3s, SbVec3d, SbVec3s, short);
// SOCONVERTALL_MULTI2MULTI_VEC3(SoMFVec3s_to_SoMFVec3d, SoMFVec3s, SoMFVec3d, SbVec3s, SbVec3d, double);
// SOCONVERTALL_MULTI2MULTI_VEC3(SoMFVec3d_to_SoMFVec3s, SoMFVec3d, SoMFVec3s, SbVec3d, SbVec3s, short);

#undef SOCONVERTALL_SINGLE2SINGLE_VEC2
#undef SOCONVERTALL_SINGLE2MULTI_VEC2
#undef SOCONVERTALL_MULTI2SINGLE_VEC2
#undef SOCONVERTALL_MULTI2MULTI_VEC2
#undef SOCONVERTALL_SINGLE2SINGLE_VEC3
#undef SOCONVERTALL_SINGLE2MULTI_VEC3
#undef SOCONVERTALL_MULTI2SINGLE_VEC3
#undef SOCONVERTALL_MULTI2MULTI_VEC3

// Defines function for converting SoSFXXX -> SoMFXXX.
#define SOCONVERTALL_SINGLE2MULTI(_fromto_, _from_, _to_) \
static void _fromto_(SoField * from, SoField * to) \
{ \
  coin_assert_cast<_to_ *>(to)->setValue(coin_assert_cast<_from_ *>(from)->getValue()); \
}

// Defines function for converting SoMFXXX -> SoSFXXX.
#define SOCONVERTALL_MULTI2SINGLE(_fromto_, _from_, _to_) \
static void _fromto_(SoField * from, SoField * to) \
{ \
  if (coin_assert_cast<_from_ *>(from)->getNum() > 0) \
    coin_assert_cast<_to_ *>(to)->setValue((*coin_assert_cast<_from_ *>(from))[0]); \
}


SOCONVERTALL_SINGLE2MULTI(SoSFBitMask_SoMFBitMask, SoSFBitMask, SoMFBitMask);
SOCONVERTALL_MULTI2SINGLE(SoMFBitMask_SoSFBitMask, SoMFBitMask, SoSFBitMask);
SOCONVERTALL_SINGLE2MULTI(SoSFBool_SoMFBool, SoSFBool, SoMFBool);
SOCONVERTALL_MULTI2SINGLE(SoMFBool_SoSFBool, SoMFBool, SoSFBool);
SOCONVERTALL_SINGLE2MULTI(SoSFColor_SoMFColor, SoSFColor, SoMFColor);
SOCONVERTALL_MULTI2SINGLE(SoMFColor_SoSFColor, SoMFColor, SoSFColor);
SOCONVERTALL_SINGLE2MULTI(SoSFEnum_SoMFEnum, SoSFEnum, SoMFEnum);
SOCONVERTALL_MULTI2SINGLE(SoMFEnum_SoSFEnum, SoMFEnum, SoSFEnum);
SOCONVERTALL_SINGLE2MULTI(SoSFFloat_SoMFFloat, SoSFFloat, SoMFFloat);
SOCONVERTALL_MULTI2SINGLE(SoMFFloat_SoSFFloat, SoMFFloat, SoSFFloat);
SOCONVERTALL_SINGLE2MULTI(SoSFInt32_SoMFInt32, SoSFInt32, SoMFInt32);
SOCONVERTALL_MULTI2SINGLE(SoMFInt32_SoSFInt32, SoMFInt32, SoSFInt32);
SOCONVERTALL_SINGLE2MULTI(SoSFMatrix_SoMFMatrix, SoSFMatrix, SoMFMatrix);
SOCONVERTALL_MULTI2SINGLE(SoMFMatrix_SoSFMatrix, SoMFMatrix, SoSFMatrix);
SOCONVERTALL_SINGLE2MULTI(SoSFName_SoMFName, SoSFName, SoMFName);
SOCONVERTALL_MULTI2SINGLE(SoMFName_SoSFName, SoMFName, SoSFName);
SOCONVERTALL_SINGLE2MULTI(SoSFNode_SoMFNode, SoSFNode, SoMFNode);
SOCONVERTALL_MULTI2SINGLE(SoMFNode_SoSFNode, SoMFNode, SoSFNode);
SOCONVERTALL_SINGLE2MULTI(SoSFPath_SoMFPath, SoSFPath, SoMFPath);
SOCONVERTALL_MULTI2SINGLE(SoMFPath_SoSFPath, SoMFPath, SoSFPath);
SOCONVERTALL_SINGLE2MULTI(SoSFPlane_SoMFPlane, SoSFPlane, SoMFPlane);
SOCONVERTALL_MULTI2SINGLE(SoMFPlane_SoSFPlane, SoMFPlane, SoSFPlane);
SOCONVERTALL_SINGLE2MULTI(SoSFRotation_SoMFRotation, SoSFRotation, SoMFRotation);
SOCONVERTALL_MULTI2SINGLE(SoMFRotation_SoSFRotation, SoMFRotation, SoSFRotation);
SOCONVERTALL_SINGLE2MULTI(SoSFShort_SoMFShort, SoSFShort, SoMFShort);
SOCONVERTALL_MULTI2SINGLE(SoMFShort_SoSFShort, SoMFShort, SoSFShort);
SOCONVERTALL_SINGLE2MULTI(SoSFString_SoMFString, SoSFString, SoMFString);
SOCONVERTALL_MULTI2SINGLE(SoMFString_SoSFString, SoMFString, SoSFString);
SOCONVERTALL_SINGLE2MULTI(SoSFTime_SoMFTime, SoSFTime, SoMFTime);
SOCONVERTALL_MULTI2SINGLE(SoMFTime_SoSFTime, SoMFTime, SoSFTime);
SOCONVERTALL_SINGLE2MULTI(SoSFUInt32_SoMFUInt32, SoSFUInt32, SoMFUInt32);
SOCONVERTALL_MULTI2SINGLE(SoMFUInt32_SoSFUInt32, SoMFUInt32, SoSFUInt32);
SOCONVERTALL_SINGLE2MULTI(SoSFUShort_SoMFUShort, SoSFUShort, SoMFUShort);
SOCONVERTALL_MULTI2SINGLE(SoMFUShort_SoSFUShort, SoMFUShort, SoSFUShort);
// SOCONVERTALL_SINGLE2MULTI(SoSFVec2s_SoMFVec2s, SoSFVec2s, SoMFVec2s);
// SOCONVERTALL_MULTI2SINGLE(SoMFVec2s_SoSFVec2s, SoMFVec2s, SoSFVec2s);
SOCONVERTALL_SINGLE2MULTI(SoSFVec2f_SoMFVec2f, SoSFVec2f, SoMFVec2f);
SOCONVERTALL_MULTI2SINGLE(SoMFVec2f_SoSFVec2f, SoMFVec2f, SoSFVec2f);
// SOCONVERTALL_SINGLE2MULTI(SoSFVec3s_SoMFVec3s, SoSFVec3s, SoMFVec3s);
// SOCONVERTALL_MULTI2SINGLE(SoMFVec3s_SoSFVec3s, SoMFVec3s, SoSFVec3s);
SOCONVERTALL_SINGLE2MULTI(SoSFVec3f_SoMFVec3f, SoSFVec3f, SoMFVec3f);
SOCONVERTALL_MULTI2SINGLE(SoMFVec3f_SoSFVec3f, SoMFVec3f, SoSFVec3f);
SOCONVERTALL_SINGLE2MULTI(SoSFVec3d_SoMFVec3d, SoSFVec3d, SoMFVec3d);
SOCONVERTALL_MULTI2SINGLE(SoMFVec3d_SoSFVec3d, SoMFVec3d, SoSFVec3d);
SOCONVERTALL_SINGLE2MULTI(SoSFVec4f_SoMFVec4f, SoSFVec4f, SoMFVec4f);
SOCONVERTALL_MULTI2SINGLE(SoMFVec4f_SoSFVec4f, SoMFVec4f, SoSFVec4f);

#undef SOCONVERTALL_SINGLE2MULTI
#undef SOCONVERTALL_MULTI2SINGLE


// Function for converting SoField -> SoSFString.
static void field_to_sfstring(SoField * from, SoField * to)
{
  SbString s;
  from->get(s);
  coin_assert_cast<SoSFString *>(to)->setValue(s);
}

// Function for converting SoSFString -> SoField.
static void sfstring_to_field(SoField * from, SoField * to)
{
  to->set(coin_assert_cast<SoSFString *>(from)->getValue().getString());
}

// Function for converting SoSField -> SoMFString.
static void sfield_to_mfstring(SoField * from, SoField * to)
{
  SbString s;
  coin_assert_cast<SoSField *>(from)->get(s);
  coin_assert_cast<SoMFString *>(to)->setValue(s);
}

// Function for converting SoMFString -> SoSField.
static void mfstring_to_sfield(SoField * from, SoField * to)
{
  if (coin_assert_cast<SoMFString *>(from)->getNum() > 0)
    coin_assert_cast<SoSField *>(to)->set((*coin_assert_cast<SoMFString *>(from))[0].getString());
}

// Function for converting SoMField -> SoMFString.
static void mfield_to_mfstring(SoField * from, SoField * to)
{
  unsigned int numvals = coin_assert_cast<SoMField *>(from)->getNum();
  coin_assert_cast<SoMField *>(to)->setNum(numvals);
  SbString s;
  for (unsigned int i=0; i < numvals; i++) {
    coin_assert_cast<SoMField *>(from)->get1(i, s);
    coin_assert_cast<SoMFString *>(to)->set1Value(i, s);
  }
}

// Function for converting SoMFString -> SoMField.
static void mfstring_to_mfield(SoField * from, SoField * to)
{
  unsigned int numvals = coin_assert_cast<SoMField *>(from)->getNum();
  coin_assert_cast<SoMField *>(to)->setNum(numvals);

  for (unsigned int i=0; i < numvals; i++)
    coin_assert_cast<SoMField *>(to)->set1(i, (*coin_assert_cast<SoMFString *>(from))[i].getString());
}

// Defines function for converting SoSField -> SoField, where
// conversion is just a typecast.
#define SOCONVERTALL_CAST_SFIELD2FIELD(_fromto_, _from_, _to_, _tobase_) \
static void _fromto_(SoField * from, SoField * to) \
{ \
  coin_assert_cast<_to_ *>(to)->setValue(static_cast<_tobase_>(coin_assert_cast<_from_ *>(from)->getValue())); \
}

SOCONVERTALL_CAST_SFIELD2FIELD(SoSFBool_SoSFFloat, SoSFBool, SoSFFloat, float);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFBool_SoMFFloat, SoSFBool, SoMFFloat, float);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFFloat_SoSFBool, SoSFFloat, SoSFBool, SbBool);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFFloat_SoMFBool, SoSFFloat, SoMFBool, SbBool);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFBool_SoSFInt32, SoSFBool, SoSFInt32, int32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFBool_SoMFInt32, SoSFBool, SoMFInt32, int32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFInt32_SoSFBool, SoSFInt32, SoSFBool, SbBool);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFInt32_SoMFBool, SoSFInt32, SoMFBool, SbBool);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFBool_SoSFShort, SoSFBool, SoSFShort, short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFBool_SoMFShort, SoSFBool, SoMFShort, short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFShort_SoSFBool, SoSFShort, SoSFBool, SbBool);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFShort_SoMFBool, SoSFShort, SoMFBool, SbBool);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFBool_SoSFUInt32, SoSFBool, SoSFUInt32, unsigned short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFBool_SoMFUInt32, SoSFBool, SoMFUInt32, unsigned short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUInt32_SoSFBool, SoSFUInt32, SoSFBool, SbBool);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUInt32_SoMFBool, SoSFUInt32, SoMFBool, SbBool);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFBool_SoSFUShort, SoSFBool, SoSFUShort, unsigned short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFBool_SoMFUShort, SoSFBool, SoMFUShort, unsigned short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUShort_SoSFBool, SoSFUShort, SoSFBool, SbBool);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUShort_SoMFBool, SoSFUShort, SoMFBool, SbBool);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFColor_SoSFVec3f, SoSFColor, SoSFVec3f, SbVec3f);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFColor_SoMFVec3f, SoSFColor, SoMFVec3f, SbVec3f);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFVec3f_SoSFColor, SoSFVec3f, SoSFColor, SbColor);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFVec3f_SoMFColor, SoSFVec3f, SoMFColor, SbColor);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFFloat_SoSFInt32, SoSFFloat, SoSFInt32, int32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFFloat_SoMFInt32, SoSFFloat, SoMFInt32, int32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFInt32_SoSFFloat, SoSFInt32, SoSFFloat, float);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFInt32_SoMFFloat, SoSFInt32, SoMFFloat, float);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFFloat_SoSFShort, SoSFFloat, SoSFShort, short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFFloat_SoMFShort, SoSFFloat, SoMFShort, short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFShort_SoSFFloat, SoSFShort, SoSFFloat, float);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFShort_SoMFFloat, SoSFShort, SoMFFloat, float);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFFloat_SoSFUInt32, SoSFFloat, SoSFUInt32, uint32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFFloat_SoMFUInt32, SoSFFloat, SoMFUInt32, uint32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUInt32_SoSFFloat, SoSFUInt32, SoSFFloat, float);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUInt32_SoMFFloat, SoSFUInt32, SoMFFloat, float);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFFloat_SoSFUShort, SoSFFloat, SoSFUShort, unsigned short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFFloat_SoMFUShort, SoSFFloat, SoMFUShort, unsigned short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUShort_SoSFFloat, SoSFUShort, SoSFFloat, float);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUShort_SoMFFloat, SoSFUShort, SoMFFloat, float);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFInt32_SoSFShort, SoSFInt32, SoSFShort, short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFInt32_SoMFShort, SoSFInt32, SoMFShort, short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFShort_SoSFInt32, SoSFShort, SoSFInt32, int32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFShort_SoMFInt32, SoSFShort, SoMFInt32, int32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFInt32_SoSFUInt32, SoSFInt32, SoSFUInt32, uint32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFInt32_SoMFUInt32, SoSFInt32, SoMFUInt32, uint32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUInt32_SoSFInt32, SoSFUInt32, SoSFInt32, int32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUInt32_SoMFInt32, SoSFUInt32, SoMFInt32, int32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFInt32_SoSFUShort, SoSFInt32, SoSFUShort, unsigned short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFInt32_SoMFUShort, SoSFInt32, SoMFUShort, unsigned short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUShort_SoSFInt32, SoSFUShort, SoSFInt32, int32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUShort_SoMFInt32, SoSFUShort, SoMFInt32, int32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFShort_SoSFUInt32, SoSFShort, SoSFUInt32, uint32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFShort_SoMFUInt32, SoSFShort, SoMFUInt32, uint32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUInt32_SoSFShort, SoSFUInt32, SoSFShort, short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUInt32_SoMFShort, SoSFUInt32, SoMFShort, short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFShort_SoSFUShort, SoSFShort, SoSFUShort, unsigned short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFShort_SoMFUShort, SoSFShort, SoMFUShort, unsigned short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUShort_SoSFShort, SoSFUShort, SoSFShort, short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUShort_SoMFShort, SoSFUShort, SoMFShort, short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUInt32_SoSFUShort, SoSFUInt32, SoSFUShort, unsigned short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUInt32_SoMFUShort, SoSFUInt32, SoMFUShort, unsigned short);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUShort_SoSFUInt32, SoSFUShort, SoSFUInt32, uint32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFUShort_SoMFUInt32, SoSFUShort, SoMFUInt32, uint32_t);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFFloat_SoSFTime, SoSFFloat, SoSFTime, SbTime);
SOCONVERTALL_CAST_SFIELD2FIELD(SoSFFloat_SoMFTime, SoSFFloat, SoMFTime, SbTime);

// Defines function for converting SoMField -> SoSField, where
// conversion is just a typecast.
#define SOCONVERTALL_CAST_MFIELD2SFIELD(_fromto_, _from_, _to_, _tobase_) \
static void _fromto_(SoField * from, SoField * to) \
{ \
  if (coin_assert_cast<_from_ *>(from)->getNum() > 0) \
    coin_assert_cast<_to_ *>(to)->setValue(static_cast<_tobase_>((*coin_assert_cast<_from_ *>(from))[0])); \
}

SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFBool_SoSFFloat, SoMFBool, SoSFFloat, float);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFFloat_SoSFBool, SoMFFloat, SoSFBool, SbBool);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFBool_SoSFInt32, SoMFBool, SoSFInt32, int32_t);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFInt32_SoSFBool, SoMFInt32, SoSFBool, SbBool);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFBool_SoSFShort, SoMFBool, SoSFShort, short);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFShort_SoSFBool, SoMFShort, SoSFBool, SbBool);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFBool_SoSFUInt32, SoMFBool, SoSFUInt32, uint32_t);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFUInt32_SoSFBool, SoMFUInt32, SoSFBool, SbBool);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFBool_SoSFUShort, SoMFBool, SoSFUShort, unsigned short);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFUShort_SoSFBool, SoMFUShort, SoSFBool, SbBool);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFColor_SoSFVec3f, SoMFColor, SoSFVec3f, SbVec3f);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFVec3f_SoSFColor, SoMFVec3f, SoSFColor, SbColor);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFFloat_SoSFInt32, SoMFFloat, SoSFInt32, int32_t);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFInt32_SoSFFloat, SoMFInt32, SoSFFloat, float);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFFloat_SoSFShort, SoMFFloat, SoSFShort, short);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFShort_SoSFFloat, SoMFShort, SoSFFloat, float);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFFloat_SoSFUInt32, SoMFFloat, SoSFUInt32, uint32_t);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFUInt32_SoSFFloat, SoMFUInt32, SoSFFloat, float);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFFloat_SoSFUShort, SoMFFloat, SoSFUShort, unsigned short);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFUShort_SoSFFloat, SoMFUShort, SoSFFloat, float);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFInt32_SoSFShort, SoMFInt32, SoSFShort, short);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFShort_SoSFInt32, SoMFShort, SoSFInt32, int32_t);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFInt32_SoSFUInt32, SoMFInt32, SoSFUInt32, uint32_t);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFUInt32_SoSFInt32, SoMFUInt32, SoSFInt32, int32_t);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFInt32_SoSFUShort, SoMFInt32, SoSFUShort, unsigned short);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFUShort_SoSFInt32, SoMFUShort, SoSFInt32, int32_t);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFShort_SoSFUInt32, SoMFShort, SoSFUInt32, uint32_t);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFUInt32_SoSFShort, SoMFUInt32, SoSFShort, short);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFShort_SoSFUShort, SoMFShort, SoSFUShort, unsigned short);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFUShort_SoSFShort, SoMFUShort, SoSFShort, short);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFUInt32_SoSFUShort, SoMFUInt32, SoSFUShort, unsigned short);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFUShort_SoSFUInt32, SoMFUShort, SoSFUInt32, uint32_t);
SOCONVERTALL_CAST_MFIELD2SFIELD(SoMFFloat_SoSFTime, SoMFFloat, SoSFTime, SbTime);

// Defines functions for converting SoMField -> SoMField, where
// conversion is just a typecast.
#define SOCONVERTALL_CAST_MFIELD2MFIELD(_fromto_, _from_, _to_, _tobase_) \
static void _fromto_(SoField * from, SoField * to) \
{ \
  unsigned int numvals = coin_assert_cast<SoMField *>(from)->getNum(); \
  coin_assert_cast<SoMField *>(to)->setNum(numvals); \
  for (unsigned int i=0; i < numvals; i++) \
    coin_assert_cast<_to_ *>(to)->set1Value(i, static_cast<_tobase_>((*coin_assert_cast<_from_ *>(from))[i])); \
}

SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFBool_SoMFFloat, SoMFBool, SoMFFloat, float);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFFloat_SoMFBool, SoMFFloat, SoMFBool, SbBool);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFBool_SoMFInt32, SoMFBool, SoMFInt32, int32_t);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFInt32_SoMFBool, SoMFInt32, SoMFBool, SbBool);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFBool_SoMFShort, SoMFBool, SoMFShort, short);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFShort_SoMFBool, SoMFShort, SoMFBool, SbBool);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFBool_SoMFUInt32, SoMFBool, SoMFUInt32, uint32_t);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFUInt32_SoMFBool, SoMFUInt32, SoMFBool, SbBool);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFBool_SoMFUShort, SoMFBool, SoMFUShort, unsigned short);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFUShort_SoMFBool, SoMFUShort, SoMFBool, SbBool);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFColor_SoMFVec3f, SoMFColor, SoMFVec3f, SbVec3f);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFVec3f_SoMFColor, SoMFVec3f, SoMFColor, SbColor);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFFloat_SoMFInt32, SoMFFloat, SoMFInt32, int32_t);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFInt32_SoMFFloat, SoMFInt32, SoMFFloat, float);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFFloat_SoMFShort, SoMFFloat, SoMFShort, short);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFShort_SoMFFloat, SoMFShort, SoMFFloat, float);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFFloat_SoMFUInt32, SoMFFloat, SoMFUInt32, uint32_t);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFUInt32_SoMFFloat, SoMFUInt32, SoMFFloat, float);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFFloat_SoMFUShort, SoMFFloat, SoMFUShort, unsigned short);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFUShort_SoMFFloat, SoMFUShort, SoMFFloat, float);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFInt32_SoMFShort, SoMFInt32, SoMFShort, short);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFShort_SoMFInt32, SoMFShort, SoMFInt32, int32_t);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFInt32_SoMFUInt32, SoMFInt32, SoMFUInt32, uint32_t);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFUInt32_SoMFInt32, SoMFUInt32, SoMFInt32, int32_t);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFInt32_SoMFUShort, SoMFInt32, SoMFUShort, unsigned short);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFUShort_SoMFInt32, SoMFUShort, SoMFInt32, int32_t);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFShort_SoMFUInt32, SoMFShort, SoMFUInt32, uint32_t);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFUInt32_SoMFShort, SoMFUInt32, SoMFShort, short);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFShort_SoMFUShort, SoMFShort, SoMFUShort, unsigned short);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFUShort_SoMFShort, SoMFUShort, SoMFShort, short);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFUInt32_SoMFUShort, SoMFUInt32, SoMFUShort, unsigned short);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFUShort_SoMFUInt32, SoMFUShort, SoMFUInt32, uint32_t);
SOCONVERTALL_CAST_MFIELD2MFIELD(SoMFFloat_SoMFTime, SoMFFloat, SoMFTime, SbTime);


// Defines function for converting SoSFTime -> So[SM]FFloat.
#define SOCONVERTALL_CAST_SFTIME2SFFLOAT(_fromto_, _to_) \
static void _fromto_(SoField * from, SoField * to) \
{ \
  coin_assert_cast<_to_ *>(to)->setValue(static_cast<float>(coin_assert_cast<SoSFTime *>(from)->getValue().getValue())); \
}

SOCONVERTALL_CAST_SFTIME2SFFLOAT(SoSFTime_SoSFFloat, SoSFFloat);
SOCONVERTALL_CAST_SFTIME2SFFLOAT(SoSFTime_SoMFFloat, SoMFFloat);

// Function for converting SoMFTime -> SoSFFloat.
static void SoMFTime_SoSFFloat(SoField * from, SoField * to)
{
  if (coin_assert_cast<SoMField *>(from)->getNum() > 0)
    coin_assert_cast<SoSFFloat *>(to)->setValue(static_cast<float>(((*coin_assert_cast<SoMFTime *>(from))[0]).getValue()));
}

// Function for converting SoMFTime -> SoMFFloat.
static void SoMFTime_SoMFFloat(SoField * from, SoField * to)
{
  unsigned int numvals = coin_assert_cast<SoMField *>(from)->getNum();
  coin_assert_cast<SoMField *>(to)->setNum(numvals);
  for (unsigned int i=0; i < numvals; i++)
    coin_assert_cast<SoMFFloat *>(to)->set1Value(i, static_cast<float>(((*coin_assert_cast<SoMFTime *>(from))[i]).getValue()));
}



// Defines function for converting SoSFMatrix -> So[SM]FRotation.
#define SOCONVERTALL_SFMATRIX2ROTATION(_fromto_, _to_) \
static void _fromto_(SoField * from, SoField * to) \
{ \
  coin_assert_cast<_to_ *>(to)->setValue(SbRotation(coin_assert_cast<SoSFMatrix *>(from)->getValue())); \
}

SOCONVERTALL_SFMATRIX2ROTATION(SoSFMatrix_SoSFRotation, SoSFRotation);
SOCONVERTALL_SFMATRIX2ROTATION(SoSFMatrix_SoMFRotation, SoMFRotation);

// Function for converting SoMFMatrix -> SoSFRotation.
static void SoMFMatrix_SoSFRotation(SoField * from, SoField * to)
{
  if (coin_assert_cast<SoMField *>(from)->getNum() > 0)
    coin_assert_cast<SoSFRotation *>(to)->setValue(SbRotation(((*coin_assert_cast<SoMFMatrix *>(from))[0])));
}

// Function for converting SoMFMatrix -> SoMFRotation.
static void SoMFMatrix_SoMFRotation(SoField * from, SoField * to)
{
  for (int i=0; i < coin_assert_cast<SoMField *>(from)->getNum(); i++)
    coin_assert_cast<SoMFRotation *>(to)->set1Value(i, SbRotation(((*coin_assert_cast<SoMFMatrix *>(from))[i])));
}


// Defines function for converting SoSFRotation -> So[SM]FMatrix.
#define SOCONVERTALL_SFROTATION2MATRIX(_fromto_, _to_) \
static void _fromto_(SoField * from, SoField * to) \
{ \
  SbMatrix mat; \
  mat.setRotate(coin_assert_cast<SoSFRotation *>(from)->getValue()); \
  coin_assert_cast<_to_ *>(to)->setValue(mat); \
}

SOCONVERTALL_SFROTATION2MATRIX(SoSFRotation_SoSFMatrix, SoSFMatrix);
SOCONVERTALL_SFROTATION2MATRIX(SoSFRotation_SoMFMatrix, SoMFMatrix);

// Function for converting SoMFRotation -> SoSFMatrix.
static void SoMFRotation_SoSFMatrix(SoField * from, SoField * to)
{
  if (coin_assert_cast<SoMField *>(from)->getNum() > 0) {
    SbMatrix mat;
    mat.setRotate((*coin_assert_cast<SoMFRotation *>(from))[0]);
    coin_assert_cast<SoSFMatrix *>(to)->setValue(mat);
  }
}

// Function for converting SoMFRotation -> SoMFMatrix.
static void SoMFRotation_SoMFMatrix(SoField * from, SoField * to)
{
  unsigned int numvals = coin_assert_cast<SoMField *>(from)->getNum();
  coin_assert_cast<SoMField *>(to)->setNum(numvals);
  for (unsigned int i=0; i < numvals; i++) {
    SbMatrix mat;
    mat.setRotate((*coin_assert_cast<SoMFRotation *>(from))[i]);
    coin_assert_cast<SoMFMatrix *>(to)->set1Value(i, mat);
  }
}


// Helper function for the So[SM]FTime -> So[SM]FString converters
// below.
static void time2string(const SbTime & t, SbString & s)
{
  // Value is less than a year, assume we're counting seconds. Use
  // resolution at millisecond accuracy.
  if (t.getValue() < (60.0*60.0*24.0*365.0)) {
    cc_string storedlocale;
    SbBool changed = coin_locale_set_portable(&storedlocale);
    s.sprintf("%.3f", t.getValue());
    if (changed) { coin_locale_reset(&storedlocale); }
  }

  // Value is more than a year, assume we're interested in the date
  // and time.
#if 0 // Don't default to ISO 8601 conformant string, ...
  // Note: if this is ever enabled, remember that the format string
  // need to be different on Microsoft Windows systems.
  else s = t.formatDate("%A %Y-%m-%d %H:%M:%S");
#else // .. follow Open Inventor instead.
  else s = t.formatDate();
#endif
}

// Function for converting SoSFTime -> SoSFString.
static void sftime_to_sfstring(SoField * from, SoField * to)
{
  SbString s;
  time2string(coin_assert_cast<SoSFTime *>(from)->getValue(), s);
  coin_assert_cast<SoSFString *>(to)->setValue(s);
}

// Function for converting SoSFTime -> SoMFString.
static void sftime_to_mfstring(SoField * from, SoField * to)
{
  SbString s;
  time2string(coin_assert_cast<SoSFTime *>(from)->getValue(), s);
  coin_assert_cast<SoMFString *>(to)->setValue(s);
}

// Function for converting SoMFTime -> SoSFString.
static void mftime_to_sfstring(SoField * from, SoField * to)
{
  SoMFTime * ff = coin_assert_cast<SoMFTime *>(from);
  if (ff->getNum() > 0) {
    SbString s;
    time2string((*ff)[0], s);
    coin_assert_cast<SoSFString *>(to)->setValue(s);
  }
}

// Function for converting SoMFTime -> SoMFString.
static void mftime_to_mfstring(SoField * from, SoField * to)
{
  SoMFTime * ff = coin_assert_cast<SoMFTime *>(from);
  unsigned int numvals = ff->getNum();
  coin_assert_cast<SoMField *>(to)->setNum(numvals);
  SbString s;
  for (unsigned int i=0; i < numvals; i++) {
    time2string((*ff)[i], s);
    coin_assert_cast<SoMFString *>(to)->set1Value(i, s);
  }
}

// Function for "converting" SoField -> SoSFTrigger _and_
// SoSFTrigger -> SoField.
static void to_and_from_sftrigger(SoField * COIN_UNUSED_ARG(from), SoField * to)
{
  to->setDirty(FALSE);
}

static void
register_convertfunc(convert_func * f, SoType from, SoType to)
{
  SoDB::addConverter(from, to, SoConvertAll::getClassTypeId());
  uint32_t val = (static_cast<uint32_t>(from.getKey()) << 16) + to.getKey();
  SbBool nonexist = convertfunc_dict->put(val, f);
  assert(nonexist);
}

extern "C" {

static void convertall_cleanup_dict(void)
{
  delete convertfunc_dict;
  convertfunc_dict = NULL;
}

} // extern "C"

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoConvertAll::initClass(void)
{
  convertfunc_dict = new UInt32ToConverterFuncMap;
  coin_atexit(convertall_cleanup_dict, CC_ATEXIT_NORMAL);

  // SoConvertAll doesn't have a createInstance() method (because it
  // doesn't have a default constructor), so use the ABSTRACT macros.
  SO_ENGINE_INTERNAL_INIT_ABSTRACT_CLASS(SoConvertAll);

  struct Conversion {
    convert_func * func;
    const char * from;
    const char * to;
  };

  struct Conversion allconverters[] = {
    { SoSFBitMask_SoMFBitMask, "SoSFBitMask", "SoMFBitMask" },
    { SoMFBitMask_SoSFBitMask, "SoMFBitMask", "SoSFBitMask" },
    { SoSFBool_SoMFBool, "SoSFBool", "SoMFBool" },
    { SoMFBool_SoSFBool, "SoMFBool", "SoSFBool" },
    { SoSFColor_SoMFColor, "SoSFColor", "SoMFColor" },
    { SoMFColor_SoSFColor, "SoMFColor", "SoSFColor" },
    { SoSFEnum_SoMFEnum, "SoSFEnum", "SoMFEnum" },
    { SoMFEnum_SoSFEnum, "SoMFEnum", "SoSFEnum" },
    { SoSFFloat_SoMFFloat, "SoSFFloat", "SoMFFloat" },
    { SoMFFloat_SoSFFloat, "SoMFFloat", "SoSFFloat" },
    { SoSFInt32_SoMFInt32, "SoSFInt32", "SoMFInt32" },
    { SoMFInt32_SoSFInt32, "SoMFInt32", "SoSFInt32" },
    { SoSFMatrix_SoMFMatrix, "SoSFMatrix", "SoMFMatrix" },
    { SoMFMatrix_SoSFMatrix, "SoMFMatrix", "SoSFMatrix" },
    { SoSFName_SoMFName, "SoSFName", "SoMFName" },
    { SoMFName_SoSFName, "SoMFName", "SoSFName" },
    { SoSFNode_SoMFNode, "SoSFNode", "SoMFNode" },
    { SoMFNode_SoSFNode, "SoMFNode", "SoSFNode" },
    { SoSFPath_SoMFPath, "SoSFPath", "SoMFPath" },
    { SoMFPath_SoSFPath, "SoMFPath", "SoSFPath" },
    { SoSFPlane_SoMFPlane, "SoSFPlane", "SoMFPlane" },
    { SoMFPlane_SoSFPlane, "SoMFPlane", "SoSFPlane" },
    { SoSFRotation_SoMFRotation, "SoSFRotation", "SoMFRotation" },
    { SoMFRotation_SoSFRotation, "SoMFRotation", "SoSFRotation" },
    { SoSFShort_SoMFShort, "SoSFShort", "SoMFShort" },
    { SoMFShort_SoSFShort, "SoMFShort", "SoSFShort" },
    { SoSFString_SoMFString, "SoSFString", "SoMFString" },
    { SoMFString_SoSFString, "SoMFString", "SoSFString" },
    { SoSFTime_SoMFTime, "SoSFTime", "SoMFTime" },
    { SoMFTime_SoSFTime, "SoMFTime", "SoSFTime" },
    { SoSFUInt32_SoMFUInt32, "SoSFUInt32", "SoMFUInt32" },
    { SoMFUInt32_SoSFUInt32, "SoMFUInt32", "SoSFUInt32" },
    { SoSFUShort_SoMFUShort, "SoSFUShort", "SoMFUShort" },
    { SoMFUShort_SoSFUShort, "SoMFUShort", "SoSFUShort" },
    { SoSFVec2f_SoMFVec2f, "SoSFVec2f", "SoMFVec2f" },
    { SoMFVec2f_SoSFVec2f, "SoMFVec2f", "SoSFVec2f" },
    { SoSFVec3f_SoMFVec3f, "SoSFVec3f", "SoMFVec3f" },
    { SoMFVec3f_SoSFVec3f, "SoMFVec3f", "SoSFVec3f" },
    { SoSFVec4f_SoMFVec4f, "SoSFVec4f", "SoMFVec4f" },
    { SoMFVec4f_SoSFVec4f, "SoMFVec4f", "SoSFVec4f" },
    { SoSFVec3d_SoMFVec3d, "SoSFVec3d", "SoMFVec3d" },
    { SoMFVec3d_SoSFVec3d, "SoMFVec3d", "SoSFVec3d" },
    { field_to_sfstring, "SoMFBitMask", "SoSFString" },
    { field_to_sfstring, "SoMFBool", "SoSFString" },
    { field_to_sfstring, "SoMFColor", "SoSFString" },
    { field_to_sfstring, "SoMFEnum", "SoSFString" },
    { field_to_sfstring, "SoMFFloat", "SoSFString" },
    { field_to_sfstring, "SoMFInt32", "SoSFString" },
    { field_to_sfstring, "SoMFMatrix", "SoSFString" },
    { field_to_sfstring, "SoMFName", "SoSFString" },
    { field_to_sfstring, "SoMFNode", "SoSFString" },
    { field_to_sfstring, "SoMFPath", "SoSFString" },
    { field_to_sfstring, "SoMFPlane", "SoSFString" },
    { field_to_sfstring, "SoMFRotation", "SoSFString" },
    { field_to_sfstring, "SoMFShort", "SoSFString" },
    { field_to_sfstring, "SoMFUInt32", "SoSFString" },
    { field_to_sfstring, "SoMFUShort", "SoSFString" },
    { field_to_sfstring, "SoMFVec2f", "SoSFString" },
    { field_to_sfstring, "SoMFVec3f", "SoSFString" },
    { field_to_sfstring, "SoMFVec4f", "SoSFString" },
    { field_to_sfstring, "SoSFBitMask", "SoSFString" },
    { field_to_sfstring, "SoSFBool", "SoSFString" },
    { field_to_sfstring, "SoSFColor", "SoSFString" },
    { field_to_sfstring, "SoSFEnum", "SoSFString" },
    { field_to_sfstring, "SoSFFloat", "SoSFString" },
    { field_to_sfstring, "SoSFInt32", "SoSFString" },
    { field_to_sfstring, "SoSFMatrix", "SoSFString" },
    { field_to_sfstring, "SoSFName", "SoSFString" },
    { field_to_sfstring, "SoSFNode", "SoSFString" },
    { field_to_sfstring, "SoSFPath", "SoSFString" },
    { field_to_sfstring, "SoSFPlane", "SoSFString" },
    { field_to_sfstring, "SoSFRotation", "SoSFString" },
    { field_to_sfstring, "SoSFShort", "SoSFString" },
    { field_to_sfstring, "SoSFUInt32", "SoSFString" },
    { field_to_sfstring, "SoSFUShort", "SoSFString" },
    { field_to_sfstring, "SoSFVec2s", "SoSFString" },
    { field_to_sfstring, "SoSFVec2f", "SoSFString" },
    { field_to_sfstring, "SoSFVec3s", "SoSFString" },
    { field_to_sfstring, "SoSFVec3f", "SoSFString" },
    { field_to_sfstring, "SoSFVec3d", "SoSFString" },
    { field_to_sfstring, "SoSFVec4f", "SoSFString" },
    { sfield_to_mfstring, "SoSFBitMask", "SoMFString" },
    { sfield_to_mfstring, "SoSFBool", "SoMFString" },
    { sfield_to_mfstring, "SoSFColor", "SoMFString" },
    { sfield_to_mfstring, "SoSFEnum", "SoMFString" },
    { sfield_to_mfstring, "SoSFFloat", "SoMFString" },
    { sfield_to_mfstring, "SoSFInt32", "SoMFString" },
    { sfield_to_mfstring, "SoSFMatrix", "SoMFString" },
    { sfield_to_mfstring, "SoSFName", "SoMFString" },
    { sfield_to_mfstring, "SoSFNode", "SoMFString" },
    { sfield_to_mfstring, "SoSFPath", "SoMFString" },
    { sfield_to_mfstring, "SoSFPlane", "SoMFString" },
    { sfield_to_mfstring, "SoSFRotation", "SoMFString" },
    { sfield_to_mfstring, "SoSFShort", "SoMFString" },
    { sfield_to_mfstring, "SoSFUInt32", "SoMFString" },
    { sfield_to_mfstring, "SoSFUShort", "SoMFString" },
    { sfield_to_mfstring, "SoSFVec2s", "SoMFString" },
    { sfield_to_mfstring, "SoSFVec2f", "SoMFString" },
    { sfield_to_mfstring, "SoSFVec3s", "SoMFString" },
    { sfield_to_mfstring, "SoSFVec3f", "SoMFString" },
    { sfield_to_mfstring, "SoSFVec3d", "SoMFString" },
    { sfield_to_mfstring, "SoSFVec4f", "SoMFString" },
    { mfield_to_mfstring, "SoMFBitMask", "SoMFString" },
    { mfield_to_mfstring, "SoMFBool", "SoMFString" },
    { mfield_to_mfstring, "SoMFColor", "SoMFString" },
    { mfield_to_mfstring, "SoMFEnum", "SoMFString" },
    { mfield_to_mfstring, "SoMFFloat", "SoMFString" },
    { mfield_to_mfstring, "SoMFInt32", "SoMFString" },
    { mfield_to_mfstring, "SoMFMatrix", "SoMFString" },
    { mfield_to_mfstring, "SoMFName", "SoMFString" },
    { mfield_to_mfstring, "SoMFNode", "SoMFString" },
    { mfield_to_mfstring, "SoMFPath", "SoMFString" },
    { mfield_to_mfstring, "SoMFPlane", "SoMFString" },
    { mfield_to_mfstring, "SoMFRotation", "SoMFString" },
    { mfield_to_mfstring, "SoMFShort", "SoMFString" },
    { mfield_to_mfstring, "SoMFUInt32", "SoMFString" },
    { mfield_to_mfstring, "SoMFUShort", "SoMFString" },
    { mfield_to_mfstring, "SoMFVec2f", "SoMFString" },
    { mfield_to_mfstring, "SoMFVec3f", "SoMFString" },
    { mfield_to_mfstring, "SoMFVec3d", "SoMFString" },
    { mfield_to_mfstring, "SoMFVec4f", "SoMFString" },
    { sfstring_to_field, "SoSFString", "SoSFBitMask" },
    { sfstring_to_field, "SoSFString", "SoSFBool" },
    { sfstring_to_field, "SoSFString", "SoSFColor" },
    { sfstring_to_field, "SoSFString", "SoSFEnum" },
    { sfstring_to_field, "SoSFString", "SoSFFloat" },
    { sfstring_to_field, "SoSFString", "SoSFInt32" },
    { sfstring_to_field, "SoSFString", "SoSFMatrix" },
    { sfstring_to_field, "SoSFString", "SoSFName" },
    { sfstring_to_field, "SoSFString", "SoSFNode" },
    { sfstring_to_field, "SoSFString", "SoSFPath" },
    { sfstring_to_field, "SoSFString", "SoSFPlane" },
    { sfstring_to_field, "SoSFString", "SoSFRotation" },
    { sfstring_to_field, "SoSFString", "SoSFShort" },
    { sfstring_to_field, "SoSFString", "SoSFTime" },
    { sfstring_to_field, "SoSFString", "SoSFUInt32" },
    { sfstring_to_field, "SoSFString", "SoSFUShort" },
    { sfstring_to_field, "SoSFString", "SoSFVec2s" },
    { sfstring_to_field, "SoSFString", "SoSFVec2f" },
    { sfstring_to_field, "SoSFString", "SoSFVec3s" },
    { sfstring_to_field, "SoSFString", "SoSFVec3f" },
    { sfstring_to_field, "SoSFString", "SoSFVec3d" },
    { sfstring_to_field, "SoSFString", "SoSFVec4f" },
    { sfstring_to_field, "SoSFString", "SoMFBitMask" },
    { sfstring_to_field, "SoSFString", "SoMFBool" },
    { sfstring_to_field, "SoSFString", "SoMFColor" },
    { sfstring_to_field, "SoSFString", "SoMFEnum" },
    { sfstring_to_field, "SoSFString", "SoMFFloat" },
    { sfstring_to_field, "SoSFString", "SoMFInt32" },
    { sfstring_to_field, "SoSFString", "SoMFMatrix" },
    { sfstring_to_field, "SoSFString", "SoMFName" },
    { sfstring_to_field, "SoSFString", "SoMFNode" },
    { sfstring_to_field, "SoSFString", "SoMFPath" },
    { sfstring_to_field, "SoSFString", "SoMFPlane" },
    { sfstring_to_field, "SoSFString", "SoMFRotation" },
    { sfstring_to_field, "SoSFString", "SoMFShort" },
    { sfstring_to_field, "SoSFString", "SoMFTime" },
    { sfstring_to_field, "SoSFString", "SoMFUInt32" },
    { sfstring_to_field, "SoSFString", "SoMFUShort" },
    { sfstring_to_field, "SoSFString", "SoMFVec2f" },
    { sfstring_to_field, "SoSFString", "SoMFVec3f" },
    { sfstring_to_field, "SoSFString", "SoMFVec3d" },
    { sfstring_to_field, "SoSFString", "SoMFVec4f" },
    { mfstring_to_sfield, "SoMFString", "SoSFBitMask" },
    { mfstring_to_sfield, "SoMFString", "SoSFBool" },
    { mfstring_to_sfield, "SoMFString", "SoSFColor" },
    { mfstring_to_sfield, "SoMFString", "SoSFEnum" },
    { mfstring_to_sfield, "SoMFString", "SoSFFloat" },
    { mfstring_to_sfield, "SoMFString", "SoSFInt32" },
    { mfstring_to_sfield, "SoMFString", "SoSFMatrix" },
    { mfstring_to_sfield, "SoMFString", "SoSFName" },
    { mfstring_to_sfield, "SoMFString", "SoSFNode" },
    { mfstring_to_sfield, "SoMFString", "SoSFPath" },
    { mfstring_to_sfield, "SoMFString", "SoSFPlane" },
    { mfstring_to_sfield, "SoMFString", "SoSFRotation" },
    { mfstring_to_sfield, "SoMFString", "SoSFShort" },
    { mfstring_to_sfield, "SoMFString", "SoSFTime" },
    { mfstring_to_sfield, "SoMFString", "SoSFUInt32" },
    { mfstring_to_sfield, "SoMFString", "SoSFUShort" },
    { mfstring_to_sfield, "SoMFString", "SoSFVec2s" },
    { mfstring_to_sfield, "SoMFString", "SoSFVec2f" },
    { mfstring_to_sfield, "SoMFString", "SoSFVec3s" },
    { mfstring_to_sfield, "SoMFString", "SoSFVec3f" },
    { mfstring_to_sfield, "SoMFString", "SoSFVec4f" },
    { mfstring_to_sfield, "SoMFString", "SoSFVec3d" },
    { mfstring_to_mfield, "SoMFString", "SoMFBitMask" },
    { mfstring_to_mfield, "SoMFString", "SoMFBool" },
    { mfstring_to_mfield, "SoMFString", "SoMFColor" },
    { mfstring_to_mfield, "SoMFString", "SoMFEnum" },
    { mfstring_to_mfield, "SoMFString", "SoMFFloat" },
    { mfstring_to_mfield, "SoMFString", "SoMFInt32" },
    { mfstring_to_mfield, "SoMFString", "SoMFMatrix" },
    { mfstring_to_mfield, "SoMFString", "SoMFName" },
    { mfstring_to_mfield, "SoMFString", "SoMFNode" },
    { mfstring_to_mfield, "SoMFString", "SoMFPath" },
    { mfstring_to_mfield, "SoMFString", "SoMFPlane" },
    { mfstring_to_mfield, "SoMFString", "SoMFRotation" },
    { mfstring_to_mfield, "SoMFString", "SoMFShort" },
    { mfstring_to_mfield, "SoMFString", "SoMFTime" },
    { mfstring_to_mfield, "SoMFString", "SoMFUInt32" },
    { mfstring_to_mfield, "SoMFString", "SoMFUShort" },
    { mfstring_to_mfield, "SoMFString", "SoMFVec2f" },
    { mfstring_to_mfield, "SoMFString", "SoMFVec3f" },
    { mfstring_to_mfield, "SoMFString", "SoMFVec3d" },
    { mfstring_to_mfield, "SoMFString", "SoMFVec4f" },
    { SoSFBool_SoSFFloat, "SoSFBool", "SoSFFloat" },
    { SoSFBool_SoMFFloat, "SoSFBool", "SoMFFloat" },
    { SoSFFloat_SoSFBool, "SoSFFloat", "SoSFBool" },
    { SoSFFloat_SoMFBool, "SoSFFloat", "SoMFBool" },
    { SoSFBool_SoSFInt32, "SoSFBool", "SoSFInt32" },
    { SoSFBool_SoMFInt32, "SoSFBool", "SoMFInt32" },
    { SoSFInt32_SoSFBool, "SoSFInt32", "SoSFBool" },
    { SoSFInt32_SoMFBool, "SoSFInt32", "SoMFBool" },
    { SoSFBool_SoSFShort, "SoSFBool", "SoSFShort" },
    { SoSFBool_SoMFShort, "SoSFBool", "SoMFShort" },
    { SoSFShort_SoSFBool, "SoSFShort", "SoSFBool" },
    { SoSFShort_SoMFBool, "SoSFShort", "SoMFBool" },
    { SoSFBool_SoSFUInt32, "SoSFBool", "SoSFUInt32" },
    { SoSFBool_SoMFUInt32, "SoSFBool", "SoMFUInt32" },
    { SoSFUInt32_SoSFBool, "SoSFUInt32", "SoSFBool" },
    { SoSFUInt32_SoMFBool, "SoSFUInt32", "SoMFBool" },
    { SoSFBool_SoSFUShort, "SoSFBool", "SoSFUShort" },
    { SoSFBool_SoMFUShort, "SoSFBool", "SoMFUShort" },
    { SoSFUShort_SoSFBool, "SoSFUShort", "SoSFBool" },
    { SoSFUShort_SoMFBool, "SoSFUShort", "SoMFBool" },
    { SoSFColor_SoSFVec3f, "SoSFColor", "SoSFVec3f" },
    { SoSFColor_SoMFVec3f, "SoSFColor", "SoMFVec3f" },
    { SoSFVec3f_SoSFColor, "SoSFVec3f", "SoSFColor" },
    { SoSFVec3f_SoMFColor, "SoSFVec3f", "SoMFColor" },
    { SoSFFloat_SoSFInt32, "SoSFFloat", "SoSFInt32" },
    { SoSFFloat_SoMFInt32, "SoSFFloat", "SoMFInt32" },
    { SoSFInt32_SoSFFloat, "SoSFInt32", "SoSFFloat" },
    { SoSFInt32_SoMFFloat, "SoSFInt32", "SoMFFloat" },
    { SoSFFloat_SoSFShort, "SoSFFloat", "SoSFShort" },
    { SoSFFloat_SoMFShort, "SoSFFloat", "SoMFShort" },
    { SoSFShort_SoSFFloat, "SoSFShort", "SoSFFloat" },
    { SoSFShort_SoMFFloat, "SoSFShort", "SoMFFloat" },
    { SoSFFloat_SoSFUInt32, "SoSFFloat", "SoSFUInt32" },
    { SoSFFloat_SoMFUInt32, "SoSFFloat", "SoMFUInt32" },
    { SoSFUInt32_SoSFFloat, "SoSFUInt32", "SoSFFloat" },
    { SoSFUInt32_SoMFFloat, "SoSFUInt32", "SoMFFloat" },
    { SoSFFloat_SoSFUShort, "SoSFFloat", "SoSFUShort" },
    { SoSFFloat_SoMFUShort, "SoSFFloat", "SoMFUShort" },
    { SoSFUShort_SoSFFloat, "SoSFUShort", "SoSFFloat" },
    { SoSFUShort_SoMFFloat, "SoSFUShort", "SoMFFloat" },
    { SoSFInt32_SoSFShort, "SoSFInt32", "SoSFShort" },
    { SoSFInt32_SoMFShort, "SoSFInt32", "SoMFShort" },
    { SoSFShort_SoSFInt32, "SoSFShort", "SoSFInt32" },
    { SoSFShort_SoMFInt32, "SoSFShort", "SoMFInt32" },
    { SoSFInt32_SoSFUInt32, "SoSFInt32", "SoSFUInt32" },
    { SoSFInt32_SoMFUInt32, "SoSFInt32", "SoMFUInt32" },
    { SoSFUInt32_SoSFInt32, "SoSFUInt32", "SoSFInt32" },
    { SoSFUInt32_SoMFInt32, "SoSFUInt32", "SoMFInt32" },
    { SoSFInt32_SoSFUShort, "SoSFInt32", "SoSFUShort" },
    { SoSFInt32_SoMFUShort, "SoSFInt32", "SoMFUShort" },
    { SoSFUShort_SoSFInt32, "SoSFUShort", "SoSFInt32" },
    { SoSFUShort_SoMFInt32, "SoSFUShort", "SoMFInt32" },
    { SoSFShort_SoSFUInt32, "SoSFShort", "SoSFUInt32" },
    { SoSFShort_SoMFUInt32, "SoSFShort", "SoMFUInt32" },
    { SoSFUInt32_SoSFShort, "SoSFUInt32", "SoSFShort" },
    { SoSFUInt32_SoMFShort, "SoSFUInt32", "SoMFShort" },
    { SoSFShort_SoSFUShort, "SoSFShort", "SoSFUShort" },
    { SoSFShort_SoMFUShort, "SoSFShort", "SoMFUShort" },
    { SoSFUShort_SoSFShort, "SoSFUShort", "SoSFShort" },
    { SoSFUShort_SoMFShort, "SoSFUShort", "SoMFShort" },
    { SoSFUInt32_SoSFUShort, "SoSFUInt32", "SoSFUShort" },
    { SoSFUInt32_SoMFUShort, "SoSFUInt32", "SoMFUShort" },
    { SoSFUShort_SoSFUInt32, "SoSFUShort", "SoSFUInt32" },
    { SoSFUShort_SoMFUInt32, "SoSFUShort", "SoMFUInt32" },
    { SoSFFloat_SoSFTime, "SoSFFloat", "SoSFTime" },
    { SoSFFloat_SoMFTime, "SoSFFloat", "SoMFTime" },
    { SoSFTime_SoSFFloat, "SoSFTime", "SoSFFloat" },
    { SoSFTime_SoMFFloat, "SoSFTime", "SoMFFloat" },
    { SoMFBool_SoSFFloat, "SoMFBool", "SoSFFloat" },
    { SoMFFloat_SoSFBool, "SoMFFloat", "SoSFBool" },
    { SoMFBool_SoSFInt32, "SoMFBool", "SoSFInt32" },
    { SoMFInt32_SoSFBool, "SoMFInt32", "SoSFBool" },
    { SoMFBool_SoSFShort, "SoMFBool", "SoSFShort" },
    { SoMFShort_SoSFBool, "SoMFShort", "SoSFBool" },
    { SoMFBool_SoSFUInt32, "SoMFBool", "SoSFUInt32" },
    { SoMFUInt32_SoSFBool, "SoMFUInt32", "SoSFBool" },
    { SoMFBool_SoSFUShort, "SoMFBool", "SoSFUShort" },
    { SoMFUShort_SoSFBool, "SoMFUShort", "SoSFBool" },
    { SoMFColor_SoSFVec3f, "SoMFColor", "SoSFVec3f" },
    { SoMFVec3f_SoSFColor, "SoMFVec3f", "SoSFColor" },
    { SoMFFloat_SoSFInt32, "SoMFFloat", "SoSFInt32" },
    { SoMFInt32_SoSFFloat, "SoMFInt32", "SoSFFloat" },
    { SoMFFloat_SoSFShort, "SoMFFloat", "SoSFShort" },
    { SoMFShort_SoSFFloat, "SoMFShort", "SoSFFloat" },
    { SoMFFloat_SoSFUInt32, "SoMFFloat", "SoSFUInt32" },
    { SoMFUInt32_SoSFFloat, "SoMFUInt32", "SoSFFloat" },
    { SoMFFloat_SoSFUShort, "SoMFFloat", "SoSFUShort" },
    { SoMFUShort_SoSFFloat, "SoMFUShort", "SoSFFloat" },
    { SoMFInt32_SoSFShort, "SoMFInt32", "SoSFShort" },
    { SoMFShort_SoSFInt32, "SoMFShort", "SoSFInt32" },
    { SoMFInt32_SoSFUInt32, "SoMFInt32", "SoSFUInt32" },
    { SoMFUInt32_SoSFInt32, "SoMFUInt32", "SoSFInt32" },
    { SoMFInt32_SoSFUShort, "SoMFInt32", "SoSFUShort" },
    { SoMFUShort_SoSFInt32, "SoMFUShort", "SoSFInt32" },
    { SoMFShort_SoSFUInt32, "SoMFShort", "SoSFUInt32" },
    { SoMFUInt32_SoSFShort, "SoMFUInt32", "SoSFShort" },
    { SoMFShort_SoSFUShort, "SoMFShort", "SoSFUShort" },
    { SoMFUShort_SoSFShort, "SoMFUShort", "SoSFShort" },
    { SoMFUInt32_SoSFUShort, "SoMFUInt32", "SoSFUShort" },
    { SoMFUShort_SoSFUInt32, "SoMFUShort", "SoSFUInt32" },
    { SoMFFloat_SoSFTime, "SoMFFloat", "SoSFTime" },
    { SoMFTime_SoSFFloat, "SoMFTime", "SoSFFloat" },
    { SoMFBool_SoMFFloat, "SoMFBool", "SoMFFloat" },
    { SoMFFloat_SoMFBool, "SoMFFloat", "SoMFBool" },
    { SoMFBool_SoMFInt32, "SoMFBool", "SoMFInt32" },
    { SoMFInt32_SoMFBool, "SoMFInt32", "SoMFBool" },
    { SoMFBool_SoMFShort, "SoMFBool", "SoMFShort" },
    { SoMFShort_SoMFBool, "SoMFShort", "SoMFBool" },
    { SoMFBool_SoMFUInt32, "SoMFBool", "SoMFUInt32" },
    { SoMFUInt32_SoMFBool, "SoMFUInt32", "SoMFBool" },
    { SoMFBool_SoMFUShort, "SoMFBool", "SoMFUShort" },
    { SoMFUShort_SoMFBool, "SoMFUShort", "SoMFBool" },
    { SoMFColor_SoMFVec3f, "SoMFColor", "SoMFVec3f" },
    { SoMFVec3f_SoMFColor, "SoMFVec3f", "SoMFColor" },
    { SoMFFloat_SoMFInt32, "SoMFFloat", "SoMFInt32" },
    { SoMFInt32_SoMFFloat, "SoMFInt32", "SoMFFloat" },
    { SoMFFloat_SoMFShort, "SoMFFloat", "SoMFShort" },
    { SoMFShort_SoMFFloat, "SoMFShort", "SoMFFloat" },
    { SoMFFloat_SoMFUInt32, "SoMFFloat", "SoMFUInt32" },
    { SoMFUInt32_SoMFFloat, "SoMFUInt32", "SoMFFloat" },
    { SoMFFloat_SoMFUShort, "SoMFFloat", "SoMFUShort" },
    { SoMFUShort_SoMFFloat, "SoMFUShort", "SoMFFloat" },
    { SoMFInt32_SoMFShort, "SoMFInt32", "SoMFShort" },
    { SoMFShort_SoMFInt32, "SoMFShort", "SoMFInt32" },
    { SoMFInt32_SoMFUInt32, "SoMFInt32", "SoMFUInt32" },
    { SoMFUInt32_SoMFInt32, "SoMFUInt32", "SoMFInt32" },
    { SoMFInt32_SoMFUShort, "SoMFInt32", "SoMFUShort" },
    { SoMFUShort_SoMFInt32, "SoMFUShort", "SoMFInt32" },
    { SoMFShort_SoMFUInt32, "SoMFShort", "SoMFUInt32" },
    { SoMFUInt32_SoMFShort, "SoMFUInt32", "SoMFShort" },
    { SoMFShort_SoMFUShort, "SoMFShort", "SoMFUShort" },
    { SoMFUShort_SoMFShort, "SoMFUShort", "SoMFShort" },
    { SoMFUInt32_SoMFUShort, "SoMFUInt32", "SoMFUShort" },
    { SoMFUShort_SoMFUInt32, "SoMFUShort", "SoMFUInt32" },
    { SoMFFloat_SoMFTime, "SoMFFloat", "SoMFTime" },
    { SoMFTime_SoMFFloat, "SoMFTime", "SoMFFloat" },
    { SoSFMatrix_SoSFRotation, "SoSFMatrix", "SoSFRotation" },
    { SoMFMatrix_SoSFRotation, "SoMFMatrix", "SoSFRotation" },
    { SoSFMatrix_SoMFRotation, "SoSFMatrix", "SoMFRotation" },
    { SoMFMatrix_SoMFRotation, "SoMFMatrix", "SoMFRotation" },
    { SoSFRotation_SoSFMatrix, "SoSFRotation", "SoSFMatrix" },
    { SoMFRotation_SoSFMatrix, "SoMFRotation", "SoSFMatrix" },
    { SoSFRotation_SoMFMatrix, "SoSFRotation", "SoMFMatrix" },
    { SoMFRotation_SoMFMatrix, "SoMFRotation", "SoMFMatrix" },
    { sftime_to_sfstring, "SoSFTime", "SoSFString" },
    { sftime_to_mfstring, "SoSFTime", "SoMFString" },
    { mftime_to_sfstring, "SoMFTime", "SoSFString" },
    { mftime_to_mfstring, "SoMFTime", "SoMFString" },
    { SoSFVec2s_to_SoSFVec2f, "SoSFVec2s", "SoSFVec2f" },
    { SoSFVec2s_to_SoMFVec2f, "SoSFVec2s", "SoMFVec2f" },
    { SoSFVec2f_to_SoSFVec2s, "SoSFVec2f", "SoSFVec2s" },
    { SoMFVec2f_to_SoSFVec2s, "SoMFVec2f", "SoSFVec2s" },
    { SoSFVec3s_to_SoSFVec3f, "SoSFVec3s", "SoSFVec3f" },
    { SoSFVec3s_to_SoMFVec3f, "SoSFVec3s", "SoMFVec3f" },
    { SoSFVec3s_to_SoSFVec3d, "SoSFVec3s", "SoSFVec3d" },
    { SoSFVec3s_to_SoMFVec3d, "SoSFVec3s", "SoMFVec3d" },
    { SoSFVec3f_to_SoSFVec3s, "SoSFVec3f", "SoSFVec3s" },
    { SoMFVec3f_to_SoSFVec3s, "SoMFVec3f", "SoSFVec3s" },
    { SoSFVec3f_to_SoSFVec3d, "SoSFVec3f", "SoSFVec3d" },
    { SoSFVec3f_to_SoMFVec3d, "SoSFVec3f", "SoMFVec3d" },
    { SoMFVec3f_to_SoSFVec3d, "SoMFVec3f", "SoSFVec3d" },
    { SoMFVec3f_to_SoMFVec3d, "SoMFVec3f", "SoMFVec3d" },
    { SoSFVec3d_to_SoSFVec3s, "SoSFVec3d", "SoSFVec3s" },
    { SoMFVec3d_to_SoSFVec3s, "SoMFVec3d", "SoSFVec3s" },
    { SoSFVec3d_to_SoSFVec3f, "SoSFVec3d", "SoSFVec3f" },
    { SoSFVec3d_to_SoMFVec3f, "SoSFVec3d", "SoMFVec3f" },
    { SoMFVec3d_to_SoSFVec3f, "SoMFVec3d", "SoSFVec3f" },
    { SoMFVec3d_to_SoMFVec3f, "SoMFVec3d", "SoMFVec3f" }
  };

  int i;
  for (i = 0; i < int(sizeof(allconverters) / sizeof(allconverters[0])); i++) {
    register_convertfunc(allconverters[i].func,
                         SoType::fromName(allconverters[i].from),
                         SoType::fromName(allconverters[i].to));
    // Performance note: it may look slow to do all that
    // SoType::fromName()'ing, but the full loop only takes about 2
    // milliseconds on an 850MHz x86 CPU, so there would be little use
    // in trying to optimize this.
  }

  // Now add conversion to and from SoSFTrigger for all other
  // non-abstract field types (all conversions done by the same
  // function).

  SoTypeList allfieldtypes;
  int nrfieldtypes = SoType::getAllDerivedFrom(SoField::getClassTypeId(),
                                               allfieldtypes);
  for (i=0; i < nrfieldtypes; i++) {
    if (allfieldtypes[i].canCreateInstance() &&
        allfieldtypes[i] != SoSFTrigger::getClassTypeId()) {
      register_convertfunc(to_and_from_sftrigger,
                           SoSFTrigger::getClassTypeId(),
                           allfieldtypes[i]);
      register_convertfunc(to_and_from_sftrigger,
                           allfieldtypes[i],
                           SoSFTrigger::getClassTypeId());
    }
  }
}

SoConvertAll::SoConvertAll(const SoType from, const SoType to)
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoConvertAll::SoConvertAll",
                         "from: %s, to: %s",
                         from.getName().getString(),
                         to.getName().getString());
#endif // debug

  // This code is instead of SO_ENGINE_CONSTRUCTOR(), which we can't
  // use due to the fact that we need dynamic lists of input fields
  // and engine outputs.
  { // SO_ENGINE_CONSTRUCTOR replacement start
    SoConvertAll::classinstances++;
    /* Catch attempts to use an engine class which has not been initialized. */
    assert(SoConvertAll::classTypeId != SoType::badType());

    this->inputdata_instance =
      new SoFieldData(SoConvertAll::parentinputdata ?
                      *SoConvertAll::parentinputdata : NULL);

    this->outputdata_instance =
      new SoEngineOutputData(SoConvertAll::parentoutputdata ?
                             *SoConvertAll::parentoutputdata : NULL);

    /* SoConvertAll is not considered native (doesn't really matter
       one way or the other). */
    this->isBuiltIn = FALSE;
  } // SO_ENGINE_CONSTRUCTOR replacement end



  this->input = static_cast<SoField *>(from.createInstance());

  this->input->setContainer(this);
  this->output.setContainer(this);
  this->outputdata_instance->addOutput(this, "output", &this->output, to);

  uint32_t val = (static_cast<uint32_t>(from.getKey()) << 16) + to.getKey();
  convert_func * ptr = NULL;
  if (!convertfunc_dict->get(val, ptr)) { assert(FALSE); }
  this->convertvalue = ptr;
}

SoConvertAll::~SoConvertAll()
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoConvertAll::~SoConvertAll", "%p", this);
#endif // debug
  delete this->input;

  delete this->inputdata_instance;
  delete this->outputdata_instance;
}

SoField *
SoConvertAll::getInput(SoType type)
{
#if COIN_DEBUG
  SoType inputtype = this->input->getTypeId();
  if (type != inputtype && type != SoType::badType()) {
    SoDebugError::postWarning("SoConvertAll::getInput",
                              "field is of type %s, not %s",
                              inputtype.getName().getString(),
                              type.getName().getString());
  }
#endif // COIN_DEBUG

  // ignore type, as we have only a single input field

  return this->input;
}

SoEngineOutput *
SoConvertAll::getOutput(SoType type)
{
#if COIN_DEBUG
  SoType outputtype = this->output.getConnectionType();
  if (type != outputtype && type != SoType::badType()) {
    SoDebugError::postWarning("SoConvertAll::getOutput",
                              "engineout is of type %s, not %s",
                              outputtype.getName().getString(),
                              type.getName().getString());
  }
#endif // COIN_DEBUG

  // ignore type, as we have only a single engineoutput

  return &this->output;
}

void
SoConvertAll::evaluate(void)
{
  // we cannot use the SO_ENGINE_OUTPUT macro, but this code should
  // do the same thing.
  if (this->output.isEnabled()) {
    for (int i = 0 ; i < this->output.getNumConnections(); i++) {
      SoField * f = this->output[i];
      if (!f->isReadOnly()) {
        // Convert directly from the "real" master field if possible,
        // to behave properly on enum fields (the this->input instance
        // doesn't contain the name<->value mappings in the case that
        // the master field is of type So[SM]FEnum or So[SM]FBitMask).
        SoField * masterfield = NULL;
        if (this->input->getConnectedField(masterfield))
          this->convertvalue(masterfield, f);
        // Couldn't get master field, this means we are connected to
        // an engine output (at least we _should_ be, could probably
        // do with an assert here).
        else
          this->convertvalue(this->input, f);
      }
    }
  }
}

#undef SOCONVERTALL_CAST_SFIELD2FIELD
#undef SOCONVERTALL_CAST_MFIELD2SFIELD
#undef SOCONVERTALL_CAST_MFIELD2MFIELD
#undef SOCONVERTALL_CAST_SFTIME2SFFLOAT
#undef SOCONVERTALL_SFMATRIX2ROTATION
#undef SOCONVERTALL_SFROTATION2MATRIX
