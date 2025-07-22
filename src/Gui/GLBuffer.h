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


#ifndef GUI_GLBUFFER_H
#define GUI_GLBUFFER_H

#include <FCGlobal.h>
#include <map>
#include <Inventor/C/glue/gl.h>

namespace Gui {

class GuiExport OpenGLBuffer
{
public:
    OpenGLBuffer(GLenum type);
    ~OpenGLBuffer();

    static bool isVBOSupported(uint32_t ctx);

    void setCurrentContext(uint32_t ctx);
    bool create();
    bool isCreated() const;

    void destroy();
    void allocate(const void *data, int count);
    bool bind();
    void release();
    GLuint getBufferId() const;
    uint32_t getBoundContext() const;
    int size() const;

private:
    static void context_destruction_cb(uint32_t context, void * userdata);
    static void buffer_delete(void * closure, uint32_t contextid);

    GLenum target;
    GLuint bufferId;
    uint32_t context;
    uint32_t currentContext;
    const cc_glglue* glue;
};

class GuiExport OpenGLMultiBuffer
{
public:
    OpenGLMultiBuffer(GLenum type);
    ~OpenGLMultiBuffer();

    void setCurrentContext(uint32_t ctx);
    bool create();
    bool isCreated(uint32_t ctx) const;

    void destroy();
    void allocate(const void *data, int count);
    bool bind();
    void release();
    GLuint getBufferId() const;
    int size() const;

private:
    static void context_destruction_cb(uint32_t context, void * userdata);
    static void buffer_delete(void * closure, uint32_t contextid);

    GLenum target;
    // map context to buffer id
    std::map<uint32_t, GLuint> bufs;
    GLuint *currentBuf;
    uint32_t currentContext;
    const cc_glglue* glue;
};

} // namespace Gui

#endif  // GUI_GLBUFFER_H
