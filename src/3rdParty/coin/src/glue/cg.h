#ifndef COIN_GLUE_CG_H
#define COIN_GLUE_CG_H

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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if 0 /* to get proper auto-indentation in emacs */
}
#endif /* emacs indentation */

#if !defined(HAVE_DYNAMIC_LINKING) && defined(HAVE_CGLIB)

/* FIXME: static linking with Cg library has not been tested
   yet. 20050125 mortene.*/

#define CGLIB_RUNTIME_LINKING 0

#include <Cg/cg.h>
#include <Cg/cgGL.h>

#else /* HAVE_DYNAMIC_LINKING || !HAVE_CGLIB */

#define CGLIB_RUNTIME_LINKING 1

/*
   We need some Cg structs and defines, so set them up here for
   runtime linking support, as we want to avoid including
   Cg/<whatever>.h.
*/

/* typedef struct _CGprogram * CGprogram; XXX */
typedef void * CGprogram;
typedef void * CGeffect;
typedef void * CGtechnique;
typedef void * CGpass;

/* typedef struct _CGcontext * CGcontext; XXX */
typedef void * CGcontext;

/* typedef struct _CGparameter *CGparameter; XXX */
typedef void * CGparameter;

typedef enum {
  CG_PROFILE_ARBVP1 = 6150,
  CG_PROFILE_ARBFP1 = 7000,
  CG_GENERIC = 7002
} CGprofile;

typedef enum {
  CG_NO_ERROR = 0
} CGerror;

typedef enum {
  CG_SOURCE = 4112
} CGenum;

typedef enum {
  CG_GL_MATRIX_IDENTITY = 0,
  CG_GL_MATRIX_TRANSPOSE = 1,
  CG_GL_MATRIX_INVERSE = 2,
  CG_GL_MATRIX_INVERSE_TRANSPOSE = 3,

  CG_GL_MODELVIEW_MATRIX = 4,
  CG_GL_PROJECTION_MATRIX = 5,
  CG_GL_TEXTURE_MATRIX = 6,
  CG_GL_MODELVIEW_PROJECTION_MATRIX = 7,

  CG_GL_VERTEX = 8,
  CG_GL_FRAGMENT = 9
} CGGLenum;

typedef enum {
  CG_UNKNOWN_TYPE = 0,
  CG_STRUCT = 1,
  CG_ARRAY = 2,

  CG_FLOAT = 1045,
  CG_FLOAT1 = 1091,
  CG_FLOAT2 = 1046,
  CG_FLOAT3 = 1047,
  CG_FLOAT4 = 1048,
  CG_FLOAT2x2 = 1054,
  CG_FLOAT3x3 = 1059,
  CG_FLOAT4x4 = 1064,
  CG_SAMPLER1D = 1065,
  CG_SAMPLER2D = 1066,
  CG_SAMPLER3D = 1067,
  CG_SAMPLERRECT = 1068,
  CG_SAMPLERCUBE = 1069,
  CG_INT = 1093,
  CG_INT1 = 1094
} CGtype;

typedef int CGbool;
typedef void (* CGerrorCallbackFunc)(void);

#endif /* HAVE_DYNAMIC_LINKING */

int cc_cgglue_available(void);

CGcontext glue_cgCreateContext(void);
void glue_cgDestroyContext(CGcontext);
CGbool glue_cgIsContext(CGcontext);
const char * glue_cgGetLastListing(CGcontext);

CGprogram glue_cgCreateProgram(CGcontext, CGenum, const char *, CGprofile,
                               const char *, const char **);
void glue_cgDestroyProgram(CGprogram);
CGbool glue_cgIsProgram(CGprogram);


const char * glue_cgGetProfileString(CGprofile);

CGerror glue_cgGetError(void);
const char * glue_cgGetErrorString(CGerror);
void glue_cgSetErrorCallback(CGerrorCallbackFunc);

CGbool glue_cgIsParameter(CGparameter);
CGtype glue_cgGetParameterType(CGparameter);
CGparameter glue_cgGetNamedParameter(CGprogram, const char *);

const char * glue_cgGetTypeString(CGtype);

CGbool glue_cgGLIsProfileSupported(CGprofile);
void glue_cgGLEnableProfile(CGprofile);
void glue_cgGLDisableProfile(CGprofile);
CGprofile glue_cgGLGetLatestProfile(CGGLenum);

void glue_cgGLLoadProgram(CGprogram);
void glue_cgGLBindProgram(CGprogram);

void glue_cgGLSetParameter1f(CGparameter, float);
void glue_cgGLSetParameter2f(CGparameter, float, float);
void glue_cgGLSetParameter3f(CGparameter, float, float, float);
void glue_cgGLSetParameter4f(CGparameter, float, float, float, float);
void glue_cgGLSetStateMatrixParameter(CGparameter, CGGLenum, CGGLenum);

void glue_cgGLSetParameterArray1f(CGparameter, long, long, const float *);
void glue_cgGLSetParameterArray2f(CGparameter, long, long, const float *);
void glue_cgGLSetParameterArray3f(CGparameter, long, long, const float *);
void glue_cgGLSetParameterArray4f(CGparameter, long, long, const float *);

void glue_cgGLSetMatrixParameterfc(CGparameter, const float *);
void glue_cgGLSetMatrixParameterArrayfc(CGparameter, long, long, const float *);

int glue_cgGetArrayDimension(CGparameter);
int glue_cgGetArraySize(CGparameter, int);

/* texture parameters */
void glue_cgGLSetManageTextureParameters(CGcontext, CGbool);

/* CgFx */
int glue_cgglue_cgfx_available();
CGeffect glue_cgCreateEffect(CGcontext, const char *, const char **);
CGprogram glue_cgCreateProgramFromEffect(CGeffect, CGprofile, const char * entry, const char ** args);
void glue_cgDestroyEffect(CGeffect);
CGbool glue_cgIsEffect(CGeffect);
void glue_cgGLRegisterStates(CGcontext);

CGtechnique glue_cgGetFirstTechnique(CGeffect);
CGtechnique glue_cgGetNextTechnique(CGtechnique);
CGbool glue_cgValidateTechnique(CGtechnique);

CGpass glue_cgGetFirstPass(CGtechnique);
CGpass glue_cgGetNextPass(CGpass);
void glue_cgSetPassState(CGpass);
void glue_cgResetPassState(CGpass);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !COIN_GLUE_CG_H */
