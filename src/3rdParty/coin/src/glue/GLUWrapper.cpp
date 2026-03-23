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

#include "glue/GLUWrapper.h"
#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else /* No config.h? Hmm. Assume the GLU library is available for linking. */
#define GLUWRAPPER_ASSUME_GLU 1
#endif /* !HAVE_CONFIG_H */

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <Inventor/C/glue/dl.h>
#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/tidbits.h>

#include "tidbitsp.h"
#include "threads/threadsutilp.h"

/* need this before superglu.h, since that file includes GL/gl.h
   without any windows.h, which may fail on Microsoft Windows dev systems */
#include <Inventor/system/gl.h>

#ifdef HAVE_SUPERGLU
#include <superglu.h>
#endif /* HAVE_SUPERGLU */

/* ********************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef HAVE_GLU /* In case we're _not_ doing runtime linking. */
#define GLUWRAPPER_ASSUME_GLU 1
#endif /* HAVE_GLU */

#ifdef HAVE_SUPERGLU
#define GLUWRAPPER_ASSUME_GLU 1
#endif /* HAVE_SUPERGLU */

/* ******************************************************************** */

static GLUWrapper_t * GLU_instance = NULL;
static cc_libhandle GLU_libhandle = NULL;
static int GLU_failed_to_load = 0;
static int GLU_is_initializing = 0;

/* ******************************************************************** */

static SbBool
GLUWrapper_debug(void)
{
  static int dbg = -1;
  if (dbg == -1) {
    const char * env = coin_getenv("COIN_DEBUG_GLU_INFO");
    dbg = (env && (atoi(env) > 0)) ? 1 : 0;
  }
  return dbg;
}

/* ******************************************************************** */

/* Cleans up at exit. */
static void
GLUWrapper_cleanup(void)
{
#ifdef GLU_RUNTIME_LINKING
  if (GLU_libhandle) {
    cc_dl_close(GLU_libhandle);
    GLU_libhandle = NULL;
  }
#endif /* GLU_RUNTIME_LINKING */

  assert(GLU_instance);
  free(GLU_instance);
  GLU_instance = NULL;
  GLU_failed_to_load = 0;
  GLU_is_initializing = 0;
}

/* ******************************************************************** */

/* Set the GLU version variables in the global GLUWrapper_t. */
static void
GLUWrapper_set_version(const GLubyte * versionstr)
{
  char buffer[256];
  char * dotptr;

  GLU_instance->version.major = 0;
  GLU_instance->version.minor = 0;
  GLU_instance->version.release = 0;

  (void)strncpy(buffer, (const char *)versionstr, 255);
  buffer[255] = '\0'; /* strncpy() will not null-terminate if strlen > 255 */
  dotptr = strchr(buffer, '.');
  if (dotptr) {
    char * spaceptr;
    char * start = buffer;
    *dotptr = '\0';
    GLU_instance->version.major = atoi(start);
    start = ++dotptr;

    dotptr = strchr(start, '.');
    spaceptr = strchr(start, ' ');
    if (!dotptr && spaceptr) dotptr = spaceptr;
    if (dotptr && spaceptr && spaceptr < dotptr) dotptr = spaceptr;
    if (dotptr) {
      int terminate = *dotptr == ' ';
      *dotptr = '\0';
      GLU_instance->version.minor = atoi(start);
      if (!terminate) {
        start = ++dotptr;
        dotptr = strchr(start, ' ');
        if (dotptr) *dotptr = '\0';
        GLU_instance->version.release = atoi(start);
      }
    }
    else {
      GLU_instance->version.minor = atoi(start);
    }
  }
  else {
    cc_debugerror_post("GLUWrapper_set_version",
                       "Invalid GLU versionstring: \"%s\"\n", versionstr);
  }

  { /* Runtime help for debugging GLU problems on remote sites. */
#ifdef HAVE_SUPERGLU
    const SbBool superglu = TRUE;
#else
    const SbBool superglu = FALSE;
#endif /* !HAVE_SUPERGLU */
#ifdef GLU_RUNTIME_LINKING
    const SbBool runtime = TRUE;
#else
    const SbBool runtime = FALSE;
#endif /* !GLU_RUNTIME_LINKING */

    if (GLUWrapper_debug()) {
      const char * extensions = (const char *)
        GLU_instance->gluGetString(GLU_EXTENSIONS);

      cc_debugerror_postinfo("GLUWrapper_set_version",
                             "gluGetString(GLU_VERSION)=='%s', "
                             "input arg: '%s' (=> %d.%d.%d)",
                             (const char *) GLU_instance->gluGetString(GLU_VERSION),
                             versionstr,
                             GLU_instance->version.major,
                             GLU_instance->version.minor,
                             GLU_instance->version.release);

      cc_debugerror_postinfo("GLUWrapper_set_version",
                             "gluGetString(GLU_EXTENSIONS)=='%s'",
                             extensions ? extensions : "<none>");

      cc_debugerror_postinfo("GLUWrapper_set_version",
                             "%susing embedded SuperGLU",
                             superglu ? "" : "not ");

      cc_debugerror_postinfo("GLUWrapper_set_version",
                             "linking with GLU at %s",
                             runtime ? "runtime" : "build-time ");
    }
  }
}

static int
GLUWrapper_versionMatchesAtLeast(unsigned int major,
                                 unsigned int minor,
                                 unsigned int release)
{
  assert(GLU_instance);
  if (GLU_instance->available == 0) return 0;
  if (GLU_instance->version.major < major) return 0;
  else if (GLU_instance->version.major > major) return 1;
  if (GLU_instance->version.minor < minor) return 0;
  else if (GLU_instance->version.minor > minor) return 1;
  if (GLU_instance->version.release < release) return 0;
  return 1;
}

/* Replacement function for gluGetString(). */
static const GLubyte * APIENTRY
GLUWrapper_gluGetString(GLenum name)
{
  static const GLubyte versionstring[] = "1.0.0";
  if (name == GLU_VERSION) return versionstring;
  return NULL;
}

/* Replacement function for gluScaleImage(). */
static GLint APIENTRY
GLUWrapper_gluScaleImage(GLenum COIN_UNUSED_ARG(a), GLsizei COIN_UNUSED_ARG(b), GLsizei COIN_UNUSED_ARG(c), GLenum COIN_UNUSED_ARG(d), const void * COIN_UNUSED_ARG(e), GLsizei COIN_UNUSED_ARG(f), GLsizei COIN_UNUSED_ARG(g), GLenum COIN_UNUSED_ARG(h), GLvoid * COIN_UNUSED_ARG(i))
{
  /* Just a void function. */

  /* gluScaleImage() should _always_ be present, as it has been
     present in GLU from version 1.0. This is just here as a paranoid
     measure to avoid a crash if we happen to stumble into a faulty
     GLU library. */

  /* FIXME: memset() to a pattern? 20011129 mortene. */

  /* 0 indicates success. */
  return 0;
}

/* ******************************************************************** */
/*
  Wraps the GLU library's gluNurbsSurface() function, to provide
  debugging facilities on the input parameters.
*/

static void
GLUWrapper_gluNurbsSurface(void * nurb, /* this is really of type "GLUnurbs *" */
                           GLint sKnotCount,
                           GLfloat * sKnots,
                           GLint tKnotCount,
                           GLfloat * tKnots,
                           GLint sStride,
                           GLint tStride,
                           GLfloat * control,
                           GLint sOrder,
                           GLint tOrder,
                           GLenum type)
{
  GLint i;
  cc_string s, tmp;

  cc_string_construct(&s);
  cc_string_construct(&tmp);

  cc_string_sprintf(&s,
                    "GLUnurbs*==%p, sKnotCount==%d, sKnots==%p, tKnotCount==%d, tKnots==%p, "
                    "sStride==%d, tStride==%d, control==%p, sOrder==%d, tOrder==%d, type==0x%0x\n",
                    nurb,
                    sKnotCount, sKnots,
                    tKnotCount, tKnots,
                    sStride, tStride,
                    control,
                    sOrder, tOrder,
                    type);

  {
    cc_string_append_text(&s, "sKnots=='");
    for (i = 0; i < sKnotCount; i++) {
      if (i > 0) { cc_string_append_text(&s, ", "); }
      cc_string_sprintf(&tmp, "%f", sKnots[i]);
      cc_string_append_string(&s, &tmp);
    }
    cc_string_append_text(&s, "'\n");
  }
  {
    cc_string_append_text(&s, "tKnots=='");
    for (i = 0; i < tKnotCount; i++) {
      if (i > 0) { cc_string_append_text(&s, ", "); }
      cc_string_sprintf(&tmp, "%f", tKnots[i]);
      cc_string_append_string(&s, &tmp);
    }
    cc_string_append_text(&s, "'\n");
  }

  cc_string_sprintf(&tmp,
                    "(sKnotCount - sOrder) * (tKnotCount - tOrder) => %d control points\n",
                    (sKnotCount - sOrder) * (tKnotCount - tOrder));
  cc_string_append_string(&s, &tmp);

  {
    GLint nrofctrlpoints = (sKnotCount - sOrder) * (tKnotCount - tOrder);
    cc_string_append_text(&s, "controlpoints=='");
#if 0 // dumping the full list is usually not what we want
    for (i = 0; i < nrofctrlpoints; i++) {
      if (i > 0) { printf(", "); }
      cc_string_sprintf(&tmp, "%f %f %f", control[i*3 + 0], control[i*3 + 1], control[i*3 + 2]);
      cc_string_append_string(&s, &tmp);
    }
#else // just dump the first and last entries
    cc_string_sprintf(&tmp, "%f %f %f, ...suppressed..., %f %f %f",
                      control[0], control[1], control[2],
                      control[(nrofctrlpoints - 1) * 3 + 0],
                      control[(nrofctrlpoints - 1) * 3 + 1],
                      control[(nrofctrlpoints - 1) * 3 + 2]);
    cc_string_append_string(&s, &tmp);
#endif
    cc_string_append_text(&s, "'\n");
  }

  cc_debugerror_postinfo("GLUWrapper_gluNurbsSurface", "%s", cc_string_get_text(&s));
  cc_string_clean(&s);
  cc_string_clean(&tmp);

  GLUWrapper()->gluNurbsSurface_in_GLU(nurb,
                                       sKnotCount, sKnots,
                                       tKnotCount, tKnots,
                                       sStride, tStride,
                                       control,
                                       sOrder, tOrder,
                                       type);
}

/* ******************************************************************** */

/* Implemented by using the singleton pattern. */
const GLUWrapper_t *
GLUWrapper(void)
{
  GLUWrapper_t * gi;

  CC_SYNC_BEGIN(GLUWrapper);

  if (GLU_instance || GLU_failed_to_load) { goto wrapperexit; }

  /* Detect recursive calls. */
  assert(GLU_is_initializing == 0);
  GLU_is_initializing = 1;


  /* First invocation, do initializations. */
  GLU_instance = gi = (GLUWrapper_t *)malloc(sizeof(GLUWrapper_t));
  /* FIXME: handle out-of-memory on malloc(). 20000928 mortene. */
  (void)coin_atexit((coin_atexit_f*) GLUWrapper_cleanup, CC_ATEXIT_DYNLIBS);

  gi->versionMatchesAtLeast = GLUWrapper_versionMatchesAtLeast;

    /* The common case is that GLU is either available from the
       linking process or we're successfully going to link it in. */
  gi->available = 1;

#ifdef GLU_RUNTIME_LINKING

  {
    const char * libname;

#ifndef GLU_IS_PART_OF_GL

    /* FIXME: should we get the system shared library name from an
       Autoconf check? 20000930 mortene. */
    const char * possiblelibnames[] = {
      NULL, /* is set below */
      /* Microsoft Windows DLL name for the GLU library */
      "glu32",

      /* UNIX-style names */
      "GLU", "MesaGLU",
      "libGLU", "libMesaGLU",
      "libGLU.so", "libMesaGLU.so",
      "libGLU.so.1", /* Some Debian distributions do not supply a symlink for libGLU.so, only libGLU.so.1 */
      NULL
    };
    possiblelibnames[0] = coin_getenv("COIN_GLU_LIBNAME");
    int idx = possiblelibnames[0] ? 0 : 1;

    while (!GLU_libhandle && possiblelibnames[idx]) {
      GLU_libhandle = cc_dl_open(possiblelibnames[idx]);
      idx++;
    }
    libname = possiblelibnames[idx-1];

#elif (defined HAVE_OPENGL_GLU_H)

    /* On Mac OS X, GLU is part of the OpenGL framework, which at this
       point is already loaded -> We can resolve symbols from the current
       process image. */
    GLU_libhandle = cc_dl_open(NULL);
    libname = "OpenGL.framework/Libraries/libGLU.dylib";

#endif /* !GLU_IS_PART_OF_GL */

  /* FIXME: Resolving GLU functions will fail on other platforms where
     GLU is considered part of OpenGL, since we never set GLU_libhandle. 
     We should probably try to dlopen the GL image (or the current
     process image as on Mac OS X?) on these platforms.  

     I don't know any platforms other than OS X that have GLU as part
     of GL though... 20051216 kyrah. */

    if (!GLU_libhandle) {
      gi->available = 0;
      GLU_failed_to_load = 1;
      goto wrapperexit;
    }

    if (GLUWrapper_debug()) {
      if (GLU_failed_to_load) {
        cc_debugerror_postinfo("GLUWrapper", "found no GLU library on system");
      }
      else {
        cc_debugerror_postinfo("GLUWrapper",
                               "Dynamically loaded GLU library as '%s'.",
                               libname);
      }
    }
  }


  /* Define GLUWRAPPER_REGISTER_FUNC macro. Casting the type is
     necessary for this file to be compatible with C++ compilers. */
#define GLUWRAPPER_REGISTER_FUNC(_funcname_, _funcsig_) \
      gi->_funcname_ = (_funcsig_)cc_dl_sym(GLU_libhandle, SO__QUOTE(_funcname_))

#elif defined(GLUWRAPPER_ASSUME_GLU) /* !GLU_RUNTIME_LINKING */

    /* Define GLUWRAPPER_REGISTER_FUNC macro. */
#define GLUWRAPPER_REGISTER_FUNC(_funcname_, _funcsig_) \
      gi->_funcname_ = (_funcsig_)_funcname_

#else /* !GLUWRAPPER_ASSUME_GLU */
  gi->available = 0;
    /* Define GLUWRAPPER_REGISTER_FUNC macro. */
#define GLUWRAPPER_REGISTER_FUNC(_funcname_, _funcsig_) \
      gi->_funcname_ = NULL

#endif /* !GLUWRAPPER_ASSUME_GLU */

  GLUWRAPPER_REGISTER_FUNC(gluScaleImage, gluScaleImage_t);
  GLUWRAPPER_REGISTER_FUNC(gluGetString, gluGetString_t);
  GLUWRAPPER_REGISTER_FUNC(gluErrorString, gluErrorString_t);
  GLUWRAPPER_REGISTER_FUNC(gluNewNurbsRenderer, gluNewNurbsRenderer_t);
  GLUWRAPPER_REGISTER_FUNC(gluDeleteNurbsRenderer, gluDeleteNurbsRenderer_t);
  GLUWRAPPER_REGISTER_FUNC(gluNurbsProperty, gluNurbsProperty_t);
  GLUWRAPPER_REGISTER_FUNC(gluLoadSamplingMatrices, gluLoadSamplingMatrices_t);
  GLUWRAPPER_REGISTER_FUNC(gluBeginSurface, gluBeginSurface_t);
  GLUWRAPPER_REGISTER_FUNC(gluEndSurface, gluEndSurface_t);
  GLUWRAPPER_REGISTER_FUNC(gluNurbsSurface, gluNurbsSurface_t);
  GLUWRAPPER_REGISTER_FUNC(gluBeginTrim, gluBeginTrim_t);
  GLUWRAPPER_REGISTER_FUNC(gluEndTrim, gluEndTrim_t);
  GLUWRAPPER_REGISTER_FUNC(gluBeginCurve, gluBeginCurve_t);
  GLUWRAPPER_REGISTER_FUNC(gluEndCurve, gluEndCurve_t);
  GLUWRAPPER_REGISTER_FUNC(gluNurbsCurve, gluNurbsCurve_t);
  GLUWRAPPER_REGISTER_FUNC(gluPwlCurve, gluPwlCurve_t);
  GLUWRAPPER_REGISTER_FUNC(gluNurbsCallback, gluNurbsCallback_t);
#if defined(GLU_VERSION_1_3) || defined(GLU_RUNTIME_LINKING)
  // Note: nurbs tessellation is also available if the GLU_EXTENSION
  // string contains GLU_EXT_nurbs_tessellator, but most platforms
  // should have GLU 1.3, and on Windows -- the sole platform we know
  // of where a GLU older than 1.3 is common -- the extension is not
  // available. So we don't bother. Yet.
  //
  // mortene & kyrah.
  GLUWRAPPER_REGISTER_FUNC(gluNurbsCallbackData, gluNurbsCallbackData_t);
#else /* !gluNurbsCallbackData */
  gi->gluNurbsCallbackData = NULL;
#endif /* !gluNurbsCallbackData */
  GLUWRAPPER_REGISTER_FUNC(gluNewTess, gluNewTess_t);
  GLUWRAPPER_REGISTER_FUNC(gluTessCallback, gluTessCallback_t);
  GLUWRAPPER_REGISTER_FUNC(gluTessProperty, gluTessProperty_t);
  GLUWRAPPER_REGISTER_FUNC(gluTessBeginPolygon, gluTessBeginPolygon_t);
  GLUWRAPPER_REGISTER_FUNC(gluTessEndPolygon, gluTessEndPolygon_t);
  GLUWRAPPER_REGISTER_FUNC(gluTessBeginContour, gluTessBeginContour_t);
  GLUWRAPPER_REGISTER_FUNC(gluTessEndContour, gluTessEndContour_t);
  GLUWRAPPER_REGISTER_FUNC(gluTessVertex, gluTessVertex_t);
  GLUWRAPPER_REGISTER_FUNC(gluDeleteTess, gluDeleteTess_t);
  GLUWRAPPER_REGISTER_FUNC(gluTessNormal, gluTessNormal_t);

  /* "Backup" functions, makes it easier to be robust even when no GLU
     library can be loaded. */
  if (gi->gluScaleImage == NULL)
    gi->gluScaleImage = GLUWrapper_gluScaleImage;
  if (gi->gluGetString == NULL) /* Was missing in GLU v1.0. */
    gi->gluGetString = GLUWrapper_gluGetString;

  /* Makes it possible to place a debugging "filter" in front of the
     gluNurbsSurface() function to dump the input arguments as debug
     output. Useful for debugging NURBS problems. */
  {
    const char * env = coin_getenv("COIN_DEBUG_GLUNURBSSURFACE");
    if (env && (atoi(env) > 0)) {
      gi->gluNurbsSurface_in_GLU = gi->gluNurbsSurface;
      if (gi->gluNurbsSurface != NULL) { gi->gluNurbsSurface = (gluNurbsSurface_t)GLUWrapper_gluNurbsSurface; }
    }
  }

  /* Parse the version string once and expose the version numbers
     through the GLUWrapper API.

     The debug override possibility is useful for testing what happens
     when an older GLU DLL is installed on a system.
  */
  {
    const GLubyte * versionstr = (const GLubyte *)coin_getenv("COIN_DEBUG_GLU_VERSION");
    if (!versionstr) { versionstr = gi->gluGetString(GLU_VERSION); }
    GLUWrapper_set_version(versionstr);
  }

wrapperexit:
  CC_SYNC_END(GLUWrapper);
  return GLU_instance;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
