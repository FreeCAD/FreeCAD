/***************************************************************************
 *   Copyright (c) 2018 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef FC_OS_WIN32
#define GL_GLEXT_PROTOTYPES
#endif

#ifndef _PreComp_
# ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
# include <OpenGL/glext.h>
# else
# include <GL/gl.h>
# include <GL/glu.h>
# include <GL/glext.h>
# endif
#endif

#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoContextHandler.h>

#include "GLBuffer.h"

using namespace Gui;


OpenGLBuffer::OpenGLBuffer(GLenum type)
  : target(type)
  , bufferId(0)
  , context(-1)
  , currentContext(-1)
  , glue(0)
{
    SoContextHandler::addContextDestructionCallback(context_destruction_cb, this);
}

OpenGLBuffer::~OpenGLBuffer()
{
    SoContextHandler::removeContextDestructionCallback(context_destruction_cb, this);

    destroy();
}

void OpenGLBuffer::setCurrentContext(uint32_t ctx)
{
    currentContext = ctx;
    glue = cc_glglue_instance(currentContext);
}

bool OpenGLBuffer::create()
{
    if (bufferId > 0)
        return true;
#ifdef FC_OS_WIN32
    PFNGLGENBUFFERSPROC glGenBuffersARB = (PFNGLGENBUFFERSPROC)cc_glglue_getprocaddress(glue, "glGenBuffersARB");
#endif
    glGenBuffersARB(1, &bufferId);
    context = currentContext;
    return true;
}

bool OpenGLBuffer::isCreated() const
{
    return (bufferId > 0);
}

void OpenGLBuffer::destroy()
{
    // schedule delete for all allocated GL resources
    if (bufferId > 0) {
        void * ptr0 = (void*) ((uintptr_t) bufferId);
        SoGLCacheContextElement::scheduleDeleteCallback(context, buffer_delete, ptr0);
        bufferId = 0;
    }
}

void OpenGLBuffer::allocate(const void *data, int count)
{
    if (bufferId > 0) {
#ifdef FC_OS_WIN32
        PFNGLBUFFERDATAPROC glBufferDataARB = (PFNGLBUFFERDATAPROC)cc_glglue_getprocaddress(glue, "glBufferDataARB");
#endif
        glBufferDataARB(target, count, data, GL_STATIC_DRAW);
    }
}

bool OpenGLBuffer::bind()
{
    if (bufferId) {
        if (context != currentContext) {
            SoDebugError::postWarning("OpenGLBuffer::bind",
                                      "buffer not created");
            return false;
        }
#ifdef FC_OS_WIN32
        PFNGLBINDBUFFERPROC glBindBufferARB = (PFNGLBINDBUFFERPROC)cc_glglue_getprocaddress(glue, "glBindBufferARB");
#endif
        glBindBufferARB(target, bufferId);
        return true;
    }
    return false;
}

void OpenGLBuffer::release()
{
    if (bufferId) {
#ifdef FC_OS_WIN32
        PFNGLBINDBUFFERPROC glBindBufferARB = (PFNGLBINDBUFFERPROC)cc_glglue_getprocaddress(glue, "glBindBufferARB");
#endif
        glBindBufferARB(target, 0);
    }
}

GLuint OpenGLBuffer::getBufferId() const
{
    return bufferId;
}

uint32_t OpenGLBuffer::getBoundContext() const
{
    return context;
}

int OpenGLBuffer::size() const
{
    GLint value = -1;
    if (bufferId > 0) {
#ifdef FC_OS_WIN32
        PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)cc_glglue_getprocaddress(glue, "glGetBufferParameterivARB");
#endif
        glGetBufferParameteriv(target, GL_BUFFER_SIZE, &value);
    }
    return value;
}

void OpenGLBuffer::context_destruction_cb(uint32_t context, void * userdata)
{
    OpenGLBuffer * self = static_cast<OpenGLBuffer*>(userdata);

    if (self->context == context && self->bufferId) {
#ifdef FC_OS_WIN32
        const cc_glglue * glue = cc_glglue_instance((int) context);
        PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)cc_glglue_getprocaddress(glue, "glDeleteBuffersARB");
#endif

        GLuint buffer = self->bufferId;
        glDeleteBuffersARB(1, &buffer);
        self->context = -1;
        self->bufferId = 0;
    }
}

void OpenGLBuffer::buffer_delete(void * closure, uint32_t contextid)
{
    const cc_glglue * glue = cc_glglue_instance((int) contextid);
    GLuint id = (GLuint) ((uintptr_t) closure);
    cc_glglue_glDeleteBuffers(glue, 1, &id);
}
