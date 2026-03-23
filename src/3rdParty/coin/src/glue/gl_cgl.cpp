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
 *   - COIN_CGLGLUE_NO_PBUFFERS: set to 1 to force software rendering of
 *     offscreen contexts.
 */

#include "glue/gl_cgl.h"
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

#ifndef HAVE_CGL

SbBool cglglue_context_is_using_pbuffer(void * COIN_UNUSED_ARG(ctx))
{
  assert(FALSE); return FALSE;
}

void * cglglue_getprocaddress(const char * COIN_UNUSED_ARG(fname))
{
  assert(FALSE); return NULL;
}

void * cglglue_context_create_offscreen(unsigned int COIN_UNUSED_ARG(width), 
                                        unsigned int COIN_UNUSED_ARG(height)) { 
  assert(FALSE); return NULL; 
}

SbBool cglglue_context_make_current(void * COIN_UNUSED_ARG(ctx))
{ 
  assert(FALSE); return FALSE; 
}

void cglglue_context_reinstate_previous(void * COIN_UNUSED_ARG(ctx)) 
{ 
  assert(FALSE); 
}

void cglglue_context_destruct(void * COIN_UNUSED_ARG(ctx)) 
{ 
  assert(FALSE); 
}

#else /* HAVE_CGL */

/* ********************************************************************** */

#include <OpenGL/OpenGL.h>

#ifndef HAVE_CGL_PBUFFER 

/* 
 * pBuffer functions are picked up at runtime, so the only thing
 * we need from the CGL headers is the CGLPBuffer type, which is
 * void* anyways...  
 */
typedef void * CGLPbuffer;  

#endif /* !HAVE_CGL_PBUFFER */

typedef CGLError (* COIN_CGLCREATEPBUFFER) (GLsizei width, 
                                             GLsizei height, 
                                             GLenum target, 
                                             GLenum internalFormat, 
                                             GLint max_level, 
                                             CGLPBufferObj *pbuffer);
typedef CGLError (* COIN_CGLDESTROYPBUFFER) (CGLPBufferObj pbuffer);
typedef CGLError (* COIN_CGLSETPBUFFER) (CGLContextObj ctx, 
                                          CGLPBufferObj pbuffer, 
                                          GLenum face, 
                                          GLint level, 
                                          GLint screen);
typedef CGLError (* COIN_CGLTEXIMAGEPBUFFER) (CGLContextObj ctx, 
                                               CGLPBufferObj pbuffer, 
                                               GLenum source);

static COIN_CGLCREATEPBUFFER cglglue_CGLCreatePBuffer = NULL;
static COIN_CGLDESTROYPBUFFER cglglue_CGLDestroyPBuffer = NULL;
static COIN_CGLSETPBUFFER cglglue_CGLSetPBuffer = NULL;
static COIN_CGLTEXIMAGEPBUFFER cglglue_CGLTexImagePBuffer = NULL;

struct cglglue_contextdata;
static SbBool (* cglglue_context_create)(struct cglglue_contextdata * ctx) = NULL;

static void
cglglue_resolve_symbols()
{
  /* Resolve symbols only once... */
  if (cglglue_CGLCreatePBuffer && cglglue_CGLDestroyPBuffer &&
      cglglue_CGLSetPBuffer && cglglue_CGLTexImagePBuffer) return; 

  cglglue_CGLCreatePBuffer = (COIN_CGLCREATEPBUFFER)cglglue_getprocaddress("CGLCreatePBuffer");
  cglglue_CGLDestroyPBuffer = (COIN_CGLDESTROYPBUFFER)cglglue_getprocaddress("CGLDestroyPBuffer");
  cglglue_CGLSetPBuffer = (COIN_CGLSETPBUFFER)cglglue_getprocaddress("CGLSetPBuffer");
  cglglue_CGLTexImagePBuffer = (COIN_CGLTEXIMAGEPBUFFER)cglglue_getprocaddress("CGLTexImagePBuffer");
}


static SbBool
cglglue_get_pbuffer_enable(void)
{
  /* Make it possible to turn off pBuffer support completely.
     Mostly relevant for debugging purposes. */
  const char * env = coin_getenv("COIN_CGLGLUE_NO_PBUFFERS");
  if (env && atoi(env) > 0) { 
    return FALSE; 
  } else { 
    cglglue_resolve_symbols();
    return (cglglue_CGLCreatePBuffer && cglglue_CGLDestroyPBuffer && 
            cglglue_CGLSetPBuffer && cglglue_CGLTexImagePBuffer);
  }
}


struct cglglue_contextdata {
  CGLContextObj storedcontext;
  size_t rowbytes;
  void * membuffer;
  CGLContextObj cglcontext;
  CGLPixelFormatObj pixformat;
  CGLPBufferObj cglpbuffer;
  unsigned int width;
  unsigned int height; 
  SbBool pbufferisbound;
};


static struct cglglue_contextdata *
cglglue_contextdata_init(unsigned int width, unsigned int height)
{
  struct cglglue_contextdata * ctx;
  ctx = (struct cglglue_contextdata *)malloc(sizeof(struct cglglue_contextdata));

  ctx->storedcontext = NULL;
  ctx->rowbytes = 0;
  ctx->membuffer = NULL;
  ctx->cglcontext = NULL;
  ctx->pixformat = NULL;
  ctx->cglpbuffer = NULL; 
  ctx->width = width;
  ctx->height = height;
  ctx->pbufferisbound = FALSE;
  return ctx;
}

static void
cglglue_contextdata_cleanup(struct cglglue_contextdata * ctx)
{
  if (ctx->cglcontext) CGLDestroyContext(ctx->cglcontext);
  if (ctx->pixformat) CGLDestroyPixelFormat(ctx->pixformat);
  if (ctx->cglpbuffer) cglglue_CGLDestroyPBuffer(ctx->cglpbuffer); 
  if (ctx->membuffer) free(ctx->membuffer);
  free(ctx);
}

static SbBool
cglglue_context_create_software(struct cglglue_contextdata * ctx)
{
  // Sets up a single-buffered pixel format
  int attrib[] = {
    kCGLPFAOffScreen,
    kCGLPFANoRecovery,
    kCGLPFAColorSize, 32,
    kCGLPFAAlphaSize, 8,
    kCGLPFADepthSize, 24,
    kCGLPFAStencilSize, 1,
    0
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
  if (v != -1) {
    size_t i;
    for (i = 0; i < (sizeof(attrib) / sizeof(attrib[0]) / 2); i++) {
      if (attrib[i] == kCGLPFAStencilSize) { attrib[i+1] = v; }
    }
  }

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("cglglue_context_create_software",
                           "Creating software buffer.");
  }
    
  GLint numPixelFormats;
  CGLError err = CGLChoosePixelFormat((CGLPixelFormatAttribute *)attrib, 
                             &ctx->pixformat, &numPixelFormats);
  if (err != kCGLNoError || !ctx->pixformat) {
    cc_debugerror_postwarning("cglglue_context_create_software",
                              "Couldn't get RGBA CGL pixelformat. %s",
                              CGLErrorString(err));
    return FALSE;
  }
  
  err = CGLCreateContext(ctx->pixformat, NULL, &ctx->cglcontext);
  if (err != kCGLNoError || !ctx->cglcontext) {
    cc_debugerror_postwarning("cglglue_context_create_software",
                              "Couldn't create CGL context. %s",
                              CGLErrorString(err));
    cglglue_contextdata_cleanup(ctx);
    return FALSE;
  } 
  
  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("cglglue_context_create_software",
                           "created new software offscreen context == %p",
                           ctx->cglcontext);
  } 
  
  ctx->rowbytes = ctx->width * 4; // We use GL_RGBA as internal format
  ctx->membuffer = malloc(ctx->height*ctx->rowbytes);
  err = CGLSetOffScreen(ctx->cglcontext, ctx->width, ctx->height,
                        ctx->rowbytes, ctx->membuffer);
  if (err != kCGLNoError) {
    cc_debugerror_post("cglglue_context_make_current",
                       "Error setting offscreen context. %s",
                       CGLError(err));
    return FALSE;
  }

  
  return TRUE;
} 


static SbBool
cglglue_context_create_pbuffer(struct cglglue_contextdata * ctx)
{
  /* FIXME: before we get here, we should have checked the requested
     dimensions in the context struct versus GLX_MAX_PBUFFER_WIDTH,
     GLX_MAX_PBUFFER_HEIGHT and GLX_MAX_PBUFFER_PIXELS
     somewhere. Copied from gl_agl.cpp by kintel 20090316, originally
     copied from gl_glx.c by kyrah 20031114, originally mentioned by mortene 20030811. 
  
     Update kyrah 20040714: The way to query maximum pBuffer size
     on CGL is (according to Geoff Stahl of Apple) GL_MAX_VIEWPORT_DIMS,
     but this won't get us very far, since the numbers reported
     reflect the theoretical maximum size. We'll in any case have
     to try and allocate a pBuffer to see if we can actually get
     so much VRAM...
  */

  GLenum error;
  GLint attribs[] = { 
    kCGLPFAColorSize, 32,
    kCGLPFAAlphaSize, 8,
    kCGLPFADepthSize, 24,
    kCGLPFAStencilSize, 1,
    kCGLPFAClosestPolicy, 
    kCGLPFAAccelerated, 
    kCGLPFANoRecovery,
    0
  };

  /* FIXME: this is a hack. See comment elsewhere in the file where
     this function is used, for an elaborate explanation. 20060307
     mortene. */
  const int v = coin_glglue_stencil_bits_hack();
  if (v != -1) {
    size_t i;
    for (i = 0; i < (sizeof(attribs) / sizeof(attribs[0]) / 2); i++) {
      if (attribs[i] == kCGLPFAStencilSize) { attribs[i+1] = v; }
    }
  }

  /* Note that unlike in WGL, where the ability to bind a pBuffer
     as a texture has to be specified when creating the pixel
     format, CGL works in a different way: We can assume that 
     when pBuffers are supported, it is also possible to bind
     the pBuffer as a texture (i.e. call cglTexImagePBuffer).
   */

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("cglglue_context_create_pbuffer",
                           "Creating pBuffer.");
  }

  GLint numPixelFormats;
  CGLError err = CGLChoosePixelFormat((CGLPixelFormatAttribute *)attribs, 
                                      &ctx->pixformat, &numPixelFormats);
  if (err != kCGLNoError) {
    cc_debugerror_post("cglglue_context_create_pbuffer",
                       "Couldn't create CGL Pixelformat: %s", 
                       CGLErrorString(err));
    return FALSE;
  }
  
  if (!ctx->pixformat) return FALSE;

  err = CGLCreateContext (ctx->pixformat, NULL, &ctx->cglcontext);
  if (err != kCGLNoError || !ctx->cglcontext) {
    cc_debugerror_post("cglglue_context_create_pbuffer",
                       "Couldn't create CGL context: %s", 
                       CGLErrorString(err));
    cglglue_contextdata_cleanup(ctx);
    return FALSE;
  }
  
  
  /* The dimension of CGL pBuffers creating with the GL_TEXTURE_2D
     flag must be both squared and powers of two; else we have to
     use GL_TEXTURE_RECTANGLE_EXT. (It would of course be ok to use
     the rectangle extension in all cases, but drivers may optimize
     for the square, pow2 case.)  
     Note that it is not necessary to check for the availability of
     GL_TEXTURE_RECTANGLE_EXT - we can always assume this extension
     to be present when pBuffers are supported.
  */
  GLenum target = GL_TEXTURE_2D;
  if (!coin_is_power_of_two(ctx->width)  ||
      !coin_is_power_of_two(ctx->height) ||
      ctx->width != ctx->height) {
    target = GL_TEXTURE_RECTANGLE_EXT;
  }
  
  err = cglglue_CGLCreatePBuffer(ctx->width, ctx->height, target, 
                                 GL_RGBA, 0, &ctx->cglpbuffer);
  if (err != kCGLNoError) {
    cc_debugerror_post("cglglue_context_create_pbuffer",
                       "Couldn't create CGL pBuffer: %s", 
                       CGLErrorString(err));
    return FALSE;
  }
  return TRUE;
}


void *
cglglue_context_create_offscreen(unsigned int width, unsigned int height)
{
  struct cglglue_contextdata * ctx;
  SbBool ok, pbuffer = FALSE, ispbuffer = FALSE;

  ctx = cglglue_contextdata_init(width, height);
  if (!ctx) return NULL;

  /* Use cached function pointer for pBuffer vs SW context creation... */
  if (cglglue_context_create != NULL) {

    ispbuffer = (cglglue_context_create == cglglue_context_create_pbuffer);

    /* Try to open a pBuffer context. If that fails, fall back to software. */
    if (cglglue_context_create(ctx)) { return ctx; }
    cglglue_contextdata_cleanup(ctx);

    if (ispbuffer) { 
      if (coin_glglue_debug()) {     
        cc_debugerror_postinfo("cglglue_context_create_offscreen",     
                              "pBuffer failed. Trying software ");
      }
      ctx = cglglue_contextdata_init(width, height);
      assert(ctx);
      if (cglglue_context_create_software(ctx)) { return ctx; } 
      cglglue_contextdata_cleanup(ctx);
    }
    return NULL;
  }

  /* ... but the first time around, we have to figure out. */
  pbuffer = cglglue_get_pbuffer_enable();

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("cglglue_context_create_offscreen",
                           "PBuffer offscreen rendering is %ssupported "
                           "by the OpenGL driver", pbuffer ? "" : "NOT ");
  }
  
  if (pbuffer && cglglue_context_create_pbuffer(ctx)) {
    cglglue_context_create = cglglue_context_create_pbuffer;
    return ctx;
  } else if (cglglue_context_create_software(ctx)) {
    cglglue_context_create = cglglue_context_create_software;
    return ctx;
  } else {
    cglglue_contextdata_cleanup(ctx);
    return NULL;
  }
}


SbBool
cglglue_context_make_current(void * ctx)
{
  struct cglglue_contextdata * context = (struct cglglue_contextdata *)ctx;

  if (!context->cglpbuffer) {
    if (context->cglcontext) {
      context->storedcontext = CGLGetCurrentContext();
    }

    if (coin_glglue_debug()) {
      cc_debugerror_postinfo("cglglue_make_context_current",
                             "store current status first => context==%p",
                             context->storedcontext);
    }

    CGLSetCurrentContext(context->cglcontext);
    return TRUE;

  } else { /* pBuffer support available */

    context->storedcontext = CGLGetCurrentContext();
    CGLError err = CGLSetCurrentContext(context->cglcontext);
    if (err != kCGLNoError) {
        cc_debugerror_post("cglglue_context_make_current",
                           "Error setting offscreen context: %s", 
                           CGLErrorString(err));
    }

    GLint vs;
    err = CGLGetVirtualScreen(context->cglcontext, &vs);
    if (err != kCGLNoError) {
      cc_debugerror_post("cglglue_context_make_current",
                         "Error getting virtual screen: %s", 
                         CGLErrorString(err));
      return FALSE;
    }
    err = cglglue_CGLSetPBuffer(context->cglcontext, context->cglpbuffer, 0, 0, vs);
    if (err != kCGLNoError) {
      cc_debugerror_post("cglglue_context_make_current",
                         "cglSetPBuffer failed: %s", 
                         CGLErrorString(err));
      return FALSE;
    }

    if (coin_glglue_debug()) {
      cc_debugerror_postinfo("cglglue_context_make_current", 
                             "PBuffer Context (0x%X) Renderer: %s\n",
                             context->cglcontext, glGetString(GL_RENDERER));    
    }
    return TRUE;
  }
}

void
cglglue_context_reinstate_previous(void * ctx)
{
  struct cglglue_contextdata * context = (struct cglglue_contextdata *)ctx;

  if (!context->cglpbuffer) {

    if (coin_glglue_debug()) {
      cc_debugerror_postinfo("cglglue_context_reinstate_previous",
                             "releasing context");
    }

    if (context->storedcontext) {

      if (coin_glglue_debug()) {
        cc_debugerror_postinfo("cglglue_context_reinstate_previous",
                               "restoring context %p to be current",
                               context->storedcontext);
      }

      CGLSetCurrentContext(context->storedcontext);
    }

  } else { /* pBuffer support available */

    if (context->storedcontext) CGLSetCurrentContext(context->storedcontext);
    else CGLSetCurrentContext(NULL);

  } 
}

void
cglglue_context_destruct(void * ctx) 
{
  /* FIXME: needs to call into the (as of yet unimplemented)
     C wrapper around the SoContextHandler. 20030310 mortene. */

  struct cglglue_contextdata * context = (struct cglglue_contextdata *)ctx;

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("cglglue_context_destruct",
                           "Destroying context %p", context->cglcontext);
  }
  cglglue_contextdata_cleanup(context);
}

void 
cglglue_context_bind_pbuffer(void * ctx)
{
  struct cglglue_contextdata * context = (struct cglglue_contextdata *)ctx;
  
  CGLError err = cglglue_CGLTexImagePBuffer(context->storedcontext, context->cglpbuffer, 
                                            GL_FRONT);

  if (err != kCGLNoError) {
    cc_debugerror_post("cglglue_context_bind_pbuffer()"                       
                       "after binding pbuffer: %s",                        
                       CGLErrorString(err)); 
  }
  else {
    context->pbufferisbound = TRUE;
  }
}

void 
cglglue_context_release_pbuffer(void * ctx)
{
  struct cglglue_contextdata * context = (struct cglglue_contextdata *)ctx;
  CGLError err = cglglue_CGLDestroyPBuffer(context->cglpbuffer);
  if (err != kCGLNoError) {
    cc_debugerror_post("cglglue_context_release_pbuffer()"                       
                       "releasing pbuffer: %s",                        
                       CGLErrorString(err)); 
  }
}

SbBool 
cglglue_context_pbuffer_is_bound(void * ctx)
{
  struct cglglue_contextdata * context = (struct cglglue_contextdata *)ctx;
  return (context->pbufferisbound);
}

SbBool 
cglglue_context_can_render_to_texture(void * ctx)
{
  struct cglglue_contextdata * context = (struct cglglue_contextdata *)ctx;
  return context->cglpbuffer != NULL;
}

void 
cglglue_cleanup(void)
{
  cglglue_CGLCreatePBuffer = NULL;
  cglglue_CGLDestroyPBuffer = NULL;
  cglglue_CGLSetPBuffer = NULL;
  cglglue_CGLTexImagePBuffer = NULL;

  cglglue_context_create = NULL;
}

// used to look up CGL specific functions
void * 
cglglue_getprocaddress(const char * fname)
{
  void * ret = NULL;
  cc_libhandle h = cc_dl_handle_with_gl_symbols();
  if (h) {
    ret = cc_dl_sym(h, fname);
    cc_dl_close(h);
  }
  return ret;
}

#endif /* HAVE_CGL */
