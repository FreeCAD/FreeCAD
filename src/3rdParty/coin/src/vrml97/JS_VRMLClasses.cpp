#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#if defined(HAVE_VRML97) && defined(COIN_HAVE_JAVASCRIPT)

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

#include "JS_VRMLClasses.h"

#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/misc/SoJavaScriptEngine.h>
#include <Inventor/C/glue/spidermonkey.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoSFTime.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoSFImage.h>
#include <Inventor/fields/SoSFVec2f.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFVec3d.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/fields/SoMFString.h>
#include <Inventor/fields/SoMFTime.h>
#include <Inventor/fields/SoMFColor.h>
#include <Inventor/fields/SoMFNode.h>
#include <Inventor/fields/SoMFVec2f.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoMFVec3d.h>
#include <Inventor/fields/SoMFRotation.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/VRMLnodes/SoVRMLGroup.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/sensors/SoNodeSensor.h>

#include "misc/SbHash.h"
#include "tidbitsp.h"

// FIXME: toString() missing for all classes

// "namespace" for all vrml classes
struct CoinVrmlJs {
  struct ClassDescriptor {
    JSClass cls;
    JSFunctionSpec * functions;

  };

  static ClassDescriptor SFColor;
  static ClassDescriptor SFNode;
  static ClassDescriptor SFRotation;
  static ClassDescriptor SFVec2f;
  static ClassDescriptor SFVec3f;
  static ClassDescriptor SFVec3d;

  static ClassDescriptor MFColor;
  static ClassDescriptor MFFloat;
  static ClassDescriptor MFInt32;
  static ClassDescriptor MFNode;
  static ClassDescriptor MFRotation;
  static ClassDescriptor MFString;
  static ClassDescriptor MFTime;
  static ClassDescriptor MFVec2f;
  static ClassDescriptor MFVec3f;
  static ClassDescriptor MFVec3d;
};

// Struct and SbHash for keeping track of SoNodeSensors for recycling
// purposes.
struct CoinVrmlJs_SensorInfo {
  SbList <JSObject *> objects;
};
SbHash<uintptr_t, void *> * CoinVrmlJs_sensorinfohash = NULL;


const char * CoinVrmlJs_SFColorAliases[] = {"r", "g", "b"};
const char * CoinVrmlJs_SFRotationAliases[] = {"x", "y", "z", "angle"};
float CoinVrmlJs_SFdefaultValues[] = {0.0, 0.0, 0.0, 0.0};
double CoinVrmlJs_SFdefaultValuesDouble[] = {0.0, 0.0, 0.0, 0.0};
float CoinVrmlJs_SFRotationDefaultValues[] = {0.0, 1.0, 0.0, 0.0};

// Macros for instance checking
#define JSVAL_IS_SFVEC2F(cx, jsval) (JSVAL_IS_OBJECT(jsval) && spidermonkey()->JS_InstanceOf(cx, JSVAL_TO_OBJECT(jsval), &CoinVrmlJs::SFVec2f.cls, NULL))
#define JSVAL_IS_SFVEC3F(cx, jsval) (JSVAL_IS_OBJECT(jsval) && spidermonkey()->JS_InstanceOf(cx, JSVAL_TO_OBJECT(jsval), &CoinVrmlJs::SFVec3f.cls, NULL))
#define JSVAL_IS_SFVEC3D(cx, jsval) (JSVAL_IS_OBJECT(jsval) && spidermonkey()->JS_InstanceOf(cx, JSVAL_TO_OBJECT(jsval), &CoinVrmlJs::SFVec3d.cls, NULL))
#define JSVAL_IS_SFCOLOR(cx, jsval) (JSVAL_IS_OBJECT(jsval) && spidermonkey()->JS_InstanceOf(cx, JSVAL_TO_OBJECT(jsval), &CoinVrmlJs::SFColor.cls, NULL))
#define JSVAL_IS_SFROTATION(cx, jsval) (JSVAL_IS_OBJECT(jsval) && spidermonkey()->JS_InstanceOf(cx, JSVAL_TO_OBJECT(jsval), &CoinVrmlJs::SFRotation.cls, NULL))

// Handlers
#define SFColorHandler CoinVrmlJsSFHandler<SbColor, 3, CoinVrmlJs_SFColorAliases, float, CoinVrmlJs_SFdefaultValues>
#define SFRotationHandler CoinVrmlJsSFHandler<SbVec4f, 4, CoinVrmlJs_SFRotationAliases, float, CoinVrmlJs_SFRotationDefaultValues>
#define SFVec2fHandler CoinVrmlJsSFHandler<SbVec2f, 2, CoinVrmlJs_SFRotationAliases, float, CoinVrmlJs_SFdefaultValues>
#define SFVec3fHandler CoinVrmlJsSFHandler<SbVec3f, 3, CoinVrmlJs_SFRotationAliases, float, CoinVrmlJs_SFdefaultValues>
#define SFVec3dHandler CoinVrmlJsSFHandler<SbVec3d, 3, CoinVrmlJs_SFRotationAliases, double, CoinVrmlJs_SFdefaultValuesDouble>

#define MFColorHandler CoinVrmlJsMFHandler<SoMFColor, SoSFColor, &CoinVrmlJs::MFColor>
#define MFFloatHandler CoinVrmlJsMFHandler<SoMFFloat, SoSFFloat, &CoinVrmlJs::MFFloat>
#define MFInt32Handler CoinVrmlJsMFHandler<SoMFInt32, SoSFInt32, &CoinVrmlJs::MFInt32>
#define MFNodeHandler CoinVrmlJsMFHandler<SoMFNode, SoSFNode, &CoinVrmlJs::MFNode>
#define MFRotationHandler CoinVrmlJsMFHandler<SoMFRotation, SoSFRotation, &CoinVrmlJs::MFRotation>
#define MFStringHandler CoinVrmlJsMFHandler<SoMFString, SoSFString, &CoinVrmlJs::MFString>
#define MFTimeHandler CoinVrmlJsMFHandler<SoMFTime, SoSFTime, &CoinVrmlJs::MFTime>
#define MFVec2fHandler CoinVrmlJsMFHandler<SoMFVec2f, SoSFVec2f, &CoinVrmlJs::MFVec2f>
#define MFVec3fHandler CoinVrmlJsMFHandler<SoMFVec3f, SoSFVec3f, &CoinVrmlJs::MFVec3f>
#define MFVec3dHandler CoinVrmlJsMFHandler<SoMFVec3d, SoSFVec3d, &CoinVrmlJs::MFVec3d>

static JSFunctionSpec MFFunctions[] = {
//  {"toString", MF_toString, 0, 0, 0},
  {NULL, NULL, 0, 0, 0}
};

static JSBool SFRotationConstructor(JSContext * cx, JSObject * obj,
                                    uintN argc, jsval * argv, jsval * rval);

// Factory methods for converting to javascript objects
static JSObject * SFColorFactory(JSContext * cx, const SbColor & self);
static JSObject * SFNodeFactory(JSContext * cx, SoNode * container);
static JSObject * SFRotationFactory(JSContext * cx, const SbRotation & self);
static JSObject * SFVec2fFactory(JSContext * cx, const SbVec2f & self);
static JSObject * SFVec3fFactory(JSContext * cx, const SbVec3f & self);
static JSObject * SFVec3dFactory(JSContext * cx, const SbVec3d & self);

static SbList <JSObject *> * garbagecollectedobjects = NULL;
static SbList <SoNodeSensor *> * nodesensorstobedeleted = NULL;

// getIndex returns -1 if id is not an alias or in range 0-max
static JSBool getIndex(JSContext * cx, jsval id, const char * aliases[], int max)
{
  int index;

  if (JSVAL_IS_INT(id)) {
    index = JSVAL_TO_INT(id);
    if (index < 0 || index >= max) {
      spidermonkey()->JS_ReportError(cx, "index must be between 0 and %d", max);
      return -1;
    }
    return index;
  }
  else {
    JSString * jsstr = spidermonkey()->JS_ValueToString(cx, id);
    const char * str = spidermonkey()->JS_GetStringBytes(jsstr);

    for (index=0; index<max; ++index) {
      if (strcmp(str, aliases[index]) == 0) {
        return index;
      }
    }
    return -1;
  }
}

// *************************************************************************
// handlers
bool jsval2int(JSContext *cx, const jsval v, int32_t &value)
{
  if (JSVAL_IS_NULL(v)) return false;
  int32_t tempval;
  if (!spidermonkey()->JS_ValueToECMAInt32(cx, v, &tempval)) {
    return false;
  }
  value = tempval;
  return true;
}

bool jsval2double(JSContext *cx, const jsval v, double &value)
{
  if (JSVAL_IS_NULL(v)) return false;
  double tempval;
  if (!spidermonkey()->JS_ValueToNumber(cx, v, &tempval)) {
    return false;
  }
  value = tempval;
  return true;
}


// FIXME: number of aliases must not be lower than max. This may lead to
// unsafe programming. 20050721 erikgors.
template <class Base, int max, const char * aliases[], class basetype, basetype defaultValues[]>
struct CoinVrmlJsSFHandler {
  static JSBool get(JSContext * cx, JSObject * obj, jsval id, jsval * rval)
  {
    int index = getIndex(cx, id, aliases, max);
    if (index == -1) {
      return JS_TRUE;
    }

    Base * data = (Base *)spidermonkey()->JS_GetPrivate(cx, obj);
    assert(data != NULL);
    basetype var = (*data)[index];
    SbBool ok = spidermonkey()->JS_NewDoubleValue(cx, (double)var, rval);
    assert(ok && "JS_NewDoubleValue failed");
    return JS_TRUE;
  }

  static JSBool set(JSContext * cx, JSObject * obj, jsval id, jsval * val)
  {
    int index = getIndex(cx, id, aliases, max);
    if (index == -1) {
      return JS_FALSE;
    }

    Base * data = (Base *)spidermonkey()->JS_GetPrivate(cx, obj);
    assert(data != NULL);

    // FIXME: number may be NaN, PositiveInfinity and NegativeInfinity.
    // Should be checked for every time we run JS_ValueToNumber.
    // ie: "blipp" will become NaN. 20050720 erikgors.
    double number;
    spidermonkey()->JS_ValueToNumber(cx, *val, &number);
    (*data)[index] = (basetype)number;
    return JS_TRUE;
  }

  static JSBool constructor(JSContext * cx, JSObject * obj,
                            uintN argc, jsval * argv, jsval * rval)
  {
    basetype vals[max];

    // convert all arguments to numbers or use defaultValues if missing
    uint32_t i;
    for (i=0; i<max; ++i) {
      vals[i] = defaultValues[i];
      if (i<argc) {
        double val;
        if (spidermonkey()->JS_ValueToNumber(cx, argv[i], &val)) {
          vals[i] = (basetype)val;
        }
        else {
          spidermonkey()->JS_ReportError(cx, "WARNING: failed converting argument %d "
                                             "to a double", i + 1);
        }
      }
    }

    Base * data = new Base(vals);
    spidermonkey()->JS_SetPrivate(cx, obj, data);
    *rval = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
  }
  static void destructor(JSContext * cx, JSObject * obj)
  {
    Base * data = (Base *)spidermonkey()->JS_GetPrivate(cx, obj);
    // FIXME: We cannot assume this since the class object itself is an
    // instance of this JSClass. kintel 20050804.
    //    assert(data != NULL);
    delete data;
  }
};

template <class MFFieldClass, class SFFieldClass, CoinVrmlJs::ClassDescriptor * desc>
struct CoinVrmlJsMFHandler {
  static JSBool constructor(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * COIN_UNUSED_ARG(rval))
  {
    jsval * val = new jsval;
    JSObject * array = spidermonkey()->JS_NewArrayObject(cx, 0, NULL);
    *val = OBJECT_TO_JSVAL(array);
    SbBool ok = spidermonkey()->JS_AddRoot(cx, val);
    assert(ok && "JS_AddRoot failed");
    spidermonkey()->JS_SetPrivate(cx, obj, val);

    SFFieldClass * field = (SFFieldClass *)SFFieldClass::createInstance();
    uintN i;

    for (i=0; i<argc; ++i) {
      if (SoJavaScriptEngine::getEngine(cx)->jsval2field(argv[i], field)) {
        SbBool ok = spidermonkey()->JS_SetElement(cx, array, i, &argv[i]);
        assert(ok && "JS_SetElement failed");
      }
      else {
        // FIXME: should we insert a default value? 20050727 erikgors.
        spidermonkey()->JS_ReportError(cx, "argv %d is of wrong type", i);
      }
    }
    delete field;
    return JS_TRUE;
  }

  static void destructor(JSContext * cx, JSObject * obj)
  {
    jsval * val = (jsval *)spidermonkey()->JS_GetPrivate(cx, obj);
    if (val != NULL) {
      SbBool ok = spidermonkey()->JS_RemoveRoot(cx, val);
      assert(ok && "JS_RemoveRoot failed");
      delete val;
    }
  }

  static JSObject * init(JSContext * cx, JSObject * obj)
  {
    return spidermonkey()->JS_InitClass(cx, obj, NULL, &desc->cls,
                                        constructor, 0,
                                        NULL, MFFunctions, NULL, NULL);
  }

  static void resize(JSContext * cx, JSObject * array, uint32_t newLength)
  {
    uint32_t length;
    SbBool ok = spidermonkey()->JS_GetArrayLength(cx, array, &length);
    assert(ok && "JS_GetArrayLength failed");

    if (length > newLength) {
      spidermonkey()->JS_SetArrayLength(cx, array, newLength);
    }
    else {
      SoType type = MFFieldClass::getClassTypeId();

      // expand and fill with new objects
      for (; length<newLength; ++length) {
        jsval val;

        if (type == SoMFInt32::getClassTypeId() ||
            type == SoMFFloat::getClassTypeId()) {
          val = INT_TO_JSVAL(0);
        }
        else if (type == SoMFString::getClassTypeId()) {
          JSString * str = spidermonkey()->JS_NewStringCopyZ(cx, "");
          val = STRING_TO_JSVAL(str);
        }
        else if (type == SoMFNode::getClassTypeId()) {
          // All elements not explicitly initialized are set to NULL
          val = JSVAL_VOID;
        }
        else if (type == SoMFColor::getClassTypeId()) {
          JSObject * newObj =
            spidermonkey()->JS_NewObject(cx, &CoinVrmlJs::SFColor.cls, NULL, NULL);
          assert(newObj != NULL);
          SFColorHandler::constructor(cx, newObj, 0, NULL, &val);
          val = OBJECT_TO_JSVAL(newObj);
        }
        else if (type == SoMFRotation::getClassTypeId()) {
          JSObject * newObj =
            spidermonkey()->JS_NewObject(cx, &CoinVrmlJs::SFRotation.cls, NULL, NULL);
          assert(newObj != NULL);
          SFRotationConstructor(cx, newObj, 0, NULL, &val);
          val = OBJECT_TO_JSVAL(newObj);
        }
        else if (type == SoMFVec2f::getClassTypeId()) {
          JSObject * newObj =
            spidermonkey()->JS_NewObject(cx, &CoinVrmlJs::SFVec2f.cls, NULL, NULL);
          assert(newObj != NULL);
          SFVec2fHandler::constructor(cx, newObj, 0, NULL, &val);
          val = OBJECT_TO_JSVAL(newObj);
        }
        else if (type == SoMFVec3f::getClassTypeId()) {
          JSObject * newObj =
            spidermonkey()->JS_NewObject(cx, &CoinVrmlJs::SFVec3f.cls, NULL, NULL);
          assert(newObj != NULL);
          SFVec3fHandler::constructor(cx, newObj, 0, NULL, &val);
          val = OBJECT_TO_JSVAL(newObj);
        }
        else if (type == SoMFVec3d::getClassTypeId()) {
          JSObject * newObj =
            spidermonkey()->JS_NewObject(cx, &CoinVrmlJs::SFVec3d.cls, NULL, NULL);
          assert(newObj != NULL);
          SFVec3dHandler::constructor(cx, newObj, 0, NULL, &val);
          val = OBJECT_TO_JSVAL(newObj);
        }
        else {
          assert(0 && "this should not happen");
        }
        SbBool ok = spidermonkey()->JS_SetElement(cx, array, length, &val);
        assert(ok && "JS_SetElement failed");
      }
    }
  }

  static JSBool get(JSContext * cx, JSObject * obj, jsval id, jsval * rval)
  {

    jsval * array = (jsval *)spidermonkey()->JS_GetPrivate(cx, obj);

    if (JSVAL_IS_INT(id)) {
      assert(array != NULL);
      int index = JSVAL_TO_INT(id);
      return spidermonkey()->JS_GetElement(cx, JSVAL_TO_OBJECT(*array), index, rval);
    }
    else if (JSVAL_IS_STRING(id)) {
      const char * str = spidermonkey()->JS_GetStringBytes(JSVAL_TO_STRING(id));
      if (SbName("length") == str) {
        assert(array != NULL);
        uint32_t length;
        SbBool ok = spidermonkey()->JS_GetArrayLength(cx, JSVAL_TO_OBJECT(*array), &length);
        assert(ok && "JS_GetArrayLength failed");
        *rval = INT_TO_JSVAL(length);
        return JS_TRUE;
      }
    }

    return JS_TRUE;
  }

  static JSBool set(JSContext * cx, JSObject * obj, jsval id, jsval * val)
  {
    jsval * array = (jsval *)spidermonkey()->JS_GetPrivate(cx, obj);

    if (JSVAL_IS_INT(id)) {
      int index = JSVAL_TO_INT(id);

      // check for bounds
      if (index < 0) {
        return JS_FALSE;
      }

      // resize if necessary
      uint32_t length;
      SbBool ok = spidermonkey()->JS_GetArrayLength(cx, JSVAL_TO_OBJECT(*array), &length);
      assert(ok && "JS_GetArrayLength failed");
      if (index >= (int)length) {
        resize(cx, JSVAL_TO_OBJECT(*array), index+1);
      }

      SFFieldClass * field = (SFFieldClass *)SFFieldClass::createInstance();
      // Check if val is not of wrong type
      if (SoJavaScriptEngine::getEngine(cx)->jsval2field(*val, field)) {
        // assign it
        SbBool ok = spidermonkey()->JS_SetElement(cx, JSVAL_TO_OBJECT(*array), index, val);
        assert(ok && "JS_SetElement failed");
        return JS_TRUE;
      }
      delete field;
    }
    else if (JSVAL_IS_STRING(id)) {
      const char * str = spidermonkey()->JS_GetStringBytes(JSVAL_TO_STRING(id));
      if (SbName("length") == str) {
        double number;
        spidermonkey()->JS_ValueToNumber(cx, *val, &number);
        if (number < 0) {
          spidermonkey()->JS_ReportError(cx, "RangeError: invalid array length");
        }
        else {
          resize(cx, JSVAL_TO_OBJECT(*array), (uint32_t)number);
        }
        return JS_TRUE;
      }
    }

    return JS_FALSE;
  }


  static SbBool jsval2field(JSContext * cx, const jsval v, SoField * f)
  {
    if (JSVAL_IS_OBJECT(v) &&
        spidermonkey()->JS_InstanceOf(cx, JSVAL_TO_OBJECT(v), &desc->cls, NULL)) {
      JSObject * obj = JSVAL_TO_OBJECT(v);
      jsval * array = (jsval *)spidermonkey()->JS_GetPrivate(cx, obj);
      assert(array != NULL);

      jsval element;
      uint32_t i;
      uint32_t num;
      JSBool ok = spidermonkey()->JS_GetArrayLength(cx, JSVAL_TO_OBJECT(*array), &num);

      SFFieldClass * field = (SFFieldClass *)SFFieldClass::createInstance();

      for (i=0; i<num; ++i) {
        ok = spidermonkey()->JS_GetElement(cx, obj, i, &element);
        assert(ok);

        ok = SoJavaScriptEngine::getEngine(cx)->jsval2field(element, field);
        assert(ok && "jsval2field failed");
        ((MFFieldClass *)f)->set1Value(i, field->getValue());
      }
      delete field;
      return TRUE;
    }
    return FALSE;
  }

  static void field2jsval(JSContext * cx, const SoField * f, jsval * v)
  {
    JSObject * obj = spidermonkey()->JS_NewObject(cx, &desc->cls, NULL, NULL);
    spidermonkey()->JS_DefineFunctions(cx, obj, desc->functions);

    int num = ((SoMField *)f)->getNum();
    jsval * vals = new jsval[num];

    MFFieldClass & mf = *(MFFieldClass *)f;

    SFFieldClass * field = (SFFieldClass *)SFFieldClass::createInstance();
    for (int i=0; i<num; ++i) {
      field->setValue(mf[i]);
      SbBool ok = SoJavaScriptEngine::getEngine(cx)->field2jsval(field, &vals[i]);
      assert(ok && "field2jsval failed");
    }

    jsval rval;
    constructor(cx, obj, num, vals, &rval);
    *v = OBJECT_TO_JSVAL(obj);
    delete field;
    delete [] vals;
  }
};

// *************************************************************************
// constructors

static JSBool SFRotationConstructor(JSContext * cx, JSObject * obj,
                                    uintN argc, jsval * argv, jsval * rval)
{
  if (argc == 2) {
    if (JSVAL_IS_SFVEC3F(cx, argv[0])) {
      SbVec3f & vec =
        *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));

      SbVec4f * data = new SbVec4f();
      spidermonkey()->JS_SetPrivate(cx, obj, data);
      *rval = OBJECT_TO_JSVAL(obj);
      // new SFRotation(SFVec3f fromVector, SFVec3f toVector)
      if (JSVAL_IS_SFVEC3F(cx, argv[1])) {
        SbVec3f & vec2 =
          *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[1]));

        SbRotation rot(vec, vec2);
        SbVec3f axis;
        float rad;
        rot.getValue(axis, rad);

        data->setValue(axis[0], axis[1], axis[2], rad);
        return JS_TRUE;
      }
      // new SFRotation(SFVec3f axis, numeric angle)
      else {
        SbVec4f * data = new SbVec4f();
        spidermonkey()->JS_SetPrivate(cx, obj, data);
        *rval = OBJECT_TO_JSVAL(obj);

        double number = 0.0;
        spidermonkey()->JS_ValueToNumber(cx, argv[1], &number);

        data->setValue(vec[0], vec[1], vec[2], (float)number);
        return JS_TRUE;
      }
    }
  }
  // new SFRotation(numeric x, numeric y, numeric z, numeric angle)
  // Missing values default to 0.0, except y, which defaults to 1.0.
  //
  // SbRotation will default to 0.0, 0.0, 1.0, when angle is 0.0
  // So we use SbVec4f to hold values for SFRotation, since we need to support
  // patterns like this:
  // var colors = new MFColor();
  // colors.length = 10;
  // colors[0].x = 1
  // colors[0].y = 0
  // colors[0].z = 0
  // colors[0].angle = 1.8
  //
  // This will not work when SbRotation holds the values. 20050714 erikgors.

  return SFRotationHandler::constructor(cx, obj, argc, argv, rval);
}

// *************************************************************************
// functions

static JSBool SFNode_ref(JSContext * cx, JSObject * obj, uintN COIN_UNUSED_ARG(argc),
                         jsval * COIN_UNUSED_ARG(argv), jsval * COIN_UNUSED_ARG(rval))
{
  // Check if the JS object has already been garbage collected. This
  // must be done to prevent a Java script from crashing the
  // application.
  if (garbagecollectedobjects->find(obj) != -1) {
    if (SoJavaScriptEngine::debug())
      SoDebugError::postInfo("SFNode_ref", "WARNING! Trying to ref a deleted node.");
    return JSVAL_FALSE;
  }

  SoNode & node = *(SoNode *)spidermonkey()->JS_GetPrivate(cx, obj);
  node.ref();
  return JSVAL_TRUE;
}

static JSBool SFNode_unref(JSContext * cx, JSObject * obj, uintN COIN_UNUSED_ARG(argc),
                           jsval * COIN_UNUSED_ARG(argv), jsval * COIN_UNUSED_ARG(rval))
{
  // Check if the JS object has already been garbage collected. This
  // must be done to prevent a Java script from crashing the
  // application.
  if (garbagecollectedobjects->find(obj) != -1) {
    if (SoJavaScriptEngine::debug())
      SoDebugError::postInfo("SFNode_unref", "WARNING! Trying to unref an already deleted node.");
    return JSVAL_FALSE;
  }

  SoNode & node = *(SoNode *)spidermonkey()->JS_GetPrivate(cx, obj);
  node.unref();
  return JSVAL_TRUE;
}

static void *
buffer_realloc(void * bufptr, size_t size)
{
  char *buffer = (char *)realloc(bufptr, size);
  return buffer;
}

static JSBool SFNode_toString(JSContext * cx, JSObject * obj, uintN COIN_UNUSED_ARG(argc),
                              jsval * COIN_UNUSED_ARG(argv), jsval * rval)
{
  // Check if the JS object has already been garbage collected. This
  // must be done to prevent a Java script from crashing the
  // application.
  if (garbagecollectedobjects->find(obj) != -1) {
    if (SoJavaScriptEngine::debug())
      SoDebugError::postInfo("SFNode_toString", "WARNING! Trying to access "
                             "an already deleted node.");
    return JSVAL_FALSE;
  }

  SoNode *node = (SoNode *)spidermonkey()->JS_GetPrivate(cx, obj);

  SoOutput out;
  out.setHeaderString("#VRML V2.0 utf8");
  size_t buffer_size = 1024;
  void *buffer = malloc(buffer_size);
  out.setBuffer(buffer, buffer_size, buffer_realloc);

  SoWriteAction wa(&out);
  wa.apply(node);

  out.getBuffer(buffer, buffer_size);

  *rval = STRING_TO_JSVAL(spidermonkey()->JS_NewStringCopyZ(cx,
    (char *)buffer));

  free(buffer);

  return JSVAL_TRUE;
}

static void SFNode_deleteCB(void * COIN_UNUSED_ARG(data), SoSensor * sensor)
{
  SoNode * node = ((SoNodeSensor *) sensor)->getAttachedNode();
  void * tmp;
  if(!CoinVrmlJs_sensorinfohash->get(reinterpret_cast<uintptr_t>(node), tmp)) {
    assert(FALSE && "Trying to delete an unregistered SoNodeSensor. Internal error.");
    return;
  }

  CoinVrmlJs_SensorInfo * si = (CoinVrmlJs_SensorInfo *) tmp;

  // Delete all JSObjects which were connected to this SoNode
  while (si->objects.getLength()) {
    JSObject * obj = si->objects[0];
    garbagecollectedobjects->append(obj);
    si->objects.removeFast(0);
  }

  // Store the sensor-pointer so that it can be properly deleted later
  nodesensorstobedeleted->append((SoNodeSensor *) sensor);
  CoinVrmlJs_sensorinfohash->erase(reinterpret_cast<uintptr_t>(node));
  delete si;
}

static void cleanupObsoleteNodeSensors(void)
{
  // Delete all SoNodeSensors which no longer have a node attached.
  while(nodesensorstobedeleted->getLength() > 0) {
    SoNodeSensor * ns = (SoNodeSensor *) (*nodesensorstobedeleted)[0];
    nodesensorstobedeleted->removeItem(ns);
    delete ns;
  }
}

static JSBool SFVec2f_add(JSContext * cx, JSObject * obj, uintN argc,
                          jsval * argv, jsval * rval)
{
  SbVec2f & vec1 = *(SbVec2f *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (argc >= 1 && JSVAL_IS_SFVEC2F(cx, argv[0])) {
    SbVec2f & vec2 = *(SbVec2f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
    SbVec2f result = vec1 + vec2;
    *rval = OBJECT_TO_JSVAL(SFVec2fFactory(cx, result));
    return JS_TRUE;
  }

  return JS_FALSE;
}

static JSBool SFVec3f_add(JSContext * cx, JSObject * obj, uintN argc,
                          jsval * argv, jsval * rval)
{
  SbVec3f & vec1 = *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (argc >= 1 && JSVAL_IS_SFVEC3F(cx, argv[0])) {
    SbVec3f & vec2 = *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
    SbVec3f result = vec1 + vec2;
    *rval = OBJECT_TO_JSVAL(SFVec3fFactory(cx, result));
    return JS_TRUE;
  }

  return JS_FALSE;
}

static JSBool SFVec3d_add(JSContext * cx, JSObject * obj, uintN argc,
                          jsval * argv, jsval * rval)
{
  SbVec3d & vec1 = *(SbVec3d *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (argc >= 1 && JSVAL_IS_SFVEC3D(cx, argv[0])) {
    SbVec3d & vec2 = *(SbVec3d *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
    SbVec3d result = vec1 + vec2;
    *rval = OBJECT_TO_JSVAL(SFVec3dFactory(cx, result));
    return JS_TRUE;
  }

  return JS_FALSE;
}

static JSBool SFVec2f_divide(JSContext * cx, JSObject * obj, uintN argc,
                             jsval * argv, jsval * rval)
{
  SbVec2f & vec = *(SbVec2f *)spidermonkey()->JS_GetPrivate(cx, obj);

  double number;
  if (argc >= 1 && jsval2double(cx, argv[0], number)) {
    SbVec2f newVec = vec / (float)number;
    *rval = OBJECT_TO_JSVAL(SFVec2fFactory(cx, newVec));
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFVec3f_divide(JSContext * cx, JSObject * obj, uintN argc,
                             jsval * argv, jsval * rval)
{
  SbVec3f & vec = *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, obj);

  double number;
  if (argc >= 1 && jsval2double(cx, argv[0], number)) {
    SbVec3f newVec = vec / (float)number;
    *rval = OBJECT_TO_JSVAL(SFVec3fFactory(cx, newVec));
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFVec3d_divide(JSContext * cx, JSObject * obj, uintN argc,
                             jsval * argv, jsval * rval)
{
  SbVec3d & vec = *(SbVec3d *)spidermonkey()->JS_GetPrivate(cx, obj);

  double number;
  if (argc >= 1 && jsval2double(cx, argv[0], number)) {
    SbVec3d newVec = vec / number;
    *rval = OBJECT_TO_JSVAL(SFVec3dFactory(cx, newVec));
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFVec2f_dot(JSContext *cx, JSObject * COIN_UNUSED_ARG(obj), uintN argc,
                          jsval *argv, jsval * COIN_UNUSED_ARG(rval))
{
  //SbVec2f & vec1 = *(SbVec2f *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (argc >= 1 && JSVAL_IS_SFVEC2F(cx, argv[0])) {
    //SbVec2f & vec2 = *(SbVec2f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));

    //float dot = vec1.dot(vec2);
    //JSBool ok = spidermonkey()->JS_NewDoubleValue(cx, dot, rval);
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFVec3f_dot(JSContext *cx, JSObject * COIN_UNUSED_ARG(obj), uintN argc,
                          jsval *argv, jsval * COIN_UNUSED_ARG(rval))
{
  //SbVec3f & vec = *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (argc >= 1 && JSVAL_IS_SFVEC3F(cx, argv[0])) {

    //SbVec3f & vec2 = *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));

    //float dot = vec.dot(vec2);
    //JSBool ok = spidermonkey()->JS_NewDoubleValue(cx, dot, rval);
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFVec3d_dot(JSContext *cx, JSObject * COIN_UNUSED_ARG(obj), uintN argc,
                          jsval *argv, jsval * COIN_UNUSED_ARG(rval))
{
  //SbVec3d & vec = *(SbVec3d *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (argc >= 1 && JSVAL_IS_SFVEC3D(cx, argv[0])) {

    //SbVec3d & vec2 = *(SbVec3d *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));

    //double dot = vec.dot(vec2);
    //JSBool ok = spidermonkey()->JS_NewDoubleValue(cx, dot, rval);
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFVec2_length(JSContext * COIN_UNUSED_ARG(cx), JSObject * COIN_UNUSED_ARG(obj), uintN COIN_UNUSED_ARG(argc),
                            jsval * COIN_UNUSED_ARG(argv), jsval * COIN_UNUSED_ARG(rval))
{
  //SbVec2f * vec = (SbVec2f *)spidermonkey()->JS_GetPrivate(cx, obj);
  //JSBool ok = spidermonkey()->JS_NewDoubleValue(cx, vec->length(), rval);
  return JS_TRUE;
}

static JSBool SFVec3f_length(JSContext * COIN_UNUSED_ARG(cx), JSObject * COIN_UNUSED_ARG(obj), uintN COIN_UNUSED_ARG(argc),
                            jsval * COIN_UNUSED_ARG(argv), jsval * COIN_UNUSED_ARG(rval))
{
  //SbVec3f * vec = (SbVec3f *)spidermonkey()->JS_GetPrivate(cx, obj);
  //JSBool ok = spidermonkey()->JS_NewDoubleValue(cx, vec->length(), rval);
  return JS_TRUE;
}

static JSBool SFVec3d_length(JSContext * COIN_UNUSED_ARG(cx), JSObject * COIN_UNUSED_ARG(obj), uintN COIN_UNUSED_ARG(argc),
                            jsval * COIN_UNUSED_ARG(argv), jsval * COIN_UNUSED_ARG(rval))
{
  //SbVec3d * vec = (SbVec3d *)spidermonkey()->JS_GetPrivate(cx, obj);
  //JSBool ok = spidermonkey()->JS_NewDoubleValue(cx, vec->length(), rval);
  return JS_TRUE;
}

static JSBool SFVec2f_multiply(JSContext * cx, JSObject * obj, uintN argc,
                               jsval * argv, jsval * rval)
{

  SbVec2f & vec = *(SbVec2f *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (argc >= 1 && JSVAL_IS_NUMBER(argv[0])) {
    double number;
    spidermonkey()->JS_ValueToNumber(cx, argv[0], &number);

    SbVec2f newVec = vec * (float)number;

    *rval = OBJECT_TO_JSVAL(SFVec2fFactory(cx, newVec));

    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFVec3f_multiply(JSContext * cx, JSObject * obj, uintN argc,
                               jsval * argv, jsval * rval)
{
  SbVec3f & vec = *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, obj);

  double number;
  if (argc >= 1 && jsval2double(cx, argv[0], number)) {
    SbVec3f newVec = vec * (float)number;
    *rval = OBJECT_TO_JSVAL(SFVec3fFactory(cx, newVec));
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFVec3d_multiply(JSContext * cx, JSObject * obj, uintN argc,
                               jsval * argv, jsval * rval)
{
  SbVec3d & vec = *(SbVec3d *)spidermonkey()->JS_GetPrivate(cx, obj);

  double number;
  if (argc >= 1 && jsval2double(cx, argv[0], number)) {
    SbVec3d newVec = vec * number;
    *rval = OBJECT_TO_JSVAL(SFVec3dFactory(cx, newVec));
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFVec2f_normalize(JSContext * cx, JSObject * obj, uintN COIN_UNUSED_ARG(argc),
                                jsval * COIN_UNUSED_ARG(argv), jsval * rval)
{
  SbVec2f vec = *(SbVec2f *)spidermonkey()->JS_GetPrivate(cx, obj);
  vec.normalize();
  *rval = OBJECT_TO_JSVAL(SFVec2fFactory(cx, vec));
  return JS_TRUE;
}

static JSBool SFVec3f_normalize(JSContext * cx, JSObject * obj, uintN COIN_UNUSED_ARG(argc),
                                jsval * COIN_UNUSED_ARG(argv), jsval * rval)
{
  SbVec3f vec = *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, obj);
  vec.normalize();
  *rval = OBJECT_TO_JSVAL(SFVec3fFactory(cx, vec));
  return JS_TRUE;
}

static JSBool SFVec3d_normalize(JSContext * cx, JSObject * obj, uintN COIN_UNUSED_ARG(argc),
                                jsval * COIN_UNUSED_ARG(argv), jsval * rval)
{
  SbVec3d vec = *(SbVec3d *)spidermonkey()->JS_GetPrivate(cx, obj);
  vec.normalize();
  *rval = OBJECT_TO_JSVAL(SFVec3dFactory(cx, vec));
  return JS_TRUE;
}

static JSBool SFVec3f_negate(JSContext * cx, JSObject * obj, uintN COIN_UNUSED_ARG(argc),
                                     jsval * COIN_UNUSED_ARG(argv), jsval * rval)
{
  SbVec3f vec = *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, obj);
  vec.negate();
  *rval = OBJECT_TO_JSVAL(SFVec3fFactory(cx, vec));
  return JS_TRUE;
}

static JSBool SFVec3d_negate(JSContext * cx, JSObject * obj, uintN COIN_UNUSED_ARG(argc),
                                     jsval * COIN_UNUSED_ARG(argv), jsval * rval)
{
  SbVec3d vec = *(SbVec3d *)spidermonkey()->JS_GetPrivate(cx, obj);
  vec.negate();
  *rval = OBJECT_TO_JSVAL(SFVec3dFactory(cx, vec));
  return JS_TRUE;
}

static JSBool SFVec2f_subtract(JSContext * cx, JSObject * obj, uintN argc,
                               jsval * argv, jsval * rval)
{
  SbVec2f & vec1 = *(SbVec2f *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (argc >= 1 && JSVAL_IS_SFVEC2F(cx, argv[0])) {
    SbVec2f & vec2 = *(SbVec2f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
    SbVec2f result = vec1 - vec2;
    *rval = OBJECT_TO_JSVAL(SFVec2fFactory(cx, result));
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFVec3f_subtract(JSContext * cx, JSObject * obj, uintN argc,
                               jsval * argv, jsval * rval)
{
  SbVec3f & vec1 = *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (argc >= 1 && JSVAL_IS_SFVEC3F(cx, argv[0])) {
    SbVec3f & vec2 = *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
    SbVec3f result = vec1 - vec2;
    *rval = OBJECT_TO_JSVAL(SFVec3fFactory(cx, result));
    return JS_TRUE;
  }

  return JS_FALSE;
}

static JSBool SFVec3d_subtract(JSContext * cx, JSObject * obj, uintN argc,
                               jsval * argv, jsval * rval)
{
  SbVec3d & vec1 = *(SbVec3d *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (argc >= 1 && JSVAL_IS_SFVEC3D(cx, argv[0])) {
    SbVec3d & vec2 = *(SbVec3d *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
    SbVec3d result = vec1 - vec2;
    *rval = OBJECT_TO_JSVAL(SFVec3dFactory(cx, result));
    return JS_TRUE;
  }

  return JS_FALSE;
}

static JSBool SFColor_setHSV(JSContext * cx, JSObject * obj, uintN argc,
                             jsval * argv, jsval * rval)
{
  if (argc != 3) {
    return JS_FALSE;
  }
  SbColor & color = *(SbColor *)spidermonkey()->JS_GetPrivate(cx, obj);

  float vals[3];
  int i;

  for (i=0; i<3; ++i) {
    double number;
    spidermonkey()->JS_ValueToNumber(cx, argv[i], &number);
    vals[i] = (float)number;
  }

  color.setHSVValue(vals);

  *rval = JSVAL_VOID;
  return JS_TRUE;
}

static JSBool SFColor_getHSV(JSContext * cx, JSObject * obj, uintN COIN_UNUSED_ARG(argc),
                             jsval * COIN_UNUSED_ARG(argv), jsval * rval)
{
  SbColor & color = *(SbColor *)spidermonkey()->JS_GetPrivate(cx, obj);

  float vals[3];
  color.getHSVValue(vals);

  jsval vector[3];
  for (int i=0; i<3; ++i) {
    spidermonkey()->JS_NewDoubleValue(cx, vals[i], &vector[i]);
  }

  *rval = OBJECT_TO_JSVAL(spidermonkey()->JS_NewArrayObject(cx, 3, vector));

  return JS_TRUE;
}

static JSBool SFRotation_getAxis(JSContext * cx, JSObject * obj, uintN COIN_UNUSED_ARG(argc),
                                 jsval * COIN_UNUSED_ARG(argv), jsval * rval)
{
  SbVec4f & rot = *(SbVec4f *)spidermonkey()->JS_GetPrivate(cx, obj);
  SbVec3f axis(rot[0], rot[1], rot[2]);
  *rval = OBJECT_TO_JSVAL(SFVec3fFactory(cx, axis));
  return JS_TRUE;
}

static JSBool SFRotation_inverse(JSContext * cx, JSObject * obj, uintN COIN_UNUSED_ARG(argc),
                                 jsval * COIN_UNUSED_ARG(argv), jsval * rval)
{
  SbVec4f & rot = *(SbVec4f *)spidermonkey()->JS_GetPrivate(cx, obj);

  SbVec3f axis(rot[0], rot[1], rot[2]);
  SbRotation newRot(axis, rot[3]);
  newRot.invert();

  *rval = OBJECT_TO_JSVAL(SFRotationFactory(cx, newRot));
  return JS_TRUE;
}

static JSBool SFRotation_multiply(JSContext * cx, JSObject * obj, uintN argc,
                                  jsval * argv, jsval * rval)
{
  SbVec4f & vec1 = *(SbVec4f *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (argc >= 1 && JSVAL_IS_SFROTATION(cx, argv[0])) {
    SbVec4f & vec2 = *(SbVec4f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
    SbVec3f axis1(vec1[0], vec1[1], vec1[2]);
    SbVec3f axis2(vec2[0], vec2[1], vec2[2]);
    SbRotation result = SbRotation(axis1, vec1[3]) * SbRotation(axis2, vec2[3]);

    *rval = OBJECT_TO_JSVAL(SFRotationFactory(cx, result));
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFRotation_multVec(JSContext * cx, JSObject * obj, uintN argc,
                                 jsval * argv, jsval * rval)
{
  SbVec4f & vec = *(SbVec4f *)spidermonkey()->JS_GetPrivate(cx, obj);
  SbVec3f axis(vec[0], vec[1], vec[2]);
  SbRotation rot(axis, vec[3]);

  if (argc >= 1 && JSVAL_IS_SFVEC3F(cx, argv[0])) {
    SbVec3f & src = *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
    SbVec3f dst;
    rot.multVec(src, dst);

    *rval = OBJECT_TO_JSVAL(SFVec3fFactory(cx, dst));
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFRotation_setAxis(JSContext * cx, JSObject * obj, uintN argc,
                                 jsval * argv, jsval * rval)
{
  SbVec4f & rot = *(SbVec4f *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (argc >= 1 && JSVAL_IS_SFVEC3F(cx, argv[0])) {
    SbVec3f & axis = *(SbVec3f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
    rot[0] = axis[0];
    rot[1] = axis[1];
    rot[2] = axis[2];

    *rval = JSVAL_VOID;
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSBool SFRotation_slerp(JSContext * cx, JSObject * obj, uintN argc,
                               jsval * argv, jsval * rval)
{
  SbVec4f & vec = *(SbVec4f *)spidermonkey()->JS_GetPrivate(cx, obj);
  SbVec3f axis(vec[0], vec[1], vec[2]);
  SbRotation rot(axis, vec[3]);

  double number;
  if (argc >= 2 && JSVAL_IS_SFROTATION(cx, argv[0]) && jsval2double(cx, argv[1], number)) {
    SbVec4f & vec2 = *(SbVec4f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
    SbVec3f axis2(vec2[0], vec2[1], vec2[2]);
    SbRotation dest(axis2, vec2[3]);

    SbRotation result = SbRotation::slerp(rot, dest, (float)number);

    *rval = OBJECT_TO_JSVAL(SFRotationFactory(cx, result));
    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSFunctionSpec SFNodeFunctions[] = {
  {"ref", SFNode_ref, 0, 0, 0},
  {"unref", SFNode_unref, 0, 0, 0},
  {"toString", SFNode_toString, 0, 0, 0},
  {NULL, NULL, 0, 0, 0}
};

static JSFunctionSpec SFVec2fFunctions[] = {
  {"add", SFVec2f_add, 1, 0, 0},
  {"divide", SFVec2f_divide, 1, 0, 0},
  {"dot", SFVec2f_dot, 1, 0, 0},
  {"length", SFVec2_length, 0, 0, 0},
  {"multiply", SFVec2f_multiply, 1, 0, 0},
  {"normalize", SFVec2f_normalize, 0, 0, 0},
  {"subtract", SFVec2f_subtract, 1, 0, 0},
  {NULL, NULL, 0, 0, 0}
};

static JSFunctionSpec SFVec3fFunctions[] = {
  {"add", SFVec3f_add, 1, 0, 0},
  {"divide", SFVec3f_divide, 1, 0, 0},
  {"dot", SFVec3f_dot, 1, 0, 0},
  {"length", SFVec3f_length, 0, 0, 0},
  {"multiply", SFVec3f_multiply, 1, 0, 0},
  {"normalize", SFVec3f_normalize, 0, 0, 0},
  {"negate", SFVec3f_negate, 0, 0, 0},
  {"subtract", SFVec3f_subtract, 1, 0, 0},
  {NULL, NULL, 0, 0, 0}
};

static JSFunctionSpec SFVec3dFunctions[] = {
  {"add", SFVec3d_add, 1, 0, 0},
  {"divide", SFVec3d_divide, 1, 0, 0},
  {"dot", SFVec3d_dot, 1, 0, 0},
  {"length", SFVec3d_length, 0, 0, 0},
  {"multiply", SFVec3d_multiply, 1, 0, 0},
  {"normalize", SFVec3d_normalize, 0, 0, 0},
  {"negate", SFVec3d_negate, 0, 0, 0},
  {"subtract", SFVec3d_subtract, 1, 0, 0},
  {NULL, NULL, 0, 0, 0}
};

static JSFunctionSpec SFColorFunctions[] = {
  {"setHSV", SFColor_setHSV, 3, 0, 0},
  {"getHSV", SFColor_getHSV, 0, 0, 0},
  {NULL, NULL, 0, 0, 0}
};

static JSFunctionSpec SFRotationFunctions[] = {
  {"getAxis", SFRotation_getAxis, 0, 0, 0},
  {"inverse", SFRotation_inverse, 0, 0, 0},
  {"multiply", SFRotation_multiply, 1, 0, 0},
  {"multVec", SFRotation_multVec, 1, 0, 0},
  {"setAxis", SFRotation_setAxis, 1, 0, 0},
  {"slerp", SFRotation_slerp, 2, 0, 0},
  {NULL, NULL, 0, 0, 0}
};

// *************************************************************************
// factory

static JSObject * SFColorFactory(JSContext * cx, const SbColor & self)
{
  JSObject * obj =
    spidermonkey()->JS_NewObject(cx, &CoinVrmlJs::SFColor.cls, NULL, NULL);
  spidermonkey()->JS_DefineFunctions(cx, obj, SFColorFunctions);

  SbColor * color = new SbColor(self);
  spidermonkey()->JS_SetPrivate(cx, obj, color);

  return obj;
}

static JSObject * SFRotationFactory(JSContext * cx, const SbRotation & self)
{
  JSObject * obj =
    spidermonkey()->JS_NewObject(cx, &CoinVrmlJs::SFRotation.cls, NULL, NULL);
  spidermonkey()->JS_DefineFunctions(cx, obj, SFRotationFunctions);

  SbVec3f axis;
  float angle;
  self.getValue(axis, angle);
  SbVec4f * data = new SbVec4f(axis[0], axis[1], axis[2], angle);
  spidermonkey()->JS_SetPrivate(cx, obj, data);

  return obj;
}

static JSObject * SFVec2fFactory(JSContext * cx, const SbVec2f & self)
{
  JSObject * obj =
    spidermonkey()->JS_NewObject(cx, &CoinVrmlJs::SFVec2f.cls, NULL, NULL);
  spidermonkey()->JS_DefineFunctions(cx, obj, SFVec2fFunctions);

  SbVec2f * data = new SbVec2f(self);
  spidermonkey()->JS_SetPrivate(cx, obj, data);
  return obj;
}

static JSObject * SFVec3fFactory(JSContext * cx, const SbVec3f & self)
{
  JSObject * obj =
    spidermonkey()->JS_NewObject(cx, &CoinVrmlJs::SFVec3f.cls, NULL, NULL);
  spidermonkey()->JS_DefineFunctions(cx, obj, SFVec3fFunctions);

  SbVec3f * data = new SbVec3f(self);
  spidermonkey()->JS_SetPrivate(cx, obj, data);

  return obj;
}

static JSObject * SFVec3dFactory(JSContext * cx, const SbVec3d & self)
{
  JSObject * obj =
    spidermonkey()->JS_NewObject(cx, &CoinVrmlJs::SFVec3d.cls, NULL, NULL);
  spidermonkey()->JS_DefineFunctions(cx, obj, SFVec3dFunctions);

  SbVec3d * data = new SbVec3d(self);
  spidermonkey()->JS_SetPrivate(cx, obj, data);

  return obj;
}

static JSObject * SFVec2f_init(JSContext * cx, JSObject * obj)
{
  return spidermonkey()->JS_InitClass(cx, obj, NULL, &CoinVrmlJs::SFVec2f.cls,
                                      SFVec2fHandler::constructor, 0,
                                      NULL, SFVec2fFunctions, NULL, NULL);
}

static JSObject * SFVec3f_init(JSContext * cx, JSObject * obj)
{
  return spidermonkey()->JS_InitClass(cx, obj, NULL, &CoinVrmlJs::SFVec3f.cls,
                                      SFVec3fHandler::constructor, 0,
                                      NULL, SFVec3fFunctions, NULL, NULL);
}

static JSObject * SFVec3d_init(JSContext * cx, JSObject * obj)
{
  return spidermonkey()->JS_InitClass(cx, obj, NULL, &CoinVrmlJs::SFVec3d.cls,
                                      SFVec3dHandler::constructor, 0,
                                      NULL, SFVec3dFunctions, NULL, NULL);
}

static JSObject * SFColor_init(JSContext * cx, JSObject * obj)
{
  return spidermonkey()->JS_InitClass(cx, obj, NULL, &CoinVrmlJs::SFColor.cls,
                                      SFColorHandler::constructor, 0,
                                      NULL, SFColorFunctions, NULL, NULL);
}

static JSObject * SFRotation_init(JSContext * cx, JSObject * obj)
{
  return spidermonkey()->JS_InitClass(cx, obj, NULL, &CoinVrmlJs::SFRotation.cls,
                                      SFRotationConstructor, 0,
                                      NULL, SFRotationFunctions, NULL, NULL);
}

// *************************************************************************
// SFNode

static JSBool SFNode_get(JSContext * cx, JSObject * obj, jsval id, jsval * rval)
{

  if (garbagecollectedobjects->find(obj) != -1) {
    spidermonkey()->JS_ReportError(cx, "Trying to access an object with refcount=0.");
    return JS_FALSE;
  }

  SoNode * container = (SoNode *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (container == NULL) {
    // this will only happen when JS_NewObject calls "constructor"
    // or the node is "undefined"

    if (JSVAL_IS_STRING(id)) {
      const char * str = spidermonkey()->JS_GetStringBytes(JSVAL_TO_STRING(id));
      if (SbName("constructor") == str) {
        return JS_TRUE;
      }
    }
    spidermonkey()->JS_ReportError(cx, "node is undefined");
    return JS_FALSE;
  }

  if (JSVAL_IS_STRING(id)) {
    SbString str(spidermonkey()->JS_GetStringBytes(JSVAL_TO_STRING(id)));

    SoField * out = container->getField(SbName(str));

    int len = str.getLength();
    static const char post[] = "_changed";
    static const size_t postLen = sizeof(post) - 1;

    if (out == NULL && len > (int)postLen &&
        str.getSubString(len - postLen) == post) {
      out = container->getField(SbName(str.getSubString(0, len - postLen - 1)));
    }

    if (out != NULL) {
      SoJavaScriptEngine::getEngine(cx)->field2jsval(out, rval);
      return JS_TRUE;
    }
  }

  /* Note: If we're unable to find the field, we return JS_TRUE
     instead of JS_FALSE, which might seem as the logical choice
     for indicating a failure. If we return JS_FALSE, execution of
     the script will halt. One side-effect of this is that it is not
     possible to extend the SFNode JavaScript object by adding
     properties, which can be very useful in some cases. This is also
     more in line with the JavaScript philosophy that one can
     dynamically add and remove properties for any object at any time.

     2005-11-23 thammer.

     Update: We should look more closely into the return values
     JS_TRUE / JS_FALSE for all getters and setters, and possibly for
     other functions as well. It might be more robust to report the
     error (using JS_ReportError) and return JS_TRUE to allow the
     script to continue running than to abort the script.

     2006-06-21 thammer.
  */
  return JS_TRUE;
}

static JSBool SFNode_set(JSContext * cx, JSObject * obj, jsval id, jsval * rval)
{
  SoNode * container = (SoNode *)spidermonkey()->JS_GetPrivate(cx, obj);

  if (container == NULL) {
    spidermonkey()->JS_ReportError(cx, "node is undefined");
    return JS_FALSE;
  }

  if (JSVAL_IS_STRING(id)) {
    SbString str(spidermonkey()->JS_GetStringBytes(JSVAL_TO_STRING(id)));

    SoField * in = container->getField(SbName(str));

    int len = str.getLength();
    static const char pre[] = "set_";
    static const size_t preLen = sizeof(pre) - 1;

    if (in == NULL && len > (int)preLen &&
        str.getSubString(0, preLen - 1) == pre) {
      in = container->getField(SbName(str.getSubString(preLen)));
    }

    if (in != NULL) {
      SoJavaScriptEngine::getEngine(cx)->jsval2field(*rval, in);
      if (SoJavaScriptEngine::debug()) {
        SoDebugError::postInfo("SFNode_set", "setting field %s", str.getString());
      }
    } else if (SoJavaScriptEngine::debug())
      SoDebugError::postWarning("SFNode_set", "no such field %s", str.getString());
  }

  // See note in SFNode_get() about return value. 2005-11-23 thammer.
  return JS_TRUE;
}

static void SFNodeDestructor(JSContext * cx, JSObject * obj)
{
  // Delete all SoNodeSensors which no longer have a node attached.
  cleanupObsoleteNodeSensors();
  if(garbagecollectedobjects->find(obj) != -1) { // Pointer is marked as garbage-collected
    garbagecollectedobjects->removeItem(obj);
  }

  SoNode * container = (SoNode *)spidermonkey()->JS_GetPrivate(cx, obj);
  // FIXME: We cannot assume this since the class object itself is an
  // instance of this JSClass. kintel 20050804.
  //  assert(container != NULL);
  if (SoJavaScriptEngine::getEngine(cx)->getAutoNodeUnrefState())
    if (container) container->unref();
}

// Called via coin_atexit() when Coin exits.
static void deleteSensorInfoHash(void)
{
  CoinVrmlJs_sensorinfohash->clear();
  delete CoinVrmlJs_sensorinfohash;
}

/*
  Attach an SoNodeSensor to node. Adds a 'delete callback' to the
  sensor which includes the 'obj' parameter.
*/
static void attachSensorToNode(SoNode * node, JSObject * obj)
{
  // Has the hash-table been initialized?
  if (!CoinVrmlJs_sensorinfohash) {
    CoinVrmlJs_sensorinfohash = new SbHash<uintptr_t, void *>;
    coin_atexit(deleteSensorInfoHash, CC_ATEXIT_NORMAL);
  }

  // Is a sensor already attached to this SoNode?
  void * tmp;
  if (CoinVrmlJs_sensorinfohash->get(reinterpret_cast<uintptr_t>(node), tmp)) {
    CoinVrmlJs_SensorInfo * si = (CoinVrmlJs_SensorInfo *) tmp;
    si->objects.append(obj);
  }
  else {
    SoNodeSensor * ns = new SoNodeSensor();
    ns->setDeleteCallback(SFNode_deleteCB, obj);
    ns->attach(node);
    CoinVrmlJs_SensorInfo * si = new CoinVrmlJs_SensorInfo;
    si->objects.append(obj);
    CoinVrmlJs_sensorinfohash->put(reinterpret_cast<uintptr_t>(node), si);
  }
}

static JSObject * SFNodeFactory(JSContext * cx, SoNode * container)
{
  // Delete all SoNodeSensors which no longer have a node attached.
  cleanupObsoleteNodeSensors();

  JSObject * obj = spidermonkey()->JS_NewObject(cx, &CoinVrmlJs::SFNode.cls, NULL, NULL);

  if(garbagecollectedobjects->find(obj) != -1) // Pointer has been used before. Remove from list.
    garbagecollectedobjects->removeItem(obj);

  spidermonkey()->JS_SetPrivate(cx, obj, container);
  spidermonkey()->JS_DefineFunctions(cx, obj, SFNodeFunctions);

  // FIXME: If the node has enums, define them here. 2007-03-08 thammer.

  attachSensorToNode(container, obj);

  if (SoJavaScriptEngine::getEngine(cx)->getAutoNodeUnrefState())
    container->ref();

  return obj;
}

static JSBool SFNodeConstructor(JSContext * cx, JSObject * obj,
                                uintN argc, jsval * argv, jsval *rval)
{
  // Delete all SoNodeSensors which no longer have a node attached.
  cleanupObsoleteNodeSensors();

  // spidermonkey ignores the return value
  if (argc >= 1 && JSVAL_IS_STRING(argv[0])) {
    JSString * js = JSVAL_TO_STRING(argv[0]);
    char * str = spidermonkey()->JS_GetStringBytes(js);
    //size_t len = spidermonkey()->JS_GetStringLength(js);

    // FIXME: what about UTF8? 20050701 erikgors.

    if (SoJavaScriptEngine::debug()) {
      SoDebugError::postInfo("SFNodeConstructor",
                             "creating new node with str = '%s'", str);
    }

    SoInput input;
    const char * array[2];
    array[0] = str;
    array[1] = NULL;
    input.setStringArray(array);

    SoGroup * group;

    if (input.isFileVRML2())
      group = SoDB::readAllVRML(&input);
    else
      group = SoDB::readAll(&input);

    if (group == NULL) {
      spidermonkey()->JS_ReportError(cx, "input is not legal VRML string");
      return JS_FALSE;
    }
    if (group->getNumChildren() == 0) {
      spidermonkey()->JS_ReportError(cx, "no top-level node, result is undefined");
      *rval = JSVAL_VOID;
      return JS_FALSE;
    }

    if(garbagecollectedobjects->find(obj) != -1) { // Pointer has been used before. Remove from list.
      garbagecollectedobjects->removeItem(obj);
    }

    attachSensorToNode(group, obj);

    if (SoJavaScriptEngine::getEngine(cx)->getAutoNodeUnrefState())
      group->ref();

    spidermonkey()->JS_SetPrivate(cx, obj, group);
    spidermonkey()->JS_DefineFunctions(cx, obj, SFNodeFunctions);

    return JS_TRUE;
  }
  return JS_FALSE;
}

static JSObject * SFNode_init(JSContext * cx, JSObject * obj)
{
  return spidermonkey()->JS_InitClass(cx, obj, NULL, &CoinVrmlJs::SFNode.cls,
                                      SFNodeConstructor, 0,
                                      NULL, NULL, NULL, NULL);
}


// *************************************************************************
// jsval2field

static SbBool SFBool_jsval2field(JSContext * cx, const jsval v, SoField * f)
{
  if (JSVAL_IS_BOOLEAN(v)) {
    const SbBool b = JSVAL_TO_BOOLEAN(v);
    ((SoSFBool *)f)->setValue(b);
    return TRUE;
  }
  else {
    JSBool b;
    if (spidermonkey()->JS_ValueToBoolean(cx, v, &b)) {
      ((SoSFBool *)f)->setValue(b);
      return TRUE;
    }
  }
  return FALSE;
}

static SbBool SFColor_jsval2field(JSContext * cx, const jsval v, SoField * f)
{
  if (JSVAL_IS_SFCOLOR(cx, v)) {
    SbColor * color = (SbColor *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(v));
    assert(color != NULL);
    ((SoSFColor *)f)->setValue(*color);
    return TRUE;
  }
  return FALSE;
}

static SbBool SFFloat_jsval2field(JSContext * cx, const jsval v, SoField * f)
{
  double number;
  if (jsval2double(cx, v, number)) {
    ((SoSFFloat *)f)->setValue((float)number);
    return TRUE;
  }
  return FALSE;
}

static SbBool SFInt32_jsval2field(JSContext * cx, const jsval v, SoField * f)
{
  int32_t val;
  if (jsval2int(cx, v, val)) {
    ((SoSFInt32 *)f)->setValue(val);
    return TRUE;
  }
  return FALSE;
}

static SbBool SFEnum_jsval2field(JSContext * cx, const jsval v, SoField * f)
{
  int32_t val;
  if (jsval2int(cx, v, val)) {
    ((SoSFInt32 *)f)->setValue(val);
    return TRUE;
  }
  return FALSE;
}

static SbBool SFNode_jsval2field(JSContext * cx, const jsval v, SoField * f)
{
  if (JSVAL_IS_NULL(v)) {
    ((SoSFNode *)f)->setValue(NULL);
    return TRUE;
  }
  if (JSVAL_IS_OBJECT(v) &&
      spidermonkey()->JS_InstanceOf(cx, JSVAL_TO_OBJECT(v), &CoinVrmlJs::SFNode.cls, NULL)) {
    SoNode * node = (SoNode *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(v));
    ((SoSFNode *)f)->setValue(node);
    return TRUE;
  }
  return FALSE;
}

static SbBool SFRotation_jsval2field(JSContext * cx, const jsval v, SoField * f)
{
  if (JSVAL_IS_SFROTATION(cx, v)) {
    SbVec4f * rot = (SbVec4f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(v));
    assert(rot != NULL);
    SbVec3f axis((*rot)[0], (*rot)[1], (*rot)[2]);
    ((SoSFRotation *)f)->setValue(SbRotation(axis, (*rot)[3]));
    return TRUE;
  }
  return FALSE;
}

static SbBool SFString_jsval2field(JSContext * COIN_UNUSED_ARG(cx), const jsval v, SoField * f)
{
  if (JSVAL_IS_STRING(v)) {
    const char * str = spidermonkey()->JS_GetStringBytes(JSVAL_TO_STRING(v));
    ((SoSFString *)f)->setValue(str);
    return TRUE;
  }
  return FALSE;
}

static SbBool SFTime_jsval2field(JSContext * cx, const jsval v, SoField * f)
{
  double number;
  if (jsval2double(cx, v, number)) {
    spidermonkey()->JS_ValueToNumber(cx, v, &number);
    ((SoSFTime*)f)->setValue(SbTime(number));
    return TRUE;
  }
  return FALSE;
}

static SbBool SFVec2f_jsval2field(JSContext * cx, const jsval v, SoField * f)
{
  if (JSVAL_IS_SFVEC2F(cx, v)) {
    SbVec2f * vec = (SbVec2f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(v));
    assert(vec != NULL);
    ((SoSFVec2f *)f)->setValue(*vec);
    return TRUE;
  }
  return FALSE;
}

static SbBool SFVec3f_jsval2field(JSContext * cx, const jsval v, SoField * f)
{
  if (JSVAL_IS_SFVEC3F(cx, v)) {
    SbVec3f * vec = (SbVec3f *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(v));
    assert(vec != NULL);
    ((SoSFVec3f *)f)->setValue(*vec);
    return TRUE;
  }
  return FALSE;
}

static SbBool SFVec3d_jsval2field(JSContext * cx, const jsval v, SoField * f)
{
  if (JSVAL_IS_SFVEC3D(cx, v)) {
    SbVec3d * vec = (SbVec3d *)spidermonkey()->JS_GetPrivate(cx, JSVAL_TO_OBJECT(v));
    assert(vec != NULL);
    ((SoSFVec3d *)f)->setValue(*vec);
    return TRUE;
  }
  return FALSE;
}

// *************************************************************************
// field2jsval

static void SFBool_field2jsval(JSContext * COIN_UNUSED_ARG(cx), const SoField * f, jsval * v)
{
  const SbBool val = ((SoSFBool *)f)->getValue();
  *v = BOOLEAN_TO_JSVAL(val);
}

static void SFColor_field2jsval(JSContext * cx, const SoField * f, jsval *v)
{
  const SbColor & val = ((SoSFColor *)f)->getValue();
  JSObject * obj = SFColorFactory(cx, val);
  *v = OBJECT_TO_JSVAL(obj);
}

static void SFFloat_field2jsval(JSContext * COIN_UNUSED_ARG(cx), const SoField * COIN_UNUSED_ARG(f), jsval * COIN_UNUSED_ARG(v))
{
  //const float val = ((SoSFFloat *)f)->getValue();
  //JSBool ok = spidermonkey()->JS_NewDoubleValue(cx, val, v);
}

static void SFInt32_field2jsval(JSContext * COIN_UNUSED_ARG(cx), const SoField * f, jsval * v)
{
  const int32_t val = ((SoSFInt32 *)f)->getValue();
  *v = INT_TO_JSVAL(val);
}

static void SFEnum_field2jsval(JSContext * COIN_UNUSED_ARG(cx), const SoField * f, jsval * v)
{
  const int32_t val = ((SoSFInt32 *)f)->getValue();
  *v = INT_TO_JSVAL(val);
}

static void SFNode_field2jsval(JSContext * cx, const SoField * f, jsval * v)
{
  SoNode * node = ((SoSFNode *)f)->getValue();
  if (node == NULL)
    *v = JSVAL_NULL;
  else {
    JSObject * obj = SFNodeFactory(cx, node);
    *v = OBJECT_TO_JSVAL(obj);
  }
}

static void SFRotation_field2jsval(JSContext * cx, const SoField * f, jsval *v)
{
  const SbRotation & val = ((SoSFRotation *)f)->getValue();
  JSObject * obj = SFRotationFactory(cx, val);
  *v = OBJECT_TO_JSVAL(obj);
}

static void SFString_field2jsval(JSContext * cx, const SoField * f, jsval * v)
{
  const SbString & val = ((SoSFString *)f)->getValue();
  JSString * str = spidermonkey()->JS_NewStringCopyZ(cx, val.getString());
  *v = STRING_TO_JSVAL(str);
}

static void SFTime_field2jsval(JSContext * cx, const SoField * f, jsval * v)
{
  const SbTime & time = ((SoSFTime *)f)->getValue();
  spidermonkey()->JS_NewDoubleValue(cx, time.getValue(), v);
}

static void SFVec2f_field2jsval(JSContext * cx, const SoField * f, jsval *v)
{
  const SbVec2f & val = ((SoSFVec2f *)f)->getValue();
  JSObject * obj = SFVec2fFactory(cx, val);
  *v = OBJECT_TO_JSVAL(obj);
}

static void SFVec3f_field2jsval(JSContext * cx, const SoField * f, jsval *v)
{
  const SbVec3f & val = ((SoSFVec3f *)f)->getValue();
  JSObject * obj = SFVec3fFactory(cx, val);
  *v = OBJECT_TO_JSVAL(obj);
}

static void SFVec3d_field2jsval(JSContext * cx, const SoField * f, jsval *v)
{
  const SbVec3d & val = ((SoSFVec3d *)f)->getValue();
  JSObject * obj = SFVec3dFactory(cx, val);
  *v = OBJECT_TO_JSVAL(obj);
}

// *************************************************************************
// classes

CoinVrmlJs::ClassDescriptor CoinVrmlJs::SFColor = {
  {
    "SFColor", JSCLASS_HAS_PRIVATE, NULL, NULL,
    SFColorHandler::get, SFColorHandler::set,
    NULL, NULL, NULL,
    SFColorHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  SFColorFunctions
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::SFNode = {
  {
    "SFNode", JSCLASS_HAS_PRIVATE, NULL, NULL,
    SFNode_get, SFNode_set,
    NULL, NULL, NULL,
    SFNodeDestructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  NULL
};


CoinVrmlJs::ClassDescriptor CoinVrmlJs::SFRotation = {
  {
    "SFRotation", JSCLASS_HAS_PRIVATE, NULL, NULL,
    SFRotationHandler::get, SFRotationHandler::set,
    NULL, NULL, NULL,
    SFRotationHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  SFRotationFunctions
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::SFVec2f = {
  {
    "SFVec2f", JSCLASS_HAS_PRIVATE, NULL, NULL,
    SFVec2fHandler::get, SFVec2fHandler::set,
    NULL, NULL, NULL,
    SFVec2fHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  SFVec2fFunctions
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::SFVec3f = {
  {
    "SFVec3f", JSCLASS_HAS_PRIVATE, NULL, NULL,
    SFVec3fHandler::get, SFVec3fHandler::set,
    NULL, NULL, NULL,
    SFVec3fHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  SFVec3fFunctions
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::SFVec3d = {
  {
    "SFVec3d", JSCLASS_HAS_PRIVATE, NULL, NULL,
    SFVec3dHandler::get, SFVec3dHandler::set,
    NULL, NULL, NULL,
    SFVec3dHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  SFVec3dFunctions
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::MFColor = {
  {
    "MFColor", JSCLASS_HAS_PRIVATE, NULL, NULL,
    MFColorHandler::get, MFColorHandler::set,
    NULL, NULL, NULL,
    MFColorHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  MFFunctions,
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::MFFloat = {
  {
    "MFFloat", JSCLASS_HAS_PRIVATE, NULL, NULL,
    MFFloatHandler::get, MFFloatHandler::set,
    NULL, NULL, NULL,
    MFFloatHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  MFFunctions,
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::MFInt32 = {
  {
    "MFInt32", JSCLASS_HAS_PRIVATE, NULL, NULL,
    MFInt32Handler::get, MFInt32Handler::set,
    NULL, NULL, NULL,
    MFInt32Handler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  MFFunctions,
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::MFNode = {
  {
    "MFNode", JSCLASS_HAS_PRIVATE, NULL, NULL,
    MFNodeHandler::get, MFNodeHandler::set,
    NULL, NULL, NULL,
    MFNodeHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  MFFunctions,
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::MFRotation = {
  {
    "MFRotation", JSCLASS_HAS_PRIVATE, NULL, NULL,
    MFRotationHandler::get, MFRotationHandler::set,
    NULL, NULL, NULL,
    MFRotationHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  MFFunctions,
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::MFString = {
  {
    "MFString", JSCLASS_HAS_PRIVATE, NULL, NULL,
    MFStringHandler::get, MFStringHandler::set,
    NULL, NULL, NULL,
    MFStringHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  MFFunctions,
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::MFTime = {
  {
    "MFTime", JSCLASS_HAS_PRIVATE, NULL, NULL,
    MFTimeHandler::get, MFTimeHandler::set,
    NULL, NULL, NULL,
    MFTimeHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  MFFunctions,
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::MFVec2f = {
  {
    "MFVec2f", JSCLASS_HAS_PRIVATE, NULL, NULL,
    MFVec2fHandler::get, MFVec2fHandler::set,
    NULL, NULL, NULL,
    MFVec2fHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  MFFunctions,
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::MFVec3f = {
  {
    "MFVec3f", JSCLASS_HAS_PRIVATE, NULL, NULL,
    MFVec3fHandler::get, MFVec3fHandler::set,
    NULL, NULL, NULL,
    MFVec3fHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  MFFunctions,
};

CoinVrmlJs::ClassDescriptor CoinVrmlJs::MFVec3d = {
  {
    "MFVec3d", JSCLASS_HAS_PRIVATE, NULL, NULL,
    MFVec3dHandler::get, MFVec3dHandler::set,
    NULL, NULL, NULL,
    MFVec3dHandler::destructor,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
  },
  MFFunctions,
};


CoinVrmlJs::ClassDescriptor * CLASSDESCRIPTORS[] = {
  &CoinVrmlJs::SFColor, &CoinVrmlJs::SFNode, &CoinVrmlJs::SFRotation,
  &CoinVrmlJs::SFVec2f, &CoinVrmlJs::SFVec3f, &CoinVrmlJs::SFVec3d,
  &CoinVrmlJs::MFColor,
  &CoinVrmlJs::MFFloat, &CoinVrmlJs::MFInt32, &CoinVrmlJs::MFNode,
  &CoinVrmlJs::MFRotation, &CoinVrmlJs::MFString, &CoinVrmlJs::MFTime,
  &CoinVrmlJs::MFVec2f, &CoinVrmlJs::MFVec3f, &CoinVrmlJs::MFVec3d
};

// *************************************************************************

// cleans up static / one-off resource allocations
static void
js_vrmlclasses_cleanup(void)
{
  delete garbagecollectedobjects;
  delete nodesensorstobedeleted;

  garbagecollectedobjects = NULL;
  nodesensorstobedeleted = NULL;
}

// *************************************************************************
// helper function to add all classes to engine

void
JS_addVRMLclasses(SoJavaScriptEngine * engine)
{
  // init static data
  if (garbagecollectedobjects == NULL) {
    garbagecollectedobjects = new SbList <JSObject *>;
    nodesensorstobedeleted = new SbList <SoNodeSensor *>;
    coin_atexit((coin_atexit_f *)js_vrmlclasses_cleanup, CC_ATEXIT_NORMAL);

    // set up default function stubs for Spidermonkey classes we
    // make. must be done at runtime to avoid calling spidermonkey()
    // early (i.e. not on demand).
    const size_t NRELEMENTS = sizeof(CLASSDESCRIPTORS) / sizeof(CLASSDESCRIPTORS[0]);
    for (size_t i=0; i < NRELEMENTS; i++) {
      CoinVrmlJs::ClassDescriptor * desc = CLASSDESCRIPTORS[i];
      desc->cls.addProperty = spidermonkey()->JS_PropertyStub;
      desc->cls.delProperty = spidermonkey()->JS_PropertyStub;
      desc->cls.enumerate = spidermonkey()->JS_EnumerateStub;
      desc->cls.resolve = spidermonkey()->JS_ResolveStub;
      desc->cls.convert = spidermonkey()->JS_ConvertStub;
    }
  }

  // Bool
  engine->addHandler(
    SoSFBool::getClassTypeId(), NULL,
    SFBool_field2jsval, SFBool_jsval2field);

  // Color
  engine->addHandler(
    SoSFColor::getClassTypeId(), SFColor_init,
    SFColor_field2jsval, SFColor_jsval2field);
  engine->addHandler(
    SoMFColor::getClassTypeId(),
    MFColorHandler::init,
    MFColorHandler::field2jsval,
    MFColorHandler::jsval2field);

  // Float
  engine->addHandler(
    SoSFFloat::getClassTypeId(), NULL,
    SFFloat_field2jsval, SFFloat_jsval2field);
  engine->addHandler(
    SoMFFloat::getClassTypeId(),
    MFFloatHandler::init,
    MFFloatHandler::field2jsval,
    MFFloatHandler::jsval2field);

  // Int32
  engine->addHandler(
    SoSFInt32::getClassTypeId(), NULL,
    SFInt32_field2jsval, SFInt32_jsval2field);
  engine->addHandler(
    SoMFInt32::getClassTypeId(),
    MFInt32Handler::init,
    MFInt32Handler::field2jsval,
    MFInt32Handler::jsval2field);

  // Enum
  engine->addHandler(
    SoSFEnum::getClassTypeId(), NULL,
    SFEnum_field2jsval, SFEnum_jsval2field);

  // Node
  engine->addHandler(
    SoSFNode::getClassTypeId(), SFNode_init,
    SFNode_field2jsval, SFNode_jsval2field);
  engine->addHandler(
    SoMFNode::getClassTypeId(),
    MFNodeHandler::init,
    MFNodeHandler::field2jsval,
    MFNodeHandler::jsval2field);

  // Rotation
  engine->addHandler(
    SoSFRotation::getClassTypeId(), SFRotation_init,
    SFRotation_field2jsval, SFRotation_jsval2field);
  engine->addHandler(
    SoMFRotation::getClassTypeId(),
    MFRotationHandler::init,
    MFRotationHandler::field2jsval,
    MFRotationHandler::jsval2field);

  // String
  engine->addHandler(
    SoSFString::getClassTypeId(), NULL,
    SFString_field2jsval, SFString_jsval2field);
  engine->addHandler(
    SoMFString::getClassTypeId(),
    MFStringHandler::init,
    MFStringHandler::field2jsval,
    MFStringHandler::jsval2field);

  // Time
  engine->addHandler(
    SoSFTime::getClassTypeId(), NULL,
    SFTime_field2jsval, SFTime_jsval2field);
  engine->addHandler(
    SoMFTime::getClassTypeId(),
    MFTimeHandler::init,
    MFTimeHandler::field2jsval,
    MFTimeHandler::jsval2field);

  // Vec2f
  engine->addHandler(
    SoSFVec2f::getClassTypeId(), SFVec2f_init,
    SFVec2f_field2jsval, SFVec2f_jsval2field);
  engine->addHandler(
    SoMFVec2f::getClassTypeId(),
    MFVec2fHandler::init,
    MFVec2fHandler::field2jsval,
    MFVec2fHandler::jsval2field);

  // Vec3f
  engine->addHandler(
    SoSFVec3f::getClassTypeId(), SFVec3f_init,
    SFVec3f_field2jsval, SFVec3f_jsval2field);
  engine->addHandler(
    SoMFVec3f::getClassTypeId(),
    MFVec3fHandler::init,
    MFVec3fHandler::field2jsval,
    MFVec3fHandler::jsval2field);

  // Vec3d
  engine->addHandler(
    SoSFVec3d::getClassTypeId(), SFVec3d_init,
    SFVec3d_field2jsval, SFVec3d_jsval2field);
  engine->addHandler(
    SoMFVec3d::getClassTypeId(),
    MFVec3dHandler::init,
    MFVec3dHandler::field2jsval,
    MFVec3dHandler::jsval2field);
}

#endif // HAVE_VRML97 && COIN_HAVE_JAVASCRIPT
