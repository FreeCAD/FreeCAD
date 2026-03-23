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

/*
  Environment variable controls available:

  - COIN_GLXGLUE_NO_PBUFFERS: set to 1 to force software rendering of
    offscreen contexts.

  - COIN_GLXGLUE_NO_GLX13_PBUFFERS: don't use GLX 1.3 pbuffers support
    (will then attempt to use pbuffers through extensions).

  - COIN_GLX_PIXMAP_DIRECT_RENDERING: set to 1 to force direct rendering of
    offscreen contexts
*/

#include "glue/gl_glx.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>

#include <Inventor/C/basic.h>
#include <Inventor/C/glue/dl.h>
#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/tidbits.h>

#include "glue/dlp.h"
#include "glue/glp.h"

/* ********************************************************************** */

#ifndef HAVE_GLX

/* Dummy versions of the functions, when built without GLX: */

void glxglue_init(cc_glglue * w)
{
  w->glx.version.major = -1;
  w->glx.version.minor = 0;
  w->glx.isdirect = 1;

  w->glx.serverversion = NULL;
  w->glx.servervendor = NULL;
  w->glx.serverextensions = NULL;
  w->glx.clientversion = NULL;
  w->glx.clientvendor = NULL;
  w->glx.clientextensions = NULL;
  w->glx.glxextensions = NULL;
}

void * glxglue_getprocaddress(const cc_glglue * glue, const char * fname) { return NULL; }
int glxglue_ext_supported(const cc_glglue * w, const char * extension) { return 0; }

void * glxglue_context_create_offscreen(unsigned int width, unsigned int height) { assert(FALSE); return NULL; }
SbBool glxglue_context_make_current(void * ctx) { assert(FALSE); return FALSE; }
void glxglue_context_reinstate_previous(void * ctx) { assert(FALSE); }
void glxglue_context_destruct(void * ctx) { assert(FALSE); }

SbBool glxglue_context_pbuffer_max(void * ctx, unsigned int * lims) { assert(FALSE); return FALSE; }

#else /* HAVE_GLX */

/* ********************************************************************** */

/*
 * GL/glx.h includes X11/Xmd.h which contains typedefs for BOOL and
 * INT32 that conflict with the definitions in windef.h (which is
 * included from windows.h, which may be included from
 * Inventor/system/gl.h).  To avoid this conflict, we rename the
 * typedefs done in X11/Xmd.h to use other names (tempbool and
 * tempint32), and try to clean up the hack after the header has been
 * parsed.  2003-06-25 larsa
 */
#ifndef BOOL
#define BOOL tempbool
#define COIN_DEFINED_BOOL
#endif /* !BOOL */
#ifndef INT32
#define INT32 tempint32
#define COIN_DEFINED_INT32
#endif /* !INT32 */

#include <GL/glx.h>

/*
 * This is the cleanup part of the X11/Xmd.h conflict fix hack set up
 * above.  2003-06-25 larsa
 */
#ifdef COIN_DEFINED_BOOL
#undef BOOL
#undef COIN_DEFINED_BOOL
#endif /* COIN_DEFINED_BOOL */
#ifdef COIN_DEFINED_INT32
#undef INT32
#undef COIN_DEFINED_INT32
#endif /* COIN_DEFINED_INT32 */

/* ********************************************************************** */

static Display * glxglue_display = NULL;
static SbBool glxglue_opendisplay_failed = FALSE;

static int glxglue_screen = -1;

struct glxglue_contextdata;
static SbBool (* glxglue_context_create)(struct glxglue_contextdata * context) = NULL;

typedef void * COIN_GLXFBConfig;
typedef COIN_GLXFBConfig * (APIENTRY * COIN_PFNGLXCHOOSEFBCONFIG)(Display * dpy, int screen, const int * attrib_list, int * nelements);
typedef GLXContext (APIENTRY * COIN_PFNGLXCREATENEWCONTEXT)(Display * dpy, COIN_GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);
typedef int (APIENTRY * COIN_PFNGLXGETFBCONFIGATTRIB)(Display * dpy, COIN_GLXFBConfig config, int attribute, int * value);

static COIN_PFNGLXCHOOSEFBCONFIG glxglue_glXChooseFBConfig;
static COIN_PFNGLXCREATENEWCONTEXT glxglue_glXCreateNewContext;
static COIN_PFNGLXGETFBCONFIGATTRIB glxglue_glXGetFBConfigAttrib;

typedef XID COIN_GLXPbuffer;

typedef COIN_GLXPbuffer (APIENTRY * COIN_PFNGLXCREATEGLXPBUFFERSGIX)(Display * dpy,
                                                                     COIN_GLXFBConfig config,
                                                                     unsigned int width,
                                                                     unsigned int height,
                                                                     int * attrib_list);
typedef COIN_GLXPbuffer (APIENTRY * COIN_PFNGLXCREATEPBUFFER_GLX_1_3)(Display * dpy,
                                                                      COIN_GLXFBConfig config,
                                                                      const int * attrib_list);
typedef void (APIENTRY * COIN_PFNGLXDESTROYPBUFFER)(Display * dpy, COIN_GLXPbuffer pbuf);

static COIN_PFNGLXCREATEPBUFFER_GLX_1_3 glxglue_glXCreatePbuffer_GLX_1_3;
static COIN_PFNGLXCREATEGLXPBUFFERSGIX glxglue_glXCreateGLXPbufferSGIX;
static COIN_PFNGLXDESTROYPBUFFER glxglue_glXDestroyPbuffer;

/* ********************************************************************** */

/* Sanity checks for enum extension value assumed to be equal to the
 * final / "proper" / standard OpenGL enum values. (If not, we could
 * end up with hard-to-find bugs because of mismatches with the
 * compiled values versus the runtime values.)
 *
 * This doesn't really _fix_ anything, it is just meant as an aid to
 * smoke out platforms where we're getting unexpected enum values.
 */

#ifdef GLX_RENDER_TYPE_SGIX
#if GLX_RENDER_TYPE != GLX_RENDER_TYPE_SGIX
#error dangerous enum mismatch
#endif /* cmp */
#endif /* GLX_RENDER_TYPE_SGIX */

#ifdef GLX_DRAWABLE_TYPE_SGIX
#if GLX_DRAWABLE_TYPE != GLX_DRAWABLE_TYPE_SGIX
#error dangerous enum mismatch
#endif /* cmp */
#endif /* GLX_DRAWABLE_TYPE_SGIX */

#ifdef GLX_RGBA_TYPE_SGIX
#if GLX_RGBA_TYPE != GLX_RGBA_TYPE_SGIX
#error dangerous enum mismatch
#endif /* cmp */
#endif /* GLX_RGBA_TYPE_SGIX */

#ifdef GLX_RGBA_BIT_SGIX
#if GLX_RGBA_BIT != GLX_RGBA_BIT_SGIX
#error dangerous enum mismatch
#endif /* cmp */
#endif /* GLX_RGBA_BIT_SGIX */

#ifdef GLX_PBUFFER_BIT_SGIX
#if GLX_PBUFFER_BIT != GLX_PBUFFER_BIT_SGIX
#error dangerous enum mismatch
#endif /* cmp */
#endif /* GLX_PBUFFER_BIT_SGIX */

/* ********************************************************************** */

struct glxglue_contextdata {
  XVisualInfo * visinfo;
  GLXContext glxcontext;
  unsigned int width, height;
  Pixmap pixmap;
  GLXPixmap glxpixmap;

  Display * storeddisplay;
  GLXDrawable storeddrawable;
  GLXContext storedcontext;

  SbBool pbuffer;
  /* the next two are only valid if the offscreen context is a
     pbuffer: */
  Display * display;
  COIN_GLXFBConfig fbconfig;
};


/*

  We try two different ways of getting the display here.

  1) if we have a current context (glw), we try using glXGetCurrentDisplay, if it exists
  2) Fall back to XOpenDisplay()

 */
static Display *
glxglue_get_display(const cc_glglue * currentcontext = NULL)
{
  if (currentcontext && currentcontext->glx.glXGetCurrentDisplay) {
    Display *disp = (Display*)currentcontext->glx.glXGetCurrentDisplay();
    if (glxglue_screen == -1 && disp != NULL) {
      glxglue_screen = XScreenNumberOfScreen(XDefaultScreenOfDisplay(disp));
    }

    if (coin_glglue_debug()) {
      cc_debugerror_postinfo("glxglue_get_display", "got Display*==%p; got Screen==%d",
			     currentcontext->glx.glXGetCurrentDisplay(),
			     glxglue_screen);
    }

    return disp;
  }

  if ((glxglue_display == NULL) && !glxglue_opendisplay_failed) {
    /* FIXME: should use the real display-setting. :-(  20020926 mortene. */

    /* UPDATE 20090218 tamer: Passing NULL through XOpenDisplay()
     * makes a POSIX-conformant system default to the value of the
     * DISPLAY environment variable. Isn't that exactly what we want?
     * Do you mean that the display_name can potentially be provided
     * by other means than the DISPLAY envvar? */
    
    if (!(glxglue_display = XOpenDisplay(NULL))) {
      cc_debugerror_post("glxglue_init",
                         "Couldn't open NULL display.");
      glxglue_opendisplay_failed = TRUE;
    }
    
    glxglue_screen = XScreenNumberOfScreen(
      XDefaultScreenOfDisplay(glxglue_display));

    if (coin_glglue_debug()) {
      cc_debugerror_postinfo("glxglue_get_display", "got Display*==%p; got Screen==%d",
			     glxglue_display, glxglue_screen);
    }
  }
  return glxglue_display;
}

static void
glxglue_set_version(const cc_glglue * w, int * major, int * minor)
{
  Bool ok = False;

  *major = -1;
  *minor = 0;

  if (glxglue_get_display(w) == NULL) { return; }

  ok = glXQueryVersion(glxglue_get_display(w), major, minor);

  if (!ok) {
    cc_debugerror_post("glxglue_version",
                       "Couldn't decide GLX version on your system!");
  }

  if (ok && coin_glglue_debug()) {
    cc_debugerror_postinfo("glxglue_version",
                           "GLX version: %d.%d", *major, *minor);
  }
}

void *
glxglue_getprocaddress(const cc_glglue * glue_in, const char * fname)
{
  void * ptr = NULL;

  if (!glue_in->glx.glXGetProcAddress && !glue_in->glx.tried_bind_glXGetProcAddress) {
    cc_glglue * glue = const_cast<cc_glglue*> (glue_in);

    cc_libhandle h = coin_glglue_dl_handle(glue);

    if (h) {
      glue->glx.glXGetProcAddress = (COIN_PFNGLXGETPROCADDRESSPROC)
        cc_dl_sym(h, "glXGetProcAddress");

      if (coin_glglue_debug()) {
        cc_debugerror_postinfo("glxglue_getprocaddress",
                               "%s glXGetProcAddress()",
                               glue->glx.glXGetProcAddress ?
                               "picked up" : "can't use");
      }

      if (!glue->glx.glXGetProcAddress) {
        glue->glx.glXGetProcAddress = (COIN_PFNGLXGETPROCADDRESSPROC)
          cc_dl_sym(h, "glXGetProcAddressARB");
      }

      if (coin_glglue_debug()) {
        cc_debugerror_postinfo("glxglue_getprocaddress",
                               "%s glXGetProcAddressARB()",
                               glue->glx.glXGetProcAddress ?
                               "picked up" : "can't use");
      }
    }

    glue->glx.tried_bind_glXGetProcAddress = TRUE;
  }

  if (glue_in->glx.glXGetProcAddress) {
    ptr = (void *)glue_in->glx.glXGetProcAddress((const GLubyte *)fname);
  }

  return ptr;
}

static SbBool
glxglue_isdirect(cc_glglue * w)
{
  GLXContext ctx = glXGetCurrentContext();

  if (!ctx) {
    cc_debugerror_postwarning("glxglue_isdirect",
                              "Couldn't get current GLX context.");
    return TRUE;
  }

  if (!glxglue_get_display(w)) return TRUE;
  return glXIsDirect(glxglue_get_display(w), ctx) ? TRUE : FALSE;
}

int
glxglue_ext_supported(const cc_glglue * w, const char * extension)
{
  return
    (w->glx.glxextensions != NULL) &&
    coin_glglue_extension_available(w->glx.glxextensions, extension);
}

#ifdef HAVE_DYNAMIC_LINKING

#define PROC(_glue_, _func_) cc_glglue_getprocaddress(_glue_, SO__QUOTE(_func_))

/* The OpenGL library's GLX part which we dynamically pick up symbols
   from /could/ have all these defined. For the code below which tries
   to dynamically resolve the methods, we will assume that they are
   all defined. By doing this little "trick", can we use the same code
   below for resolving stuff dynamically as we need anyway to resolve
   in a static manner. */

#define GLX_VERSION_1_1 1
#define GLX_VERSION_1_2 1
#define GLX_VERSION_1_3 1

#define GLX_EXT_import_context 1
#define GLX_SGIX_fbconfig 1
#define GLX_SGIX_pbuffer 1

#else /* static binding */

#define PROC(_glue_, _func_) (&_func_)

#endif /* static binding */

static void
glxglue_resolve_symbols(cc_glglue * w)
{
  SbBool glx13pbuffer;
  const char * env;
  struct cc_glxglue * g = &(w->glx);

  glxglue_glXChooseFBConfig = NULL;
  glxglue_glXCreateNewContext = NULL;
  glxglue_glXGetFBConfigAttrib = NULL;

  env = coin_getenv("COIN_GLXGLUE_NO_GLX13_PBUFFERS");
  glx13pbuffer = (env == NULL) || (atoi(env) < 1);

#ifdef GLX_EXT_import_context
  if (!g->glXGetCurrentDisplay && glxglue_ext_supported(w, "GLX_EXT_import_context")) {
    g->glXGetCurrentDisplay = (COIN_PFNGLXGETCURRENTDISPLAYPROC)PROC(w, glXGetCurrentDisplayEXT);
  }
#endif /* GLX_EXT_import_context */

#ifdef GLX_VERSION_1_3
  if (glx13pbuffer && cc_glglue_glxversion_matches_at_least(w, 1, 3)) {
    glxglue_glXChooseFBConfig = (COIN_PFNGLXCHOOSEFBCONFIG)PROC(w, glXChooseFBConfig);
    glxglue_glXCreateNewContext = (COIN_PFNGLXCREATENEWCONTEXT)PROC(w, glXCreateNewContext);
    glxglue_glXGetFBConfigAttrib = (COIN_PFNGLXGETFBCONFIGATTRIB)PROC(w, glXGetFBConfigAttrib);
  }
#endif /* GLX_VERSION_1_3 */
#ifdef GLX_SGIX_fbconfig
  if (!glxglue_glXChooseFBConfig && glxglue_ext_supported(w, "GLX_SGIX_fbconfig")) {
    glxglue_glXChooseFBConfig = (COIN_PFNGLXCHOOSEFBCONFIG)PROC(w, glXChooseFBConfigSGIX);
    glxglue_glXCreateNewContext = (COIN_PFNGLXCREATENEWCONTEXT)PROC(w, glXCreateContextWithConfigSGIX);
    glxglue_glXGetFBConfigAttrib = (COIN_PFNGLXGETFBCONFIGATTRIB)PROC(w, glXGetFBConfigAttribSGIX);
  }
#endif /* GLX_SGIX_fbconfig */

  glxglue_glXCreatePbuffer_GLX_1_3 = NULL;
  glxglue_glXCreateGLXPbufferSGIX = NULL;
  glxglue_glXDestroyPbuffer = NULL;

#ifdef GLX_VERSION_1_3
  if (glx13pbuffer && cc_glglue_glxversion_matches_at_least(w, 1, 3)) {
    glxglue_glXCreatePbuffer_GLX_1_3 = (COIN_PFNGLXCREATEPBUFFER_GLX_1_3)PROC(w, glXCreatePbuffer);
    glxglue_glXDestroyPbuffer = (COIN_PFNGLXDESTROYPBUFFER)PROC(w, glXDestroyPbuffer);
  }
#endif /* GLX_VERSION_1_3 */

#ifdef GLX_SGIX_pbuffer
  if (!glxglue_glXCreatePbuffer_GLX_1_3 && glxglue_ext_supported(w, "GLX_SGIX_pbuffer")) {
    glxglue_glXCreateGLXPbufferSGIX = (COIN_PFNGLXCREATEGLXPBUFFERSGIX)PROC(w, glXCreateGLXPbufferSGIX);
    glxglue_glXDestroyPbuffer = (COIN_PFNGLXDESTROYPBUFFER)PROC(w, glXDestroyGLXPbufferSGIX);
  }
#endif /* GLX_SGIX_pbuffer */
}

static SbBool
glxglue_has_pbuffer_support(void)
{
  /* Make it possible to turn off pbuffers support completely. Mostly
     relevant for debugging purposes. */
  const char * env = coin_getenv("COIN_GLXGLUE_NO_PBUFFERS");
  void *uniquememptr;
  if (env && atoi(env) > 0) { return FALSE; }

  /* Dummy invocation of the glxglue_init() function, which is
     necessary to bind the below functions related to pbuffer
     support. */
  /* FIXME: this is a hack -- should come up with a better manner of
     getting non-context-specific functions bound (or perhaps we
     should treat this methods as context-specific and change code
     accordingly?). 20030815 mortene. */
  uniquememptr = (void *)glxglue_has_pbuffer_support;
  (void)cc_glglue_instance_from_context_ptr(uniquememptr);

  return
    glxglue_glXChooseFBConfig && glxglue_glXCreateNewContext &&
    (glxglue_glXCreatePbuffer_GLX_1_3 || glxglue_glXCreateGLXPbufferSGIX) &&
    glxglue_glXDestroyPbuffer;
}

void
glxglue_init(cc_glglue * w)
{
  /* SGI's glx.h header file shipped with the NVidia Linux drivers
     identifies glXGetCurrentDisplay() as a GLX 1.3 method, but Sun's
     GL man pages lists it as a GLX 1.2 function, ditto for HP's GL
     man pages, and ditto for AIX's man pages. (See top of this file
     for URL). So we will assume the man pages are correct.
  */
  struct cc_glxglue * g = &(w->glx);
  g->glXGetCurrentDisplay = NULL;
#ifdef GLX_VERSION_1_2
  g->glXGetCurrentDisplay = (COIN_PFNGLXGETCURRENTDISPLAYPROC)PROC(w, glXGetCurrentDisplay);
#endif /* GLX_VERSION_1_2 */

  glxglue_set_version(w, &w->glx.version.major, &w->glx.version.minor);
  w->glx.isdirect = glxglue_isdirect(w);


  w->glx.serverversion = NULL;
  w->glx.servervendor = NULL;
  w->glx.serverextensions = NULL;
  w->glx.clientversion = NULL;
  w->glx.clientvendor = NULL;
  w->glx.clientextensions = NULL;
  w->glx.glxextensions = NULL;

  if (glxglue_get_display(w)) {

    /* Note: be aware that glXQueryServerString(),
       glXGetClientString() and glXQueryExtensionsString() are all
       from GLX 1.1 -- just in case there are ever compile-time,
       link-time or runtime problems with this.  */

    Display * d = glxglue_get_display(w);
    w->glx.serverversion = glXQueryServerString(d, glxglue_screen, GLX_VERSION);
    w->glx.servervendor = glXQueryServerString(d, glxglue_screen, GLX_VENDOR);
    w->glx.serverextensions = glXQueryServerString(d, glxglue_screen, GLX_EXTENSIONS);

    w->glx.clientversion = glXGetClientString(d, GLX_VERSION);
    w->glx.clientvendor = glXGetClientString(d, GLX_VENDOR);
    w->glx.clientextensions = glXGetClientString(d, GLX_EXTENSIONS);

    w->glx.glxextensions = glXQueryExtensionsString(d, glxglue_screen);

    if (coin_glglue_debug()) {
      cc_debugerror_postinfo("glxglue_init",
                             "glXQueryServerString(GLX_VERSION)=='%s'",
                             w->glx.serverversion);
      cc_debugerror_postinfo("glxglue_init",
                             "glXQueryServerString(GLX_VENDOR)=='%s'",
                             w->glx.servervendor);
      cc_debugerror_postinfo("glxglue_init",
                             "glXQueryServerString(GLX_EXTENSIONS)=='%s'",
                             w->glx.serverextensions);

      cc_debugerror_postinfo("glxglue_init",
                             "glXClientString(GLX_VERSION)=='%s'",
                             w->glx.clientversion);
      cc_debugerror_postinfo("glxglue_init",
                             "glXClientString(GLX_VENDOR)=='%s'",
                             w->glx.clientvendor);
      cc_debugerror_postinfo("glxglue_init",
                             "glXClientString(GLX_EXTENSIONS)=='%s'",
                             w->glx.clientextensions);

      cc_debugerror_postinfo("glxglue_init",
                             "glXQueryExtensionsString()=='%s'",
                             w->glx.glxextensions);
    }
  }

  glxglue_resolve_symbols(w);
}

/* NOTE: the strategy applied here for iterating through OpenGL canvas
   settings is exactly the same as the one applied in
   SoXt/src/Inventor/Xt/SoXtGLWidget.cpp. So if you make any fixes or
   other improvements here, migrate your changes. */
static int
glxglue_build_GL_attrs(int * attrs, int trynum)
{
  int pos = 0;
  attrs[pos++] = GLX_RGBA;
  attrs[pos++] = GLX_DEPTH_SIZE;
  attrs[pos++] = 1;

  /* FIXME: An accumulator buffer should be added if numpasses >
     1. Mesa returns a GL_INVALID_OPERATION error if the accumulator
     buffer is missing, while NVIDIA automatically creates
     one. 20021127 handegar. */

  if (! (trynum & 0x04)) {
    /* FIXME: the following is a hack to get around a problem which
       really demands more effort to be solved properly.

       The problem is that there is no way in the API of the
       SoOffscreenRenderer class to specify what particular attributes
       to request. This most often manifests itself as a problem for
       app programmers in that they have made some kind of extension
       node which uses the OpenGL stencil buffer. If no stencil buffer
       happens to be part of the GL context format for the offscreen
       renderer, these will not work properly. At the same time, we
       don't want to default to requesting a stencil buffer, as that
       takes a non-trivial amount of extra memory resources on the gfx
       card.

       So until we have implemented the proper solution for making it
       possible to pass in a detailed specification of which
       attributes to request from offscreen GL contexts, we provide
       this temporary work-around: the app programmer can set an
       envvar with a value specifying the number of stencil buffer
       bits he/she wants.

       20060223 mortene.
    */
    const int v = coin_glglue_stencil_bits_hack();
    attrs[pos++] = GLX_STENCIL_SIZE;
    attrs[pos++] = (v == -1) ? 1 : v;
  }
  if (! (trynum & 0x02)) {
    attrs[pos++] = GLX_ALPHA_SIZE;
    attrs[pos++] = 4;
  }
  if (! (trynum & 0x01)) {
    attrs[pos++] = GLX_RED_SIZE;
    attrs[pos++] = 4;
    attrs[pos++] = GLX_GREEN_SIZE;
    attrs[pos++] = 4;
    attrs[pos++] = GLX_BLUE_SIZE;
    attrs[pos++] = 4;
  }


  /* FIXME: we're currently not giving any hint as to whether we want
     a single- / double- / quad-buffer visual. Using anything than a
     single-buffer visual for offscreen rendering is probably
     wasteful, so we're likely to often use up more resources than we
     really need to do.

     20061025 mortene.
  */

  attrs[pos++] = None;

  return pos;
}

static XVisualInfo *
glxglue_find_gl_visual(void)
{
  int trynum = 0;
  /* This used to be an "const int", but not all C compilers can
     handle arrays declaration with const int sizes. (We had a report
     of this failing for some compiler under HP-UX 10.20.) */
#define ARRAYSIZE 32
  int attrs[ARRAYSIZE];
  XVisualInfo * visinfo = NULL;

  if (glxglue_get_display() == NULL) { return NULL; }

  while (visinfo == NULL && trynum < 8) {
    int arraysize = glxglue_build_GL_attrs(attrs, trynum);
    assert(arraysize < ARRAYSIZE);
    visinfo = glXChooseVisual(glxglue_get_display(), DefaultScreen(glxglue_get_display()),
                              attrs);
    trynum++;
  }

  if (!visinfo) {
    cc_debugerror_postwarning("glxglue_find_gl_visual",
                              "Couldn't get any OpenGL-capable RGBA X11 visual.");
    return NULL;
  }

  return visinfo;
#undef ARRAYSIZE
}

/*** GLX offscreen contexts **************************************************/


static struct glxglue_contextdata *
glxglue_contextdata_init(unsigned int width, unsigned int height)
{
  struct glxglue_contextdata * ctx;

  XVisualInfo * vi = glxglue_find_gl_visual();
  if (vi == NULL) { return NULL; }

  ctx = (struct glxglue_contextdata *)malloc(sizeof(struct glxglue_contextdata));

  ctx->visinfo = vi;
  ctx->glxcontext = NULL;
  ctx->width = width;
  ctx->height = height;

  ctx->pixmap = 0;
  ctx->glxpixmap = 0;

  ctx->storeddisplay = NULL;
  ctx->storeddrawable = 0;
  ctx->storedcontext = NULL;
  ctx->pbuffer = FALSE;

  return ctx;
}

static void
glxglue_contextdata_cleanup(struct glxglue_contextdata * ctx)
{
  if (ctx == NULL) { return; }

  Display * display = glxglue_get_display(NULL);
  if (ctx->glxcontext) glXDestroyContext(display, ctx->glxcontext);
  if (ctx->glxpixmap) {
    if (ctx->pbuffer) { glxglue_glXDestroyPbuffer(display, ctx->glxpixmap); }
    else { glXDestroyGLXPixmap(display, ctx->glxpixmap); }
  }
  if (ctx->pixmap) XFreePixmap(display, ctx->pixmap);
  if (ctx->visinfo) XFree(ctx->visinfo);

  free(ctx);
}

static SbBool
glxglue_context_create_software(struct glxglue_contextdata * context)
{
  /* Note that the value of the last argument of glXCreateContext()
      was "False" on purpose, as the man pages where saying:

          [...] direct rendering contexts [...] may be unable to
          render to GLX pixmaps [...]

     However, it still mentions:

          It may not be possible to render to a GLX pixmap with a 
	  direct rendering context.

     But in for example in RHEL8 indirect rendering has been disabled
     following X.Org Secutiry Advisory:
     https://www.x.org/wiki/Development/Security/Advisory-2014-12-09/

     This argument can now be forced to a specific value by using the
     environment variable COIN_GLX_PIXMAP_DIRECT_RENDERING
     */

  static Bool direct_rendering = False;
  static int check_direct = -1;

  if (check_direct == -1) {
    check_direct = 0;
    const char * env = coin_getenv("COIN_GLX_PIXMAP_DIRECT_RENDERING");
    direct_rendering = env && strtol(env, NULL, 10) >= 1 ? True : False;
  }

  Display * display = glxglue_get_display(NULL);
  context->glxcontext = glXCreateContext(display, context->visinfo, 0,
                                         direct_rendering);

  if (context->glxcontext == NULL) {
    cc_debugerror_postwarning("glxglue_context_create_software",
                              "Couldn't create GLX context.");
    return FALSE;
  }

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("glxglue_context_create_software",
                           "made new offscreen context == %p",
                           context->glxcontext);
  }

  context->pixmap = XCreatePixmap(display,
                                  DefaultRootWindow(display),
                                  context->width, context->height, context->visinfo->depth);
  if (context->pixmap == 0) {
    cc_debugerror_postwarning("glxglue_context_create_software",
                              "Couldn't create %dx%dx%d X11 Pixmap.",
                              context->width, context->height, context->visinfo->depth);
    return FALSE;
  }

  context->glxpixmap = glXCreateGLXPixmap(display,
                                          context->visinfo, context->pixmap);
  if (context->glxpixmap == 0) {
    cc_debugerror_postwarning("glxglue_context_create_software",
                              "Couldn't create GLX Pixmap.");
    return FALSE;
  }

  return TRUE;
}

static COIN_GLXPbuffer
glxglue_glXCreatePbuffer(Display * dpy, COIN_GLXFBConfig config, int width, int height)
{
  int glx13_attrs[] = {
    GLX_PBUFFER_WIDTH, -1,
    GLX_PBUFFER_HEIGHT, -1,
    None
  };

  int sgix_attrs[] = {
    None
  };

  glx13_attrs[1] = width;
  glx13_attrs[3] = height;

  if (glxglue_glXCreatePbuffer_GLX_1_3) {
    return glxglue_glXCreatePbuffer_GLX_1_3(dpy, config, glx13_attrs);
  }

  assert(glxglue_glXCreateGLXPbufferSGIX);
  /* The official SGIX pbuffer extensions documentation says the
     following about the glXCreateGLXPbufferSGIX() function:

         <attrib_list> can be either NULL, in which case all the
         attributes assume their default values as described
         below. [...]

     ..but leaving attrib_list (i.e. the last argument) as NULL causes
     a crash with NVidia's Linux driver, at least in version 41.91.
  */
  return glxglue_glXCreateGLXPbufferSGIX(dpy, config, width, height, sgix_attrs);
}

static SbBool
glxglue_context_create_pbuffer(struct glxglue_contextdata * context)
{
  /* FIXME: before we get here, we should have checked the requested
     dimensions in the context struct versus GLX_MAX_PBUFFER_WIDTH,
     GLX_MAX_PBUFFER_HEIGHT and GLX_MAX_PBUFFER_PIXELS
     somewhere. 20030811 mortene. */

  COIN_GLXPbuffer pb;
  COIN_GLXFBConfig * fbc;
  Display * dpy;

  /* number of FBConfigs returned */
  int fbc_cnt;

  /* set frame buffer attributes */
  /* FIXME: should refactor the attribute selection / setting process
     to one common with the software offscreen buffer creation. This
     is lame. 20060307 mortene. */
  int attrs[] = {
    GLX_STENCIL_SIZE, 1, /* FIXME: must be first, due to the hack
                            below where we may change the value of the
                            stencil bit setting. 20060307 mortene. */
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_RED_SIZE,   8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE,  8,
    GLX_ALPHA_SIZE, 8,
    GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
    GLX_DEPTH_SIZE, 24,
    None
  };

  /* FIXME: hack. See comments in source code elsewhere in this file
     where the same function is used. 20060307 mortene. */
  const int v = coin_glglue_stencil_bits_hack();
  assert(attrs[0] == GLX_STENCIL_SIZE);
  if (v != -1) { attrs[1] = v; };

  dpy = glxglue_get_display(NULL);
  if (!dpy) { return FALSE; }

  /* get a list of matching GLX frame buffer configurations. the list is
     sorted according to precedence rules where the first entry should be
     fine. */

  fbc = glxglue_glXChooseFBConfig(dpy, DefaultScreen(dpy), attrs, &fbc_cnt);
  assert(fbc_cnt >= 0);
  if ((fbc_cnt == 0) || (fbc == NULL)) {
    /* FIXME: we have had reports of this hitting. Is it possible to
       improve the selection technique so we can be absolutely sure no
       usable fb-config is available, e.g. by iterating over all
       available ones? 20040706 mortene. */
    cc_debugerror_postwarning("glxglue_context_create_pbuffer",
                              "glXChooseFBConfig() gave no valid configs");
    return FALSE;
  }

  pb = glxglue_glXCreatePbuffer(dpy, fbc[0], context->width, context->height);

  if (pb == 0) {
    cc_debugerror_postwarning("glxglue_context_create_pbuffer",
                              "glXCreatePbuffer(..., ..., %d, %d) failed",
                              context->width, context->height);
    return FALSE;
  }

  /* direct rendering graphic context creation == Hardware use */

  context->glxcontext = glxglue_glXCreateNewContext(dpy, fbc[0],
                                                    GLX_RGBA_TYPE, NULL, TRUE);

  /* must store this before freeing the array */
  context->fbconfig = fbc[0];

  /* free the config list */
  XFree(fbc);

  if (context->glxcontext == NULL) {
    cc_debugerror_postwarning("glxglue_context_create_pbuffer",
                              "Couldn't create GLX context.");
    return FALSE;
  }

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("glxglue_context_create_pbuffer",
                           "made new pbuffer offscreen context == %p",
                           context->glxcontext);
  }

  /* assign our pbuffer to glxpixmap */
  context->glxpixmap = pb;
  context->pbuffer = TRUE;
  context->display = dpy;

  return TRUE;
}

/* ********************************************************************** */

/* Create and return a handle to an offscreen OpenGL buffer.

   Where p-buffer support is available that will be used instead of a
   standard offscreen GLX context, as it should render much faster
   (due to hardware acceleration).

   See: http://www.oss.sgi.com/projects/ogl-sample/registry/SGIX/pbuffer.txt

   The initial pbuffer implementation was contributed by Tamer Fahmy
   and Hannes Kaufmann.
*/
void *
glxglue_context_create_offscreen(unsigned int width, unsigned int height)
{
  SbBool ok, pbuffer;
  struct glxglue_contextdata * swctx, * pbctx;

  swctx = glxglue_contextdata_init(width, height);
  if (swctx == NULL) { return NULL; }

  if (glxglue_context_create != NULL) {
    ok = glxglue_context_create(swctx);
    if (ok) { return swctx; }

    glxglue_contextdata_cleanup(swctx);
    return NULL;
  }

  /* As there could possibly be no valid glx context at this moment,
     we have to first make a context and set it current to be able
     to query pbuffer extension availability. */

  ok = glxglue_context_create_software(swctx);
  if (!ok || !glxglue_context_make_current(swctx)) {
    glxglue_contextdata_cleanup(swctx);
    return NULL;
  }

  /* ok, so we can at least use a non-pbuffer offscreen context */
  glxglue_context_create = glxglue_context_create_software;

  /* next, check if pbuffer support is available in the OpenGL
     library image */

  pbuffer = glxglue_has_pbuffer_support();

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("glxglue_context_create_offscreen",
                           "PBuffer offscreen rendering is %ssupported "
                           "by the OpenGL driver", pbuffer ? "" : "NOT ");
  }

  glxglue_context_reinstate_previous(swctx);

  if (!pbuffer) { return swctx; }

  /* attempt to make a pbuffer, to make sure the system is actually
     set up with that capability (just having the GLX methods
     available doesn't really prove it) */

  pbctx = glxglue_contextdata_init(width, height);
  if (pbctx == NULL) { return swctx; }

  ok = glxglue_context_create_pbuffer(pbctx);

  if (!ok) {
    glxglue_contextdata_cleanup(pbctx);
    return swctx;
  }

  /* pbuffers are really supported, kill the software offscreen
     context and use the pbuffer-enabled one */

  glxglue_contextdata_cleanup(swctx);

  glxglue_context_create = glxglue_context_create_pbuffer;

  return pbctx;
}

SbBool
glxglue_context_make_current(void * ctx)
{
  struct glxglue_contextdata * context = (struct glxglue_contextdata *)ctx;
  Bool r;

  context->storedcontext = glXGetCurrentContext();
  if (context->storedcontext) {
    /* Must know for sure that there's a current context before
       instantiating a glglue, or we'll get a crash due to the OpenGL
       calls within GLWrapper(). */
    const cc_glglue * glw = cc_glglue_instance_from_context_ptr(context->storedcontext);
    context->storeddisplay = (Display *)cc_glglue_glXGetCurrentDisplay(glw);
    context->storeddrawable = glXGetCurrentDrawable();
  }

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("glxglue_make_context_current",
                           "store current status first => context==%p, "
                           "drawable==%p, display==%p",
                           context->storedcontext,
                           context->storeddrawable,
                           context->storeddisplay);
  }

  Display * display = glxglue_get_display(NULL);
  r = glXMakeCurrent(display, context->glxpixmap, context->glxcontext);

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("glxglue_make_context_current",
                           "%s context %p current",
                           (r == True) ? "successfully made" : "failed to make",
                           context->glxcontext);
  }

  return (r == True) ? TRUE : FALSE;
}

void
glxglue_context_reinstate_previous(void * ctx)
{
  struct glxglue_contextdata * context = (struct glxglue_contextdata *)ctx;

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("glxglue_context_reinstate_previous",
                           "releasing context (glxMakeCurrent(%p, None, NULL))",
                           glxglue_get_display(NULL));
  }

  Display * display = glxglue_get_display(NULL);
  /* FIXME: this causes a crash with ATI on Linux for me. ATI and Mesa
     is somehow mixed together, which is probably the reason why the
     crash happens..? 20041105 mortene. */
  (void)glXMakeCurrent(display, None, NULL); /* release */

  /* The previous context is stored and reset to make it possible to
     use an SoOffscreenRenderer from for instance an SoCallback node
     callback during SoGLRenderAction traversal, without the need for
     any extra book-keeping on the application side. */

  if (context->storedcontext && context->storeddrawable && context->storeddisplay) {
    if (coin_glglue_debug()) {
      cc_debugerror_postinfo("glxglue_context_reinstate_previous",
                             "restoring context %p to be current "
                             "(drawable==%p, display==%p)",
                             context->storedcontext,
                             context->storeddrawable,
                             context->storeddisplay);
    }

    /* FIXME: this causes a crash for the Mesa version 3.4.2 that
       comes with XFree86 v4, on the third invocation after two
       successful runs first. This is _bad_. 20020729 mortene.

       UPDATE: this might be our bug, and could be fixed now --
       test. 20020802 mortene. */
    (void)glXMakeCurrent(context->storeddisplay, context->storeddrawable,
                         context->storedcontext);
  }
}

void
glxglue_context_destruct(void * ctx)
{
  /* FIXME: needs to call into the (as of yet unimplemented)
     C wrapper around the SoContextHandler. 20030310 mortene. */

  struct glxglue_contextdata * context = (struct glxglue_contextdata *)ctx;

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("glxglue_context_destruct",
                           "destroy context %p", context->glxcontext);
  }

  glxglue_contextdata_cleanup(context);
}

/* ********************************************************************** */

/* If ctx does not point at a pbuffer context, but rather a "normal"
   offscreen context, it will return FALSE.

   Upon other error conditions, FALSE will also be returned.
*/
SbBool
glxglue_context_pbuffer_max(void * ctx, unsigned int * lims)
{
  int returnval, attribval, i;
  const int attribs[] = {
    GLX_MAX_PBUFFER_WIDTH, GLX_MAX_PBUFFER_HEIGHT, GLX_MAX_PBUFFER_PIXELS
  };
  struct glxglue_contextdata * context = (struct glxglue_contextdata *)ctx;

  if (!context->pbuffer) { return FALSE; }
  if (!glxglue_glXGetFBConfigAttrib) { return FALSE; }

  for (i = 0; i < 3; i++) {
    returnval = glxglue_glXGetFBConfigAttrib(context->display,
                                             context->fbconfig,
                                             attribs[i],
                                             &attribval);
    if (returnval != Success) {
      cc_debugerror_post("glxglue_context_pbuffer_max",
                         "glXGetFBConfigAttrib() failed, "
                         "returned error code %d", returnval);
      return FALSE;
    }
    assert(attribval >= 0);
    lims[i] = (unsigned int)attribval;
  }

  return TRUE;
}

/* ********************************************************************** */

void glxglue_cleanup(void)
{
  glxglue_screen = -1;
  glxglue_context_create = NULL;

  glxglue_glXChooseFBConfig = NULL;
  glxglue_glXCreateNewContext = NULL;
  glxglue_glXGetFBConfigAttrib = NULL;
  glxglue_glXCreatePbuffer_GLX_1_3 = NULL;
  glxglue_glXCreateGLXPbufferSGIX = NULL;
  glxglue_glXDestroyPbuffer = NULL;

  /* FIXME: We used to not close the display due to potential problems
     on some NVidia drivers (see original comment, reproduced
     below). I think it's wrong to not cleanup properly on all systems
     just because of an issue with *one* driver -- so I've enabled it
     now. When the problem re-appears, we should disable it *for that
     specific driver*. 20060210 kyrah

     Original problem description:

       The Display resource is never deallocated explicitly (but of
       course implicitly by the system on application close
       down). This to work around some strange problems with the
       NVidia-driver 29.60 on XFree86 v4 when using XCloseDisplay() --
       like doublebuffered visuals coming up just blank.
   */
  if (glxglue_display) XCloseDisplay(glxglue_display);
  glxglue_display = NULL;
  glxglue_opendisplay_failed = FALSE;
}

#endif /* HAVE_GLX */
