#ifndef __openglwrapper_h__
#define __openglwrapper_h__
#ifdef HAVE_OPENGL_EXT
#include "DlgCAMSimulator.h"
extern QOpenGLContext* gOpenGlContext;
extern CAMSimulator::OpenGLWindow* gWindow;
#define glGenBuffers gWindow->glGenBuffers
#define glBindBuffer gWindow->glBindBuffer
#define glBufferData gWindow->glBufferData
#define glGenVertexArrays gWindow->glGenVertexArrays
#define glBindVertexArray gWindow->glBindVertexArray
#define glEnableVertexAttribArray gWindow->glEnableVertexAttribArray
#define glVertexAttribPointer gWindow->glVertexAttribPointer
#define glShaderSource gWindow->glShaderSource
#define glCompileShader gWindow->glCompileShader
#define glAttachShader gWindow->glAttachShader
#define glLinkProgram gWindow->glLinkProgram
#define glGetProgramiv gWindow->glGetProgramiv
#define glGetUniformLocation gWindow->glGetUniformLocation
#define glGetError gWindow->glGetError
#define glEnable gWindow->glEnable
#define glColorMask gWindow->glColorMask
#define glCullFace gWindow->glCullFace
#define glDepthFunc gWindow->glDepthFunc
#define glStencilFunc gWindow->glStencilFunc
#define glStencilOp gWindow->glStencilOp
#define glDepthMask gWindow->glDepthMask
#define glDisable gWindow->glDisable
#define glMatrixMode gWindow->glMatrixMode
#define glUseProgram gWindow->glUseProgram
#define glDrawElements gWindow->glDrawElements
#define glDeleteVertexArrays gWindow->glDeleteVertexArrays
#define glUniformMatrix4fv gWindow->glUniformMatrix4fv
#define glUniform3fv gWindow->glUniform3fv
#define glUniform1i gWindow->glUniform1i
#define glCreateShader gWindow->glCreateShader
#define glCreateProgram gWindow->glCreateProgram
#define glDeleteBuffers gWindow->glDeleteBuffers
#define glActiveTexture gWindow->glActiveTexture
#define glBindTexture gWindow->glBindTexture
#define glGenTextures gWindow->glGenTextures
#define glTexParameteri gWindow->glTexParameteri
#define glTexImage2D gWindow->glTexImage2D
#define glDeleteTextures gWindow->glDeleteTextures
#else
#include "GL/glew.h"
#endif // HAVE_OPENGL_EXT

#endif // !__openglwrapper_h__
