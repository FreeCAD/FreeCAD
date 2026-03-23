#ifndef COIN_GLUE_GL_H
#define COIN_GLUE_GL_H

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

/* Documentation for the cc_glglue OpenGL wrapper abstraction
   interface can be found at the top of the Coin/src/glue/gl.cpp source
   code file. */

/* ********************************************************************** */

#include <Inventor/system/gl.h>
#include <Inventor/C/basic.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if 0 /* to get proper auto-indentation in emacs */
}
#endif /* emacs indentation */

/* ********************************************************************** */

/* Pre-declare this. Actual definition of struct is hidden in
   implementation. Client code must treat structure as opaque. */
typedef struct cc_glglue cc_glglue;

/* ********************************************************************** */


/* Singleton functions for getting hold of cc_glglue instance for
   context. ***/

/*
  Returns the glue instance for the given context ID.

  The context ID can be any number chosen to match the current OpenGL
  context in a _unique_ manner (this is important!).

  (Note: internally in Coin, we use the context ids defined by
  SoGLCacheContextElement. Make sure context ids from external code
  doesn't crash with those.)
*/
COIN_DLL_API const cc_glglue * cc_glglue_instance(int contextid);

/*** General interface. ***********************************************/

/*
  Fetch version number information for the underlying OpenGL
  implementation.
*/
COIN_DLL_API void cc_glglue_glversion(const cc_glglue * glue,
                                      unsigned int * major,
                                      unsigned int * minor,
                                      unsigned int * release);

/*
  Returns TRUE if the OpenGL implementation of the wrapper context is
  at least as "late" as what is given with the input
  arguments. Otherwise returns FALSE.
*/
COIN_DLL_API SbBool cc_glglue_glversion_matches_at_least(const cc_glglue * glue,
                                                         unsigned int major,
                                                         unsigned int minor,
                                                         unsigned int release);

/*
  Returns TRUE if the GLX implementation of the wrapper context is at
  least as "late" as what is given with the input arguments. Otherwise
  returns FALSE.
*/
COIN_DLL_API SbBool cc_glglue_glxversion_matches_at_least(const cc_glglue * glue,
                                                          int major,
                                                          int minor);
/*
  Returns TRUE if the given extension is supported by this context,
  FALSE if not.
*/
COIN_DLL_API SbBool cc_glglue_glext_supported(const cc_glglue * glue, const char * extname);

/*
  Returns address of the symbol (usually a function) named by
  "symname".

  Note that you should also check that the extension(s) needed are
  properly defined before using the symbol, as a symbol can be present
  in a GL library without being implemented, or being partly
  implemented, or being implemented but not active for your particular
  hardware card (for "unified" drivers from vendors with many
  different types or generations of graphics cards).
 */
COIN_DLL_API void * cc_glglue_getprocaddress(const cc_glglue * glue, const char * symname);

/* Returns TRUE if rendering is done directly on the display (ie not
   through any software indirection layer over GLX). */
COIN_DLL_API SbBool cc_glglue_isdirect(const cc_glglue * w);


/*** Wrapped OpenGL 1.1+ features and extensions. *********************/

/* Z-buffer offsetting ***/

COIN_DLL_API SbBool cc_glglue_has_polygon_offset(const cc_glglue * glue);
/* Bitflags for the last argument of cc_glglue_glPolygonOffsetEnable(). */
enum cc_glglue_Primitives { cc_glglue_FILLED = 1 << 0,
                            cc_glglue_LINES  = 1 << 1,
                            cc_glglue_POINTS = 1 << 2 };
COIN_DLL_API void cc_glglue_glPolygonOffsetEnable(const cc_glglue * glue,
                                                  SbBool enable, int m);
COIN_DLL_API void cc_glglue_glPolygonOffset(const cc_glglue * glue,
                                            GLfloat factor,
                                            GLfloat units);

/* Texture objects ***/

COIN_DLL_API SbBool cc_glglue_has_texture_objects(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glGenTextures(const cc_glglue * glue,
                                          GLsizei n,
                                          GLuint *textures);
COIN_DLL_API void cc_glglue_glBindTexture(const cc_glglue * glue,
                                          GLenum target,
                                          GLuint texture);
COIN_DLL_API void cc_glglue_glDeleteTextures(const cc_glglue * glue,
                                             GLsizei n,
                                             const GLuint * textures);

/* 3D textures ***/

COIN_DLL_API SbBool cc_glglue_has_3d_textures(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glTexImage3D(const cc_glglue * glue,
                                         GLenum target,
                                         GLint level,
                                         GLenum internalformat,
                                         GLsizei width,
                                         GLsizei height,
                                         GLsizei depth,
                                         GLint border,
                                         GLenum format,
                                         GLenum type,
                                         const GLvoid *pixels);
COIN_DLL_API void cc_glglue_glTexSubImage3D(const cc_glglue * glue,
                                            GLenum target,
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
COIN_DLL_API void cc_glglue_glCopyTexSubImage3D(const cc_glglue * glue,
                                                GLenum target,
                                                GLint level,
                                                GLint xoffset,
                                                GLint yoffset,
                                                GLint zoffset,
                                                GLint x,
                                                GLint y,
                                                GLsizei width,
                                                GLsizei height);

/* Multi-texturing ***/

COIN_DLL_API SbBool cc_glglue_has_multitexture(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glMultiTexCoord2f(const cc_glglue * glue,
                                              GLenum target,
                                              GLfloat s,
                                              GLfloat t);
COIN_DLL_API void cc_glglue_glMultiTexCoord2fv(const cc_glglue * glue,
                                               GLenum target,
                                               const GLfloat * v);
COIN_DLL_API void cc_glglue_glMultiTexCoord3fv(const cc_glglue * glue,
                                               GLenum target,
                                               const GLfloat * v);
COIN_DLL_API void cc_glglue_glMultiTexCoord4fv(const cc_glglue * glue,
                                               GLenum target,
                                               const GLfloat * v);

COIN_DLL_API void cc_glglue_glActiveTexture(const cc_glglue * glue,
                                            GLenum texture);
COIN_DLL_API void cc_glglue_glClientActiveTexture(const cc_glglue * glue,
                                                  GLenum texture);

/* Sub-texture operations ***/

COIN_DLL_API SbBool cc_glglue_has_texsubimage(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glTexSubImage2D(const cc_glglue * glue,
                                            GLenum target,
                                            GLint level,
                                            GLint xoffset,
                                            GLint yoffset,
                                            GLsizei width,
                                            GLsizei height,
                                            GLenum format,
                                            GLenum type,
                                            const GLvoid * pixels);

/* Misc texture operations ***/

COIN_DLL_API SbBool cc_glglue_has_2d_proxy_textures(const cc_glglue * glue);

COIN_DLL_API SbBool cc_glglue_has_texture_edge_clamp(const cc_glglue * glue);

COIN_DLL_API void cc_glglue_glPushClientAttrib(const cc_glglue * glue, GLbitfield mask);
COIN_DLL_API void cc_glglue_glPopClientAttrib(const cc_glglue * glue);


/* Texture compression ***/

COIN_DLL_API SbBool cc_glue_has_texture_compression(const cc_glglue * glue);

COIN_DLL_API void cc_glglue_glCompressedTexImage3D(const cc_glglue * glue,
                                                   GLenum target, 
                                                   GLint level, 
                                                   GLenum internalformat, 
                                                   GLsizei width, 
                                                   GLsizei height, 
                                                   GLsizei depth, 
                                                   GLint border, 
                                                   GLsizei imageSize, 
                                                   const GLvoid * data);
COIN_DLL_API void cc_glglue_glCompressedTexImage2D(const cc_glglue * glue,
                                                   GLenum target, 
                                                   GLint level, 
                                                   GLenum internalformat, 
                                                   GLsizei width, 
                                                   GLsizei height, 
                                                   GLint border, 
                                                   GLsizei imageSize, 
                                                   const GLvoid *data);
COIN_DLL_API void cc_glglue_glCompressedTexImage1D(const cc_glglue * glue,
                                                   GLenum target, 
                                                   GLint level, 
                                                   GLenum internalformat, 
                                                   GLsizei width, 
                                                   GLint border, 
                                                   GLsizei imageSize, 
                                                   const GLvoid *data);
COIN_DLL_API void cc_glglue_glCompressedTexSubImage3D(const cc_glglue * glue,
                                                      GLenum target, 
                                                      GLint level, 
                                                      GLint xoffset, 
                                                      GLint yoffset, 
                                                      GLint zoffset, 
                                                      GLsizei width, 
                                                      GLsizei height, 
                                                      GLsizei depth, 
                                                      GLenum format, 
                                                      GLsizei imageSize, 
                                                      const GLvoid *data);
COIN_DLL_API void cc_glglue_glCompressedTexSubImage2D(const cc_glglue * glue,
                                                      GLenum target, 
                                                      GLint level, 
                                                      GLint xoffset, 
                                                      GLint yoffset, 
                                                      GLsizei width, 
                                                      GLsizei height, 
                                                      GLenum format, 
                                                      GLsizei imageSize, 
                                                      const GLvoid *data);
COIN_DLL_API void cc_glglue_glCompressedTexSubImage1D(const cc_glglue * glue,
                                                      GLenum target, 
                                                      GLint level, 
                                                      GLint xoffset, 
                                                      GLsizei width, 
                                                      GLenum format, 
                                                      GLsizei imageSize, 
                                                      const GLvoid *data);
COIN_DLL_API void cc_glglue_glGetCompressedTexImage(const cc_glglue * glue,
                                                    GLenum target, 
                                                    GLint level, 
                                                    void *img);


/* Palette textures ***/

COIN_DLL_API SbBool cc_glglue_has_color_tables(const cc_glglue * glue);
COIN_DLL_API SbBool cc_glglue_has_color_subtables(const cc_glglue * glue);
/* TRUE from the next check also guarantees that the two color table
   checks above returns TRUE. */
COIN_DLL_API SbBool cc_glglue_has_paletted_textures(const cc_glglue * glue);

COIN_DLL_API void cc_glglue_glColorTable(const cc_glglue * glue,
                                         GLenum target, 
                                         GLenum internalFormat, 
                                         GLsizei width, 
                                         GLenum format, 
                                         GLenum type, 
                                         const GLvoid *table);
COIN_DLL_API void cc_glglue_glColorSubTable(const cc_glglue * glue,
                                            GLenum target,
                                            GLsizei start,
                                            GLsizei count,
                                            GLenum format,
                                            GLenum type,
                                            const GLvoid * data);
COIN_DLL_API void cc_glglue_glGetColorTable(const cc_glglue * glue,
                                            GLenum target, 
                                            GLenum format, 
                                            GLenum type, 
                                            GLvoid *data);
COIN_DLL_API void cc_glglue_glGetColorTableParameteriv(const cc_glglue * glue,
                                                       GLenum target, 
                                                       GLenum pname, 
                                                       GLint *params);
COIN_DLL_API void cc_glglue_glGetColorTableParameterfv(const cc_glglue * glue,
                                                       GLenum target, 
                                                       GLenum pname, 
                                                       GLfloat *params);


/* Texture blending settings ***/

COIN_DLL_API SbBool cc_glglue_has_blendequation(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glBlendEquation(const cc_glglue * glue, GLenum mode);

/* Texture blend separate */
COIN_DLL_API SbBool cc_glglue_has_blendfuncseparate(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glBlendFuncSeparate(const cc_glglue * glue, 
                                                GLenum srgb, GLenum drgb,
                                                GLenum salpha, GLenum dalpha);

/* OpenGL vertex array ***/

COIN_DLL_API SbBool cc_glglue_has_vertex_array(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glVertexPointer(const cc_glglue * glue,
                                            GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
COIN_DLL_API void cc_glglue_glTexCoordPointer(const cc_glglue * glue,
                                              GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
COIN_DLL_API void cc_glglue_glNormalPointer(const cc_glglue * glue,
                                            GLenum type, GLsizei stride, const GLvoid *pointer);
COIN_DLL_API void cc_glglue_glColorPointer(const cc_glglue * glue,
                                           GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
COIN_DLL_API void cc_glglue_glIndexPointer (const cc_glglue * glue,
                                            GLenum type, GLsizei stride, const GLvoid * pointer);
COIN_DLL_API void cc_glglue_glEnableClientState(const cc_glglue * glue, GLenum array);
COIN_DLL_API void cc_glglue_glDisableClientState(const cc_glglue * glue, GLenum array);
COIN_DLL_API void cc_glglue_glInterleavedArrays(const cc_glglue * glue, 
                                                GLenum format, GLsizei stride, const GLvoid * pointer);
COIN_DLL_API void cc_glglue_glDrawArrays(const cc_glglue * glue, 
                                         GLenum mode, GLint first, GLsizei count);
COIN_DLL_API void cc_glglue_glDrawElements(const cc_glglue * glue, 
                                           GLenum mode, GLsizei count, GLenum type, const GLvoid * indices);
COIN_DLL_API void cc_glglue_glDrawRangeElements(const cc_glglue * glue, 
                                                GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices);
COIN_DLL_API void cc_glglue_glArrayElement(const cc_glglue * glue, GLint i);

COIN_DLL_API int cc_glglue_max_texture_units(const cc_glglue * glue);
COIN_DLL_API int cc_glglue_has_multidraw_vertex_arrays(const cc_glglue * glue);

COIN_DLL_API void cc_glglue_glMultiDrawArrays(const cc_glglue * glue, GLenum mode, const GLint * first, 
                                              const GLsizei * count, GLsizei primcount);
COIN_DLL_API void cc_glglue_glMultiDrawElements(const cc_glglue * glue, GLenum mode, const GLsizei * count, 
                                                GLenum type, const GLvoid ** indices, GLsizei primcount);

/* NV_vertex_array_range */
COIN_DLL_API SbBool cc_glglue_has_nv_vertex_array_range(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glFlushVertexArrayRangeNV(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glVertexArrayRangeNV(const cc_glglue * glue, GLsizei size, const GLvoid * pointer);
COIN_DLL_API void * cc_glglue_glAllocateMemoryNV(const cc_glglue * glue,
                                                 GLsizei size, GLfloat readfreq,
                                                 GLfloat writefreq, GLfloat priority);
COIN_DLL_API void cc_glglue_glFreeMemoryNV(const cc_glglue * glue, GLvoid * buffer);

/* ARB_vertex_buffer_object */
COIN_DLL_API SbBool cc_glglue_has_vertex_buffer_object(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glBindBuffer(const cc_glglue * glue, GLenum target, GLuint buffer);
COIN_DLL_API void cc_glglue_glDeleteBuffers(const cc_glglue * glue, GLsizei n, const GLuint *buffers);
COIN_DLL_API void cc_glglue_glGenBuffers(const cc_glglue * glue, GLsizei n, GLuint *buffers);
COIN_DLL_API GLboolean cc_glglue_glIsBuffer(const cc_glglue * glue, GLuint buffer);
COIN_DLL_API void cc_glglue_glBufferData(const cc_glglue * glue,
                                         GLenum target, 
                                         intptr_t size, /* 64 bit on 64 bit systems */ 
                                         const GLvoid *data, 
                                         GLenum usage);
COIN_DLL_API void cc_glglue_glBufferSubData(const cc_glglue * glue,
                                            GLenum target, 
                                            intptr_t offset, /* 64 bit */ 
                                            intptr_t size, /* 64 bit */ 
                                            const GLvoid * data);
COIN_DLL_API void cc_glglue_glGetBufferSubData(const cc_glglue * glue,
                                               GLenum target, 
                                               intptr_t offset, /* 64 bit */ 
                                               intptr_t size, /* 64 bit */ 
                                               GLvoid *data);
COIN_DLL_API GLvoid * cc_glglue_glMapBuffer(const cc_glglue * glue,
                                            GLenum target, GLenum access);
COIN_DLL_API GLboolean cc_glglue_glUnmapBuffer(const cc_glglue * glue,
                                               GLenum target);
COIN_DLL_API void cc_glglue_glGetBufferParameteriv(const cc_glglue * glue,
                                                   GLenum target, 
                                                   GLenum pname, 
                                                   GLint * params);
COIN_DLL_API void cc_glglue_glGetBufferPointerv(const cc_glglue * glue,
                                                GLenum target, 
                                                GLenum pname, 
                                                GLvoid ** params);

/* GL_ARB_fragment_program */
COIN_DLL_API SbBool cc_glglue_has_arb_fragment_program(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glProgramString(const cc_glglue * glue, GLenum target, GLenum format, 
                                            GLsizei len, const GLvoid *string);
COIN_DLL_API void cc_glglue_glBindProgram(const cc_glglue * glue, GLenum target, 
                                          GLuint program);
COIN_DLL_API void cc_glglue_glDeletePrograms(const cc_glglue * glue, GLsizei n, 
                                             const GLuint *programs);
COIN_DLL_API void cc_glglue_glGenPrograms(const cc_glglue * glue, GLsizei n, GLuint *programs);
COIN_DLL_API void cc_glglue_glProgramEnvParameter4d(const cc_glglue * glue, GLenum target,
                                                    GLuint index, GLdouble x, GLdouble y, 
                                                    GLdouble z, GLdouble w);
COIN_DLL_API void cc_glglue_glProgramEnvParameter4dv(const cc_glglue * glue, GLenum target,
                                                     GLuint index, const GLdouble *params);
COIN_DLL_API void cc_glglue_glProgramEnvParameter4f(const cc_glglue * glue, GLenum target, 
                                                    GLuint index, GLfloat x, 
                                                    GLfloat y, GLfloat z, 
                                                    GLfloat w);
COIN_DLL_API void cc_glglue_glProgramEnvParameter4fv(const cc_glglue * glue, GLenum target, 
                                                     GLuint index, const GLfloat *params);
COIN_DLL_API void cc_glglue_glProgramLocalParameter4d(const cc_glglue * glue, GLenum target, 
                                                      GLuint index, GLdouble x, 
                                                      GLdouble y, GLdouble z, 
                                                      GLdouble w);
COIN_DLL_API void cc_glglue_glProgramLocalParameter4dv(const cc_glglue * glue, GLenum target, 
                                                       GLuint index, const GLdouble *params);
COIN_DLL_API void cc_glglue_glProgramLocalParameter4f(const cc_glglue * glue, GLenum target, 
                                                      GLuint index, GLfloat x, GLfloat y, 
                                                      GLfloat z, GLfloat w);
COIN_DLL_API void cc_glglue_glProgramLocalParameter4fv(const cc_glglue * glue, GLenum target, 
                                                       GLuint index, const GLfloat *params);
COIN_DLL_API void cc_glglue_glGetProgramEnvParameterdv(const cc_glglue * glue, GLenum target, 
                                                       GLuint index, GLdouble *params);
COIN_DLL_API void cc_glglue_glGetProgramEnvParameterfv(const cc_glglue * glue, GLenum target, 
                                                       GLuint index, GLfloat *params);
COIN_DLL_API void cc_glglue_glGetProgramLocalParameterdv(const cc_glglue * glue, GLenum target, 
                                                         GLuint index, GLdouble *params);
COIN_DLL_API void cc_glglue_glGetProgramLocalParameterfv(const cc_glglue * glue, GLenum target, 
                                                         GLuint index, GLfloat *params);
COIN_DLL_API void cc_glglue_glGetProgramiv(const cc_glglue * glue, GLenum target, 
                                           GLenum pname, GLint *params);
COIN_DLL_API void cc_glglue_glGetProgramString(const cc_glglue * glue, GLenum target, 
                                               GLenum pname, GLvoid *string);
COIN_DLL_API SbBool cc_glglue_glIsProgram(const cc_glglue * glue, GLuint program);

/* ARB_vertex_program */
COIN_DLL_API SbBool cc_glglue_has_arb_vertex_program(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glVertexAttrib1s(const cc_glglue * glue, GLuint index, GLshort x);
COIN_DLL_API void cc_glglue_glVertexAttrib1f(const cc_glglue * glue, GLuint index, GLfloat x);
COIN_DLL_API void cc_glglue_glVertexAttrib1d(const cc_glglue * glue, GLuint index, GLdouble x);
COIN_DLL_API void cc_glglue_glVertexAttrib2s(const cc_glglue * glue, GLuint index, GLshort x, GLshort y);
COIN_DLL_API void cc_glglue_glVertexAttrib2f(const cc_glglue * glue, GLuint index, GLfloat x, GLfloat y);
COIN_DLL_API void cc_glglue_glVertexAttrib2d(const cc_glglue * glue, GLuint index, GLdouble x, GLdouble y);
COIN_DLL_API void cc_glglue_glVertexAttrib3s(const cc_glglue * glue, GLuint index, 
                                             GLshort x, GLshort y, GLshort z);
COIN_DLL_API void cc_glglue_glVertexAttrib3f(const cc_glglue * glue, GLuint index, 
                                             GLfloat x, GLfloat y, GLfloat z);
COIN_DLL_API void cc_glglue_glVertexAttrib3d(const cc_glglue * glue, GLuint index, 
                                             GLdouble x, GLdouble y, GLdouble z);
COIN_DLL_API void cc_glglue_glVertexAttrib4s(const cc_glglue * glue, GLuint index, GLshort x, 
                                             GLshort y, GLshort z, GLshort w);
COIN_DLL_API void cc_glglue_glVertexAttrib4f(const cc_glglue * glue, GLuint index, GLfloat x, 
                                             GLfloat y, GLfloat z, GLfloat w);
COIN_DLL_API void cc_glglue_glVertexAttrib4d(const cc_glglue * glue, GLuint index, GLdouble x, 
                                             GLdouble y, GLdouble z, GLdouble w);
COIN_DLL_API void cc_glglue_glVertexAttrib4Nub(const cc_glglue * glue, GLuint index, GLubyte x, 
                                               GLubyte y, GLubyte z, GLubyte w);
COIN_DLL_API void cc_glglue_glVertexAttrib1sv(const cc_glglue * glue, GLuint index, const GLshort *v);
COIN_DLL_API void cc_glglue_glVertexAttrib1fv(const cc_glglue * glue, GLuint index, const GLfloat *v);
COIN_DLL_API void cc_glglue_glVertexAttrib1dv(const cc_glglue * glue, GLuint index, const GLdouble *v);
COIN_DLL_API void cc_glglue_glVertexAttrib2sv(const cc_glglue * glue, GLuint index, const GLshort *v);
COIN_DLL_API void cc_glglue_glVertexAttrib2fv(const cc_glglue * glue, GLuint index, const GLfloat *v);
COIN_DLL_API void cc_glglue_glVertexAttrib2dv(const cc_glglue * glue, GLuint index, const GLdouble *v);
COIN_DLL_API void cc_glglue_glVertexAttrib3sv(const cc_glglue * glue, GLuint index, const GLshort *v);
COIN_DLL_API void cc_glglue_glVertexAttrib3fv(const cc_glglue * glue, GLuint index, const GLfloat *v);
COIN_DLL_API void cc_glglue_glVertexAttrib3dv(const cc_glglue * glue, GLuint index, const GLdouble *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4bv(const cc_glglue * glue, GLuint index, const GLbyte *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4sv(const cc_glglue * glue, GLuint index, const GLshort *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4iv(const cc_glglue * glue, GLuint index, const GLint *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4ubv(const cc_glglue * glue, GLuint index, const GLubyte *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4usv(const cc_glglue * glue, GLuint index, const GLushort *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4uiv(const cc_glglue * glue, GLuint index, const GLuint *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4fv(const cc_glglue * glue, GLuint index, const GLfloat *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4dv(const cc_glglue * glue, GLuint index, const GLdouble *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4Nbv(const cc_glglue * glue, GLuint index, const GLbyte *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4Nsv(const cc_glglue * glue, GLuint index, const GLshort *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4Niv(const cc_glglue * glue, GLuint index, const GLint *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4Nubv(const cc_glglue * glue, GLuint index, const GLubyte *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4Nusv(const cc_glglue * glue, GLuint index, const GLushort *v);
COIN_DLL_API void cc_glglue_glVertexAttrib4Nuiv(const cc_glglue * glue, GLuint index, const GLuint *v);
COIN_DLL_API void cc_glglue_glVertexAttribPointer(const cc_glglue * glue, GLuint index, GLint size, 
                                                  GLenum type, GLboolean normalized, GLsizei stride, 
                                                  const GLvoid *pointer);
COIN_DLL_API void cc_glglue_glEnableVertexAttribArray(const cc_glglue * glue, GLuint index);
COIN_DLL_API void cc_glglue_glDisableVertexAttribArray(const cc_glglue * glue, GLuint index);
COIN_DLL_API void cc_glglue_glGetVertexAttribdv(const cc_glglue * glue, GLuint index, GLenum pname, 
                                                GLdouble *params);
COIN_DLL_API void cc_glglue_glGetVertexAttribfv(const cc_glglue * glue, GLuint index, GLenum pname, 
                                                GLfloat *params);
COIN_DLL_API void cc_glglue_glGetVertexAttribiv(const cc_glglue * glue, GLuint index, GLenum pname, 
                                                GLint *params);
COIN_DLL_API void cc_glglue_glGetVertexAttribPointerv(const cc_glglue * glue, GLuint index, GLenum pname, 
                                                      GLvoid **pointer);

/* ARB_vertex_shader */
COIN_DLL_API SbBool cc_glglue_has_arb_vertex_shader(const cc_glglue * glue);

/* ARB_occlusion_query */
COIN_DLL_API SbBool cc_glglue_has_occlusion_query(const cc_glglue * glue);
COIN_DLL_API void cc_glglue_glGenQueries(const cc_glglue * glue, 
                                         GLsizei n, GLuint * ids);
COIN_DLL_API void cc_glglue_glDeleteQueries(const cc_glglue * glue, 
                                            GLsizei n, const GLuint *ids);
COIN_DLL_API GLboolean cc_glglue_glIsQuery(const cc_glglue * glue, 
                                         GLuint id);
COIN_DLL_API void cc_glglue_glBeginQuery(const cc_glglue * glue, 
                                         GLenum target, GLuint id);
COIN_DLL_API void cc_glglue_glEndQuery(const cc_glglue * glue, 
                                       GLenum target);
COIN_DLL_API void cc_glglue_glGetQueryiv(const cc_glglue * glue, 
                                         GLenum target, GLenum pname, 
                                         GLint * params);
COIN_DLL_API void cc_glglue_glGetQueryObjectiv(const cc_glglue * glue, 
                                               GLuint id, GLenum pname, 
                                               GLint * params);
COIN_DLL_API void cc_glglue_glGetQueryObjectuiv(const cc_glglue * glue, 
                                                GLuint id, GLenum pname, 
                                                GLuint * params);

/* framebuffer_object */
COIN_DLL_API void cc_glglue_glIsRenderbuffer(const cc_glglue * glue, GLuint renderbuffer);
COIN_DLL_API void cc_glglue_glBindRenderbuffer(const cc_glglue * glue, GLenum target, GLuint renderbuffer);
COIN_DLL_API void cc_glglue_glDeleteRenderbuffers(const cc_glglue * glue, GLsizei n, const GLuint *renderbuffers);
COIN_DLL_API void cc_glglue_glGenRenderbuffers(const cc_glglue * glue, GLsizei n, GLuint *renderbuffers);
COIN_DLL_API void cc_glglue_glRenderbufferStorage(const cc_glglue * glue, GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
COIN_DLL_API void cc_glglue_glGetRenderbufferParameteriv(const cc_glglue * glue, GLenum target, GLenum pname, GLint *params);
COIN_DLL_API GLboolean cc_glglue_glIsFramebuffer(const cc_glglue * glue, GLuint framebuffer);
COIN_DLL_API void cc_glglue_glBindFramebuffer(const cc_glglue * glue, GLenum target, GLuint framebuffer);
COIN_DLL_API void cc_glglue_glDeleteFramebuffers(const cc_glglue * glue, GLsizei n, const GLuint *framebuffers);
COIN_DLL_API void cc_glglue_glGenFramebuffers(const cc_glglue * glue, GLsizei n, GLuint *framebuffers);
COIN_DLL_API GLenum cc_glglue_glCheckFramebufferStatus(const cc_glglue * glue, GLenum target);
COIN_DLL_API void cc_glglue_glFramebufferTexture1D(const cc_glglue * glue, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
COIN_DLL_API void cc_glglue_glFramebufferTexture2D(const cc_glglue * glue, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
COIN_DLL_API void cc_glglue_glFramebufferTexture3D(const cc_glglue * glue, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
COIN_DLL_API void cc_glglue_glFramebufferRenderbuffer(const cc_glglue * glue, GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
COIN_DLL_API void cc_glglue_glGetFramebufferAttachmentParameteriv(const cc_glglue * glue, GLenum target, GLenum attachment, GLenum pname, GLint *params);
COIN_DLL_API void cc_glglue_glGenerateMipmap(const cc_glglue * glue, GLenum target);
COIN_DLL_API SbBool cc_glglue_has_framebuffer_objects(const cc_glglue * glue);


/* GL feature queries */
COIN_DLL_API SbBool cc_glglue_can_do_bumpmapping(const cc_glglue * glue);
COIN_DLL_API SbBool cc_glglue_can_do_sortedlayersblend(const cc_glglue * glue);
COIN_DLL_API SbBool cc_glglue_can_do_anisotropic_filtering(const cc_glglue * glue);
COIN_DLL_API SbBool cc_glglue_has_framebuffer_objects(const cc_glglue * glue);

/* GL limits */
COIN_DLL_API int cc_glglue_get_max_lights(const cc_glglue * glue);
COIN_DLL_API const float * cc_glglue_get_line_width_range(const cc_glglue * glue);
COIN_DLL_API const float * cc_glglue_get_point_size_range(const cc_glglue * glue);

COIN_DLL_API float cc_glglue_get_max_anisotropy(const cc_glglue * glue);

/* ********************************************************************** */

/* GLX extensions ***/

COIN_DLL_API void * cc_glglue_glXGetCurrentDisplay(const cc_glglue * w);


/* ********************************************************************** */

/* Offscreen buffer creation ***/

/* Functions to make and handle offscreen contexts. The interface is a
   common interface which provides an abstraction over the
   system-specific implementations on GLX, WGL and AGL.

   It also hides whether or not hardware-acceleration is employed
   (which is automatically done if the driver is found capable of
   doing that.

   Note that these does not need a cc_glglue instance.
*/

COIN_DLL_API void cc_glglue_context_max_dimensions(unsigned int * width, unsigned int * height);

COIN_DLL_API void * cc_glglue_context_create_offscreen(unsigned int width, unsigned int height);
COIN_DLL_API SbBool cc_glglue_context_make_current(void * ctx);
COIN_DLL_API void cc_glglue_context_reinstate_previous(void * ctx);
COIN_DLL_API void cc_glglue_context_destruct(void * ctx);

COIN_DLL_API void cc_glglue_context_bind_pbuffer(void * ctx);
COIN_DLL_API void cc_glglue_context_release_pbuffer(void * ctx);
COIN_DLL_API SbBool cc_glglue_context_pbuffer_is_bound(void * ctx);
COIN_DLL_API SbBool cc_glglue_context_can_render_to_texture(void * ctx);

/* This abomination is needed to support SoOffscreenRenderer::getDC(). */
COIN_DLL_API const void * cc_glglue_win32_HDC(void * ctx);
COIN_DLL_API void cc_glglue_win32_updateHDCBitmap(void * ctx);

/* ********************************************************************** */

/* Interface for setting external offscreen renderer functionality:
 * Makes it possible to provide an external implementation for doing
 * offscreen rendering (such as accelerated rendering into a hidden
 * window). This is useful to avoid having to do slow software
 * rendering when pBuffers cannot be used (not available on the
 * system, buggy implementation, mismatch of onscreen context and
 * offscreen context).
 */

typedef void * cc_glglue_offscreen_data;

typedef struct cc_glglue_offscreen_cb_functions {
    cc_glglue_offscreen_data (*create_offscreen)(unsigned int width, unsigned int height);
    SbBool (*make_current)(cc_glglue_offscreen_data context);
    void (*reinstate_previous)(cc_glglue_offscreen_data context);
    void (*destruct)(cc_glglue_offscreen_data context);
} cc_glglue_offscreen_cb_functions; 

/* Set callback functions for external offscreen rendering. Pass NULL 
   to restore default, built-in offscreen rendering. 
 */
COIN_DLL_API void cc_glglue_context_set_offscreen_cb_functions(cc_glglue_offscreen_cb_functions* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !COIN_GLUE_GL_H */
