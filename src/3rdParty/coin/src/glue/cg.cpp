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

#include "glue/cg.h"
#include "coindefs.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <Inventor/C/basic.h>
#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/C/glue/dl.h>

#include "threads/threadsutilp.h"
#include "tidbitsp.h"

/* ********************************************************************** */

typedef CGcontext (*glue_cgCreateContext_t)(void);
typedef void (*glue_cgDestroyContext_t)(CGcontext);
typedef CGbool (*glue_cgIsContext_t)(CGcontext);
typedef const char * (*glue_cgGetLastListing_t)(CGcontext);
typedef CGprogram (*glue_cgCreateProgram_t)(CGcontext, CGenum, const char *, CGprofile, const char *, const char **);
typedef void (*glue_cgDestroyProgram_t)(CGprogram);
typedef CGbool (*glue_cgIsProgram_t)(CGprogram);
typedef const char * (*glue_cgGetProfileString_t)(CGprofile);
typedef CGerror (*glue_cgGetError_t)(void);
typedef const char * (*glue_cgGetErrorString_t)(CGerror);
typedef void (*glue_cgSetErrorCallback_t)(CGerrorCallbackFunc);
typedef CGbool (*glue_cgIsParameter_t)(CGparameter);
typedef CGtype (*glue_cgGetParameterType_t)(CGparameter);
typedef CGparameter (*glue_cgGetNamedParameter_t)(CGprogram, const char *);
typedef const char * (*glue_cgGetTypeString_t)(CGtype);
typedef CGbool (*glue_cgGLIsProfileSupported_t)(CGprofile);
typedef void (*glue_cgGLEnableProfile_t)(CGprofile);
typedef void (*glue_cgGLDisableProfile_t)(CGprofile);
typedef CGprofile (*glue_cgGLGetLatestProfile_t)(CGGLenum);
typedef void (*glue_cgGLLoadProgram_t)(CGprogram);
typedef void (*glue_cgGLBindProgram_t)(CGprogram);
typedef void (*glue_cgGLSetParameter1f_t)(CGparameter, float);
typedef void (*glue_cgGLSetParameter2f_t)(CGparameter, float, float);
typedef void (*glue_cgGLSetParameter3f_t)(CGparameter, float, float, float);
typedef void (*glue_cgGLSetParameter4f_t)(CGparameter, float, float, float, float);
typedef void (*glue_cgGLSetStateMatrixParameter_t)(CGparameter, CGGLenum, CGGLenum);
typedef void (*glue_cgGLSetParameterArray1f_t)(CGparameter, long, long, const float *);
typedef void (*glue_cgGLSetParameterArray2f_t)(CGparameter, long, long, const float *);
typedef void (*glue_cgGLSetParameterArray3f_t)(CGparameter, long, long, const float *);
typedef void (*glue_cgGLSetParameterArray4f_t)(CGparameter, long, long, const float *);
typedef void (*glue_cgGLSetMatrixParameterfc_t)(CGparameter, const float *);
typedef void (*glue_cgGLSetMatrixParameterArrayfc_t)(CGparameter, long, long, const float *);
typedef int (*glue_cgGetArrayDimension_t)(CGparameter);
typedef int (*glue_cgGetArraySize_t)(CGparameter, int);

/* texture parameters */
typedef void (*glue_cgGLSetManageTextureParameters_t)(CGcontext, CGbool flag);

/* CgFX */
typedef CGeffect (*glue_cgCreateEffect_t)(CGcontext, const char *, const char **);
typedef CGprogram (*glue_cgCreateProgramFromEffect_t)(CGeffect, CGprofile, const char * entry, const char ** args);
typedef void (*glue_cgDestroyEffect_t)(CGeffect);
typedef CGbool (*glue_cgIsEffect_t)(CGeffect);
typedef void (*glue_cgGLRegisterStates_t)(CGcontext);

typedef CGtechnique (*glue_cgGetFirstTechnique_t)(CGeffect);
typedef CGtechnique (*glue_cgGetNextTechnique_t)(CGtechnique);
typedef CGbool (*glue_cgValidateTechnique_t)(CGtechnique);

typedef CGpass (*glue_cgGetFirstPass_t)(CGtechnique);
typedef CGpass (*glue_cgGetNextPass_t)(CGpass);
typedef void (*glue_cgSetPassState_t)(CGpass);
typedef void (*glue_cgResetPassState_t)(CGpass);



typedef struct {
  int available;
  int cgfx_available;

  glue_cgCreateContext_t cgCreateContext;
  glue_cgDestroyContext_t cgDestroyContext;
  glue_cgIsContext_t cgIsContext;
  glue_cgGetLastListing_t cgGetLastListing;
  glue_cgCreateProgram_t cgCreateProgram;
  glue_cgDestroyProgram_t cgDestroyProgram;
  glue_cgIsProgram_t cgIsProgram;
  glue_cgGetProfileString_t cgGetProfileString;
  glue_cgGetError_t cgGetError;
  glue_cgGetErrorString_t cgGetErrorString;
  glue_cgSetErrorCallback_t cgSetErrorCallback;
  glue_cgIsParameter_t cgIsParameter;
  glue_cgGetParameterType_t cgGetParameterType;
  glue_cgGetNamedParameter_t cgGetNamedParameter;
  glue_cgGetTypeString_t cgGetTypeString;
  glue_cgGLIsProfileSupported_t cgGLIsProfileSupported;
  glue_cgGLEnableProfile_t cgGLEnableProfile;
  glue_cgGLDisableProfile_t cgGLDisableProfile;
  glue_cgGLGetLatestProfile_t cgGLGetLatestProfile;
  glue_cgGLLoadProgram_t cgGLLoadProgram;
  glue_cgGLBindProgram_t cgGLBindProgram;
  glue_cgGLSetParameter1f_t cgGLSetParameter1f;
  glue_cgGLSetParameter2f_t cgGLSetParameter2f;
  glue_cgGLSetParameter3f_t cgGLSetParameter3f;
  glue_cgGLSetParameter4f_t cgGLSetParameter4f;
  glue_cgGLSetStateMatrixParameter_t cgGLSetStateMatrixParameter;
  glue_cgGLSetParameterArray1f_t cgGLSetParameterArray1f;
  glue_cgGLSetParameterArray2f_t cgGLSetParameterArray2f;
  glue_cgGLSetParameterArray3f_t cgGLSetParameterArray3f;      
  glue_cgGLSetParameterArray4f_t cgGLSetParameterArray4f;
  glue_cgGLSetMatrixParameterfc_t cgGLSetMatrixParameterfc;
  glue_cgGLSetMatrixParameterArrayfc_t cgGLSetMatrixParameterArrayfc;
  glue_cgGetArrayDimension_t cgGetArrayDimension;
  glue_cgGetArraySize_t cgGetArraySize;

  /* texture parameters */
  glue_cgGLSetManageTextureParameters_t cgGLSetManageTextureParameters; 

  /* CgFX */
  glue_cgCreateEffect_t cgCreateEffect;
  glue_cgCreateProgramFromEffect_t cgCreateProgramFromEffect;
  glue_cgDestroyEffect_t cgDestroyEffect;
  glue_cgIsEffect_t cgIsEffect;
  glue_cgGLRegisterStates_t cgGLRegisterStates;

  glue_cgGetFirstTechnique_t cgGetFirstTechnique;
  glue_cgGetNextTechnique_t cgGetNextTechnique;
  glue_cgValidateTechnique_t cgValidateTechnique;
  
  glue_cgGetFirstPass_t cgGetFirstPass;
  glue_cgGetNextPass_t cgGetNextPass;
  glue_cgSetPassState_t cgSetPassState;
  glue_cgResetPassState_t cgResetPassState;

} cc_cgglue_t;

/* ********************************************************************** */

static cc_cgglue_t * cg_instance = NULL;
/* Cg is split into two dll's under Windows. Need two libhandles */
static cc_libhandle cg_libhandle = NULL;
static cc_libhandle cg_libhandle2 = NULL;
static int cg_failed_to_load = 0;

/* ********************************************************************** */

/* Cleans up at exit. */
static void
cgglue_cleanup(void)
{
#ifdef CGLIB_RUNTIME_LINKING
  if (cg_libhandle) { cc_dl_close(cg_libhandle); }
  if (cg_libhandle2) { cc_dl_close(cg_libhandle2); }
#endif /* CGLIB_RUNTIME_LINKING */
  assert(cg_instance);
  free(cg_instance);

  /* restore variables to initial value */
  cg_instance = NULL;
  cg_libhandle = NULL;
  cg_libhandle2 = NULL;
  cg_failed_to_load = 0;
}

/* ********************************************************************** */

static const cc_cgglue_t *
cgglue_init(void)
{
  CC_SYNC_BEGIN(cgglue_init);
    
  if (!cg_instance && !cg_failed_to_load) {
    /* First invocation, do initializations. */
    cc_cgglue_t * zi = (cc_cgglue_t *)malloc(sizeof(cc_cgglue_t));
    (void)coin_atexit((coin_atexit_f *)cgglue_cleanup, CC_ATEXIT_DYNLIBS);

    /* The common case is that the library is either available from
       the linking process or we're successfully going to link it
       in. */
    zi->available = 1;
    zi->cgfx_available = 1;

#ifdef CGLIB_RUNTIME_LINKING
    {
      int idx;
      /* FIXME: should we get the system shared library name from an
         Autoconf check? 20000930 mortene. */
      const char * possiblelibnames[] = {
        NULL, /* is set below */
        "CgGL", "libCgGL", "libCgGL.so",

        /* FIXME: not yet tested on Mac OS X. 20050125 mortene. */
        "libCgGL.dylib", 
        NULL
      };

      possiblelibnames[0] = coin_getenv("COIN_CG_LIBNAME");
      idx = possiblelibnames[0] ? 0 : 1;

      while (!cg_libhandle && possiblelibnames[idx]) {
        cg_libhandle = cc_dl_open(possiblelibnames[idx]);
        idx++;
      }

      if (!cg_libhandle) {
        zi->available = 0;
        zi->cgfx_available = 0;
        cg_failed_to_load = 1;
      }
      /* Cg is split into two dll's under Windows */
      if (cg_libhandle) {
        /* FIXME: do a proper possiblenames while loop? pederb, 2005-11-28 */
        cg_libhandle2 = cc_dl_open("Cg");
      }
    }
    /* Define CGGLUE_REGISTER_FUNC macro. Casting the type is
       necessary for this file to be compatible with C++ compilers. */
#define CGGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    do { \
      zi->_funcname_ = (_funcsig_)cc_dl_sym(cg_libhandle, SO__QUOTE(_funcname_)); \
      if (!zi->_funcname_ && cg_libhandle2) { \
        zi->_funcname_ = (_funcsig_)cc_dl_sym(cg_libhandle2, SO__QUOTE(_funcname_)); \
      } \
      if (zi->_funcname_ == NULL) { \
        zi->available = 0; \
        zi->cgfx_available = 0; \
      } \
    } while (0)
#define CGGLUE_REGISTER_FXFUNC(_funcsig_, _funcname_) \
    do { \
      zi->_funcname_ = (_funcsig_)cc_dl_sym(cg_libhandle, SO__QUOTE(_funcname_)); \
      if (!zi->_funcname_ && cg_libhandle2) { \
        zi->_funcname_ = (_funcsig_)cc_dl_sym(cg_libhandle2, SO__QUOTE(_funcname_)); \
      } \
      if (zi->_funcname_ == NULL) { \
        zi->cgfx_available = 0; \
      } \
    } while (0)

#elif defined(HAVE_CGLIB) /* no dynamic linking, but static linking possible */

    /* Define CGGLUE_REGISTER_FUNC macro. */
#define CGGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    zi->_funcname_ = (_funcsig_)_funcname_

#define CGGLUE_REGISTER_FXFUNC(_funcsig_, _funcname_) \
    zi->_funcname_ = (_funcsig_)_funcname_

#else /* no dynamic linking, and Cg-lib not available for static linking */
    zi->available = 0;
    zi->cgfx_available = 0;
    /* Define CGGLUE_REGISTER_FUNC macro. */
#define CGGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    zi->_funcname_ = NULL
#define CGGLUE_REGISTER_FXFUNC(_funcsig_, _funcname_) \
    zi->_funcname_ = NULL

#endif /* no linking */

    if (zi->available) {
      CGGLUE_REGISTER_FUNC(glue_cgCreateContext_t, cgCreateContext);
      CGGLUE_REGISTER_FUNC(glue_cgDestroyContext_t, cgDestroyContext);
      CGGLUE_REGISTER_FUNC(glue_cgIsContext_t, cgIsContext);
      CGGLUE_REGISTER_FUNC(glue_cgGetLastListing_t, cgGetLastListing);
      CGGLUE_REGISTER_FUNC(glue_cgCreateProgram_t, cgCreateProgram);
      CGGLUE_REGISTER_FUNC(glue_cgDestroyProgram_t, cgDestroyProgram);
      CGGLUE_REGISTER_FUNC(glue_cgIsProgram_t, cgIsProgram);
      CGGLUE_REGISTER_FUNC(glue_cgGetProfileString_t, cgGetProfileString);
      CGGLUE_REGISTER_FUNC(glue_cgGetError_t, cgGetError);
      CGGLUE_REGISTER_FUNC(glue_cgGetErrorString_t, cgGetErrorString);
      CGGLUE_REGISTER_FUNC(glue_cgSetErrorCallback_t, cgSetErrorCallback);
      CGGLUE_REGISTER_FUNC(glue_cgIsParameter_t, cgIsParameter);
      CGGLUE_REGISTER_FUNC(glue_cgGetParameterType_t, cgGetParameterType);
      CGGLUE_REGISTER_FUNC(glue_cgGetNamedParameter_t, cgGetNamedParameter);
      CGGLUE_REGISTER_FUNC(glue_cgGetTypeString_t, cgGetTypeString);
      CGGLUE_REGISTER_FUNC(glue_cgGLIsProfileSupported_t, cgGLIsProfileSupported);
      CGGLUE_REGISTER_FUNC(glue_cgGLEnableProfile_t, cgGLEnableProfile);
      CGGLUE_REGISTER_FUNC(glue_cgGLDisableProfile_t, cgGLDisableProfile);
      CGGLUE_REGISTER_FUNC(glue_cgGLGetLatestProfile_t, cgGLGetLatestProfile);
      CGGLUE_REGISTER_FUNC(glue_cgGLLoadProgram_t, cgGLLoadProgram);
      CGGLUE_REGISTER_FUNC(glue_cgGLBindProgram_t, cgGLBindProgram);
      CGGLUE_REGISTER_FUNC(glue_cgGLSetParameter1f_t, cgGLSetParameter1f);
      CGGLUE_REGISTER_FUNC(glue_cgGLSetParameter2f_t, cgGLSetParameter2f);
      CGGLUE_REGISTER_FUNC(glue_cgGLSetParameter3f_t, cgGLSetParameter3f);
      CGGLUE_REGISTER_FUNC(glue_cgGLSetParameter4f_t, cgGLSetParameter4f);
      CGGLUE_REGISTER_FUNC(glue_cgGLSetStateMatrixParameter_t, cgGLSetStateMatrixParameter);
      CGGLUE_REGISTER_FUNC(glue_cgGLSetParameterArray1f_t, cgGLSetParameterArray1f);
      CGGLUE_REGISTER_FUNC(glue_cgGLSetParameterArray2f_t, cgGLSetParameterArray2f);
      CGGLUE_REGISTER_FUNC(glue_cgGLSetParameterArray3f_t, cgGLSetParameterArray3f);      
      CGGLUE_REGISTER_FUNC(glue_cgGLSetParameterArray4f_t, cgGLSetParameterArray4f);
      CGGLUE_REGISTER_FUNC(glue_cgGLSetMatrixParameterfc_t, cgGLSetMatrixParameterfc);
      CGGLUE_REGISTER_FUNC(glue_cgGLSetMatrixParameterArrayfc_t, cgGLSetMatrixParameterArrayfc);
      CGGLUE_REGISTER_FUNC(glue_cgGetArrayDimension_t, cgGetArrayDimension);
      CGGLUE_REGISTER_FUNC(glue_cgGetArraySize_t, cgGetArraySize);

      CGGLUE_REGISTER_FUNC(glue_cgGLSetManageTextureParameters_t, cgGLSetManageTextureParameters);

      /* CgFX */
      CGGLUE_REGISTER_FXFUNC(glue_cgCreateEffect_t, cgCreateEffect);
      CGGLUE_REGISTER_FXFUNC(glue_cgCreateProgramFromEffect_t, cgCreateProgramFromEffect);
      CGGLUE_REGISTER_FXFUNC(glue_cgDestroyEffect_t, cgDestroyEffect);
      CGGLUE_REGISTER_FXFUNC(glue_cgIsEffect_t, cgIsEffect);
      CGGLUE_REGISTER_FXFUNC(glue_cgGLRegisterStates_t, cgGLRegisterStates);
      CGGLUE_REGISTER_FXFUNC(glue_cgGetFirstTechnique_t, cgGetFirstTechnique);
      CGGLUE_REGISTER_FXFUNC(glue_cgGetNextTechnique_t, cgGetNextTechnique);
      CGGLUE_REGISTER_FXFUNC(glue_cgValidateTechnique_t, cgValidateTechnique);      
      CGGLUE_REGISTER_FXFUNC(glue_cgGetFirstPass_t, cgGetFirstPass);
      CGGLUE_REGISTER_FXFUNC(glue_cgGetNextPass_t, cgGetNextPass);
      CGGLUE_REGISTER_FXFUNC(glue_cgSetPassState_t, cgSetPassState);
      CGGLUE_REGISTER_FXFUNC(glue_cgResetPassState_t, cgResetPassState);
    }
    /* Do this late, so we can detect recursive calls to this function. */
    cg_instance = zi;
  }
  CC_SYNC_END(cgglue_init);
  return cg_instance;
}

/* ********************************************************************** */

int 
cc_cgglue_available(void)
{
  if (cg_instance && cg_instance->available) { return 1; }
  else { cgglue_init(); }
  return (cg_instance && cg_instance->available);
}

int
glue_cgglue_cgfx_available()
{
  if (cg_instance && cg_instance->available && cg_instance->cgfx_available) { return 1; }
  else { cgglue_init(); }
  return (cg_instance && cg_instance->available && cg_instance->cgfx_available);
}

/* ********************************************************************** */

CGcontext
glue_cgCreateContext(void)
{
  return cg_instance->cgCreateContext();
}

void
glue_cgDestroyContext(CGcontext c)
{
  cg_instance->cgDestroyContext(c);
}

CGbool
glue_cgIsContext(CGcontext c)
{
  return cg_instance->cgIsContext(c);
}

const char *
glue_cgGetLastListing(CGcontext c)
{
  return cg_instance->cgGetLastListing(c);
}

CGprogram
glue_cgCreateProgram(CGcontext c, CGenum e, const char * cp0, CGprofile p,
                     const char * cp1, const char ** cpp)
{
  return cg_instance->cgCreateProgram(c, e, cp0, p, cp1, cpp);
}

void
glue_cgDestroyProgram(CGprogram p)
{
  cg_instance->cgDestroyProgram(p);
}

CGbool
glue_cgIsProgram(CGprogram p)
{
  return cg_instance->cgIsProgram(p);
}

const char *
glue_cgGetProfileString(CGprofile p)
{
  return cg_instance->cgGetProfileString(p);
}

CGerror
glue_cgGetError(void)
{
  return cg_instance->cgGetError();
}

const char *
glue_cgGetErrorString(CGerror e)
{
  return cg_instance->cgGetErrorString(e);
}

void
glue_cgSetErrorCallback(CGerrorCallbackFunc f)
{
  cg_instance->cgSetErrorCallback(f);
}

CGbool
glue_cgIsParameter(CGparameter p)
{
  return cg_instance->cgIsParameter(p);
}

CGtype
glue_cgGetParameterType(CGparameter p)
{
  return cg_instance->cgGetParameterType(p);
}

CGparameter
glue_cgGetNamedParameter(CGprogram p, const char * c)
{
  return cg_instance->cgGetNamedParameter(p, c);
}

const char *
glue_cgGetTypeString(CGtype t)
{
  return cg_instance->cgGetTypeString(t);
}

CGbool
glue_cgGLIsProfileSupported(CGprofile p)
{
  return cg_instance->cgGLIsProfileSupported(p);
}

void
glue_cgGLEnableProfile(CGprofile p)
{
  cg_instance->cgGLEnableProfile(p);
}

void
glue_cgGLDisableProfile(CGprofile p)
{
  cg_instance->cgGLDisableProfile(p);
}

CGprofile
glue_cgGLGetLatestProfile(CGGLenum e)
{
  return cg_instance->cgGLGetLatestProfile(e);
}

void
glue_cgGLLoadProgram(CGprogram p)
{
  cg_instance->cgGLLoadProgram(p);
}

void
glue_cgGLBindProgram(CGprogram p)
{
  cg_instance->cgGLBindProgram(p);
}

void
glue_cgGLSetParameter1f(CGparameter p, float f)
{
  cg_instance->cgGLSetParameter1f(p, f);
}

void
glue_cgGLSetParameter2f(CGparameter p, float f0, float f1)
{
  cg_instance->cgGLSetParameter2f(p, f0, f1);
}

void
glue_cgGLSetParameter3f(CGparameter p, float f0, float f1, float f2)
{
  cg_instance->cgGLSetParameter3f(p, f0, f1, f2);
}

void
glue_cgGLSetParameter4f(CGparameter p, float f0, float f1, float f2, float f3)
{
  cg_instance->cgGLSetParameter4f(p, f0, f1, f2, f3);
}

void
glue_cgGLSetStateMatrixParameter(CGparameter p, CGGLenum e0, CGGLenum e1)
{
  cg_instance->cgGLSetStateMatrixParameter(p, e0, e1);
}

void
glue_cgGLSetParameterArray1f(CGparameter p, long o, long n, const float *v)
{
  cg_instance->cgGLSetParameterArray1f(p, o, n, v);
}

void
glue_cgGLSetParameterArray2f(CGparameter p, long o, long n, const float *v)
{
  cg_instance->cgGLSetParameterArray2f(p, o, n, v);
}

void
glue_cgGLSetParameterArray3f(CGparameter p, long o, long n, const float *v)
{
  cg_instance->cgGLSetParameterArray3f(p, o, n, v);
}

void
glue_cgGLSetParameterArray4f(CGparameter p, long o, long n, const float *v)
{
  cg_instance->cgGLSetParameterArray4f(p, o, n, v);
}

void
glue_cgGLSetMatrixParameterfc(CGparameter p, const float *v)
{
  cg_instance->cgGLSetMatrixParameterfc(p, v);
}

void
glue_cgGLSetMatrixParameterArrayfc(CGparameter p, long o, long n, const float *v)
{
  cg_instance->cgGLSetMatrixParameterArrayfc(p, o, n, v);
}

int
glue_cgGetArrayDimension(CGparameter p)
{
  return cg_instance->cgGetArrayDimension(p);
}

int
glue_cgGetArraySize(CGparameter p, int dim)
{
  return cg_instance->cgGetArraySize(p, dim);
}

/* texture parameters */
void 
glue_cgGLSetManageTextureParameters(CGcontext context, CGbool flag)
{
  cg_instance->cgGLSetManageTextureParameters(context, flag);
}

/* CgFX */
CGeffect 
glue_cgCreateEffect(CGcontext context, const char * program, const char ** args)
{
  assert(cg_instance);
  return cg_instance->cgCreateEffect(context, program, args);
}

CGprogram 
glue_cgCreateProgramFromEffect(CGeffect effect, CGprofile profile, const char * entry, const char ** args)
{
  assert(cg_instance);
  return cg_instance->cgCreateProgramFromEffect(effect, profile, entry, args);
}

void 
glue_cgDestroyEffect(CGeffect COIN_UNUSED_ARG(effect))
{
}

CGbool 
glue_cgIsEffect(CGeffect COIN_UNUSED_ARG(effect))
{
  return FALSE;
}

void 
glue_cgGLRegisterStates(CGcontext c)
{
  assert(cg_instance);
  cg_instance->cgGLRegisterStates(c);
}

/* ********************************************************************** */
