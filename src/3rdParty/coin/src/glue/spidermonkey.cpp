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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* !HAVE_CONFIG_H */

#ifdef COIN_HAVE_JAVASCRIPT

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
                                                      
#if !defined(SPIDERMONKEY_RUNTIME_LINKING) && defined(HAVE_SPIDERMONKEY_VIA_LINKTIME_LINKING)
#ifdef _WIN32
 #define XP_WIN
 #ifdef HAVE_WINDOWS_H
  #include <windows.h>
 #else
  #error Cannot compile SpiderMonkey support without windows.h available.
 #endif /* !HAVE_WINDWOS_H */
#else /* For UNIX and Mac OS X */
 #define XP_UNIX 
#endif
#include <jsapi.h>
#endif /* !defined(SPIDERMONKEY_RUNTIME_LINKING) && defined(HAVE_SPIDERMONKEY_VIA_LINKTIME_LINKING) */

#include <Inventor/C/glue/spidermonkey.h>

#include <Inventor/C/glue/dl.h>
#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/tidbits.h>

#include "threads/threadsutilp.h"
#include "tidbitsp.h"

/* ********************************************************************** */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ******************************************************************** */

static SpiderMonkey_t * spidermonkey_instance = NULL;
static cc_libhandle spidermonkey_libhandle = NULL;
static SbBool spidermonkey_failed_to_load = FALSE;
static int spidermonkey_is_initializing = 0;

/* ******************************************************************** */

static SbBool
spidermonkey_debug(void)
{
  static int dbg = -1;
  if (dbg == -1) {
    const char * env = coin_getenv("COIN_DEBUG_SPIDERMONKEY_BINDING");
    dbg = (env && (atoi(env) > 0)) ? 1 : 0;
  }
  return dbg;
}

/* ******************************************************************** */

/* Cleans up at exit. */
static void
spidermonkey_cleanup(void)
{
#ifdef SPIDERMONKEY_RUNTIME_LINKING
  if (spidermonkey_libhandle) {
    cc_dl_close(spidermonkey_libhandle);
    spidermonkey_libhandle = NULL;
  }
#endif /* SPIDERMONKEY_RUNTIME_LINKING */

  assert(spidermonkey_instance);
  free(spidermonkey_instance);
  spidermonkey_instance = NULL;
  spidermonkey_failed_to_load = FALSE;
  spidermonkey_is_initializing = 0;
}

/* ******************************************************************** */

/* Implemented by using the singleton pattern. */
const SpiderMonkey_t *
spidermonkey(void)
{
  SpiderMonkey_t * sm;

  CC_SYNC_BEGIN(spidermonkey);

  if (spidermonkey_instance || spidermonkey_failed_to_load) { goto wrapperexit; }

  /* Detect recursive calls. */
  {
    assert(spidermonkey_is_initializing == 0);
    spidermonkey_is_initializing = 1;
  }


  /* First invocation, do initializations. */
  spidermonkey_instance = sm = (SpiderMonkey_t *)malloc(sizeof(SpiderMonkey_t));
  /* FIXME: handle out-of-memory on malloc(). 20000928 mortene. */
  (void)coin_atexit((coin_atexit_f*) spidermonkey_cleanup, CC_ATEXIT_DYNLIBS);

  /* Be optimistic. */
  sm->available = 1;

#ifdef SPIDERMONKEY_RUNTIME_LINKING

  {
    unsigned int idx;

    /* FIXME: there's a configure mortene. */
    const char * possiblelibnames[] = {
      NULL, /* is set below */
      /* Microsoft Windows DLL name */
      "js32",

      /* UNIX-style names (SpiderMonkey compiled from source) */
      "js", "libjs", "libjs.so", "libjs.dylib",

      /* Debian package 'spidermonkey-bin' (SpiderMonkey v1.5) */
      "libsmjs.so.1",

      /* terminator */
      NULL
    };
    possiblelibnames[0] = coin_getenv("COIN_SPIDERMONKEY_LIBNAME");
    idx = possiblelibnames[0] ? 0 : 1;

    while (!spidermonkey_libhandle && possiblelibnames[idx]) {
      if (spidermonkey_debug()) {
        cc_debugerror_postinfo("spidermonkey", "Trying to dynamically load library '%s'", 
                               possiblelibnames[idx]);
      }
      spidermonkey_libhandle = cc_dl_open(possiblelibnames[idx]);
      idx++;
    }

    if (!spidermonkey_libhandle) {
      sm->available = 0;
      spidermonkey_failed_to_load = 1;
      if (spidermonkey_debug()) {
        cc_debugerror_postinfo("spidermonkey", "SpiderMonkey library failed to load.");
      }
      goto wrapperexit;
    }

    if (spidermonkey_debug()) {      
      if (spidermonkey_failed_to_load) {
        /* FIXME: This message will never be reached, as far as I can see (20050906 handegar) */
        cc_debugerror_postinfo("spidermonkey", "Found no SpiderMonkey library on system.");
      }
      else {
        cc_debugerror_postinfo("spidermonkey",
                               "Dynamically loaded SpiderMonkey library as '%s'.",
                               possiblelibnames[idx-1]);
      }
    }
  }


  /* Define macro for grabbing function symbols. Casting the type is
     necessary for this file to be compatible with C++ compilers. */
  #define REGISTER_FUNC(_funcname_, _funcsig_) \
          sm->_funcname_ = (_funcsig_)cc_dl_sym(spidermonkey_libhandle, SO__QUOTE(_funcname_)); \
          assert(sm->_funcname_)

  /* Some functions in SpiderMonkey may have a symbol name different
     from the API name. */
  #define REGISTER_FUNC_ALTERNATE(_funcname_, _altname_, _funcsig_) \
          sm->_funcname_ = (_funcsig_)cc_dl_sym(spidermonkey_libhandle, SO__QUOTE(_funcname_)); \
          if (sm->_funcname_ == NULL) { sm->_funcname_ = (_funcsig_)cc_dl_sym(spidermonkey_libhandle, SO__QUOTE(_altname_)); } \
          assert(sm->_funcname_)

#elif defined(HAVE_SPIDERMONKEY_VIA_LINKTIME_LINKING) /* static linking */

  #define REGISTER_FUNC(_funcname_, _funcsig_) \
          sm->_funcname_ = (_funcsig_)_funcname_; \
          assert(sm->_funcname_)

  #define REGISTER_FUNC_ALTERNATE(_funcname_, _altname_, _funcsig_) \
          REGISTER_FUNC(_funcname_, _funcsig_)

#else /* neither dynamic nor static linking */

  sm->available = 0;

  #define REGISTER_FUNC(_funcname_, _funcsig_) \
          sm->_funcname_ = NULL

  #define REGISTER_FUNC_ALTERNATE(_funcname_, _altname_, _funcsig_) \
          REGISTER_FUNC(_funcname_, _funcsig_)

#endif /* done setting up REGISTER_FUNC */

  REGISTER_FUNC(JS_GetImplementationVersion, JS_GetImplementationVersion_t);
  if (spidermonkey_debug() && sm->JS_GetImplementationVersion) {
    const char * version = sm->JS_GetImplementationVersion();
    cc_debugerror_postinfo("spidermonkey", "%s", version);
  }

  REGISTER_FUNC(JS_EvaluateScript, JS_EvaluateScript_t);
  REGISTER_FUNC(JS_ValueToString, JS_ValueToString_t);
  REGISTER_FUNC(JS_GetStringBytes, JS_GetStringBytes_t);
  REGISTER_FUNC(JS_SetProperty, JS_SetProperty_t);
  REGISTER_FUNC(JS_GetProperty, JS_GetProperty_t);
  REGISTER_FUNC(JS_CallFunctionName, JS_CallFunctionName_t);
  REGISTER_FUNC(JS_CallFunctionValue, JS_CallFunctionValue_t);
  REGISTER_FUNC(JS_ConstructObjectWithArguments, JS_ConstructObjectWithArguments_t);
  REGISTER_FUNC_ALTERNATE(JS_NewRuntime, JS_Init, JS_NewRuntime_t);
  REGISTER_FUNC_ALTERNATE(JS_DestroyRuntime, JS_Finish, JS_DestroyRuntime_t);
  REGISTER_FUNC(JS_NewContext, JS_NewContext_t);
  REGISTER_FUNC(JS_DestroyContext, JS_DestroyContext_t);
  REGISTER_FUNC(JS_ShutDown, JS_ShutDown_t);
  REGISTER_FUNC(JS_SetErrorReporter, JS_SetErrorReporter_t);
  REGISTER_FUNC(JS_PropertyStub, JS_PropertyStub_t);
  REGISTER_FUNC(JS_EnumerateStub, JS_EnumerateStub_t);
  REGISTER_FUNC(JS_ResolveStub, JS_ResolveStub_t);
  REGISTER_FUNC(JS_ConvertStub, JS_ConvertStub_t);
  REGISTER_FUNC(JS_FinalizeStub, JS_FinalizeStub_t);
  REGISTER_FUNC(JS_NewObject, JS_NewObject_t);
  REGISTER_FUNC(JS_InitStandardClasses, JS_InitStandardClasses_t);
  REGISTER_FUNC(JS_DefineObject, JS_DefineObject_t);
  REGISTER_FUNC(JS_DefineProperties, JS_DefineProperties_t);
  REGISTER_FUNC(JS_GetPrivate, JS_GetPrivate_t);
  REGISTER_FUNC(JS_SetPrivate, JS_SetPrivate_t);
  REGISTER_FUNC(JS_NewFunction, JS_NewFunction_t);
  REGISTER_FUNC(JS_GetFunctionObject, JS_GetFunctionObject_t);
  REGISTER_FUNC(JS_GetParent, JS_GetParent_t);
  REGISTER_FUNC(JS_SetParent, JS_SetParent_t);
  REGISTER_FUNC(JS_DefineFunctions, JS_DefineFunctions_t);
  REGISTER_FUNC(JS_NewStringCopyZ, JS_NewStringCopyZ_t);
  REGISTER_FUNC(JS_TypeOfValue, JS_TypeOfValue_t);
  REGISTER_FUNC(JS_GetTypeName, JS_GetTypeName_t);
  REGISTER_FUNC(JS_InstanceOf, JS_InstanceOf_t);
  REGISTER_FUNC(JS_InitClass, JS_InitClass_t);
  REGISTER_FUNC(JS_NewDoubleValue, JS_NewDoubleValue_t);
  REGISTER_FUNC(JS_SetContextPrivate, JS_SetContextPrivate_t);
  REGISTER_FUNC(JS_GetContextPrivate, JS_GetContextPrivate_t);
  REGISTER_FUNC(JS_ValueToBoolean, JS_ValueToBoolean_t);
  REGISTER_FUNC(JS_ValueToNumber, JS_ValueToNumber_t);
  REGISTER_FUNC(JS_NewArrayObject, JS_NewArrayObject_t);
  REGISTER_FUNC(JS_GetArrayLength, JS_GetArrayLength_t);
  REGISTER_FUNC(JS_SetArrayLength, JS_SetArrayLength_t);
  REGISTER_FUNC(JS_HasArrayLength, JS_HasArrayLength_t);
  REGISTER_FUNC(JS_GetElement, JS_GetElement_t);
  REGISTER_FUNC(JS_SetElement, JS_SetElement_t);
  REGISTER_FUNC(JS_AddRoot, JS_AddRoot_t);
  REGISTER_FUNC(JS_RemoveRoot, JS_RemoveRoot_t);
  REGISTER_FUNC(JS_GetStringLength, JS_GetStringLength_t);
  REGISTER_FUNC(JS_LookupProperty, JS_LookupProperty_t);
  REGISTER_FUNC(JS_DefineProperty, JS_DefineProperty_t);
  REGISTER_FUNC(JS_CompileFile, JS_CompileFile_t);
  REGISTER_FUNC(JS_ValueToObject, JS_ValueToObject_t);
  REGISTER_FUNC(JS_ExecuteScript, JS_ExecuteScript_t);
  REGISTER_FUNC(JS_IsExceptionPending, JS_IsExceptionPending_t);
  REGISTER_FUNC(JS_GetPendingException, JS_GetPendingException_t);
  REGISTER_FUNC(JS_SetPendingException, JS_SetPendingException_t);
  REGISTER_FUNC(JS_ClearPendingException, JS_ClearPendingException_t);
  REGISTER_FUNC(JS_NewDouble, JS_NewDouble_t);
  REGISTER_FUNC(JS_CallFunction, JS_CallFunction_t);
  REGISTER_FUNC(JS_ValueToFunction, JS_ValueToFunction_t);
  REGISTER_FUNC(JS_ReportError, JS_ReportError_t);
  REGISTER_FUNC(JS_IsArrayObject, JS_IsArrayObject_t);
  REGISTER_FUNC(JS_ObjectIsFunction, JS_ObjectIsFunction_t);
  REGISTER_FUNC(JS_ValueToECMAInt32, JS_ValueToECMAInt32_t);
  REGISTER_FUNC(JS_DefineFunction, JS_DefineFunction_t);
  REGISTER_FUNC(JS_GetGlobalObject, JS_GetGlobalObject_t);
  REGISTER_FUNC(JS_SetGCCallback, JS_SetGCCallback_t);
  REGISTER_FUNC(JS_MaybeGC, JS_MaybeGC_t);
  REGISTER_FUNC(JS_GC, JS_GC_t);
  REGISTER_FUNC(JS_IsRunning, JS_IsRunning_t);
  REGISTER_FUNC(JS_DeleteProperty, JS_DeleteProperty_t);
  REGISTER_FUNC(JS_CompileScript, JS_CompileScript_t);
  REGISTER_FUNC(JS_GetNaNValue, JS_GetNaNValue_t);
  REGISTER_FUNC(JS_GetNegativeInfinityValue, JS_GetNegativeInfinityValue_t);
  REGISTER_FUNC(JS_GetPositiveInfinityValue, JS_GetPositiveInfinityValue_t);
  REGISTER_FUNC(JS_GetEmptyStringValue, JS_GetEmptyStringValue_t);
  REGISTER_FUNC(JS_SetPropertyAttributes, JS_SetPropertyAttributes_t);
  REGISTER_FUNC(JS_GetPropertyAttributes, JS_GetPropertyAttributes_t);
  REGISTER_FUNC(JS_GetClass, JS_GetClass_t);
  REGISTER_FUNC(JS_GetPrototype, JS_GetPrototype_t);
  REGISTER_FUNC(JS_SetPrototype, JS_SetPrototype_t);
  REGISTER_FUNC(JS_CompareStrings, JS_CompareStrings_t);
  REGISTER_FUNC(JS_GetOptions, JS_GetOptions_t);
  REGISTER_FUNC(JS_SetOptions, JS_SetOptions_t);
  REGISTER_FUNC(JS_ToggleOptions, JS_ToggleOptions_t);
  REGISTER_FUNC(JS_Enumerate, JS_Enumerate_t);
  REGISTER_FUNC(JS_IdToValue, JS_IdToValue_t);
  REGISTER_FUNC(JS_GetFunctionName, JS_GetFunctionName_t);
  REGISTER_FUNC(JS_GetConstructor, JS_GetConstructor_t);
  REGISTER_FUNC(JS_DestroyIdArray, JS_DestroyIdArray_t);

wrapperexit:
  CC_SYNC_END(spidermonkey);
  return spidermonkey_instance;
}

#undef REGISTER_FUNC

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !COIN_HAVE_JAVASCRIPT */
