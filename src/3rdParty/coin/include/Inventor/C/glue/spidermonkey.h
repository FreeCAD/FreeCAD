#ifndef COIN_GLUE_SPIDERMONKEY_H
#define COIN_GLUE_SPIDERMONKEY_H

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

#include <Inventor/C/basic.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if 0 /* to get proper auto-indentation in emacs */
}
#endif /* emacs indentation */


/*
  This is used to detect whether the 'jsapi.h' was included by the
  user or not. The JSVERSION_IS_ECMA is defined in the 'jspubtd.h'
  file in the SpiderMonkey header directory. 
*/
#ifndef JSVERSION_IS_ECMA


/*
   Structs and defines.
*/
typedef int JSBool;
typedef intptr_t jsword;
typedef jsword jsval;
typedef jsword jsid;
typedef int intN;
typedef unsigned int uintN;
typedef uint16_t jschar;

typedef int32_t jsrefcount;
typedef uint8_t jsbytecode;
typedef uint32_t JSHashNumber;
typedef uint32_t jsatomid;

typedef enum JSType {
  JSTYPE_VOID,
  JSTYPE_OBJECT,
  JSTYPE_FUNCTION,
  JSTYPE_STRING,
  JSTYPE_NUMBER,
  JSTYPE_BOOLEAN,
  JSTYPE_LIMIT
} JSType;

typedef enum JSAccessMode {
  JSACC_PROTO = 0,
  JSACC_PARENT = 1,
  JSACC_IMPORT = 2,
  JSACC_WATCH = 3,
  JSACC_READ = 4,
  JSACC_WRITE = 8,
  JSACC_LIMIT
} JSAccessMode;

typedef enum JSGCStatus {
  JSGC_BEGIN,
  JSGC_END,
  JSGC_MARK_END,
  JSGC_FINALIZE_END
} JSGCStatus;

struct JSIdArray {
    int32_t length;
    jsid vector[1];
};

typedef void JSRuntime;
typedef void JSContext;
typedef void JSObject;
typedef void JSObjectOps;
typedef void JSXDRState;
typedef void JSString;
typedef struct JSClass JSClass;
typedef struct JSPropertySpec JSPropertySpec;
typedef int JSVersion;
typedef void JSFunction;
typedef struct JSFunctionSpec JSFunctionSpec;
typedef struct JSErrorReport JSErrorReport;
typedef void JSScript;

#define JS_DLL_CALLBACK /* FIXME: set up this define properly. 20050601 mortene. */

typedef JSBool (* JS_DLL_CALLBACK JSPropertyOp)(JSContext *, JSObject *, jsval, jsval *);
typedef JSBool (* JS_DLL_CALLBACK JSEnumerateOp)(JSContext *, JSObject *);
typedef JSBool (* JS_DLL_CALLBACK JSResolveOp)(JSContext *, JSObject *, jsval);
typedef JSBool (* JS_DLL_CALLBACK JSConvertOp)(JSContext *, JSObject *, JSType, jsval *);
typedef void (* JS_DLL_CALLBACK JSFinalizeOp)(JSContext *, JSObject *);
typedef JSObjectOps * (* JS_DLL_CALLBACK JSGetObjectOps)(JSContext *, JSClass *);
typedef JSBool (* JS_DLL_CALLBACK JSCheckAccessOp)(JSContext *, JSObject *, jsval, JSAccessMode, jsval *);
typedef JSBool (* JS_DLL_CALLBACK JSNative)(JSContext *, JSObject *, uintN, jsval *, jsval *);
typedef JSBool (* JS_DLL_CALLBACK JSXDRObjectOp)(JSXDRState *, JSObject **);
typedef JSBool (* JS_DLL_CALLBACK JSHasInstanceOp)(JSContext *, JSObject *, jsval, JSBool *);
typedef uint32_t (* JS_DLL_CALLBACK JSMarkOp)(JSContext *, JSObject *, void *);

struct JSClass {
  const char * name;
  uint32_t flags;
  JSPropertyOp addProperty;
  JSPropertyOp delProperty;
  JSPropertyOp getProperty;
  JSPropertyOp setProperty;
  JSEnumerateOp enumerate;
  JSResolveOp resolve;
  JSConvertOp convert;
  JSFinalizeOp finalize;
  JSGetObjectOps getObjectOps;
  JSCheckAccessOp checkAccess;
  JSNative call;
  JSNative construct;
  JSXDRObjectOp xdrObject;
  JSHasInstanceOp hasInstance;
  JSMarkOp mark;
  jsword spare;
};

struct JSPropertySpec {
  const char * name;
  int8_t tinyid;
  uint8_t flags;
  JSPropertyOp getter;
  JSPropertyOp setter;
};

struct JSFunctionSpec {
  const char *name;
  JSNative call;
  uint8_t nargs;
  uint8_t flags;
  uint16_t extra;
};

struct JSErrorReport {
  const char * filename;
  uintN lineno;
  const char * linebuf;
  const char * tokenptr;
  const jschar * uclinebuf;
  const jschar * uctokenptr;
  uintN flags;
  uintN errorNumber;
  const jschar * ucmessage;
  const jschar ** messageArgs;
};


/* Defines and macros. ************************************************** */

#define JSVAL_OBJECT 0x0
#define JSVAL_INT 0x1
#define JSVAL_DOUBLE 0x2
#define JSVAL_STRING 0x4
#define JSVAL_BOOLEAN 0x6

#define JS_BIT(n)       ((uint32_t)1 << (n))
#define JS_BITMASK(n)   (JS_BIT(n) - 1)

#define JSVAL_TAGBITS 3
#define JSVAL_TAGMASK           JS_BITMASK(JSVAL_TAGBITS)
#define JSVAL_TAG(v)            ((v) & JSVAL_TAGMASK)
#define JSVAL_SETTAG(v,t) ((v) | (t))
#define JSVAL_CLRTAG(v)         ((v) & ~(jsval)JSVAL_TAGMASK)

#define JSVAL_IS_PRIMITIVE(v)   (!JSVAL_IS_OBJECT(v) || JSVAL_IS_NULL(v))
#define JSVAL_IS_OBJECT(v)      (JSVAL_TAG(v) == JSVAL_OBJECT)
#define JSVAL_IS_NUMBER(v)      (JSVAL_IS_INT(v) || JSVAL_IS_DOUBLE(v))
#define JSVAL_IS_INT(v)         (((v) & JSVAL_INT) && (v) != JSVAL_VOID)
#define JSVAL_IS_DOUBLE(v)      (JSVAL_TAG(v) == JSVAL_DOUBLE)
#define JSVAL_IS_STRING(v)      (JSVAL_TAG(v) == JSVAL_STRING)
#define JSVAL_IS_BOOLEAN(v)     (JSVAL_TAG(v) == JSVAL_BOOLEAN)
#define JSVAL_IS_NULL(v)        ((v) == JSVAL_NULL)
#define JSVAL_IS_VOID(v)        ((v) == JSVAL_VOID)

#define BOOLEAN_TO_JSVAL(b) JSVAL_SETTAG((jsval)(b) << JSVAL_TAGBITS, JSVAL_BOOLEAN)
#define JSVAL_TO_BOOLEAN(v) ((JSBool)((v) >> JSVAL_TAGBITS))

#define JSVAL_INT_BITS          31
#define JSVAL_INT_POW2(n)       ((jsval)1 << (n))
#define JSVAL_INT_MIN           ((jsval)1 - JSVAL_INT_POW2(30))
#define JSVAL_INT_MAX           (JSVAL_INT_POW2(30) - 1)
#define INT_FITS_IN_JSVAL(i)    ((uint32_t)((i)+JSVAL_INT_MAX) <= 2*JSVAL_INT_MAX)
#define JSVAL_TO_INT(v)         ((int32_t)(v) >> 1)
#define INT_TO_JSVAL(i)         (((jsval)(i) << 1) | JSVAL_INT)

#define JSVAL_TO_GCTHING(v)     ((void *)JSVAL_CLRTAG(v))
#define JSVAL_TO_OBJECT(v)      ((JSObject *)JSVAL_TO_GCTHING(v))
#define JSVAL_TO_DOUBLE(v)      ((double *)JSVAL_TO_GCTHING(v))
#define JSVAL_TO_STRING(v)      ((JSString *)JSVAL_TO_GCTHING(v))
#define OBJECT_TO_JSVAL(obj)    ((jsval)(obj))
#define DOUBLE_TO_JSVAL(dp)     JSVAL_SETTAG((jsval)(dp), JSVAL_DOUBLE)
#define STRING_TO_JSVAL(str)    JSVAL_SETTAG((jsval)(str), JSVAL_STRING)
#define JSVAL_TO_PRIVATE(v)     ((void *)((v) & ~JSVAL_INT))
#define PRIVATE_TO_JSVAL(p)     ((jsval)(p) | JSVAL_INT)

#define JSPROP_ENUMERATE 0x01
#define JSPROP_READONLY 0x02
#define JSPROP_PERMANENT 0x04
#define JSPROP_EXPORTED 0x08
#define JSPROP_GETTER 0x10
#define JSPROP_SETTER 0x20
#define JSPROP_SHARED 0x40
#define JSPROP_INDEX 0x80

#define JS_FALSE (int)0
#define JS_TRUE (int)1

#define JSVAL_VOID   INT_TO_JSVAL(0 - JSVAL_INT_POW2(30))
#define JSVAL_NULL   OBJECT_TO_JSVAL(0)
#define JSVAL_ZERO   INT_TO_JSVAL(0)
#define JSVAL_ONE    INT_TO_JSVAL(1)
#define JSVAL_FALSE  BOOLEAN_TO_JSVAL(JS_FALSE)
#define JSVAL_TRUE   BOOLEAN_TO_JSVAL(JS_TRUE)

#define JSCLASS_HAS_PRIVATE             (1<<0)
#define JSCLASS_NEW_ENUMERATE           (1<<1)
#define JSCLASS_NEW_RESOLVE             (1<<2)
#define JSCLASS_PRIVATE_IS_NSISUPPORTS  (1<<3)
#define JSCLASS_SHARE_ALL_PROPERTIES    (1<<4)
#define JSCLASS_NEW_RESOLVE_GETS_START  (1<<5)

#define JSFUN_BOUND_METHOD 0x40

#define JSOPTION_STRICT                 JS_BIT(0)
#define JSOPTION_WERROR                 JS_BIT(1)
#define JSOPTION_VAROBJFIX              JS_BIT(2)
#define JSOPTION_PRIVATE_IS_NSISUPPORTS JS_BIT(3)
#define JSOPTION_COMPILE_N_GO           JS_BIT(4)


/* Function typedefs. *************************************************** */

typedef void (* JS_DLL_CALLBACK JSErrorReporter)(JSContext *, const char *, JSErrorReport *);
typedef JSBool (* JS_DLL_CALLBACK JSGCCallback)(JSContext *, JSGCStatus);

#endif /* !JSVERSION_IS_ECMA */

typedef JSBool (* JS_EvaluateScript_t)(JSContext *, JSObject *, const char *, uintN, const char *, uintN, jsval *);
typedef JSString * (* JS_ValueToString_t)(JSContext *, jsval);
typedef char * (* JS_GetStringBytes_t)(JSString *);
typedef JSBool (* JS_SetProperty_t)(JSContext *, JSObject *, const char *, jsval *);
typedef JSBool (* JS_GetProperty_t)(JSContext *, JSObject *, const char *, jsval *);
typedef JSBool (* JS_CallFunctionName_t)(JSContext *, JSObject *, const char *, uintN, jsval *, jsval *);
typedef JSBool (* JS_CallFunctionValue_t)(JSContext *, JSObject *, jsval, uintN, jsval *, jsval *);
typedef JSObject * (* JS_ConstructObjectWithArguments_t)(JSContext *, JSClass *, JSObject *, JSObject *, uintN, jsval *);
typedef JSRuntime * (* JS_NewRuntime_t)(uint32_t);
typedef void (* JS_DestroyRuntime_t)(JSRuntime *);
typedef JSContext * (* JS_NewContext_t)(JSRuntime *, size_t);
typedef void (* JS_DestroyContext_t)(JSContext *);
typedef void (* JS_ShutDown_t)(void);
typedef JSObject * (* JS_NewObject_t)(JSContext *, JSClass *, JSObject *, JSObject *);
typedef JSBool (* JS_InitStandardClasses_t)(JSContext *, JSObject *);
typedef JSErrorReporter (* JS_SetErrorReporter_t)(JSContext *, JSErrorReporter);
typedef JSBool (* JS_PropertyStub_t)(JSContext *, JSObject *, jsval, jsval *);
typedef JSBool (* JS_EnumerateStub_t)(JSContext *, JSObject *);
typedef JSBool (* JS_ResolveStub_t)(JSContext *, JSObject *, jsval);
typedef JSBool (* JS_ConvertStub_t)(JSContext *, JSObject *, JSType, jsval *);
typedef void (* JS_FinalizeStub_t)(JSContext *, JSObject *);
typedef const char * (* JS_GetImplementationVersion_t)(void);
typedef void * (* JS_GetPrivate_t)(JSContext *, JSObject *);
typedef JSBool (* JS_SetPrivate_t)(JSContext *, JSObject *, void *);
typedef JSFunction * (* JS_NewFunction_t)(JSContext *, JSNative, uintN, uintN flags, JSObject *, const char *);
typedef JSObject * (* JS_GetFunctionObject_t)(JSFunction *);
typedef JSObject * (* JS_DefineObject_t)(JSContext *, JSObject *, const char *, JSClass *, JSObject *, uintN);
typedef JSBool (* JS_DefineProperties_t)(JSContext *, JSObject *, JSPropertySpec *);
typedef JSObject * (* JS_GetParent_t)(JSContext *, JSObject *);
typedef JSBool (* JS_SetParent_t)(JSContext *, JSObject *, JSObject *);
typedef JSBool (* JS_DefineFunctions_t)(JSContext *, JSObject *, JSFunctionSpec *);
typedef JSString * (* JS_NewStringCopyZ_t)(JSContext *, const char *);
typedef JSType (* JS_TypeOfValue_t)(JSContext *, jsval);
typedef const char * (* JS_GetTypeName_t)(JSContext *, JSType);
typedef JSBool (* JS_InstanceOf_t)(JSContext *, JSObject *, JSClass *, jsval *);
typedef JSObject * (* JS_InitClass_t)(JSContext *, JSObject *, JSObject *, JSClass *,
                                      JSNative, uintN, JSPropertySpec *, JSFunctionSpec *,
                                      JSPropertySpec *, JSFunctionSpec *);
typedef JSBool (* JS_NewDoubleValue_t)(JSContext *, double, jsval *);
typedef void * (* JS_GetContextPrivate_t)(JSContext *);
typedef void (* JS_SetContextPrivate_t)(JSContext *, void *);
typedef JSBool (* JS_ValueToBoolean_t)(JSContext *, jsval, JSBool *);
typedef JSBool (* JS_ValueToNumber_t)(JSContext *, jsval, double *);
typedef JSObject * (* JS_NewArrayObject_t)(JSContext *, int32_t, jsval *);
typedef JSBool (* JS_GetArrayLength_t)(JSContext *, JSObject *, uint32_t *);
typedef JSBool (* JS_SetArrayLength_t)(JSContext *, JSObject *, uint32_t);
typedef JSBool (* JS_HasArrayLength_t)(JSContext *, JSObject *, uint32_t *);
typedef JSBool (* JS_GetElement_t)(JSContext *, JSObject *, int32_t, jsval *);
typedef JSBool (* JS_SetElement_t)(JSContext *, JSObject *, int32_t, jsval *);
typedef JSBool (* JS_AddRoot_t)(JSContext *, void *);
typedef JSBool (* JS_RemoveRoot_t)(JSContext *, void *);
typedef size_t (* JS_GetStringLength_t)(JSString *);
typedef JSBool (* JS_LookupProperty_t)(JSContext *, JSObject *, const char *, jsval *);
typedef JSBool (* JS_DefineProperty_t)(JSContext *, JSObject *, const char *, jsval, JSPropertyOp, JSPropertyOp, uintN);
typedef JSScript * (* JS_CompileFile_t)(JSContext *, JSObject *, const char *);
typedef JSBool (* JS_ValueToObject_t)(JSContext *, jsval, JSObject **);
typedef JSBool (* JS_ExecuteScript_t)(JSContext *, JSObject *, JSScript *, jsval *);
typedef JSBool (* JS_IsExceptionPending_t)(JSContext *);
typedef JSBool (* JS_GetPendingException_t)(JSContext *, jsval *);
typedef void (* JS_SetPendingException_t)(JSContext *, jsval);
typedef void (* JS_ClearPendingException_t)(JSContext *);
typedef double * (* JS_NewDouble_t)(JSContext *, double);
typedef JSBool (* JS_CallFunction_t)(JSContext *, JSObject *, JSFunction *, uintN, jsval *, jsval *);
typedef JSFunction * (* JS_ValueToFunction_t)(JSContext *, jsval);
typedef void (* JS_ReportError_t)(JSContext *, const char *, ...);
typedef JSBool (* JS_IsArrayObject_t)(JSContext *, JSObject *);
typedef JSBool (* JS_ObjectIsFunction_t)(JSContext *, JSObject *);
typedef JSBool (* JS_ValueToECMAInt32_t)(JSContext *, jsval, int32_t *);
typedef JSFunction * (* JS_DefineFunction_t)(JSContext *, JSObject *, const char *, JSNative, uintN, uintN);
typedef JSObject * (* JS_GetGlobalObject_t)(JSContext *);
typedef JSGCCallback (* JS_SetGCCallback_t)(JSContext *, JSGCCallback);
typedef void (* JS_GC_t)(JSContext *);
typedef void (* JS_MaybeGC_t)(JSContext *);
typedef JSBool (* JS_IsRunning_t)(JSContext *);
typedef JSBool (* JS_DeleteProperty_t)(JSContext *, JSObject *, const char *);
typedef JSScript * (* JS_CompileScript_t)(JSContext *, JSObject *,
                                          const char *, size_t,
                                          const char *, uintN);
typedef jsval (* JS_GetNaNValue_t)(JSContext *);
typedef jsval (* JS_GetNegativeInfinityValue_t)(JSContext *);
typedef jsval (* JS_GetPositiveInfinityValue_t)(JSContext *);
typedef jsval (* JS_GetEmptyStringValue_t)(JSContext *);
typedef JSBool (* JS_SetPropertyAttributes_t)(JSContext *, JSObject *, const char *, uintN, JSBool *);
typedef JSBool (* JS_GetPropertyAttributes_t)(JSContext *, JSObject *, const char *, uintN *, JSBool *);
typedef JSClass * (* JS_GetClass_t)(JSObject *);
typedef JSObject * (* JS_GetPrototype_t)(JSContext *, JSObject *);
typedef JSObject * (* JS_SetPrototype_t)(JSContext *, JSObject *, JSObject *);
typedef intN (* JS_CompareStrings_t)(JSString *, JSString *);
typedef uint32_t (* JS_GetOptions_t)(JSContext *);
typedef uint32_t (* JS_SetOptions_t)(JSContext *, uint32_t);
typedef uint32_t (* JS_ToggleOptions_t)(JSContext *, uint32_t);
typedef struct JSIdArray * (* JS_Enumerate_t)(JSContext *, JSObject *);
typedef JSBool (* JS_IdToValue_t)(JSContext *, jsid, jsval *);
typedef const char * (* JS_GetFunctionName_t)(JSFunction *);
typedef JSObject * (* JS_GetConstructor_t)(JSContext *, JSObject *);
typedef void (* JS_DestroyIdArray_t)(JSContext *, struct JSIdArray *);


/* Access interface. **************************************************** */

typedef struct {
  int available;

  JS_CallFunctionName_t JS_CallFunctionName;
  JS_CallFunctionValue_t JS_CallFunctionValue;
  JS_ConstructObjectWithArguments_t JS_ConstructObjectWithArguments;
  JS_ConvertStub_t JS_ConvertStub;
  JS_DestroyContext_t JS_DestroyContext;
  JS_DestroyRuntime_t JS_DestroyRuntime;
  JS_EnumerateStub_t JS_EnumerateStub;
  JS_EvaluateScript_t JS_EvaluateScript;
  JS_FinalizeStub_t JS_FinalizeStub;
  JS_GetClass_t JS_GetClass;
  JS_GetImplementationVersion_t JS_GetImplementationVersion;
  JS_GetProperty_t JS_GetProperty;
  JS_GetStringBytes_t JS_GetStringBytes;
  JS_InitStandardClasses_t JS_InitStandardClasses;
  JS_NewContext_t JS_NewContext;
  JS_NewObject_t JS_NewObject;
  JS_NewRuntime_t JS_NewRuntime;
  JS_PropertyStub_t JS_PropertyStub;
  JS_ResolveStub_t JS_ResolveStub;
  JS_SetErrorReporter_t JS_SetErrorReporter;
  JS_SetProperty_t JS_SetProperty;
  JS_ShutDown_t JS_ShutDown;
  JS_ValueToString_t JS_ValueToString;
  JS_DefineObject_t JS_DefineObject;
  JS_DefineProperties_t JS_DefineProperties;
  JS_GetPrivate_t JS_GetPrivate;
  JS_SetPrivate_t JS_SetPrivate;
  JS_NewFunction_t JS_NewFunction;
  JS_GetFunctionObject_t JS_GetFunctionObject;
  JS_GetParent_t JS_GetParent;
  JS_SetParent_t JS_SetParent;
  JS_DefineFunctions_t JS_DefineFunctions;
  JS_NewStringCopyZ_t JS_NewStringCopyZ;
  JS_TypeOfValue_t JS_TypeOfValue;
  JS_GetTypeName_t JS_GetTypeName;
  JS_InstanceOf_t JS_InstanceOf;
  JS_InitClass_t JS_InitClass;
  JS_NewDoubleValue_t JS_NewDoubleValue;
  JS_GetContextPrivate_t JS_GetContextPrivate;
  JS_SetContextPrivate_t JS_SetContextPrivate;
  JS_ValueToBoolean_t JS_ValueToBoolean;
  JS_ValueToNumber_t JS_ValueToNumber;
  JS_NewArrayObject_t JS_NewArrayObject;
  JS_GetArrayLength_t JS_GetArrayLength;
  JS_SetArrayLength_t JS_SetArrayLength;
  JS_HasArrayLength_t JS_HasArrayLength;
  JS_GetElement_t JS_GetElement;
  JS_SetElement_t JS_SetElement;
  JS_AddRoot_t JS_AddRoot;
  JS_RemoveRoot_t JS_RemoveRoot;
  JS_GetStringLength_t JS_GetStringLength;
  JS_LookupProperty_t JS_LookupProperty;
  JS_DefineProperty_t JS_DefineProperty;
  JS_CompileFile_t JS_CompileFile;
  JS_ValueToObject_t JS_ValueToObject;
  JS_ExecuteScript_t JS_ExecuteScript;
  JS_IsExceptionPending_t JS_IsExceptionPending;
  JS_GetPendingException_t JS_GetPendingException;
  JS_SetPendingException_t JS_SetPendingException;
  JS_ClearPendingException_t JS_ClearPendingException;
  JS_NewDouble_t JS_NewDouble;
  JS_CallFunction_t JS_CallFunction;
  JS_ValueToFunction_t JS_ValueToFunction;
  JS_ReportError_t JS_ReportError;
  JS_IsArrayObject_t JS_IsArrayObject;
  JS_ObjectIsFunction_t JS_ObjectIsFunction;
  // Note: We use this function instead of JS_ValueToInt32() since the 
  // latter is buggy in versions of SpiderMonkey older than 2005-09-29, 
  // see Mozilla bug #284032.
  JS_ValueToECMAInt32_t JS_ValueToECMAInt32;
  JS_DefineFunction_t JS_DefineFunction;
  JS_GetGlobalObject_t JS_GetGlobalObject;
  JS_SetGCCallback_t JS_SetGCCallback;
  JS_GC_t JS_GC;
  JS_MaybeGC_t JS_MaybeGC;
  JS_IsRunning_t JS_IsRunning;
  JS_DeleteProperty_t JS_DeleteProperty;
  JS_CompileScript_t JS_CompileScript;
  JS_GetNaNValue_t JS_GetNaNValue;
  JS_GetNegativeInfinityValue_t JS_GetNegativeInfinityValue;
  JS_GetPositiveInfinityValue_t JS_GetPositiveInfinityValue;
  JS_GetEmptyStringValue_t JS_GetEmptyStringValue;
  JS_SetPropertyAttributes_t JS_SetPropertyAttributes;
  JS_GetPropertyAttributes_t JS_GetPropertyAttributes;
  JS_GetPrototype_t JS_GetPrototype;
  JS_SetPrototype_t JS_SetPrototype;
  JS_CompareStrings_t JS_CompareStrings;
  JS_GetOptions_t JS_GetOptions;
  JS_SetOptions_t JS_SetOptions;
  JS_ToggleOptions_t JS_ToggleOptions;
  JS_Enumerate_t JS_Enumerate;
  JS_IdToValue_t JS_IdToValue;
  JS_GetFunctionName_t JS_GetFunctionName;
  JS_GetConstructor_t JS_GetConstructor;
  JS_DestroyIdArray_t JS_DestroyIdArray;

} SpiderMonkey_t;

COIN_DLL_API const SpiderMonkey_t * spidermonkey(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !COIN_GLUE_SPIDERMONKEY_H */
