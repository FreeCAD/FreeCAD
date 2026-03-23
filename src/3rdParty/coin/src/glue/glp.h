#ifndef COIN_GLUE_GLP_H
#define COIN_GLUE_GLP_H

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
#endif /* ! COIN_INTERNAL */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

/* ********************************************************************** */

#include <Inventor/system/gl.h>
#include "base/dict.h"
#include <Inventor/C/base/string.h>
#include <Inventor/C/glue/dl.h>

/* ********************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if 0 /* to get proper auto-indentation in emacs */
}
#endif /* emacs indentation */



/* Under Win32, we need to make sure we use the correct calling method
   by using the APIENTRY define for the function signature types (or
   else we'll get weird stack errors). On other platforms, just define
   APIENTRY empty. */
#ifndef APIENTRY
#define APIENTRY
#endif /* !APIENTRY */

/* Our own typedefs for OpenGL functions. Prefixed with COIN_ to avoid
   namespace collisions. */
typedef void (APIENTRY * COIN_PFNGLTEXIMAGE3DPROC)(GLenum target,
                                                   GLint level,
                                                   GLenum internalformat,
                                                   GLsizei width,
                                                   GLsizei height,
                                                   GLsizei depth,
                                                   GLint border,
                                                   GLenum format,
                                                   GLenum type,
                                                   const GLvoid * pixels);

typedef void (APIENTRY * COIN_PFNGLTEXSUBIMAGE3DPROC)(GLenum target,
                                                      GLint level,
                                                      GLint xoffset,
                                                      GLint yoffset,
                                                      GLint zoffset,
                                                      GLsizei width,
                                                      GLsizei height,
                                                      GLsizei depth,
                                                      GLenum format,
                                                      GLenum type,
                                                      const GLvoid * pixels);

typedef void (APIENTRY * COIN_PFNGLCOPYTEXSUBIMAGE3DPROC)(GLenum target,
                                                          GLint level,
                                                          GLint xoffset,
                                                          GLint yoffset,
                                                          GLint zoffset,
                                                          GLint x,
                                                          GLint y,
                                                          GLsizei width,
                                                          GLsizei height);

typedef void (APIENTRY * COIN_PFNGLPOLYGONOFFSETPROC)(GLfloat factor,
                                                      GLfloat bias);

typedef void (APIENTRY * COIN_PFNGLBINDTEXTUREPROC)(GLenum target,
                                                    GLuint texture);

typedef void (APIENTRY * COIN_PFNGLDELETETEXTURESPROC)(GLsizei n,
                                                       const GLuint * textures);

typedef void (APIENTRY * COIN_PFNGLGENTEXTURESPROC)(GLsizei n,
                                                    GLuint *textures);

typedef void (APIENTRY * COIN_PFNGLTEXSUBIMAGE2DPROC)(GLenum target,
                                                      GLint level,
                                                      GLint xoffset,
                                                      GLint yoffset,
                                                      GLsizei width,
                                                      GLsizei height,
                                                      GLenum format,
                                                      GLenum type,
                                                      const GLvoid * pixels);

typedef void (APIENTRY * COIN_PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void (APIENTRY * COIN_PFNGLCLIENTACTIVETEXTUREPROC)(GLenum texture);
typedef void (APIENTRY * COIN_PFNGLMULTITEXCOORD2FPROC)(GLenum target,
                                                        GLfloat s,
                                                        GLfloat t);
typedef void (APIENTRY * COIN_PFNGLMULTITEXCOORD2FVPROC)(GLenum target,
                                                         const GLfloat * v);
typedef void (APIENTRY * COIN_PFNGLMULTITEXCOORD3FVPROC)(GLenum target,
                                                         const GLfloat * v);
typedef void (APIENTRY * COIN_PFNGLMULTITEXCOORD4FVPROC)(GLenum target,
                                                         const GLfloat * v);

typedef void (APIENTRY * COIN_PFNGLPUSHCLIENTATTRIBPROC)(GLbitfield mask);
typedef void (APIENTRY * COIN_PFNGLPOPCLIENTATTRIBPROC)(void);

/* typedefs for texture compression */
typedef void (APIENTRY * COIN_PFNGLCOMPRESSEDTEXIMAGE3DPROC)(GLenum target,
                                                             GLint level,
                                                             GLenum internalformat,
                                                             GLsizei width,
                                                             GLsizei height,
                                                             GLsizei depth,
                                                             GLint border,
                                                             GLsizei imageSize,
                                                             const GLvoid * data);
typedef void (APIENTRY * COIN_PFNGLCOMPRESSEDTEXIMAGE2DPROC)(GLenum target,
                                                             GLint level,
                                                             GLenum internalformat,
                                                             GLsizei width,
                                                             GLsizei height,
                                                             GLint border,
                                                             GLsizei imageSize,
                                                             const GLvoid * data);
typedef void (APIENTRY * COIN_PFNGLCOMPRESSEDTEXIMAGE1DPROC)(GLenum target,
                                                             GLint level,
                                                             GLenum internalformat,
                                                             GLsizei width,
                                                             GLint border,
                                                             GLsizei imageSize,
                                                             const GLvoid * data);
typedef void (APIENTRY * COIN_PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)(GLenum target,
                                                                GLint level,
                                                                GLint xoffset,
                                                                GLint yoffset,
                                                                GLint zoffset,
                                                                GLsizei width,
                                                                GLsizei height,
                                                                GLsizei depth,
                                                                GLenum format,
                                                                GLsizei imageSize,
                                                                const GLvoid * data);
typedef void (APIENTRY * COIN_PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)(GLenum target,
                                                                GLint level,
                                                                GLint xoffset,
                                                                GLint yoffset,
                                                                GLsizei width,
                                                                GLsizei height,
                                                                GLenum format,
                                                                GLsizei imageSize,
                                                                const GLvoid * data);
typedef void (APIENTRY * COIN_PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)(GLenum target,
                                                                GLint level,
                                                                GLint xoffset,
                                                                GLsizei width,
                                                                GLenum format,
                                                                GLsizei imageSize,
                                                                const GLvoid *data);
typedef void (APIENTRY * COIN_PFNGLGETCOMPRESSEDTEXIMAGEPROC)(GLenum target,
                                                              GLint level,
                                                              void * img);


/* typedefs for palette tetures */
typedef void (APIENTRY * COIN_PFNGLCOLORTABLEPROC)(GLenum target,
                                                   GLenum internalFormat,
                                                   GLsizei width,
                                                   GLenum format,
                                                   GLenum type,
                                                   const GLvoid * table);
typedef void (APIENTRY * COIN_PFNGLCOLORSUBTABLEPROC)(GLenum target,
                                                      GLsizei start,
                                                      GLsizei count,
                                                      GLenum format,
                                                      GLenum type,
                                                      const GLvoid * data);
typedef void (APIENTRY * COIN_PFNGLGETCOLORTABLEPROC)(GLenum target,
                                                      GLenum format,
                                                      GLenum type,
                                                      GLvoid * data);
typedef void (APIENTRY * COIN_PFNGLGETCOLORTABLEPARAMETERIVPROC)(GLenum target,
                                                                 GLenum pname,
                                                                 GLint *params);
typedef void (APIENTRY * COIN_PFNGLGETCOLORTABLEPARAMETERFVPROC)(GLenum target,
                                                                 GLenum pname,
                                                                 GLfloat * params);

/* Typedefs for glBlendEquation[EXT]. */
typedef void *(APIENTRY * COIN_PFNGLBLENDEQUATIONPROC)(GLenum);

/* Typedef for glBlendFuncSeparate */
typedef void *(APIENTRY * COIN_PFNGLBLENDFUNCSEPARATEPROC)(GLenum, GLenum, GLenum, GLenum);

/* typedefs for OpenGL vertex arrays */
typedef void (APIENTRY * COIN_PFNGLVERTEXPOINTERPROC)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
typedef void (APIENTRY * COIN_PFNGLTEXCOORDPOINTERPROC)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
typedef void (APIENTRY * COIN_PFNGLNORMALPOINTERPROC)(GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY * COIN_PNFGLCOLORPOINTERPROC)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
typedef void (APIENTRY * COIN_PFNGLINDEXPOINTERPROC)(GLenum type, GLsizei stride, const GLvoid * pointer);
typedef void (APIENTRY * COIN_PFNGLENABLECLIENTSTATEPROC)(GLenum array);
typedef void (APIENTRY * COIN_PFNGLDISABLECLIENTSTATEPROC)(GLenum array);
typedef void (APIENTRY * COIN_PFNGLINTERLEAVEDARRAYSPROC)(GLenum format, GLsizei stride, const GLvoid * pointer);
typedef void (APIENTRY * COIN_PFNGLDRAWARRAYSPROC)(GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRY * COIN_PFNGLDRAWELEMENTSPROC)(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices);
typedef void (APIENTRY * COIN_PFNGLDRAWRANGEELEMENTSPROC)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices);
typedef void (APIENTRY * COIN_PFNGLARRAYELEMENTPROC)(GLint i);

typedef void (APIENTRY * COIN_PFNGLMULTIDRAWARRAYSPROC)(GLenum mode, const GLint * first,
                                                        const GLsizei * count, GLsizei primcount);
typedef void (APIENTRY * COIN_PFNGLMULTIDRAWELEMENTSPROC)(GLenum mode, const GLsizei * count,
                                                          GLenum type, const GLvoid ** indices, GLsizei primcount);

/* Typedefs for NV_vertex_array_range */
typedef void (APIENTRY * COIN_PFNGLFLUSHVERTEXARRAYRANGENVPROC)(void);
typedef void (APIENTRY * COIN_PFNGLVERTEXARRAYRANGENVPROC)(GLsizei size, const GLvoid * pointer);
typedef void * (APIENTRY * COIN_PFNGLALLOCATEMEMORYNVPROC)(GLsizei size, GLfloat readfreq,
                                                           GLfloat writefreq, GLfloat priority);
typedef void (APIENTRY * COIN_PFNGLFREEMEMORYNVPROC)(GLvoid * buffer);


/* typedefs for GL_ARB_vertex_buffer_object */
typedef void (APIENTRY * COIN_PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (APIENTRY * COIN_PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint * buffers);
typedef void (APIENTRY * COIN_PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef GLboolean (APIENTRY * COIN_PFNGLISBUFFERPROC)(GLuint buffer);
typedef void (APIENTRY * COIN_PFNGLBUFFERDATAPROC)(GLenum target,
                                                   intptr_t size, /* 64 bit on 64 bit systems */
                                                   const GLvoid *data,
                                                   GLenum usage);
typedef void (APIENTRY * COIN_PFNGLBUFFERSUBDATAPROC)(GLenum target,
                                                      intptr_t offset, /* 64 bit */
                                                      intptr_t size, /* 64 bit */
                                                      const GLvoid * data);
typedef void (APIENTRY * COIN_PFNGLGETBUFFERSUBDATAPROC)(GLenum target,
                                                         intptr_t offset, /* 64 bit */
                                                         intptr_t size, /* 64 bit */
                                                         GLvoid *data);
typedef GLvoid * (APIENTRY * COIN_PNFGLMAPBUFFERPROC)(GLenum target, GLenum access);
typedef GLboolean (APIENTRY * COIN_PFNGLUNMAPBUFFERPROC)(GLenum target);
typedef void (APIENTRY * COIN_PFNGLGETBUFFERPARAMETERIVPROC)(GLenum target,
                                                             GLenum pname,
                                                             GLint * params);
typedef void (APIENTRY * COIN_PFNGLGETBUFFERPOINTERVPROC)(GLenum target,
                                                          GLenum pname,
                                                          GLvoid ** params);

/* Typedefs for GL_NV_register_combiners */
typedef void (APIENTRY * COIN_PFNGLCOMBINERPARAMETERFVNVPROC)(GLenum pname,
                                                              const GLfloat *params);
typedef void (APIENTRY * COIN_PFNGLCOMBINERPARAMETERIVNVPROC)(GLenum pname,
                                                              const GLint *params);
typedef void (APIENTRY * COIN_PFNGLCOMBINERPARAMETERFNVPROC)(GLenum pname,
                                                             GLfloat param);
typedef void (APIENTRY * COIN_PFNGLCOMBINERPARAMETERINVPROC)(GLenum pname,
                                                            GLint param);
typedef void (APIENTRY * COIN_PFNGLCOMBINERINPUTNVPROC)(GLenum stage,
                                                        GLenum portion,
                                                        GLenum variable,
                                                        GLenum input,
                                                        GLenum mapping,
                                                        GLenum componentUsage);
typedef void (APIENTRY * COIN_PFNGLCOMBINEROUTPUTNVPROC)(GLenum stage,
                                                         GLenum portion,
                                                         GLenum abOutput,
                                                         GLenum cdOutput,
                                                         GLenum sumOutput,
                                                         GLenum scale,
                                                         GLenum bias,
                                                         GLboolean abDotProduct,
                                                         GLboolean cdDotProduct,
                                                         GLboolean muxSum);
typedef void (APIENTRY * COIN_PFNGLFINALCOMBINERINPUTNVPROC)(GLenum variable,
                                                             GLenum input,
                                                             GLenum mapping,
                                                             GLenum componentUsage);
typedef void (APIENTRY * COIN_PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC)(GLenum stage,
                                                                      GLenum portion,
                                                                      GLenum variable,
                                                                      GLenum pname,
                                                                      GLfloat *params);
typedef void (APIENTRY * COIN_PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC)(GLenum stage,
                                                                      GLenum portion,
                                                                      GLenum variable,
                                                                      GLenum pname,
                                                                      GLint *params);
typedef void (APIENTRY * COIN_PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC)(GLenum stage,
                                                                       GLenum portion,
                                                                       GLenum pname,
                                                                       GLfloat *params);
typedef void (APIENTRY * COIN_PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC)(GLenum stage,
                                                                       GLenum portion,
                                                                       GLenum pname,
                                                                       GLint *params);
typedef void (APIENTRY * COIN_PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC)(GLenum variable,
                                                                           GLenum pname,
                                                                           GLfloat *params);
typedef void (APIENTRY * COIN_PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC)(GLenum variable,
                                                                           GLenum pname,
                                                                           GLint *params);
/* Typedefs for GL_ARB_fragment_program */
typedef void (APIENTRY * COIN_PFNGLPROGRAMSTRINGARBPROC)(GLenum target,
                                                         GLenum format,
                                                         GLsizei len,
                                                         const GLvoid *string);

typedef void (APIENTRY * COIN_PFNGLBINDPROGRAMARBPROC)(GLenum target,
                                                       GLuint program);

typedef void (APIENTRY * COIN_PFNGLDELETEPROGRAMSARBPROC)(GLsizei n,
                                                          const GLuint *programs);

typedef void (APIENTRY * COIN_PFNGLGENPROGRAMSARBPROC)(GLsizei n,
                                                       GLuint *programs);

typedef void (APIENTRY * COIN_PFNGLPROGRAMENVPARAMETER4DARBPROC)(GLenum target,
                                                                 GLuint index,
                                                                 GLdouble x,
                                                                 GLdouble y,
                                                                 GLdouble z,
                                                                 GLdouble w);
typedef void (APIENTRY * COIN_PFNGLPROGRAMENVPARAMETER4DVARBPROC)(GLenum target,
                                                                  GLuint index,
                                                                  const GLdouble *params);
typedef void (APIENTRY * COIN_PFNGLPROGRAMENVPARAMETER4FARBPROC)(GLenum target,
                                                                 GLuint index,
                                                                 GLfloat x,
                                                                 GLfloat y,
                                                                 GLfloat z,
                                                                 GLfloat w);
typedef void (APIENTRY * COIN_PFNGLPROGRAMENVPARAMETER4FVARBPROC)(GLenum target,
                                                                  GLuint index,
                                                                  const GLfloat *params);
typedef void (APIENTRY * COIN_PFNGLPROGRAMLOCALPARAMETER4DARBPROC)(GLenum target,
                                                                   GLuint index,
                                                                   GLdouble x,
                                                                   GLdouble y,
                                                                   GLdouble z,
                                                                   GLdouble w);
typedef void (APIENTRY * COIN_PFNGLPROGRAMLOCALPARAMETER4DVARBPROC)(GLenum target,
                                                                    GLuint index,
                                                                    const GLdouble *params);
typedef void (APIENTRY * COIN_PFNGLPROGRAMLOCALPARAMETER4FARBPROC)(GLenum target,
                                                                   GLuint index,
                                                                   GLfloat x,
                                                                   GLfloat y,
                                                                   GLfloat z,
                                                                   GLfloat w);
typedef void (APIENTRY * COIN_PFNGLPROGRAMLOCALPARAMETER4FVARBPROC)(GLenum target,
                                                                    GLuint index,
                                                                    const GLfloat *params);
typedef void (APIENTRY * COIN_PFNGLGETPROGRAMENVPARAMETERDVARBPROC)(GLenum target,
                                                                    GLuint index,
                                                                    GLdouble *params);
typedef void (APIENTRY * COIN_PFNGLGETPROGRAMENVPARAMETERFVARBPROC)(GLenum target,
                                                                    GLuint index,
                                                                    GLfloat *params);
typedef void (APIENTRY * COIN_PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC)(GLenum target,
                                                                      GLuint index,
                                                                      GLdouble *params);
typedef void (APIENTRY * COIN_PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC)(GLenum target,
                                                                      GLuint index,
                                                                      GLfloat *params);
typedef void (APIENTRY * COIN_PFNGLGETPROGRAMIVARBPROC)(GLenum target,
                                                        GLenum pname,
                                                        GLint *params);
typedef void (APIENTRY * COIN_PFNGLGETPROGRAMSTRINGARBPROC)(GLenum target,
                                                            GLenum pname,
                                                            GLvoid *string);
typedef GLboolean (APIENTRY * COIN_PFNGLISPROGRAMARBPROC)(GLuint program);


/* Typedefs for GL_ARB_vertex_program */
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB1SARBPROC)(GLuint index, GLshort x);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB1FARBPROC)(GLuint index, GLfloat x);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB1DARBPROC)(GLuint index, GLdouble x);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB2SARBPROC)(GLuint index, GLshort x, GLshort y);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB2FARBPROC)(GLuint index, GLfloat x, GLfloat y);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB2DARBPROC)(GLuint index, GLdouble x, GLdouble y);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB3SARBPROC)(GLuint index, GLshort x,
                                                          GLshort y, GLshort z);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB3FARBPROC)(GLuint index, GLfloat x,
                                                          GLfloat y, GLfloat z);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB3DARBPROC)(GLuint index, GLdouble x,
                                                          GLdouble y, GLdouble z);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4SARBPROC)(GLuint index, GLshort x,
                                                          GLshort y, GLshort z, GLshort w);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4FARBPROC)(GLuint index, GLfloat x,
                                                          GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4DARBPROC)(GLuint index, GLdouble x,
                                                          GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4NUBARBPROC)(GLuint index, GLubyte x,
                                                            GLubyte y, GLubyte z, GLubyte w);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB1SVARBPROC)(GLuint index, const GLshort *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB1FVARBPROC)(GLuint index, const GLfloat *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB1DVARBPROC)(GLuint index, const GLdouble *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB2SVARBPROC)(GLuint index, const GLshort *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB2FVARBPROC)(GLuint index, const GLfloat *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB2DVARBPROC)(GLuint index, const GLdouble *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB3SVARBPROC)(GLuint index, const GLshort *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB3FVARBPROC)(GLuint index, const GLfloat *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB3DVARBPROC)(GLuint index, const GLdouble *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4BVARBPROC)(GLuint index, const GLbyte *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4SVARBPROC)(GLuint index, const GLshort *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4IVARBPROC)(GLuint index, const GLint *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4UBVARBPROC)(GLuint index, const GLubyte *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4USVARBPROC)(GLuint index, const GLushort *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4UIVARBPROC)(GLuint index, const GLuint *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4FVARBPROC)(GLuint index, const GLfloat *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4DVARBPROC)(GLuint index, const GLdouble *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4NBVARBPROC)(GLuint index, const GLbyte *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4NSVARBPROC)(GLuint index, const GLshort *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4NIVARBPROC)(GLuint index, const GLint *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4NUBVARBPROC)(GLuint index, const GLubyte *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4NUSVARBPROC)(GLuint index, const GLushort *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIB4NUIVARBPROC)(GLuint index, const GLuint *v);
typedef void (APIENTRY * COIN_PFNGLVERTEXATTRIBPOINTERARBPROC)(GLuint index, GLint size,
                                                               GLenum type, GLboolean normalized,
                                                               GLsizei stride,
                                                               const GLvoid *pointer);
typedef void (APIENTRY * COIN_PFNGLENABLEVERTEXATTRIBARRAYARBPROC)(GLuint index);
typedef void (APIENTRY * COIN_PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)(GLuint index);
typedef void (APIENTRY * COIN_PFNGLGETVERTEXATTRIBDVARBPROC)(GLuint index, GLenum pname,
                                                             GLdouble *params);
typedef void (APIENTRY * COIN_PFNGLGETVERTEXATTRIBFVARBPROC)(GLuint index, GLenum pname,
                                                             GLfloat *params);
typedef void (APIENTRY * COIN_PFNGLGETVERTEXATTRIBIVARBPROC)(GLuint index, GLenum pname,
                                                             GLint *params);
typedef void (APIENTRY * COIN_PFNGLGETVERTEXATTRIBPOINTERVARBPROC)(GLuint index, GLenum pname,
                                                                   GLvoid **pointer);

/* FIXME: according to the GL_ARB_shader_objects doc, these types must
   be at least 8 bits wide and 32 bits wide, respectively. Apart from
   that, there does not seem to be any other limitations on them, so
   these types may not match the actual types used on the platform
   (these were taken from NVIDIA's glext.h for their 32-bit Linux
   drivers). How should this be properly handled? Is there any way at
   all one could possibly pick up these at the correct size in a
   dynamic manner? 20050124 mortene. */
typedef char COIN_GLchar;
typedef unsigned long COIN_GLhandle;

/* Typedefs for GL_ARB_vertex_shader */
typedef void (APIENTRY * COIN_PFNGLBINDATTRIBLOCATIONARBPROC)(COIN_GLhandle programobj, GLuint index, COIN_GLchar * name);
typedef int (APIENTRY * COIN_PFNGLGETATTRIBLOCATIONARBPROC)(COIN_GLhandle programobj, const COIN_GLchar * name);
typedef void (APIENTRY * COIN_PFNGLGETACTIVEATTRIBARBPROC)(COIN_GLhandle programobj, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, COIN_GLchar * name);


/* Typedefs for shader objects -- GL_ARB_shader_objects */
typedef void (APIENTRY * COIN_PFNGLPROGRAMPARAMETERIEXT)(COIN_GLhandle, GLenum, GLenum);

typedef int (APIENTRY * COIN_PFNGLGETUNIFORMLOCATIONARBPROC)(COIN_GLhandle,
                                                             const COIN_GLchar *);
typedef void (APIENTRY * COIN_PFNGLGETACTIVEUNIFORMARBPROC)(COIN_GLhandle,
                                                            GLuint index,
                                                            GLsizei maxLength,
                                                            GLsizei * length,
                                                            GLint * size,
                                                            GLenum * type,
                                                            COIN_GLchar * name);
typedef void (APIENTRY * COIN_PFNGLUNIFORM1FARBPROC)(GLint location, GLfloat v0);
typedef void (APIENTRY * COIN_PFNGLUNIFORM2FARBPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY * COIN_PFNGLUNIFORM3FARBPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY * COIN_PFNGLUNIFORM4FARBPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef COIN_GLhandle (APIENTRY * COIN_PFNGLCREATESHADEROBJECTARBPROC)(GLenum);
typedef void (APIENTRY * COIN_PFNGLSHADERSOURCEARBPROC)(COIN_GLhandle, GLsizei, const COIN_GLchar **, const GLint *);
typedef void (APIENTRY * COIN_PFNGLCOMPILESHADERARBPROC)(COIN_GLhandle);
typedef void (APIENTRY * COIN_PFNGLGETOBJECTPARAMETERIVARBPROC)(COIN_GLhandle, GLenum, GLint *);
typedef void (APIENTRY * COIN_PFNGLDELETEOBJECTARBPROC)(COIN_GLhandle);
typedef void (APIENTRY * COIN_PFNGLATTACHOBJECTARBPROC)(COIN_GLhandle, COIN_GLhandle);
typedef void (APIENTRY * COIN_PFNGLDETACHOBJECTARBPROC)(COIN_GLhandle, COIN_GLhandle);
typedef void (APIENTRY * COIN_PFNGLGETINFOLOGARBPROC)(COIN_GLhandle, GLsizei, GLsizei *, COIN_GLchar *);
typedef void (APIENTRY * COIN_PFNGLLINKPROGRAMARBPROC)(COIN_GLhandle);
typedef void (APIENTRY * COIN_PFNGLUSEPROGRAMOBJECTARBPROC)(COIN_GLhandle);
typedef COIN_GLhandle (APIENTRY * COIN_PFNGLCREATEPROGRAMOBJECTARBPROC)(void);
typedef void (APIENTRY * COIN_PFNGLUNIFORM1FVARBPROC)(COIN_GLhandle, GLsizei, const GLfloat *);
typedef void (APIENTRY * COIN_PFNGLUNIFORM2FVARBPROC)(COIN_GLhandle, GLsizei, const GLfloat *);
typedef void (APIENTRY * COIN_PFNGLUNIFORM3FVARBPROC)(COIN_GLhandle, GLsizei, const GLfloat *);
typedef void (APIENTRY * COIN_PFNGLUNIFORM4FVARBPROC)(COIN_GLhandle, GLsizei, const GLfloat *);
typedef void (APIENTRY * COIN_PFNGLUNIFORM1IARBPROC)(COIN_GLhandle, const GLint);
typedef void (APIENTRY * COIN_PFNGLUNIFORM2IARBPROC)(COIN_GLhandle, const GLint, GLint);
typedef void (APIENTRY * COIN_PFNGLUNIFORM3IARBPROC)(COIN_GLhandle, const GLint, GLint, GLint);
typedef void (APIENTRY * COIN_PFNGLUNIFORM4IARBPROC)(COIN_GLhandle, const GLint, GLint, GLint, GLint);
typedef void (APIENTRY * COIN_PFNGLUNIFORM1IVARBPROC)(COIN_GLhandle, GLsizei, const GLint *);
typedef void (APIENTRY * COIN_PFNGLUNIFORM2IVARBPROC)(COIN_GLhandle, GLsizei, const GLint *);
typedef void (APIENTRY * COIN_PFNGLUNIFORM3IVARBPROC)(COIN_GLhandle, GLsizei, const GLint *);
typedef void (APIENTRY * COIN_PFNGLUNIFORM4IVARBPROC)(COIN_GLhandle, GLsizei, const GLint *);

typedef void (APIENTRY * COIN_PFNGLUNIFORMMATRIX2FVARBPROC)(COIN_GLhandle, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY * COIN_PFNGLUNIFORMMATRIX3FVARBPROC)(COIN_GLhandle, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY * COIN_PFNGLUNIFORMMATRIX4FVARBPROC)(COIN_GLhandle, GLsizei, GLboolean, const GLfloat *);


/* Typedefs for occlusion queries -- GL_ARB_occlusion_query */

typedef void (APIENTRY * COIN_PFNGLGENQUERIESPROC)(GLsizei n, GLuint * ids);
typedef void (APIENTRY * COIN_PFNGLDELETEQUERIESPROC)(GLsizei n, const GLuint *ids);
typedef GLboolean (APIENTRY * COIN_PFNGLISQUERYPROC)(GLuint id);
typedef void (APIENTRY * COIN_PFNGLBEGINQUERYPROC)(GLenum target, GLuint id);
typedef void (APIENTRY * COIN_PFNGLENDQUERYPROC)(GLenum target);
typedef void (APIENTRY * COIN_PFNGLGETQUERYIVPROC)(GLenum target, GLenum pname, GLint * params);
typedef void (APIENTRY * COIN_PFNGLGETQUERYOBJECTIVPROC)(GLuint id, GLenum pname, GLint * params);
typedef void (APIENTRY * COIN_PFNGLGETQUERYOBJECTUIVPROC)(GLuint id, GLenum pname, GLuint * params);

/* Typedefs for GLX functions. */
typedef void *(APIENTRY * COIN_PFNGLXGETCURRENTDISPLAYPROC)(void);
typedef void *(APIENTRY * COIN_PFNGLXGETPROCADDRESSPROC)(const GLubyte *);


/* Typedefs for Framebuffer objects */

typedef void (APIENTRY * COIN_PFNGLISRENDERBUFFERPROC)(GLuint renderbuffer);
typedef void (APIENTRY * COIN_PFNGLBINDRENDERBUFFERPROC)(GLenum target, GLuint renderbuffer);
typedef void (APIENTRY * COIN_PFNGLDELETERENDERBUFFERSPROC)(GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRY * COIN_PFNGLGENRENDERBUFFERSPROC)(GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRY * COIN_PFNGLRENDERBUFFERSTORAGEPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY * COIN_PFNGLGETRENDERBUFFERPARAMETERIVPROC)(GLenum target, GLenum pname, GLint *params);
typedef GLboolean (APIENTRY * COIN_PFNGLISFRAMEBUFFERPROC)(GLuint framebuffer);
typedef void (APIENTRY * COIN_PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void (APIENTRY * COIN_PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY * COIN_PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRY * COIN_PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
typedef void (APIENTRY * COIN_PFNGLFRAMEBUFFERTEXTURE1DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY * COIN_PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY * COIN_PFNGLFRAMEBUFFERTEXTURE3DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
typedef void (APIENTRY * COIN_PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRY * COIN_PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
typedef void (APIENTRY * COIN_PFNGLGENERATEMIPMAPPROC)(GLenum target);

/* Typedef for new extension string method */

typedef GLubyte* (APIENTRY * COIN_PFNGLGETSTRINGIPROC) (GLenum target, GLuint idx);


/* ********************************************************************** */

/* Type specification for GLX info storage structure, embedded within
   the main GL info structure below. */
struct cc_glxglue {
  struct {
    int major, minor;
  } version;

  SbBool isdirect;

  const char * serverversion;
  const char * servervendor;
  const char * serverextensions;

  const char * clientversion;
  const char * clientvendor;
  const char * clientextensions;

  const char * glxextensions;

  COIN_PFNGLXGETCURRENTDISPLAYPROC glXGetCurrentDisplay;
  COIN_PFNGLXGETPROCADDRESSPROC glXGetProcAddress;
  SbBool tried_bind_glXGetProcAddress;
};

/* ********************************************************************** */

/* GL info storage structure. An instance will be allocated and
   initialized for each new GL context id. */
struct cc_glglue {

  uint32_t contextid;
  struct { /* OpenGL versioning. */
    unsigned int major, minor, release;
  } version;

  /* OpenGL calls. Will be NULL if not available, otherwise they
     contain a valid function pointer into the OpenGL library. */
  COIN_PFNGLPOLYGONOFFSETPROC glPolygonOffset;
  COIN_PFNGLPOLYGONOFFSETPROC glPolygonOffsetEXT;

  COIN_PFNGLGENTEXTURESPROC glGenTextures;
  COIN_PFNGLBINDTEXTUREPROC glBindTexture;
  COIN_PFNGLDELETETEXTURESPROC glDeleteTextures;

  COIN_PFNGLTEXIMAGE3DPROC glTexImage3D;
  COIN_PFNGLCOPYTEXSUBIMAGE3DPROC glCopyTexSubImage3D;
  COIN_PFNGLTEXSUBIMAGE3DPROC glTexSubImage3D;
  COIN_PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D;

  COIN_PFNGLACTIVETEXTUREPROC glActiveTexture;
  COIN_PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture;
  COIN_PFNGLMULTITEXCOORD2FPROC glMultiTexCoord2f;
  COIN_PFNGLMULTITEXCOORD2FVPROC glMultiTexCoord2fv;
  COIN_PFNGLMULTITEXCOORD3FVPROC glMultiTexCoord3fv;
  COIN_PFNGLMULTITEXCOORD4FVPROC glMultiTexCoord4fv;

  COIN_PFNGLCOLORTABLEPROC glColorTable;
  COIN_PFNGLCOLORSUBTABLEPROC glColorSubTable;
  COIN_PFNGLGETCOLORTABLEPROC glGetColorTable;
  COIN_PFNGLGETCOLORTABLEPARAMETERIVPROC glGetColorTableParameteriv;
  COIN_PFNGLGETCOLORTABLEPARAMETERFVPROC glGetColorTableParameterfv;

  SbBool supportsPalettedTextures;

  COIN_PFNGLCOMPRESSEDTEXIMAGE3DPROC glCompressedTexImage3D;
  COIN_PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D;
  COIN_PFNGLCOMPRESSEDTEXIMAGE1DPROC glCompressedTexImage1D;
  COIN_PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glCompressedTexSubImage3D;
  COIN_PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D;
  COIN_PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glCompressedTexSubImage1D;
  COIN_PFNGLGETCOMPRESSEDTEXIMAGEPROC glGetCompressedTexImage;

  COIN_PFNGLBLENDEQUATIONPROC glBlendEquation;
  COIN_PFNGLBLENDEQUATIONPROC glBlendEquationEXT;

  COIN_PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;

  COIN_PFNGLVERTEXPOINTERPROC glVertexPointer;
  COIN_PFNGLTEXCOORDPOINTERPROC glTexCoordPointer;
  COIN_PFNGLNORMALPOINTERPROC glNormalPointer;
  COIN_PNFGLCOLORPOINTERPROC glColorPointer;
  COIN_PFNGLINDEXPOINTERPROC glIndexPointer;
  COIN_PFNGLENABLECLIENTSTATEPROC glEnableClientState;
  COIN_PFNGLDISABLECLIENTSTATEPROC glDisableClientState;
  COIN_PFNGLINTERLEAVEDARRAYSPROC glInterleavedArrays;
  COIN_PFNGLDRAWARRAYSPROC glDrawArrays;
  COIN_PFNGLDRAWELEMENTSPROC glDrawElements;
  COIN_PFNGLDRAWRANGEELEMENTSPROC glDrawRangeElements;
  COIN_PFNGLARRAYELEMENTPROC glArrayElement;

  COIN_PFNGLMULTIDRAWARRAYSPROC glMultiDrawArrays;
  COIN_PFNGLMULTIDRAWELEMENTSPROC glMultiDrawElements;

  COIN_PFNGLVERTEXARRAYRANGENVPROC glVertexArrayRangeNV;
  COIN_PFNGLFLUSHVERTEXARRAYRANGENVPROC glFlushVertexArrayRangeNV;
  COIN_PFNGLALLOCATEMEMORYNVPROC glAllocateMemoryNV;
  COIN_PFNGLFREEMEMORYNVPROC glFreeMemoryNV;

  COIN_PFNGLBINDBUFFERPROC glBindBuffer;
  COIN_PFNGLDELETEBUFFERSPROC glDeleteBuffers;
  COIN_PFNGLGENBUFFERSPROC glGenBuffers;
  COIN_PFNGLISBUFFERPROC glIsBuffer;
  COIN_PFNGLBUFFERDATAPROC glBufferData;
  COIN_PFNGLBUFFERSUBDATAPROC glBufferSubData;
  COIN_PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData;
  COIN_PNFGLMAPBUFFERPROC glMapBuffer;
  COIN_PFNGLUNMAPBUFFERPROC glUnmapBuffer;
  COIN_PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv;
  COIN_PFNGLGETBUFFERPOINTERVPROC glGetBufferPointerv;

  /* NV register combiners */
  COIN_PFNGLCOMBINERPARAMETERFVNVPROC glCombinerParameterfvNV;
  COIN_PFNGLCOMBINERPARAMETERIVNVPROC glCombinerParameterivNV;
  COIN_PFNGLCOMBINERPARAMETERFNVPROC glCombinerParameterfNV;
  COIN_PFNGLCOMBINERPARAMETERINVPROC glCombinerParameteriNV;
  COIN_PFNGLCOMBINERINPUTNVPROC glCombinerInputNV;
  COIN_PFNGLCOMBINEROUTPUTNVPROC glCombinerOutputNV;
  COIN_PFNGLFINALCOMBINERINPUTNVPROC glFinalCombinerInputNV;
  COIN_PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC glGetCombinerInputParameterfvNV;
  COIN_PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC glGetCombinerInputParameterivNV;
  COIN_PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC glGetCombinerOutputParameterfvNV;
  COIN_PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC glGetCombinerOutputParameterivNV;
  COIN_PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC glGetFinalCombinerInputParameterfvNV;
  COIN_PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC glGetFinalCombinerInputParameterivNV;

  /* fragment program */
  COIN_PFNGLPROGRAMSTRINGARBPROC glProgramStringARB;
  COIN_PFNGLBINDPROGRAMARBPROC glBindProgramARB;
  COIN_PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB;
  COIN_PFNGLGENPROGRAMSARBPROC glGenProgramsARB;
  COIN_PFNGLPROGRAMENVPARAMETER4DARBPROC glProgramEnvParameter4dARB;
  COIN_PFNGLPROGRAMENVPARAMETER4DVARBPROC glProgramEnvParameter4dvARB;
  COIN_PFNGLPROGRAMENVPARAMETER4FARBPROC glProgramEnvParameter4fARB;
  COIN_PFNGLPROGRAMENVPARAMETER4FVARBPROC glProgramEnvParameter4fvARB;
  COIN_PFNGLPROGRAMLOCALPARAMETER4DARBPROC glProgramLocalParameter4dARB;
  COIN_PFNGLPROGRAMLOCALPARAMETER4DVARBPROC glProgramLocalParameter4dvARB;
  COIN_PFNGLPROGRAMLOCALPARAMETER4FARBPROC glProgramLocalParameter4fARB;
  COIN_PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB;
  COIN_PFNGLGETPROGRAMENVPARAMETERDVARBPROC glGetProgramEnvParameterdvARB;
  COIN_PFNGLGETPROGRAMENVPARAMETERFVARBPROC glGetProgramEnvParameterfvARB;
  COIN_PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glGetProgramLocalParameterdvARB;
  COIN_PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC glGetProgramLocalParameterfvARB;
  COIN_PFNGLGETPROGRAMIVARBPROC glGetProgramivARB;
  COIN_PFNGLGETPROGRAMSTRINGARBPROC glGetProgramStringARB;
  COIN_PFNGLISPROGRAMARBPROC glIsProgramARB;

  /* vertex program */
  COIN_PFNGLVERTEXATTRIB1SARBPROC glVertexAttrib1sARB;
  COIN_PFNGLVERTEXATTRIB1FARBPROC glVertexAttrib1fARB;
  COIN_PFNGLVERTEXATTRIB1DARBPROC glVertexAttrib1dARB;
  COIN_PFNGLVERTEXATTRIB2SARBPROC glVertexAttrib2sARB;
  COIN_PFNGLVERTEXATTRIB2FARBPROC glVertexAttrib2fARB;
  COIN_PFNGLVERTEXATTRIB2DARBPROC glVertexAttrib2dARB;
  COIN_PFNGLVERTEXATTRIB3SARBPROC glVertexAttrib3sARB;
  COIN_PFNGLVERTEXATTRIB3FARBPROC glVertexAttrib3fARB;
  COIN_PFNGLVERTEXATTRIB3DARBPROC glVertexAttrib3dARB;
  COIN_PFNGLVERTEXATTRIB4SARBPROC glVertexAttrib4sARB;
  COIN_PFNGLVERTEXATTRIB4FARBPROC glVertexAttrib4fARB;
  COIN_PFNGLVERTEXATTRIB4DARBPROC glVertexAttrib4dARB;
  COIN_PFNGLVERTEXATTRIB4NUBARBPROC glVertexAttrib4NubARB;
  COIN_PFNGLVERTEXATTRIB1SVARBPROC glVertexAttrib1svARB;
  COIN_PFNGLVERTEXATTRIB1FVARBPROC glVertexAttrib1fvARB;
  COIN_PFNGLVERTEXATTRIB1DVARBPROC glVertexAttrib1dvARB;
  COIN_PFNGLVERTEXATTRIB2SVARBPROC glVertexAttrib2svARB;
  COIN_PFNGLVERTEXATTRIB2FVARBPROC glVertexAttrib2fvARB;
  COIN_PFNGLVERTEXATTRIB2DVARBPROC glVertexAttrib2dvARB;
  COIN_PFNGLVERTEXATTRIB3SVARBPROC glVertexAttrib3svARB;
  COIN_PFNGLVERTEXATTRIB3FVARBPROC glVertexAttrib3fvARB;
  COIN_PFNGLVERTEXATTRIB3DVARBPROC glVertexAttrib3dvARB;
  COIN_PFNGLVERTEXATTRIB4BVARBPROC glVertexAttrib4bvARB;
  COIN_PFNGLVERTEXATTRIB4SVARBPROC glVertexAttrib4svARB;
  COIN_PFNGLVERTEXATTRIB4IVARBPROC glVertexAttrib4ivARB;
  COIN_PFNGLVERTEXATTRIB4UBVARBPROC glVertexAttrib4ubvARB;
  COIN_PFNGLVERTEXATTRIB4USVARBPROC glVertexAttrib4usvARB;
  COIN_PFNGLVERTEXATTRIB4UIVARBPROC glVertexAttrib4uivARB;
  COIN_PFNGLVERTEXATTRIB4FVARBPROC glVertexAttrib4fvARB;
  COIN_PFNGLVERTEXATTRIB4DVARBPROC glVertexAttrib4dvARB;
  COIN_PFNGLVERTEXATTRIB4NBVARBPROC glVertexAttrib4NbvARB;
  COIN_PFNGLVERTEXATTRIB4NSVARBPROC glVertexAttrib4NsvARB;
  COIN_PFNGLVERTEXATTRIB4NIVARBPROC glVertexAttrib4NivARB;
  COIN_PFNGLVERTEXATTRIB4NUBVARBPROC glVertexAttrib4NubvARB;
  COIN_PFNGLVERTEXATTRIB4NUSVARBPROC glVertexAttrib4NusvARB;
  COIN_PFNGLVERTEXATTRIB4NUIVARBPROC glVertexAttrib4NuivARB;
  COIN_PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB;
  COIN_PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB;
  COIN_PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB;
  COIN_PFNGLGETVERTEXATTRIBDVARBPROC glGetVertexAttribdvARB;
  COIN_PFNGLGETVERTEXATTRIBFVARBPROC glGetVertexAttribfvARB;
  COIN_PFNGLGETVERTEXATTRIBIVARBPROC glGetVertexAttribivARB;
  COIN_PFNGLGETVERTEXATTRIBPOINTERVARBPROC glGetVertexAttribPointervARB;

  /* vertex shader */
  COIN_PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB;
  COIN_PFNGLGETACTIVEATTRIBARBPROC glGetActiveAttribARB;
  COIN_PFNGLGETATTRIBLOCATIONARBPROC glGetAttribLocationARB;

  /* shader objects */
  COIN_PFNGLPROGRAMPARAMETERIEXT glProgramParameteriEXT;
  COIN_PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
  COIN_PFNGLGETACTIVEUNIFORMARBPROC glGetActiveUniformARB;
  COIN_PFNGLUNIFORM1FARBPROC glUniform1fARB;
  COIN_PFNGLUNIFORM2FARBPROC glUniform2fARB;
  COIN_PFNGLUNIFORM3FARBPROC glUniform3fARB;
  COIN_PFNGLUNIFORM4FARBPROC glUniform4fARB;
  COIN_PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
  COIN_PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
  COIN_PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
  COIN_PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
  COIN_PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
  COIN_PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
  COIN_PFNGLDETACHOBJECTARBPROC glDetachObjectARB;
  COIN_PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
  COIN_PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
  COIN_PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
  COIN_PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
  COIN_PFNGLUNIFORM1FVARBPROC glUniform1fvARB;
  COIN_PFNGLUNIFORM2FVARBPROC glUniform2fvARB;
  COIN_PFNGLUNIFORM3FVARBPROC glUniform3fvARB;
  COIN_PFNGLUNIFORM4FVARBPROC glUniform4fvARB;
  COIN_PFNGLUNIFORM1IARBPROC glUniform1iARB;
  COIN_PFNGLUNIFORM2IARBPROC glUniform2iARB;
  COIN_PFNGLUNIFORM3IARBPROC glUniform3iARB;
  COIN_PFNGLUNIFORM4IARBPROC glUniform4iARB;
  COIN_PFNGLUNIFORM1IVARBPROC glUniform1ivARB;
  COIN_PFNGLUNIFORM2IVARBPROC glUniform2ivARB;
  COIN_PFNGLUNIFORM3IVARBPROC glUniform3ivARB;
  COIN_PFNGLUNIFORM4IVARBPROC glUniform4ivARB;
  COIN_PFNGLUNIFORMMATRIX2FVARBPROC glUniformMatrix2fvARB;
  COIN_PFNGLUNIFORMMATRIX3FVARBPROC glUniformMatrix3fvARB;
  COIN_PFNGLUNIFORMMATRIX4FVARBPROC glUniformMatrix4fvARB;

  COIN_PFNGLPUSHCLIENTATTRIBPROC glPushClientAttrib;
  COIN_PFNGLPOPCLIENTATTRIBPROC glPopClientAttrib;

  COIN_PFNGLGENQUERIESPROC glGenQueries;
  COIN_PFNGLDELETEQUERIESPROC glDeleteQueries;
  COIN_PFNGLISQUERYPROC glIsQuery;
  COIN_PFNGLBEGINQUERYPROC glBeginQuery;
  COIN_PFNGLENDQUERYPROC glEndQuery;
  COIN_PFNGLGETQUERYIVPROC glGetQueryiv;
  COIN_PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv;
  COIN_PFNGLGETQUERYOBJECTUIVPROC glGetQueryObjectuiv;

  /* FBO */
  COIN_PFNGLISRENDERBUFFERPROC glIsRenderbuffer;
  COIN_PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
  COIN_PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
  COIN_PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
  COIN_PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
  COIN_PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv;
  COIN_PFNGLISFRAMEBUFFERPROC glIsFramebuffer;
  COIN_PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
  COIN_PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
  COIN_PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
  COIN_PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
  COIN_PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D;
  COIN_PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
  COIN_PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D;
  COIN_PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
  COIN_PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv;
  COIN_PFNGLGENERATEMIPMAPPROC glGenerateMipmap;

  /* glGetStringi - part of replacement for obsolete glGetString(GL_EXTENSIONS) in OpenGL 3.0 */
  COIN_PFNGLGETSTRINGIPROC glGetStringi;

  const char * versionstr;
  const char * vendorstr;
  SbBool vendor_is_SGI;
  SbBool vendor_is_nvidia;
  SbBool vendor_is_intel;
  SbBool vendor_is_ati;
  SbBool vendor_is_3dlabs;
  SbBool nvidia_color_per_face_bug;
  const char * rendererstr;
  const char * extensionsstr;
  int maxtextureunits;
  struct cc_glxglue glx;
  float max_anisotropy;

  /* normalization cube map */
  GLuint normalizationcubemap;
  GLuint specularlookup;

  SbBool can_do_bumpmapping;
  SbBool can_do_sortedlayersblend;
  SbBool can_do_anisotropic_filtering;

  SbBool has_nv_register_combiners;
  SbBool has_ext_texture_rectangle;
  SbBool has_nv_texture_shader;
  SbBool has_depth_texture;
  SbBool has_shadow;
  SbBool has_arb_fragment_program;
  SbBool has_arb_vertex_program;
  SbBool has_arb_shader_objects;
  SbBool has_arb_vertex_shader;
  SbBool has_texture_env_combine;
  SbBool has_fbo;

  SbBool vbo_in_displaylist_ok;
  SbBool non_power_of_two_textures;
  int max_lights;
  float line_width_range[2];
  float point_size_range[2];
  int max_texture_size;
  cc_dict * glextdict;

  cc_libhandle dl_handle;
};

/* ********************************************************************** */

/* Called from SoContextHandler::destructingContext() to be able
   to deallocated the cc_glglue instance. */

void coin_glglue_destruct(uint32_t contextid);

/* ********************************************************************** */

/* Primarily used internally from functions that are badly designed,
   lacking a cc_glglue* argument in the function signature.

   Note: you should try to avoid using this function if possible! */

void * coin_gl_current_context(void);

/* ********************************************************************** */

/*
 * Needed for a hack in SoVertexShader and SoFramgmentShader
 * Will be removed soon.
 */
const cc_glglue * cc_glglue_instance_from_context_ptr(void * ptr);

/* ********************************************************************** */

/* Scanning for and printing info about current set of glGetError()s. */
unsigned int coin_catch_gl_errors(cc_string *);
/* Convert OpenGL glGetError() error code to string. */
const char * coin_glerror_string(GLenum errorcode);

/* ********************************************************************** */

/* Exported internally to gl_glx.c / gl_wgl.c / gl_agl.c. */
int coin_glglue_debug(void);
int coin_glglue_extension_available(const char * extensions, const char * ext);

int coin_glglue_stencil_bits_hack(void);

/* ********************************************************************** */

/* needed for bumpmap rendering */
void coin_apply_normalization_cube_map(const cc_glglue * glue);

/* ********************************************************************** */

/* ARB_shader_objects */
SbBool cc_glglue_has_arb_shader_objects(const cc_glglue * glue);

/* Moved from gl.h and added compressed parameter.
   Original function is deprecated for internal use.
*/
SbBool coin_glglue_is_texture_size_legal(const cc_glglue * glw,
                                         int xsize, int ysize, int zsize,
                                         GLenum internalformat,
                                         GLenum format,
                                         GLenum type,
                                         SbBool mipmap);

GLint coin_glglue_get_internal_texture_format(const cc_glglue * glw,
                                              int numcomponents,
                                              SbBool compress);

GLenum coin_glglue_get_texture_format(const cc_glglue * glw, int numcomponents);
SbBool coin_glglue_vbo_in_displaylist_supported(const cc_glglue * glw);
SbBool coin_glglue_non_power_of_two_textures(const cc_glglue * glue);
SbBool coin_glglue_has_generate_mipmap(const cc_glglue * glue);

/* context creation callback */
typedef void coin_glglue_instance_created_cb(const uint32_t contextid, void * closure);
void coin_glglue_add_instance_created_callback(coin_glglue_instance_created_cb * cb,
                                               void * closure);

cc_libhandle coin_glglue_dl_handle(const cc_glglue * glw);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !COIN_GLUE_GLP_H */
