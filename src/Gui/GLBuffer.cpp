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

#ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
# include <OpenGL/glext.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
# include <GL/glext.h>
#endif

#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoContextHandler.h>

#include <string>

#include "GLBuffer.h"

using namespace Gui;


OpenGLBuffer::OpenGLBuffer(GLenum type)
  : target(type)
  , currentContext(-1)
  , currentBuf(0)
  , glue(0)
{
    SoContextHandler::addContextDestructionCallback(context_destruction_cb, this);
}

OpenGLBuffer::~OpenGLBuffer()
{
    SoContextHandler::removeContextDestructionCallback(context_destruction_cb, this);

    destroy();
}

/*!
 * \brief OpenGLBuffer::isVBOSupported returns if the OpenGL driver
 * supports the VBO extension.
 * When calling this function there must be a current OpenGL context.
 * \return
 */
bool OpenGLBuffer::isVBOSupported(uint32_t ctx)
{
    auto glue = cc_glglue_instance(ctx);
    if (!glue || !cc_glglue_has_vertex_buffer_object(glue))
        return false;
    const GLubyte * str = glGetString(GL_EXTENSIONS);
    if (!str)
        return false;
    std::string ext = reinterpret_cast<const char*>(str);
    return (ext.find("GL_ARB_vertex_buffer_object") != std::string::npos);
}

void OpenGLBuffer::setCurrentContext(uint32_t ctx)
{
    currentContext = ctx;
    glue = cc_glglue_instance(currentContext);
    currentBuf = &bufs[ctx];
}

bool OpenGLBuffer::create()
{
    if(!currentBuf)
        return false;

    auto &bufferId = *currentBuf;
    if (bufferId > 0)
        return true;

    if (!cc_glglue_has_vertex_buffer_object(glue))
        return false;

    cc_glglue_glGenBuffers(glue, 1, &bufferId);
    return true;
}

bool OpenGLBuffer::isCreated(uint32_t context) const
{
    auto it = bufs.find(context);
    return it!=bufs.end() && it->second;
}

void OpenGLBuffer::destroy()
{
    // schedule delete for all allocated GL resources
    for(auto &v : bufs) {
        if(v.second) {
            void * ptr0 = (void*) ((uintptr_t) v.second);
            SoGLCacheContextElement::scheduleDeleteCallback(v.first, buffer_delete, ptr0);
        }
    }
    bufs.clear();
    currentBuf = 0;
}

void OpenGLBuffer::allocate(const void *data, int count)
{
    if (currentBuf && *currentBuf) {
        cc_glglue_glBufferData(glue, target, count, data, GL_STATIC_DRAW);
    }
}

bool OpenGLBuffer::bind()
{
    if (currentBuf && *currentBuf) {
        cc_glglue_glBindBuffer(glue, target, *currentBuf);
        return true;
    }
    return false;
}

void OpenGLBuffer::release()
{
    if (currentBuf && *currentBuf) {
        cc_glglue_glBindBuffer(glue, target, 0);
    }
}

GLuint OpenGLBuffer::getBufferId() const
{
    return currentBuf?*currentBuf:0;
}

uint32_t OpenGLBuffer::getBoundContext() const
{
    return currentContext;
}

int OpenGLBuffer::size() const
{
    GLint value = -1;
    if (currentBuf && *currentBuf) {
        cc_glglue_glGetBufferParameteriv(glue, target, GL_BUFFER_SIZE, &value);
    }
    return value;
}

void OpenGLBuffer::context_destruction_cb(uint32_t context, void * userdata)
{
    OpenGLBuffer * self = static_cast<OpenGLBuffer*>(userdata);

    auto it = self->bufs.find(context);
    if(it != self->bufs.end() && it->second) {
        const cc_glglue * glue = cc_glglue_instance((int) context);
        GLuint buffer = it->second;
        cc_glglue_glDeleteBuffers(glue, 1, &buffer);
        if(self->currentBuf == &it->second)
            self->currentBuf = 0;
        self->bufs.erase(it);
    }
}

void OpenGLBuffer::buffer_delete(void * closure, uint32_t contextid)
{
    const cc_glglue * glue = cc_glglue_instance((int) contextid);
    GLuint id = (GLuint) ((uintptr_t) closure);
    cc_glglue_glDeleteBuffers(glue, 1, &id);
}
