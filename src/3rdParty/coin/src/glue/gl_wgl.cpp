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

#include "glue/gl_wgl.h"
#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <cstdlib>
#include <cassert>

#include <Inventor/C/glue/gl.h>
#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/C/glue/dl.h>
#include <Inventor/elements/SoGLCacheContextElement.h>

#include "glue/glp.h"
#include "glue/dlp.h"

#ifdef HAVE_WIN32_API
/* Conditional inclusion, as the functions in win32api.h will not be
   implemented unless the Win32 API is available. */
#include "glue/win32api.h"
#endif /* HAVE_WIN32_API */

#ifndef HAVE_WGL

/* ********************************************************************** */

/* Dummy implementations, for when WGL is not available: */

void * coin_wgl_getprocaddress(const cc_glglue * COIN_UNUSED_ARG(glue), const char * COIN_UNUSED_ARG(fname)) { return NULL; }

void * wglglue_context_create_offscreen(unsigned int COIN_UNUSED_ARG(width), unsigned int COIN_UNUSED_ARG(height)) { assert(FALSE); return NULL; }
SbBool wglglue_context_make_current(void * COIN_UNUSED_ARG(ctx)) { assert(FALSE); return FALSE; }
void wglglue_context_reinstate_previous(void * COIN_UNUSED_ARG(ctx)) { assert(FALSE); }
void wglglue_context_destruct(void * COIN_UNUSED_ARG(ctx)) { assert(FALSE); }

void wglglue_context_bind_pbuffer(void * COIN_UNUSED_ARG(ctx)) { assert(FALSE); }
void wglglue_context_release_pbuffer(void * COIN_UNUSED_ARG(ctx)) { assert(FALSE); }
SbBool wglglue_context_pbuffer_is_bound(void * COIN_UNUSED_ARG(ctx)) { assert(FALSE); return FALSE; }
SbBool wglglue_context_can_render_to_texture(void * COIN_UNUSED_ARG(ctx)) { assert(FALSE); return FALSE; }

SbBool wglglue_context_pbuffer_max(void * COIN_UNUSED_ARG(ctx), unsigned int * COIN_UNUSED_ARG(lims)) { assert(FALSE); return FALSE; }

/* ********************************************************************** */

#else /* HAVE_WGL */

// This method is for tagging casts that actually need to be the old C-style
// way (http://www.trilithium.com/johan/2004/12/problem-with-dlsym/) so they
// are not rewritten to static_cast<> or something similar in the future.

namespace { namespace gl_wgl_internal {

template <typename Type>
Type cstyle_cast(PROC procaddr)
{
  return (Type) procaddr;
}

} }

struct wglglue_contextdata;
static SbBool (* wglglue_context_create)(struct wglglue_contextdata * context, SbBool warnonerrors) = NULL;

/* ********************************************************************** */

/* Declared in the same manner as in the OpenGL ARB document for the
   pbuffer extension, but with a different name to avoid clashes upon
   static binding.

   Note that I couldn't find any documentation on the DECLARE_HANDLE
   thinga-majingy, so I would appreciate it if someone could confirm
   that this is correct.

   -mortene.
*/
DECLARE_HANDLE(WGLGLUE_HPBUFFER);

/* The following are from either the WGL_ARB_pbuffer or the
   WGL_EXT_pbuffer extensions: */

typedef WGLGLUE_HPBUFFER (WINAPI * COIN_PFNWGLCREATEPBUFFERPROC)(HDC hDC,
                                                               int iPixelFormat,
                                                               int iWidth,
                                                               int iHeight,
                                                               const int * piAttribList);
typedef HDC (WINAPI * COIN_PFNWGLGETPBUFFERDCPROC)(WGLGLUE_HPBUFFER hPbuffer);
typedef int (WINAPI * COIN_PFNWGLRELEASEPBUFFERDCPROC)(WGLGLUE_HPBUFFER hPbuffer,
                                                          HDC hDC);
typedef BOOL (WINAPI * COIN_PFNWGLDESTROYPBUFFERPROC)(WGLGLUE_HPBUFFER hPbuffer);
typedef BOOL (WINAPI * COIN_PFNWGLQUERYPBUFFERPROC)(WGLGLUE_HPBUFFER hPbuffer,
                                                       int iAttribute,
                                                       int * piValue);

typedef BOOL (WINAPI * COIN_PFNWGLBINDTEXIMAGEARBPROC)(WGLGLUE_HPBUFFER hPbuffer, int iBuffer);
typedef BOOL (WINAPI * COIN_PFNWGLRELEASETEXIMAGEARBPROC)(WGLGLUE_HPBUFFER hPbuffer, int iBuffer);

static COIN_PFNWGLCREATEPBUFFERPROC wglglue_wglCreatePbuffer = NULL;
static COIN_PFNWGLGETPBUFFERDCPROC wglglue_wglGetPbufferDC = NULL;
static COIN_PFNWGLRELEASEPBUFFERDCPROC wglglue_wglReleasePbufferDC = NULL;
static COIN_PFNWGLDESTROYPBUFFERPROC wglglue_wglDestroyPbuffer = NULL;
static COIN_PFNWGLQUERYPBUFFERPROC wglglue_wglQueryPbuffer = NULL;
static COIN_PFNWGLBINDTEXIMAGEARBPROC wglglue_wglBindTexImageARB = NULL;
static COIN_PFNWGLBINDTEXIMAGEARBPROC wglglue_wglReleaseTexImageARB = NULL;


/* The following is from either the WGL_ARB_pixel_format or the
   WGL_EXT_pixel_format extensions: */

typedef BOOL (WINAPI * COIN_PFNWGLCHOOSEPIXELFORMATPROC)(HDC hdc,
                                                         const int * piAttribIList,
                                                         const FLOAT * pfAttribFList,
                                                         UINT nMaxFormats,
                                                         int * piFormats,
                                                         UINT * nNumFormats);

typedef BOOL (WINAPI * COIN_PFNWGLGETPIXELFORMATATTRIBIVPROC)(HDC hdc,
                                                              int iPixelFormat,
                                                              int iLayerPlane,
                                                              UINT nAttribautes,
                                                              const int * piAttributes,
                                                              int * piValues);

static COIN_PFNWGLCHOOSEPIXELFORMATPROC wglglue_wglChoosePixelFormat = NULL;
static COIN_PFNWGLGETPIXELFORMATATTRIBIVPROC wglglue_wglGetPixelFormatAttribiv = NULL;

/* The function for finding extension strings is itself an extension
   string. */

typedef const char * (WINAPI * COIN_PFNWGLGETEXTENSIONSSTRING)(HDC hDC);

static COIN_PFNWGLGETEXTENSIONSSTRING wglglue_wglGetExtensionsString = NULL;

/* ********************************************************************** */

#ifdef HAVE_DYNAMIC_LINKING
#define PROC(_glue_, _func_) cc_glglue_getprocaddress(_glue_, SO__QUOTE(_func_))

/* The OpenGL library's WGL part which we dynamically pick up symbols
   from /could/ have these defined. For the code below which tries to
   dynamically resolve the methods, we will assume that they are
   defined. By doing this little "trick", can we use the same code
   below for resolving stuff dynamically as we need anyway to resolve
   in a static manner. */

#define WGL_ARB_pixel_format 1
#define WGL_EXT_pixel_format 1

#define WGL_ARB_pbuffer 1
#define WGL_EXT_pbuffer 1

#define WGL_ARB_extensions_string 1
#define WGL_EXT_extensions_string 1

#define WGL_ARB_render_texture 1
#else /* static binding */

#define PROC(_glue_, _func_) (&_func_)

#endif /* static binding */

/* ********************************************************************** */

static SbBool attemptedextresolved = FALSE;

/* ********************************************************************** */

void *
coin_wgl_getprocaddress(const cc_glglue * glue, const char * fname)
{
  void * ptr = gl_wgl_internal::cstyle_cast<void *>(wglGetProcAddress(fname));

  /* wglGetProcAddress() seems to only be able to fetch
     function-addresses for *extension* functions, not "proper" OpenGL
     (1.1+) functions. */
  
  if (ptr == NULL) {
    cc_libhandle glhnd = coin_glglue_dl_handle(glue);
    
    
    if (!glhnd && coin_glglue_debug()) {
      cc_debugerror_postwarning("coin_wgl_getprocaddress",
                                "couldn't get hold of any workable module "
                                "handle for picking up OpenGL symbols");
    }
    if (glhnd) {
      ptr = cc_dl_sym(glhnd, fname);
      
      if (ptr && coin_glglue_debug()) {
        cc_debugerror_postinfo("coin_wgl_getprocaddress",
                               "wglGetProcAddress() missed \"%s\", "
                               "but found with cc_dl_sym()", fname);
      }
    }
  }
  return ptr;
}

/*** WGL offscreen contexts ***********************************************/

struct wglglue_contextdata {
  unsigned int width, height;

  HDC memorydc;
  HWND pbufferwnd;
  SbBool didcreatememorydc;
  SbBool shouldreleasememorydc;
  HBITMAP bitmap, oldbitmap;
  HGLRC wglcontext;

  HGLRC storedcontext;
  HDC storeddc;

  WGLGLUE_HPBUFFER hpbuffer;
  SbBool noappglcontextavail;

  SbBool supports_render_to_texture;
  SbBool wanted_render_to_texture;
  SbBool pbufferisbound;
  void *pvBits;
  int pixelformat;
};

/* This abomination is needed to support SoOffscreenRenderer::getDC(). */
const void *
wglglue_context_win32_HDC(void * ctx)
{
  struct wglglue_contextdata * context = (struct wglglue_contextdata *)ctx;
  return context->memorydc;
}

void 
wglglue_copy_to_bitmap_win32_HDC(void * ctx)
{
  struct wglglue_contextdata * context = (struct wglglue_contextdata *)ctx;
  wglglue_context_make_current(context);
  glReadPixels(0, 0, context->width,context->height,
                 GL_BGR , GL_UNSIGNED_BYTE, context->pvBits);
  wglglue_context_reinstate_previous(context);
}

static SbBool
wglglue_pbuffer_symbols_resolved(void)
{
  return (wglglue_wglCreatePbuffer && wglglue_wglGetPbufferDC &&
          wglglue_wglReleasePbufferDC && wglglue_wglDestroyPbuffer &&
          wglglue_wglQueryPbuffer);
}

static SbBool
wglglue_ext_supported(struct wglglue_contextdata * context, const cc_glglue * glue, const char * reqext)
{
  /*
   * wgl extensions are not necessarily listed in the string returned
   * by glGetString(GL_EXTENSIONS), see e.g.
   *
   *   http://www.gamedev.net/reference/articles/article1929.asp
   *
   * ..so we try to get hold of wglGetExtensionsString[ARB|EXT]() and
   * use that as well.
   */
  if (!attemptedextresolved) {
    attemptedextresolved = TRUE;
#ifdef WGL_ARB_extensions_string
    wglglue_wglGetExtensionsString = (COIN_PFNWGLGETEXTENSIONSSTRING)PROC(glue, wglGetExtensionsStringARB);
#endif /* WGL_ARB_extensions_string */
#ifdef WGL_EXT_extensions_string
    if (!wglglue_wglGetExtensionsString) {
      wglglue_wglGetExtensionsString = (COIN_PFNWGLGETEXTENSIONSSTRING)PROC(glue, wglGetExtensionsStringEXT);
    }
#endif /* WGL_EXT_extensions_string */
  }

  if (wglglue_wglGetExtensionsString) {
    const char * wglext = wglglue_wglGetExtensionsString(context->memorydc);
    if (coin_glglue_extension_available(wglext, reqext)) { return TRUE; }
  }

  if (coin_glglue_extension_available((const char *) glGetString(GL_EXTENSIONS), reqext)) { return TRUE; }

  return FALSE;
}

static SbBool
wglglue_resolve_symbols(struct wglglue_contextdata * context)
{
  /* Short circuit out if symbols have already been resolved. */
  if (wglglue_pbuffer_symbols_resolved()) { return TRUE; }

  /* We need a(ny) current context to resolve symbols. */
  if (!wglglue_context_make_current(context)) { return FALSE; }

  // just create a cc_glglue instance to use for looking up symbols
  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::getUniqueCacheContext());

  /* Attempt to resolve the symbols: */

  /* Check EXT before ARB, to let the latter override the former if
     both are present, as ARB should always be more recent than
     EXT. */

#ifdef WGL_EXT_pixel_format
  if (wglglue_ext_supported(context, glue, "WGL_EXT_pixel_format")) {
    wglglue_wglChoosePixelFormat = (COIN_PFNWGLCHOOSEPIXELFORMATPROC)PROC(glue, wglChoosePixelFormatEXT);
    wglglue_wglGetPixelFormatAttribiv = (COIN_PFNWGLGETPIXELFORMATATTRIBIVPROC)PROC(glue, wglGetPixelFormatAttribivEXT);
  }
#endif /* WGL_EXT_pixel_format */

#ifdef WGL_ARB_pixel_format
  if (wglglue_ext_supported(context, glue, "WGL_ARB_pixel_format")) {
    wglglue_wglChoosePixelFormat = (COIN_PFNWGLCHOOSEPIXELFORMATPROC)PROC(glue, wglChoosePixelFormatARB);
    wglglue_wglGetPixelFormatAttribiv = (COIN_PFNWGLGETPIXELFORMATATTRIBIVPROC)PROC(glue, wglGetPixelFormatAttribivARB);
  }
#endif /* WGL_ARB_pixel_format */


  /* Now check ARB before EXT, as EXT-check will be blocked if
     ARB-versions of symbols are found. */

#ifdef WGL_ARB_pbuffer
  if (wglglue_wglChoosePixelFormat && /* <- WGL_*_pbuffer depends on WGL_*_pixel_format */
      wglglue_ext_supported(context, glue, "WGL_ARB_pbuffer")) {
    wglglue_wglCreatePbuffer = (COIN_PFNWGLCREATEPBUFFERPROC)PROC(glue, wglCreatePbufferARB);
    wglglue_wglGetPbufferDC = (COIN_PFNWGLGETPBUFFERDCPROC)PROC(glue, wglGetPbufferDCARB);
    wglglue_wglReleasePbufferDC = (COIN_PFNWGLRELEASEPBUFFERDCPROC)PROC(glue, wglReleasePbufferDCARB);
    wglglue_wglDestroyPbuffer = (COIN_PFNWGLDESTROYPBUFFERPROC)PROC(glue, wglDestroyPbufferARB);
    wglglue_wglQueryPbuffer = (COIN_PFNWGLQUERYPBUFFERPROC)PROC(glue, wglQueryPbufferARB);
  }
#endif /* WGL_ARB_pbuffer */

#ifdef WGL_EXT_pbuffer
  if (!wglglue_pbuffer_symbols_resolved() &&
      wglglue_wglChoosePixelFormat && /* <- WGL_*_pbuffer depends on WGL_*_pixel_format */
      wglglue_ext_supported(context, glue, "WGL_EXT_pbuffer")) {
    wglglue_wglCreatePbuffer = (COIN_PFNWGLCREATEPBUFFERPROC)PROC(glue, wglCreatePbufferEXT);
    wglglue_wglGetPbufferDC = (COIN_PFNWGLGETPBUFFERDCPROC)PROC(glue, wglGetPbufferDCEXT);
    wglglue_wglReleasePbufferDC = (COIN_PFNWGLRELEASEPBUFFERDCPROC)PROC(glue, wglReleasePbufferDCEXT);
    wglglue_wglDestroyPbuffer = (COIN_PFNWGLDESTROYPBUFFERPROC)PROC(glue, wglDestroyPbufferEXT);
    wglglue_wglQueryPbuffer = (COIN_PFNWGLQUERYPBUFFERPROC)PROC(glue, wglQueryPbufferEXT);
  }
#endif /* WGL_EXT_pbuffer */

#ifdef WGL_ARB_render_texture
  if (wglglue_ext_supported(context, glue, "WGL_ARB_render_texture")) {
    wglglue_wglBindTexImageARB = (COIN_PFNWGLBINDTEXIMAGEARBPROC) PROC(glue, wglBindTexImageARB);
    wglglue_wglReleaseTexImageARB = (COIN_PFNWGLBINDTEXIMAGEARBPROC) PROC(glue, wglReleaseTexImageARB);
  }
#endif /* WGL_ARB_render_texture */

  wglglue_context_reinstate_previous(context);

  return wglglue_pbuffer_symbols_resolved();
}

static struct wglglue_contextdata *
wglglue_contextdata_init(unsigned int width, unsigned int height)
{
  struct wglglue_contextdata * context;

  context = (struct wglglue_contextdata *)malloc(sizeof(struct wglglue_contextdata));

  context->width = width;
  context->height = height;
  context->memorydc = NULL;
  context->pbufferwnd = NULL;
  context->didcreatememorydc = FALSE;
  context->shouldreleasememorydc = FALSE;
  context->bitmap = NULL;
  context->hpbuffer = NULL;
  context->oldbitmap = NULL;
  context->wglcontext = NULL;
  context->storedcontext = NULL;
  context->storeddc = NULL;
  context->noappglcontextavail = FALSE;
  context->supports_render_to_texture = FALSE;
  context->wanted_render_to_texture = TRUE;
  context->pbufferisbound = FALSE;
  context->pixelformat = 0;

  return context;
}

static void
wglglue_contextdata_cleanup(struct wglglue_contextdata * ctx)
{
  if (ctx == NULL) { return; }

  /* FIXME: the error handling below can and should be simplified, by
     implementing and using exception catching wrappers from
     glue/win32api. 20031124 mortene. */

  if (ctx->wglcontext && ctx->noappglcontextavail) {
    const BOOL r = wglDeleteContext(ctx->wglcontext);
    if (!r) {
      cc_win32_print_error("wglglue_contextdata_cleanup",
                           "wglDeleteContext", GetLastError());
    }
  }
  if (ctx->oldbitmap) {
    const HGDIOBJ o = SelectObject(ctx->memorydc, ctx->oldbitmap);
    if (!o) {
      cc_win32_print_error("wglglue_contextdata_cleanup",
                           "SelectObject", GetLastError());
    }
  }
  if (ctx->bitmap) {
    const BOOL r = DeleteObject(ctx->bitmap);
    if (!r) {
      cc_win32_print_error("wglglue_contextdata_cleanup",
                           "DeleteObject", GetLastError());
    }
  }
  if (ctx->hpbuffer) {
    {
      const int r = wglglue_wglReleasePbufferDC(ctx->hpbuffer, wglglue_wglGetPbufferDC(ctx->hpbuffer));
      if (!r) {
        cc_win32_print_error("wglglue_contextdata_cleanup",
                             "wglReleasePbufferDC", GetLastError());
      }
    }
    {
      const BOOL r = wglglue_wglDestroyPbuffer(ctx->hpbuffer);
      if (!r) {
        cc_win32_print_error("wglglue_contextdata_cleanup",
                             "wglDestroyPbuffer", GetLastError());
      }
    }
  }
  if (ctx->memorydc) {
    if (ctx->didcreatememorydc) {
      const BOOL r = DeleteDC(ctx->memorydc);
      if (!r) {
        cc_win32_print_error("wglglue_contextdata_cleanup",
                             "DeleteDC", GetLastError());
      }
    }
    else if (ctx->shouldreleasememorydc && ctx->pbufferwnd) {
      int ok = ReleaseDC(ctx->pbufferwnd, ctx->memorydc);
      if (!ok) {
        cc_win32_print_error("wglglue_contextdata_cleanup",
                             "ReleaseDC", GetLastError());
      }
    }
  }
  if (ctx->pbufferwnd) {
    BOOL r = DestroyWindow(ctx->pbufferwnd);
    if (!r) {
      cc_win32_print_error("wglglue_contextdata_cleanup",
                           "DestroyWindow", GetLastError());
    }
  }

  free(ctx);
}

static SbBool
wglglue_context_create_context(struct wglglue_contextdata * ctx, DWORD bitWin)
{
  struct wglglue_contextdata * context = (struct wglglue_contextdata *)ctx;
  int pixelformat;

  /* FIXME: we're currently not giving any hint as to whether we want
     a single- / double- / quad-buffer visual. Using anything than a
     single-buffer visual for offscreen rendering is probably
     wasteful, so we're likely to often use up more resources than we
     really need to do.

     20061025 mortene.
  */

  PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR),   /* size of this pfd */
    1,                     /* version number */
    bitWin |               /* support bitmap or window */
    PFD_SUPPORT_OPENGL,    /* support OpenGL */
    PFD_TYPE_RGBA,         /* RGBA type */
    24,                    /* 24-bit color depth */
    0, 0, 0, 0, 0, 0,      /* color bits ignored */
    8,                     /* 8 bit alpha channel */
    0,                     /* shift bit ignored */
    0,                     /* no accumulation buffer */
    0, 0, 0, 0,            /* accum bits ignored */
    32,                    /* 32-bit z-buffer */
    1,                     /* minimum size stencil buffer */
    0,                     /* no auxiliary buffer */
    PFD_MAIN_PLANE,        /* main layer */
    0,                     /* reserved */
    0, 0, 0                /* layer masks ignored */
  };

  /* FIXME: the following is a hack to get around a problem which
     really demands more effort to be solved properly.

     The problem is that there is no way in the API of the
     SoOffscreenRenderer class to specify what particular attributes
     to request. This most often manifests itself as a problem for app
     programmers in that they have made some kind of extension node
     which uses the OpenGL stencil buffer. If no stencil buffer
     happens to be part of the GL context format for the offscreen
     renderer, these will not work properly. At the same time, we
     don't want to default to requesting a stencil buffer, as that
     takes a non-trivial amount of extra memory resources on the gfx
     card.

     So until we have implemented the proper solution for making it
     possible to pass in a detailed specification of which attributes
     to request from offscreen GL contexts, we provide this temporary
     work-around: the app programmer can set an envvar with a value
     specifying the number of stencil buffer bits he/she wants.

     20060223 mortene.
  */
  const int v = coin_glglue_stencil_bits_hack();
  if (v != -1) { pfd.cStencilBits = (BYTE)v; }


  /* get the best available match of pixel format for the device
     context: */

  /* FIXME: instead of ChoosePixelFormat(), we should use the code
     from SoWin which runs through the available formats and weights
     them according to our own rules. Simplifies debugging immensely
     when something goes wrong, and provides ways to override parts of
     the selection process. 20040608 mortene. */

  pixelformat = ChoosePixelFormat(context->memorydc, &pfd);
  if (pixelformat == 0) {
    DWORD dwError = GetLastError();
    cc_debugerror_postwarning("wglglue_context_create_context",
                              "ChoosePixelFormat() failed with "
                              "error code %d.", dwError);
    return FALSE;
  }

  /* make that the pixel format of the device context: */

  /* I've seen a driver (3Dlabs GLINT R3 PT, 1.1.28) where
     SetPixelFormat() failed but didn't set GetLastError(), resulting
     in a nonsensical error message, so we reset this
     first. -mortene. */
  SetLastError(0);

  if (!SetPixelFormat(context->memorydc, pixelformat, &pfd)) {
    DWORD dwError = GetLastError();
    cc_string str;

    cc_string_construct(&str);
    cc_string_sprintf(&str, "SetPixelFormat(%p, %d, ...)", context->memorydc, pixelformat);
    cc_win32_print_error("wglglue_context_create_context", cc_string_get_text(&str), dwError);
    cc_string_clean(&str);

    /* We have had an external error report about this, plus I've been
       able to reproduce it locally on ASK (Win NT 4, 3Dlabs 1.1.28):
       just set the display to "Truecolor" (instead of "256 colors" or
       "65536 colors" in the system display settings panel) and the
       SetPixelFormat() above will fail. Changing the display settings
       to a lower color resolution fixes it. (Using a lower screen
       resolution makes no difference, so it doesn't seem to be
       related to lack of memory resources.)

       I tried to make less demanding requirements for the pixel
       format in the above PIXELFORMATDESCRIPTOR, but that didn't help
       either.

       So, this seems very much like a system / driver bug. 
    */

    cc_debugerror_post("wglglue_context_create_context",
                       "This is most likely a bug with your system. "
                       "Try changing display settings (to e.g. 16-bit, "
                       "24-bit or 32-bit truecolor display) and re-run.");

    return FALSE;
  }

  context->pixelformat = pixelformat;

  context->wglcontext = wglCreateContext(context->memorydc);
  if (context->wglcontext == NULL) {
    DWORD dwError = GetLastError();
    cc_win32_print_error("wglglue_context_create_context",
                         "wglCreateContext()", dwError);
    return FALSE;
  }

  return TRUE;
}

static SbBool
wglglue_context_create_software(struct wglglue_contextdata * ctx, SbBool warnonerrors)
{
  struct wglglue_contextdata * context = (struct wglglue_contextdata *)ctx;

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("wglglue_context_create_software",
                           "creating software buffer");
  }

  context->memorydc = CreateCompatibleDC(NULL);
  context->didcreatememorydc = TRUE;
  context->shouldreleasememorydc = FALSE;
  if (context->memorydc == NULL) {
    if (warnonerrors || coin_glglue_debug()) {
      DWORD dwError = GetLastError();
      cc_debugerror_postwarning("wglglue_context_create_software",
                                "CreateCompatibleDC(NULL) failed with "
                                "error code %d.", dwError);
    }
    return FALSE;
  }

  /* make a bitmap to draw to */
  {
    BITMAPINFO bmi;

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = context->width;
    bmi.bmiHeader.biHeight = context->height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed  = 0;
    bmi.bmiHeader.biClrImportant = 0;
    bmi.bmiColors[0].rgbBlue = 0;
    bmi.bmiColors[0].rgbGreen = 0;
    bmi.bmiColors[0].rgbRed = 0;
    bmi.bmiColors[0].rgbReserved = 0;

    context->bitmap = CreateDIBSection(context->memorydc, &bmi, DIB_RGB_COLORS,
                                      &(context->pvBits), NULL, 0);
    if (context->bitmap == NULL) {
      if (warnonerrors || coin_glglue_debug()) {
        DWORD dwError = GetLastError();
        cc_debugerror_postwarning("wglglue_context_create_software",
                                  "CreateDIBSection() failed with error "
                                  "code %d.", dwError);
      }
      return FALSE;
    }
  }

  context->oldbitmap = (HBITMAP)
    SelectObject(context->memorydc, context->bitmap);
  if (context->oldbitmap == NULL) {
    if (warnonerrors || coin_glglue_debug()) {
      DWORD dwError = GetLastError();
      cc_debugerror_postwarning("wglglue_context_create_software",
                                "SelectObject() failed with error code %d.",
                                dwError);
    }
    return FALSE;
  }

  if (!(wglglue_context_create_context(context, PFD_DRAW_TO_BITMAP))) {
    return FALSE;
  }

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("wglglue_context_create_software",
                           "success creating software buffer, HGLRC==%p",
                           context->wglcontext);
  }

  return TRUE;
}

static SbBool
wglglue_context_create_pbuffer(struct wglglue_contextdata * ctx, SbBool warnonerrors)
{
  struct wglglue_contextdata * context = (struct wglglue_contextdata *)ctx;

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("wglglue_context_create_pbuffer", "creating pbuffer");
  }

  if ((context->memorydc = wglGetCurrentDC())) {
    context->shouldreleasememorydc = FALSE;
    context->didcreatememorydc = FALSE;
    context->wglcontext = wglGetCurrentContext();
  }
  else { context->noappglcontextavail = TRUE; }

  if (context->noappglcontextavail) {
    /* FIXME: This should be reset in wglglue_cleanup() once we
       also properly unregister there... 20060207 kyrah */
    static int didregister = 0;
    if (!didregister) {
      WNDCLASS wc;
      didregister = 1;

      wc.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
      wc.lpfnWndProc    = DefWindowProc;
      wc.cbClsExtra     = 0;
      wc.cbWndExtra     = 0;
      wc.hInstance      = GetModuleHandle(NULL);
      wc.hIcon          = NULL;
      wc.hCursor        = NULL;
      wc.hbrBackground  = NULL;
      wc.lpszMenuName   = NULL;
      wc.lpszClassName  = "coin_gl_wgl";

      if (!RegisterClass(&wc)) {
        DWORD dwError = GetLastError();
        cc_debugerror_postwarning("wglglue_context_create_pbuffer",
                                  "RegisterClass(&wc) failed with "
                                  "error code %d.", dwError);
        return FALSE;
      }
      /* FIXME: unregister at app exit? pederb, 2003-12-15 */
    }

    {
      HWND hWnd;
      HINSTANCE hInstance = GetModuleHandle(NULL);

      if (!(hWnd = CreateWindow(
                     "coin_gl_wgl",   /* class name */
                     "coin_gl_wgl",   /* window title */
                     0,               /* selected window style */
                     0, 0,            /* window position */
                     context->width,  /* calculate adjusted window width */
                     context->height, /* calculate adjusted window height */
                     NULL,            /* no parent window */
                     NULL,            /* no menu */
                     hInstance,       /* Instance */
                     NULL)))          /* don't pass anything to WM_CREATE */
      {
        DWORD dwError = GetLastError();
        cc_debugerror_postwarning("wglglue_context_create_pbuffer",
                                  "CreateWindow(...) failed with "
                                  "error code %d.", dwError);
        return FALSE;
      }

      context->pbufferwnd = hWnd;
      context->memorydc = GetDC(hWnd);
      context->shouldreleasememorydc = TRUE;
      context->didcreatememorydc = FALSE;
      if (context->memorydc == NULL) {
        DWORD dwError = GetLastError();
        cc_debugerror_postwarning("wglglue_context_create_pbuffer",
                                  "GetDC(hWnd) failed with "
                                  "error code %d.", dwError);
        return FALSE;
      }
    }

    if (!(wglglue_context_create_context(context, PFD_DRAW_TO_WINDOW))) {
      return FALSE;
    }
  }

  {
    const SbBool pbuffer = wglglue_resolve_symbols(context);

    if (coin_glglue_debug()) {
      cc_debugerror_postinfo("wglglue_context_create_pbuffer",
                             "PBuffer offscreen rendering is %ssupported "
                             "by the OpenGL driver", pbuffer ? "" : "NOT ");
    }

    if (!pbuffer) {
      return FALSE;
    }
  }

  {
    GLint pixformat;
    unsigned int numFormats;
    const float fAttribList[] = { 0 };

    int nontex_attrs[] = {
      WGL_STENCIL_BITS_ARB, 1, /* FIXME: must be first, since we may
                                  want to modify it later, and need to
                                  know where it is. This is a
                                  wart. 20060307 mortene. */
      WGL_DRAW_TO_PBUFFER_ARB, TRUE,
      WGL_COLOR_BITS_ARB, 32,
      WGL_ALPHA_BITS_ARB, 8,
      WGL_DEPTH_BITS_ARB, 24,
      0
    };
    int tex_attrs[] = {
      WGL_STENCIL_BITS_ARB, 1, /* FIXME: must be first, since we may
                                  want to modify it later, and need to
                                  know where it is. This is a
                                  wart. 20060307 mortene. */
      WGL_DRAW_TO_PBUFFER_ARB, TRUE,
      WGL_BIND_TO_TEXTURE_RGBA_ARB, TRUE,
      WGL_COLOR_BITS_ARB, 32,
      WGL_ALPHA_BITS_ARB, 8,
      WGL_DEPTH_BITS_ARB, 24,
      0
    };
    const int * attrs[] = { nontex_attrs, tex_attrs };

    const int nontex_pbufferflags[] = { 0 };
    const int tex_pbufferflags[] = {
      WGL_TEXTURE_FORMAT_ARB, WGL_TEXTURE_RGBA_ARB,
      WGL_TEXTURE_TARGET_ARB, WGL_TEXTURE_2D_ARB,
      0
    };
    const int * pbufferflags[] = { nontex_pbufferflags, tex_pbufferflags };

    unsigned int thetry;

    {
      /* FIXME: the following is a hack to get around a problem which
         really demands more effort to be solved properly. See further
         up in this source file, in the function to create a software
         context, for a more elaborate explanation. 20060307 mortene.
      */
      const int v = coin_glglue_stencil_bits_hack();
      if (v != -1) {
        assert(nontex_attrs[0] == WGL_STENCIL_BITS_ARB);
        nontex_attrs[1] = v;
        assert(tex_attrs[0] == WGL_STENCIL_BITS_ARB);
        tex_attrs[1] = v;
      }
    }

    /* iterate from end of arrays, which contains "best" option */
    thetry = (sizeof(attrs) / sizeof(attrs[0]));
    /* if render-to-texture extension not supported, don't attempt to
       set up a pbuffer with those capabilities (could in theory cause
       nasty WGL errors): */
    if (!context->wanted_render_to_texture || wglglue_wglBindTexImageARB == NULL) {
      thetry--;
    }

    while (thetry > 0) {
      thetry--;

      /* choose pixel format */
      if (!wglglue_wglChoosePixelFormat(context->memorydc, attrs[thetry],
                                        fAttribList, 1, &pixformat,
                                        &numFormats)) {
        if (warnonerrors || coin_glglue_debug()) {
          cc_debugerror_postwarning("wglglue_context_create_pbuffer",
                                    "wglChoosePixelFormat() failed, try %u",
                                    thetry);
        }
        continue;
      }

      /* create the pbuffer */
      context->hpbuffer = wglglue_wglCreatePbuffer(context->memorydc,
                                                   pixformat,
                                                   context->width,
                                                   context->height,
                                                   pbufferflags[thetry]);
      if (!context->hpbuffer) {
        if (warnonerrors || coin_glglue_debug()) {
          cc_debugerror_postwarning("wglglue_context_create_pbuffer",
                                    "wglCreatePbuffer(HDC, PixelFormat==%d, "
                                    "Width==%d, Height==%d, AttribList==%p) "
                                    "failed, try %u",
                                    pixformat,
                                    context->width, context->height,
                                    pbufferflags[thetry], thetry);
        }
        continue;
      }

      /* success, set capability flag, and break loop */
      if (coin_glglue_debug()) {
        cc_debugerror_postinfo("wglglue_context_create_pbuffer",
                               "wglCreatePbuffer() success, try %u", thetry);
      }
      context->supports_render_to_texture =
        (pbufferflags[thetry] == tex_pbufferflags);
      break;
    }

    /* if no way to construct a pbuffer was found: */
    if (!context->hpbuffer) { return FALSE; }

    context->pixelformat = pixformat;

    /* delete/release device context and window in case we created it
       ourselves */
    if (context->memorydc) {
      if (context->didcreatememorydc) {
        BOOL r = DeleteDC(context->memorydc);
        if (!r) {
          cc_win32_print_error("wglglue_context_create_pbuffer",
                               "DeleteDC", GetLastError());
        }
      }
      else if (context->shouldreleasememorydc && context->pbufferwnd) {
        BOOL r = ReleaseDC(context->pbufferwnd, context->memorydc);
        if (!r) {
          cc_win32_print_error("wglglue_context_create_pbuffer",
                               "ReleaseDC", GetLastError());
        }
      } 
    }
    if (context->pbufferwnd) {
      BOOL r = DestroyWindow(context->pbufferwnd);
      if (!r) {
        cc_win32_print_error("wglglue_context_create_pbuffer",
                             "DestroyWindow", GetLastError());
      }
      context->pbufferwnd = NULL;
    }


	context->memorydc = CreateCompatibleDC(wglglue_wglGetPbufferDC(context->hpbuffer));

    BITMAPINFO bmi;

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = context->width;
    bmi.bmiHeader.biHeight = context->height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed  = 0;
    bmi.bmiHeader.biClrImportant = 0;
    bmi.bmiColors[0].rgbBlue = 0;
    bmi.bmiColors[0].rgbGreen = 0;
    bmi.bmiColors[0].rgbRed = 0;
    bmi.bmiColors[0].rgbReserved = 0;

    context->bitmap = CreateDIBSection(context->memorydc, &bmi, DIB_RGB_COLORS,
                                      &(context->pvBits), NULL, 0);
	SelectObject(context->memorydc, context->bitmap);

    context->didcreatememorydc = TRUE;
    context->shouldreleasememorydc = FALSE;
    if (!context->memorydc) {
      if (warnonerrors || coin_glglue_debug()) {
        cc_debugerror_postwarning("wglglue_context_create_pbuffer",
                                  "Couldn't create pbuffer's device context.");
      }
      return FALSE;
    }

    /* delete wgl context in case we created it ourselves */
    if (context->noappglcontextavail) {
      BOOL r = wglDeleteContext(context->wglcontext);
      if (!r) {
        if (warnonerrors || coin_glglue_debug()) {
          cc_debugerror_postwarning("wglglue_context_create_pbuffer",
                                    "Couldn't create pbuffer's device context.");
        }
      }
    }
    context->wglcontext = wglCreateContext(wglglue_wglGetPbufferDC(context->hpbuffer));
    if (!context->wglcontext) {
      if (warnonerrors || coin_glglue_debug()) {
        cc_debugerror_postwarning("wglglue_context_create_pbuffer",
                                  "Couldn't create rendering context for the pbuffer.");
      }
      return FALSE;
    }

    /* set and output the actual pBuffer dimensions */
    if (!wglglue_wglQueryPbuffer(context->hpbuffer,
                                    WGL_PBUFFER_WIDTH_ARB,
                                    (int *) &(context->width))) {
      if (warnonerrors || coin_glglue_debug()) {
        cc_debugerror_postwarning("wglglue_context_create_pbuffer",
                                  "Couldn't query the pbuffer width.");
      }
      return FALSE;
    }

    if (!wglglue_wglQueryPbuffer(context->hpbuffer,
                                 WGL_PBUFFER_HEIGHT_ARB,
                                 (int *) &(context->height))) {
      if (warnonerrors || coin_glglue_debug()) {
        cc_debugerror_postwarning("wglglue_context_create_pbuffer",
                                  "Couldn't query the pbuffer height.");
      }
      return FALSE;
    }
  }

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("wglglue_context_create_pbuffer",
                           "success creating pbuffer, HGLRC==%p",
                           context->wglcontext);
  }

  return TRUE;
}

/* ********************************************************************** */

/* Create and return a handle to an offscreen OpenGL buffer.

   Where p-buffer support is available that will be used instead of a
   standard offscreen WGL context, as it should render much faster
   (due to hardware acceleration).

   See:
     http://www.oss.sgi.com/projects/ogl-sample/registry/ARB/wgl_pbuffer.txt
   Or the older version:
     http://www.oss.sgi.com/projects/ogl-sample/registry/EXT/wgl_pbuffer.txt
*/
void *
wglglue_context_create_offscreen(unsigned int width, unsigned int height, SbBool texture)
{
  struct wglglue_contextdata * swctx, * pbctx;
  SbBool ispbuffer;

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("wglglue_context_create_offscreen",
                           "method called ");
  }

  swctx = wglglue_contextdata_init(width, height);
  assert(swctx);

  swctx->wanted_render_to_texture = texture;

  if (wglglue_context_create != NULL) {

    ispbuffer = wglglue_context_create == wglglue_context_create_pbuffer;

    /* don't warn if we fail to open a pbuffer context. we will try software  */
    if (wglglue_context_create(swctx, !ispbuffer)) { return swctx; }
    wglglue_contextdata_cleanup(swctx);

    /* fall back to a software context */
    if (ispbuffer) {
      if (coin_glglue_debug()) {
        cc_debugerror_postinfo("wglglue_context_create_offscreen",
                               "pbuffer failed. Trying software ");
      }
      swctx = wglglue_contextdata_init(width, height);
      assert(swctx);
      if (wglglue_context_create_software(swctx, TRUE)) { return swctx; }
      wglglue_contextdata_cleanup(swctx);
    }
    return NULL;
  }

  /* As there could possibly be no valid wgl context at this moment,
     we have to first make a context and set it current to be able
     to query pbuffer extension availability. */

  if (!wglglue_context_create_software(swctx, TRUE)) {
    wglglue_contextdata_cleanup(swctx);
    return NULL;
  }

  /* ok, so we can at least use a non-pbuffer offscreen context */
  wglglue_context_create = wglglue_context_create_software;

  /* developer or user can force pbuffer support off with this envvar */
  {
    const char * env = coin_getenv("COIN_WGLGLUE_NO_PBUFFERS");
    if (env && atoi(env) > 0) { return swctx; }
  }

  /* next, check if pbuffer support is available in the OpenGL
     library image */

  pbctx = wglglue_contextdata_init(width, height);
  assert(pbctx);

  pbctx->wanted_render_to_texture = texture;

  /* attempt to create a pbuffer */
  if (!wglglue_context_create_pbuffer(pbctx, FALSE)) {
    wglglue_contextdata_cleanup(pbctx);
    return swctx;
  }

  /* pbuffers are really supported, kill the software offscreen
     context and use the pbuffer-enabled one */
  wglglue_contextdata_cleanup(swctx);

  wglglue_context_create = wglglue_context_create_pbuffer;

  return pbctx;
}

SbBool
wglglue_context_make_current(void * ctx)
{
  struct wglglue_contextdata * context = (struct wglglue_contextdata *)ctx;

  context->storedcontext = wglGetCurrentContext();
  if (context->storedcontext) { context->storeddc = wglGetCurrentDC(); }
  HDC hdc;
  if(context->hpbuffer)
	  hdc = wglglue_wglGetPbufferDC(context->hpbuffer);
  else
	  hdc = context->memorydc;
  return wglMakeCurrent(hdc, context->wglcontext) ? TRUE : FALSE;
}

void
wglglue_context_reinstate_previous(void * ctx)
{
  /* The previous context is stored and reset to make it possible to
     use an SoOffscreenRenderer from for instance an SoCallback node
     callback during SoGLRenderAction traversal, without the need for
     any extra book-keeping on the application side. */

  struct wglglue_contextdata * context = (struct wglglue_contextdata *)ctx;

  if (context->storedcontext && context->storeddc) {
    (void)wglMakeCurrent(context->storeddc, context->storedcontext);
    context->storedcontext = NULL;
  }
  else { (void)wglMakeCurrent(NULL, NULL); }
}

void
wglglue_context_destruct(void * ctx)
{
  /* FIXME: needs to call into the (as of yet unimplemented)
     "destructing GL context" handler. 20030310 mortene. */

  struct wglglue_contextdata * context = (struct wglglue_contextdata *)ctx;

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("wglglue_context_destruct",
                           "destroy context, HGLRC==%p", context->wglcontext);
  }

  wglglue_contextdata_cleanup(context);
}

/* ********************************************************************** */

void
wglglue_context_bind_pbuffer(void * ctx)
{
  BOOL ok;

  struct wglglue_contextdata * context = (struct wglglue_contextdata *)ctx;
  assert(wglglue_wglBindTexImageARB != NULL);
  assert(context->supports_render_to_texture);

  ok = wglglue_wglBindTexImageARB(context->hpbuffer, WGL_FRONT_LEFT_ARB);
  assert(ok);
  context->pbufferisbound = TRUE;
}

void
wglglue_context_release_pbuffer(void * ctx)
{
  BOOL ok;

  struct wglglue_contextdata * context = (struct wglglue_contextdata *)ctx;
  assert(wglglue_wglReleaseTexImageARB != NULL);

  ok = wglglue_wglReleaseTexImageARB(context->hpbuffer, WGL_FRONT_LEFT_ARB);
  assert(ok);
  context->pbufferisbound = FALSE;
}

SbBool
wglglue_context_pbuffer_is_bound(void * ctx)
{
  struct wglglue_contextdata * context = (struct wglglue_contextdata *)ctx;
  return context->pbufferisbound;
}

SbBool
wglglue_context_can_render_to_texture(void * ctx)
{
  struct wglglue_contextdata * context = (struct wglglue_contextdata *)ctx;
  return context->supports_render_to_texture;
}

/* ********************************************************************** */

/* If ctx does not point at a pbuffer context, but rather a "normal"
   offscreen context, it will return FALSE.

   Upon other error conditions, FALSE will also be returned.
*/
SbBool
wglglue_context_pbuffer_max(void * c, unsigned int * lims)
{
  BOOL ok;
  int attribvals[3], i;
  const int attribs[3] = {
    WGL_MAX_PBUFFER_WIDTH_ARB, WGL_MAX_PBUFFER_HEIGHT_ARB,
    WGL_MAX_PBUFFER_PIXELS_ARB
  };
  struct wglglue_contextdata * ctx = (struct wglglue_contextdata *)c;

  if (!ctx->hpbuffer) { return FALSE; }
  if (!wglglue_wglGetPixelFormatAttribiv) { return FALSE; }
 
  ok = wglglue_wglGetPixelFormatAttribiv(ctx->memorydc, ctx->pixelformat,
                                         0, /* main plane */
                                         3, attribs, attribvals);
  if (!ok) {
    if (coin_glglue_debug()) {
      DWORD dwError = GetLastError();
      cc_debugerror_postwarning("wglglue_context_pbuffer_max",
                                "wglGetPixelFormatAttribiv() failed with "
                                "error code %d.", dwError);
    }
    return FALSE;
  }

  for (i = 0; i < 3; i++) {
    assert(attribvals[i] >= 0);
    lims[i] = (unsigned int)attribvals[i];
  }
  return TRUE;
}

/* ********************************************************************** */

void wglglue_cleanup(void)
{
  wglglue_context_create = NULL;

  wglglue_wglCreatePbuffer = NULL;
  wglglue_wglGetPbufferDC = NULL;
  wglglue_wglReleasePbufferDC = NULL;
  wglglue_wglDestroyPbuffer = NULL;
  wglglue_wglQueryPbuffer = NULL;
  wglglue_wglBindTexImageARB = NULL;
  wglglue_wglReleaseTexImageARB = NULL;

  wglglue_wglChoosePixelFormat = NULL;
  wglglue_wglGetPixelFormatAttribiv = NULL;

  wglglue_wglGetExtensionsString = NULL;

  attemptedextresolved = FALSE;

  /* FIXME: We should probably do an UnregisterClass() here (see
     wglglue_context_create_pbuffer()) -- but since I don't know the
     details of how to do this, I'll leave it to one of the Windows
     gurus...  20060207 kyrah */
}


#endif /* HAVE_WGL */

/* ********************************************************************** */
