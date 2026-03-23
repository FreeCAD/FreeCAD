#ifndef COIN_GLUWRAPPER_H
#define COIN_GLUWRAPPER_H

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

#ifdef HAVE_WINDOWS_H
#include <windows.h> /* to pick up the APIENTRY define */
#endif /* HAVE_WINDOWS_H */

#include <Inventor/system/gl.h>

/* Under Win32, we need to make sure we use the correct calling method
   by using the APIENTRY define for the function signature types (or
   else we'll get weird stack errors). On other platforms, just define
   APIENTRY empty. */
#ifndef APIENTRY
#define APIENTRY
#endif /* !APIENTRY */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Callback func type. */
typedef void (APIENTRY * gluNurbsCallback_cb_t)(void *, ...);

/* Typedefinitions of function signatures for GLU calls we use. We
   need these for casting from the void-pointer return of dlsym().*/
typedef const GLubyte * (APIENTRY *gluGetString_t)(GLenum);
typedef const GLubyte * (APIENTRY *gluErrorString_t)(GLenum);
typedef GLint (APIENTRY *gluScaleImage_t)(GLenum, GLsizei, GLsizei, GLenum, const void *, GLsizei, GLsizei, GLenum, GLvoid *);

/* The first argument for these methods is actually either GLUnurbs or
   GLUnurbsObj, depending on the GLU version (yes, they managed to
   change the API over version 1.x to 1.y, for some value of [0, 3]
   for x and y, where x < y). */
typedef void * (APIENTRY *gluNewNurbsRenderer_t)(void);
typedef void (APIENTRY *gluDeleteNurbsRenderer_t)(void *);
typedef void (APIENTRY *gluNurbsProperty_t)(void *, GLenum, GLfloat);
typedef void (APIENTRY *gluLoadSamplingMatrices_t)(void *, const GLfloat *, const GLfloat *, const GLint *);
typedef void (APIENTRY *gluBeginSurface_t)(void *);
typedef void (APIENTRY *gluEndSurface_t)(void *);
typedef void (APIENTRY *gluNurbsSurface_t)(void *, GLint, GLfloat *, GLint, GLfloat *, GLint, GLint, GLfloat *, GLint, GLint, GLenum);
typedef void (APIENTRY *gluBeginTrim_t)(void *);
typedef void (APIENTRY *gluEndTrim_t)(void *);
typedef void (APIENTRY *gluBeginCurve_t)(void *);
typedef void (APIENTRY *gluEndCurve_t)(void *);
typedef void (APIENTRY *gluNurbsCurve_t)(void *, GLint, GLfloat *, GLint, GLfloat *, GLint, GLenum);
typedef void (APIENTRY *gluPwlCurve_t)(void *, GLint, GLfloat *, GLint, GLenum);
typedef void (APIENTRY *gluNurbsCallback_t)(void *, GLenum, gluNurbsCallback_cb_t);
typedef void (APIENTRY *gluNurbsCallbackData_t)(void *, GLvoid *);

/* gluTessellator routines */
typedef struct coin_GLUtessellator coin_GLUtessellator;
typedef coin_GLUtessellator * (APIENTRY *gluNewTess_t)(void);
typedef void (APIENTRY *gluTessCallback_cb_t)(void);

typedef void (APIENTRY *gluTessCallback_t)(coin_GLUtessellator *, GLenum, gluTessCallback_cb_t);
typedef void (APIENTRY *gluTessProperty_t)(coin_GLUtessellator * tessobj, GLenum property, GLdouble value);
typedef void (APIENTRY *gluTessBeginPolygon_t)(coin_GLUtessellator * tessobj, void * user_data);
typedef void (APIENTRY *gluTessEndPolygon_t)(coin_GLUtessellator * tessobj);
typedef void (APIENTRY *gluTessBeginContour_t)(coin_GLUtessellator * tessobj);
typedef void (APIENTRY *gluTessEndContour_t)(coin_GLUtessellator * tessobj);
typedef void (APIENTRY *gluTessVertex_t)(coin_GLUtessellator * tessobj, GLdouble coords[3], void * vertex_data);
typedef void (APIENTRY *gluDeleteTess_t)(coin_GLUtessellator * tessobj);
typedef void (APIENTRY *gluTessNormal_t)(coin_GLUtessellator * tessobj, GLdouble x, GLdouble y, GLdouble z);

typedef struct {
  /* Is the GLU library at all available? */
  int available;

  /* GLU versioning. */
  struct {
    unsigned int major, minor, release;
  } version;
  int (*versionMatchesAtLeast)(unsigned int major,
                               unsigned int minor,
                               unsigned int release);

  /* 
     GLU calls which might be used. Note that any of these can be NULL
     pointers if the function is not available, unless marked as being
     always available. (That is, as long as GLU itself is available.)
  */

  gluGetString_t gluGetString;
  gluErrorString_t gluErrorString;
  gluScaleImage_t gluScaleImage; /* always present */
  gluNewNurbsRenderer_t gluNewNurbsRenderer;
  gluDeleteNurbsRenderer_t gluDeleteNurbsRenderer;
  gluNurbsProperty_t gluNurbsProperty;
  gluLoadSamplingMatrices_t gluLoadSamplingMatrices;
  gluBeginSurface_t gluBeginSurface;
  gluEndSurface_t gluEndSurface;
  gluNurbsSurface_t gluNurbsSurface;
  gluBeginTrim_t gluBeginTrim;
  gluEndTrim_t gluEndTrim;
  gluBeginCurve_t gluBeginCurve;
  gluEndCurve_t gluEndCurve;
  gluNurbsCurve_t gluNurbsCurve;
  gluPwlCurve_t gluPwlCurve;
  gluNurbsCallback_t gluNurbsCallback;
  gluNurbsCallbackData_t gluNurbsCallbackData;

  gluNewTess_t gluNewTess;
  gluTessCallback_t gluTessCallback; 
  gluTessProperty_t gluTessProperty;
  gluTessBeginPolygon_t gluTessBeginPolygon;
  gluTessEndPolygon_t gluTessEndPolygon;
  gluTessBeginContour_t gluTessBeginContour;
  gluTessEndContour_t gluTessEndContour;
  gluTessVertex_t gluTessVertex;
  gluDeleteTess_t gluDeleteTess;
  gluTessNormal_t gluTessNormal;

  gluNurbsSurface_t gluNurbsSurface_in_GLU;

} GLUWrapper_t;


const GLUWrapper_t * GLUWrapper(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COIN_GLUWRAPPER_H */
