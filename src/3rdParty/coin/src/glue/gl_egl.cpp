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
 *  Environment variable controls available:
 * 
 *   - COIN_EGLGLUE_NO_PBUFFERS: set to 1 to force software rendering of
 *     offscreen contexts.
 */

#include "glue/gl_egl.h"
#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <cstdlib>
#include <cstring>
#include <cassert>

#include <Inventor/C/tidbits.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/glue/dl.h>

#include "glue/glp.h"
#include "glue/dlp.h"

/* ********************************************************************** */

#ifndef HAVE_EGL

void * eglglue_getprocaddress(const cc_glglue * glue_in, const char * fname)
{
  return NULL;
}

void * eglglue_context_create_offscreen(unsigned int COIN_UNUSED_ARG(width),
                                        unsigned int COIN_UNUSED_ARG(height)) {
  return NULL;
}

SbBool eglglue_context_make_current(void * COIN_UNUSED_ARG(ctx))
{
  return FALSE;
}

void eglglue_context_reinstate_previous(void * COIN_UNUSED_ARG(ctx))
{
  assert(FALSE);
}

void eglglue_context_destruct(void * COIN_UNUSED_ARG(ctx))
{
  assert(FALSE);
}

SbBool eglglue_context_pbuffer_max(void * ctx, unsigned int * lims)
{
  assert(FALSE); return FALSE;
}

#else /* HAVE_EGL */

/* ********************************************************************** */

#include <EGL/egl.h>
#include <EGL/eglext.h>

EGLDisplay eglglue_display = EGL_NO_DISPLAY;
struct eglglue_contextdata;

#define CASE_STR( value ) case value: return #value;
const char* eglErrorString( EGLint error )
{
    switch( error )
    {
    CASE_STR( EGL_SUCCESS             )
    CASE_STR( EGL_NOT_INITIALIZED     )
    CASE_STR( EGL_BAD_ACCESS          )
    CASE_STR( EGL_BAD_ALLOC           )
    CASE_STR( EGL_BAD_ATTRIBUTE       )
    CASE_STR( EGL_BAD_CONTEXT         )
    CASE_STR( EGL_BAD_CONFIG          )
    CASE_STR( EGL_BAD_CURRENT_SURFACE )
    CASE_STR( EGL_BAD_DISPLAY         )
    CASE_STR( EGL_BAD_SURFACE         )
    CASE_STR( EGL_BAD_MATCH           )
    CASE_STR( EGL_BAD_PARAMETER       )
    CASE_STR( EGL_BAD_NATIVE_PIXMAP   )
    CASE_STR( EGL_BAD_NATIVE_WINDOW   )
    CASE_STR( EGL_CONTEXT_LOST        )
    default: return "Unknown";
    }
}
const char* eglAPIString( EGLenum api )
{
    switch( api )
    {
    CASE_STR( EGL_OPENGL_API          )
    CASE_STR( EGL_OPENGL_ES_API          )
    CASE_STR( EGL_OPENVG_API          )
    default: return "Unknown";
    }
}
#undef CASE_STR

struct eglglue_contextdata {
  EGLContext context;
  EGLSurface surface;
  EGLContext storedcontext;
  EGLSurface storedsurface;
  unsigned int width;
  unsigned int height;
};

static struct eglglue_contextdata *
eglglue_contextdata_init(unsigned int width, unsigned int height)
{
  struct eglglue_contextdata * ctx;
  ctx = (struct eglglue_contextdata *)malloc(sizeof(struct eglglue_contextdata));

  ctx->context = EGL_NO_CONTEXT;
  ctx->surface = EGL_NO_SURFACE;
  ctx->storedcontext = EGL_NO_CONTEXT;
  ctx->storedsurface = EGL_NO_SURFACE;
  ctx->width = width;
  ctx->height = height;
  return ctx;
}

static EGLDisplay
eglglue_get_display(void)
{
  PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;

  if (eglglue_display != EGL_NO_DISPLAY) {
      return eglglue_display;
  }

  eglglue_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (eglglue_display != EGL_NO_DISPLAY) {
    goto found;
  }

  eglGetPlatformDisplayEXT =
      (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
  if (!eglGetPlatformDisplayEXT) {
    return EGL_NO_DISPLAY;
  }

  eglglue_display = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, EGL_DEFAULT_DISPLAY, NULL);
  if (eglglue_display != EGL_NO_DISPLAY) {
    goto found;
  }

  eglglue_display = eglGetPlatformDisplay(EGL_PLATFORM_X11_EXT, EGL_DEFAULT_DISPLAY, NULL);

  if (eglglue_display == EGL_NO_DISPLAY) {
    cc_debugerror_post("eglglue_get_display", "Display not found.");
    return EGL_NO_DISPLAY;
  }

found:
  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("eglglue_get_display",
                            "got EGLDisplay==%p",
                            eglglue_display);
  }

  return eglglue_display;
}

void
eglglue_init(cc_glglue * w)
{
  w->glx.isdirect = 1;
  w->glx.serverversion = NULL;
  w->glx.servervendor = NULL;
  w->glx.serverextensions = NULL;
  w->glx.clientversion = NULL;
  w->glx.clientvendor = NULL;
  w->glx.clientextensions = NULL;
  w->glx.glxextensions = NULL;

  w->glx.glXGetCurrentDisplay = (COIN_PFNGLXGETCURRENTDISPLAYPROC)eglglue_getprocaddress(w, "eglglue_get_display");

  if (eglInitialize(eglglue_get_display(), &w->glx.version.major, &w->glx.version.minor) == EGL_FALSE) {
    cc_debugerror_post("eglglue_init",
                       "Couldn't initialize EGL. %s",
                        eglErrorString(eglGetError()));
    return;
  }

  if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
    cc_debugerror_post("eglglue_init",
                       "eglBindAPI(EGL_OPENGL_API) failed. %s",
                       eglErrorString(eglGetError()));
    return;
  }

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("eglglue_init",
                           "EGL version: %d.%d",
                            w->glx.version.major,
                            w->glx.version.minor);
    cc_debugerror_postinfo("eglglue_init",
                           "eglQueryString(EGL_VERSION)=='%s'",
                            eglQueryString(eglglue_get_display(), EGL_VERSION));
    cc_debugerror_postinfo("eglglue_init",
                           "eglQueryString(EGL_VENDOR)=='%s'",
                            eglQueryString(eglglue_get_display(), EGL_VENDOR));
    cc_debugerror_postinfo("eglglue_init",
                           "eglQueryString(EGL_CLIENT_APIS)=='%s'",
                            eglQueryString(eglglue_get_display(), EGL_CLIENT_APIS));
    cc_debugerror_postinfo("eglglue_init",
                           "eglQueryAPI()=='%s'",
                           eglAPIString(eglQueryAPI()));
    cc_debugerror_postinfo("eglglue_init",
                           "eglQueryString(EGL_EXTENSIONS)=='%s'",
                            eglQueryString(eglglue_get_display(), EGL_EXTENSIONS));
  }
}

static void
eglglue_contextdata_cleanup(struct eglglue_contextdata * ctx)
{
  if (ctx == NULL) { return; }
  if (eglglue_get_display() != EGL_NO_DISPLAY && ctx->context != EGL_NO_CONTEXT) eglDestroyContext(eglglue_get_display(), ctx->context);
  if (eglglue_get_display() != EGL_NO_DISPLAY && ctx->surface != EGL_NO_SURFACE) eglDestroySurface(eglglue_get_display(), ctx->surface);
  if (eglglue_get_display() != EGL_NO_DISPLAY && ctx->storedcontext != EGL_NO_CONTEXT) eglDestroyContext(eglglue_get_display(), ctx->storedcontext);
  if (eglglue_get_display() != EGL_NO_DISPLAY && ctx->storedsurface != EGL_NO_SURFACE) eglDestroySurface(eglglue_get_display(), ctx->storedsurface);
  free(ctx);
}

void *
eglglue_context_create_offscreen(unsigned int width, unsigned int height)
{
  struct eglglue_contextdata * ctx;
  EGLint format;
  EGLint numConfigs;
  EGLConfig config;
  EGLint attrib[] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 24,
    EGL_STENCIL_SIZE, 1,
    EGL_NONE
  };

  EGLAttrib surface_attrib[] = {
    EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
    EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
    EGL_WIDTH, (EGLint) ctx->width,
    EGL_HEIGHT, (EGLint) ctx->height,
    EGL_NONE
  };

  ctx = eglglue_contextdata_init(width, height);
  if (!ctx) return NULL;

  const int v = coin_glglue_stencil_bits_hack();
  if (v != -1) {
    attrib[15] = v;
  }

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("eglglue_context_create_offscreen",
                           "Creating offscreen context.");
  }

  if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
    cc_debugerror_post("eglglue_context_create_offscreen",
                       "eglBindAPI(EGL_OPENGL_API) failed. %s",
                       eglErrorString(eglGetError()));
    return NULL;
  }

  const char * env = coin_getenv("COIN_EGLGLUE_NO_PBUFFERS");
  if (env && atoi(env) > 0) {
    attrib[3] = EGL_PIXMAP_BIT;
    if (coin_glglue_debug()) {
      cc_debugerror_postinfo("eglglue_context_create_offscreen",
                             "Force software rendering.");
    }
  }

  eglChooseConfig(eglglue_get_display(), attrib, &config, 1, &numConfigs);
  if (numConfigs == 0) {
    if (attrib[3] == EGL_PBUFFER_BIT) {
      if (coin_glglue_debug()) {
        cc_debugerror_postinfo("eglglue_context_create_offscreen",
                               "PBuffer offscreen rendering is NOT supported "
                               "by the OpenGL driver. Try software rendering.");
      }
      attrib[3] = EGL_PIXMAP_BIT;
      eglChooseConfig(eglglue_get_display(), attrib, &config, 1, &numConfigs);
    }
  }
  if (numConfigs == 0) {
    cc_debugerror_post("eglglue_context_create_offscreen",
                       "No matching EGL config. %s",
                       eglErrorString(eglGetError()));
    eglglue_contextdata_cleanup(ctx);
    return NULL;
  }

  if (attrib[3] == EGL_PBUFFER_BIT) {
    ctx->surface = eglCreatePlatformWindowSurface(eglglue_get_display(), config, 0, surface_attrib);
  } else {
    ctx->surface = eglCreatePlatformPixmapSurface(eglglue_get_display(), config, 0, surface_attrib);
  }

  if (ctx->surface == EGL_NO_SURFACE) {
    cc_debugerror_post("eglglue_context_create_offscreen",
                       "Couldn't create EGL surface. %s",
                       eglErrorString(eglGetError()));
    eglglue_contextdata_cleanup(ctx);
    return NULL;
  }

  ctx->context = eglCreateContext(eglglue_get_display(), config, EGL_NO_CONTEXT, NULL);

  if (ctx->context == EGL_NO_CONTEXT) {
    cc_debugerror_post("eglglue_context_create_offscreen",
                       "Couldn't create EGL context. %s",
                       eglErrorString(eglGetError()));
    eglglue_contextdata_cleanup(ctx);
    return NULL;
  }

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("eglglue_context_create_offscreen",
                           "created new %s offscreen context == %p",
                           attrib[3] == EGL_PBUFFER_BIT ? "pBuffer" : "software",
                           ctx->context);
  }
  return ctx;
}

SbBool
eglglue_context_make_current(void * ctx)
{
  struct eglglue_contextdata * context = (struct eglglue_contextdata *)ctx;

  context->storedcontext = eglGetCurrentContext();
  context->storedsurface = eglGetCurrentSurface(EGL_DRAW);
  if (eglMakeCurrent(eglglue_get_display(), context->surface, context->surface, context->context) == EGL_FALSE) {
      cc_debugerror_post("eglglue_context_make_current",
                         "eglMakeCurrent failed: %s",
                         eglErrorString(eglGetError()));
      return FALSE;
  }

  if (coin_glglue_debug()) {
      cc_debugerror_postinfo("eglglue_context_make_current",
                             "EGL Context (0x%X)\n",
                             context->context);
  }
  return TRUE;
}

void
eglglue_context_reinstate_previous(void * ctx)
{
  struct eglglue_contextdata * context = (struct eglglue_contextdata *)ctx;

  if (context->storedcontext != EGL_NO_CONTEXT && context->storedsurface != EGL_NO_SURFACE) {
    if (eglMakeCurrent(eglglue_get_display(), context->storedsurface, context->storedsurface, context->storedcontext) == EGL_TRUE) {
      if (coin_glglue_debug()) {
        cc_debugerror_postinfo("eglglue_context_make_current",
                               "EGL Context (0x%X)\n",
                               context->context);
      }
    } else {
      cc_debugerror_post("eglglue_context_make_current",
                         "eglMakeCurrent failed: %s",
                         eglErrorString(eglGetError()));
    }
  }
}

void
eglglue_context_destruct(void * ctx)
{
  struct eglglue_contextdata * context = (struct eglglue_contextdata *)ctx;

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("eglglue_context_destruct",
                           "Destroying context %p", context->context);
  }
  eglglue_contextdata_cleanup(context);
}

void
eglglue_context_bind_pbuffer(void * ctx)
{
  struct eglglue_contextdata * context = (struct eglglue_contextdata *)ctx;

  if (eglBindTexImage(eglglue_get_display(), context->surface, EGL_BACK_BUFFER) == EGL_FALSE) {
    cc_debugerror_post("eglglue_context_bind_pbuffer()"
                       "after binding pbuffer: %s",
                       eglErrorString(eglGetError()));
  }
}

void
eglglue_context_release_pbuffer(void * ctx)
{
  struct eglglue_contextdata * context = (struct eglglue_contextdata *)ctx;

  if (eglReleaseTexImage(eglglue_get_display(), context->surface, EGL_BACK_BUFFER) == EGL_FALSE) {
    cc_debugerror_post("eglglue_context_release_pbuffer()"
                       "releasing pbuffer: %s",
                       eglErrorString(eglGetError()));
  }
}

SbBool
eglglue_context_pbuffer_is_bound(void * ctx)
{
  struct eglglue_contextdata * context = (struct eglglue_contextdata *)ctx;
  GLint buffer = EGL_NONE;

  if(eglQueryContext(eglglue_get_display(), context->context, EGL_RENDER_BUFFER, &buffer) == EGL_FALSE) {
    cc_debugerror_post("eglglue_context_pbuffer_is_bound()"
                       "after query pbuffer: %s",
                       eglErrorString(eglGetError()));
  }
  return buffer == EGL_BACK_BUFFER;
}

SbBool
eglglue_context_can_render_to_texture(void * ctx)
{
  struct eglglue_contextdata * context = (struct eglglue_contextdata *)ctx;
  return context->surface != EGL_NO_SURFACE;
}

SbBool
eglglue_context_pbuffer_max(void * ctx, unsigned int * lims)
{
  int returnval, attribval, i;
  const int attribs[] = {
    EGL_MAX_PBUFFER_WIDTH, EGL_MAX_PBUFFER_HEIGHT, EGL_MAX_PBUFFER_PIXELS
  };
  struct eglglue_contextdata * context = (struct eglglue_contextdata *)ctx;

  if (context->surface == EGL_NO_SURFACE) { return FALSE; }

  for (i = 0; i < 3; i++) {
    if(eglQuerySurface(eglglue_get_display(), context->surface, attribs[i], &attribval) == EGL_FALSE) {
      cc_debugerror_post("eglglue_context_pbuffer_max",
                         "eglQuerySurface() failed, "
                         "returned error code %s",
                         eglErrorString(eglGetError()));
      return FALSE;
    }
    assert(attribval >= 0);
    lims[i] = (unsigned int)attribval;
  }
  return TRUE;
}

void *
eglglue_getprocaddress(const cc_glglue * glue_in, const char * fname)
{
  return (void *)eglGetProcAddress(fname);
}

void
eglglue_cleanup(void)
{
  if (eglglue_display != EGL_NO_DISPLAY) eglTerminate(eglglue_display);
  eglglue_display = EGL_NO_DISPLAY;
}

#endif /* HAVE_EGL */
