/****************************************************************************
 *   Copyright (c) 2021 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#include "BGFXRenderer.h"

#ifndef HAVE_BGFX
BGFXRenderer::BGFXRenderer(QOpenGLWidget *)
{
}

BGFXRenderer::~BGFXRenderer()
{
}

bool BGFXRendere::render(const QColor &col,
                         const void * viewMatrix,
                         const void * projMatrix)
{
    (void)viewMatrix;
    (void)projMatrix;
    glClearColor(col.redF(), col.greenF(), col.blueF(), 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return false;
}
#else // HAVE_BGFX

#ifndef FC_OS_WIN32
# ifndef GL_GLEXT_PROTOTYPES
#   define GL_GLEXT_PROTOTYPES 1
# endif
#endif

#ifndef _PreComp_
# include <float.h>
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# else
# include <GL/gl.h>
# endif
#endif

#if !defined(FC_OS_MACOSX)
# include <GL/gl.h>
# include <GL/glu.h>
# include <GL/glext.h>
#endif

#include <unordered_map>

#include <QColor>
#include <QVariant>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

#include <Base/Console.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/timer.h>
#include <bx/math.h>
#include <bgfx_utils.h>

#ifdef FC_OS_LINUX
#   include <QtPlatformHeaders/QGLXNativeContext>
typedef QGLXNativeContext OpenGLContext;
#elif defined FC_OS_WIN
#  include <QtPlatformHeaders/QWGLNativeContext>
typedef QWGLNativeContext OpenGLContext;
#elif defined FC_OS_MACOSX
#  include <QtPlatformHeaders/QCocoaNativeContext>
typedef QCocoaNativeContext OpenGLContext;
#endif
#undef KeyPress
#undef Status
#undef None

FC_LOG_LEVEL_INIT("bgfx", true, true);

using namespace Gui;

extern "C" int _main_(int, char**) {
    FC_LOG("begin");
    return 0;
}

struct PosColorVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_abgr;

	static void init()
	{
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
			.end();
	};

	static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosColorVertex::ms_layout;

static PosColorVertex s_cubeVertices[8] =
{
	{-1.0f,  1.0f,  1.0f, 0xff000000 },
	{ 1.0f,  1.0f,  1.0f, 0xff0000ff },
	{-1.0f, -1.0f,  1.0f, 0xff00ff00 },
	{ 1.0f, -1.0f,  1.0f, 0xff00ffff },
	{-1.0f,  1.0f, -1.0f, 0xffff0000 },
	{ 1.0f,  1.0f, -1.0f, 0xffff00ff },
	{-1.0f, -1.0f, -1.0f, 0xffffff00 },
	{ 1.0f, -1.0f, -1.0f, 0xffffffff },
};

static const uint16_t s_cubeIndices[36] =
{
	0, 1, 2, // 0
	1, 3, 2,
	4, 6, 5, // 2
	5, 6, 7,
	0, 2, 4, // 4
	4, 2, 6,
	1, 5, 3, // 6
	5, 7, 3,
	0, 4, 1, // 8
	4, 5, 1,
	2, 3, 6, // 10
	6, 3, 7,
};

typedef void (*FreeResourceFunc)(QOpenGLFunctions *functions, GLuint id);
typedef std::vector<std::pair<GLuint, FreeResourceFunc>> PendingRemoves;

namespace
{
    void freeFramebufferFunc(QOpenGLFunctions *funcs, GLuint id)
    {
        funcs->glDeleteFramebuffers(1, &id);
    }

    /*
    void freeRenderbufferFunc(QOpenGLFunctions *funcs, GLuint id)
    {
        funcs->glDeleteRenderbuffers(1, &id);
    }

    void freeTextureFunc(QOpenGLFunctions *funcs, GLuint id)
    {
        funcs->glDeleteTextures(1, &id);
    }
    */

    bool _checkGLError(int line, const char *msg)
    {
        GLenum error = glGetError();
        if (error == GL_NO_ERROR)
            return false;
        unsigned clamped = qMin(unsigned(error - GL_INVALID_ENUM), 4U);
        const char *errors[] = { "GL_INVALID_ENUM", "GL_INVALID_VALUE", "GL_INVALID_OPERATION", "Unknown" };
        _FC_ERR(__FILE__, line, msg << " (" << errors[clamped] << ")");
        return true;
    }
    #define checkGLError(msg) _checkGLError(__LINE__, msg)

    bool _checkFramebufferStatus(int line)
    {
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        switch(status) {
        case GL_NO_ERROR:
        case GL_FRAMEBUFFER_COMPLETE:
            return true;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            _FC_ERR(__FILE__, line, "Unsupported framebuffer format.");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            _FC_ERR(__FILE__, line, "Framebuffer incomplete attachment.");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            _FC_ERR(__FILE__, line, "Framebuffer incomplete, missing attachment.");
            break;
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT
        case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT:
            _FC_ERR(__FILE__, line, "Framebuffer incomplete, duplicate attachment.");
            break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
            _FC_ERR(__FILE__, line, "Framebuffer incomplete, attached images must have same dimensions.");
            break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_FORMATS
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
            _FC_ERR(__FILE__, line, "Framebuffer incomplete, attached images must have same format.");
            break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            _FC_ERR(__FILE__, line, "Framebuffer incomplete, missing draw buffer.");
            break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            _FC_ERR(__FILE__, line, "Framebuffer incomplete, missing read buffer.");
            break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            _FC_ERR(__FILE__, line, "Framebuffer incomplete, attachments must have same number of samples per pixel.");
            break;
#endif
        default:
            _FC_ERR(__FILE__, line, "An undefined error has occurred: " << status);
            break;
        }
        return false;
    }
    #define checkFramebufferStatus() _checkFramebufferStatus(__LINE__)
}

class BGFXView
{
public:
    ~BGFXView()
    {
        destroy();
    }

    void destroy()
    {
        if (bgfx::isValid(bgfxFbo)) {
            bgfx::destroy(bgfxFbo);
            bgfxFbo = BGFX_INVALID_HANDLE;
        }
        if (bgfx::isValid(m_ibh)) {
            bgfx::destroy(m_ibh);
            m_ibh = BGFX_INVALID_HANDLE;
        }
        if (bgfx::isValid(m_vbh)) {
            bgfx::destroy(m_vbh);
            m_vbh = BGFX_INVALID_HANDLE;
        }
        if (bgfx::isValid(m_program)) {
            bgfx::destroy(m_program);
            m_program = BGFX_INVALID_HANDLE;
        }
        if (bgfx::isValid(m_program_non_instanced)) {
            bgfx::destroy(m_program_non_instanced);
            m_program_non_instanced = BGFX_INVALID_HANDLE;
        }
        if (hasFBO) {
            assert(pendingRemoves);
            pendingRemoves->emplace_back(fbo, freeFramebufferFunc);
            hasFBO = false;
        }
    }

    bgfx::TextureHandle createTexture(bgfx::TextureFormat::Enum format, uint64_t flags = 0)
    {
        const uint64_t tsFlags = 0
            | BGFX_SAMPLER_MIN_POINT
            | BGFX_SAMPLER_MAG_POINT
            | BGFX_SAMPLER_MIP_POINT
            | BGFX_SAMPLER_U_CLAMP
            | BGFX_SAMPLER_V_CLAMP
            | BGFX_TEXTURE_RT_WRITE_ONLY;
        return bgfx::createTexture2D(width, height, false, 1, format, tsFlags | flags);
    }

    void init()
    {
        destroy();
        width = uint16_t(widget->width());
        height = uint16_t(widget->height());

        int samples = widget->format().samples();
        uint64_t flags = 0;
        if (samples >= 8)
            flags = BGFX_TEXTURE_RT_MSAA_X8;
        else if (samples >= 4)
            flags = BGFX_TEXTURE_RT_MSAA_X4;
        else if (samples >= 2)
            flags = BGFX_TEXTURE_RT_MSAA_X2;
        else
            flags = BGFX_TEXTURE_RT;

        bgfxColor = createTexture(bgfx::TextureFormat::RGBA8, flags);
        //GL_DEPTH24_STENCIL8
        bgfxDepth = createTexture(bgfx::TextureFormat::D24S8);
        bgfx::Attachment attachment[2];
        attachment[0].init(bgfxColor);
        attachment[1].init(bgfxDepth);
        bgfxFbo = bgfx::createFrameBuffer(2, attachment, true);

        bgfx::setViewFrameBuffer(viewId, bgfxFbo);


        m_timeOffset = bx::getHPCounter();

        // Create vertex stream declaration.
        PosColorVertex::init();

        // Create static vertex buffer.
        m_vbh = bgfx::createVertexBuffer(
                    bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices) )
                    , PosColorVertex::ms_layout
                    );

        // Create static index buffer.
        m_ibh = bgfx::createIndexBuffer(
                    bgfx::makeRef(s_cubeIndices, sizeof(s_cubeIndices) )
                    );

        // Create program from shaders.
        m_program = loadProgram("vs_instancing", "fs_instancing");
		m_program_non_instanced = loadProgram("vs_cubes", "fs_cubes");
    }

    void render()
    {
		uint32_t m_sideSize = 101;
        float offset = -int(m_sideSize) * 1.5f;

        float time = (float)( (bx::getHPCounter() - m_timeOffset)/double(bx::getHPFrequency() ) );

        if (true) {
            // 80 bytes stride = 64 bytes for 4x4 matrix + 16 bytes for RGBA color.
            const uint16_t instanceStride = 80;
            // to total number of instances to draw
            uint32_t totalCubes = m_sideSize * m_sideSize;

            // figure out how big of a buffer is available
            uint32_t drawnCubes = bgfx::getAvailInstanceDataBuffer(totalCubes, instanceStride);

            bgfx::InstanceDataBuffer idb;
            bgfx::allocInstanceDataBuffer(&idb, drawnCubes, instanceStride);

            uint8_t* data = idb.data;

            for (uint32_t ii = 0; ii < drawnCubes; ++ii)
            {
                uint32_t yy = ii / m_sideSize;
                uint32_t xx = ii % m_sideSize;

                float* mtx = (float*)data;
                bx::mtxRotateXY(mtx, time + xx * 0.21f, time + yy * 0.37f);
                mtx[12] = offset + float(xx) * 3.0f;
                mtx[13] = offset + float(yy) * 3.0f;
                mtx[14] = 0.0f;

                float* color = (float*)&data[64];
                color[0] = bx::sin(time + float(xx) / 11.0f) * 0.5f + 0.5f;
                color[1] = bx::cos(time + float(yy) / 11.0f) * 0.5f + 0.5f;
                color[2] = bx::sin(time * 3.0f) * 0.5f + 0.5f;
                color[3] = 1.0f;

                data += instanceStride;
            }

            // Set vertex and index buffer.
            bgfx::setVertexBuffer(0, m_vbh);
            bgfx::setIndexBuffer(m_ibh);

            // Set instance data buffer.
            bgfx::setInstanceDataBuffer(&idb);

            // Set render states.
            bgfx::setState(BGFX_STATE_DEFAULT);

            // Submit primitive for rendering to view 0.
            bgfx::submit(viewId, m_program);
        } else {
            // non-instanced path
            for (uint32_t yy = 0; yy < m_sideSize; ++yy)
            {
                for (uint32_t xx = 0; xx < m_sideSize; ++xx)
                {
                    float mtx[16];
                    bx::mtxRotateXY(mtx, time + xx * 0.21f, time + yy * 0.37f);
                    mtx[12] = offset + float(xx) * 3.0f;
                    mtx[13] = offset + float(yy) * 3.0f;
                    mtx[14] = 0.0f;

                    // Set model matrix for rendering.
                    bgfx::setTransform(mtx);

                    // Set vertex and index buffer.
                    bgfx::setVertexBuffer(0, m_vbh);
                    bgfx::setIndexBuffer(m_ibh);

                    // Set render states.
                    bgfx::setState(BGFX_STATE_DEFAULT);

                    // Submit primitive for rendering to view 0.
                    bgfx::submit(viewId, m_program_non_instanced);
                }
            }
        }
    }

    void blit()
    {
        GLint prevFbo;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *) &prevFbo);
        if (!hasFBO) {
            hasFBO = true;
            GLuint colorBuffer = bgfx::getInternal(bgfxColor);
            GLuint depthBuffer = bgfx::getInternal(bgfxDepth);
            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                        GL_RENDERBUFFER, colorBuffer);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                        GL_RENDERBUFFER, depthBuffer);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                        GL_RENDERBUFFER, depthBuffer);
            if (!checkFramebufferStatus()) {
                glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
                destroy();
                return;
            }
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevFbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBlitFramebuffer(0, 0, width, height,
                          0, 0, width, height,
                          GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);
        checkGLError("blit");
        glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
    }

    QOpenGLWidget *widget = nullptr;
    uint16_t viewId = 0;
    uint16_t width;
    uint16_t height;
    bgfx::FrameBufferHandle bgfxFbo = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle bgfxColor = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle bgfxDepth = BGFX_INVALID_HANDLE;
    int64_t m_timeOffset;
	bgfx::VertexBufferHandle m_vbh = BGFX_INVALID_HANDLE;
	bgfx::IndexBufferHandle  m_ibh = BGFX_INVALID_HANDLE;
	bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
	bgfx::ProgramHandle m_program_non_instanced = BGFX_INVALID_HANDLE;
    GLuint fbo = 0;
    bool hasFBO = false;
    PendingRemoves *pendingRemoves;
};

class BGFXSharedData : public QObject
{
public:
    BGFXSharedData(QOpenGLWidget *widget)
    {
        auto format = widget->format();
        context.setShareContext(QOpenGLContext::globalShareContext());
        context.setFormat(format);
        context.create();

        offscreen.setFormat(format);
        offscreen.create();

        makeCurrent();

        bgfx::renderFrame();
        bgfx::Init init;
        init.platformData.context = qvariant_cast<OpenGLContext>(
                context.nativeHandle()).context();
        init.resolution.width = widget->width();
        init.resolution.height = widget->height();
        init.resolution.reset = BGFX_RESET_VSYNC;
        bgfx::renderFrame();
        if (!bgfx::init(init)) {
            FC_ERR("bgfx init failed");
        }
    }

    ~BGFXSharedData()
    {
        bgfx::shutdown();
    }

    static std::shared_ptr<BGFXSharedData> instance(QOpenGLWidget *widget)
    {
        static std::weak_ptr<BGFXSharedData> _instance;
        if (auto res = _instance.lock())
            return res;
        auto res = std::make_shared<BGFXSharedData>(widget);
        _instance = res;
        return res;
    }

    BGFXView &getView(QOpenGLWidget *widget)
    {
        auto &view = views[widget];
        if (!view.widget) {
            view.pendingRemoves = &pendingRemoves;
            view.widget = widget;
            view.viewId = 0;
            for (int id : viewIds) {
                if (view.viewId == id)
                    ++view.viewId;
                else
                    break;
            }
            viewIds.insert(view.viewId);
        } 
        
        if (widget->width() != int(view.width)
                || widget->height() != int(view.height))
            view.init();

        return view;
    }

    void makeCurrent()
    {
        context.makeCurrent(&offscreen);
        for (auto &v : pendingRemoves)
            v.second(context.functions(), v.first);
        pendingRemoves.clear();
    }

    void doneCurrent()
    {
        context.doneCurrent();
    }

    void removeView(QOpenGLWidget *widget)
    {
        auto it = views.find(widget);
        if (it != views.end()) {
            viewIds.erase(it->second.viewId);
            views.erase(it);
        }
    }

    PendingRemoves pendingRemoves;
    std::unordered_map<QOpenGLWidget *, BGFXView> views;
    std::set<uint16_t> viewIds;
    QOpenGLContext context;
    QOffscreenSurface offscreen;
};

#include <3rdParty/bgfx/bgfx/examples/00-helloworld/logo.h>

class BGFXRenderer::Private 
{
public:
    Private(QOpenGLWidget *widget)
        :widget(widget)
    {
    }

    ~Private()
    {
        if (shared)
            shared->removeView(widget);
    }

    bool render(const QColor &col,
                const void * viewMatrix,
                const void * projMatrix)
    {
        if (!shared)
            shared = BGFXSharedData::instance(widget);

        auto &view = shared->getView(widget);
        if (!bgfx::isValid(view.bgfxFbo)) {
            widget->makeCurrent();
            return false;
        }

        int id =  view.viewId;
        uint16_t width = view.width;
        uint16_t height = view.height;
		bgfx::setDebug(BGFX_DEBUG_TEXT);
		bgfx::setViewClear(id, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH , col.rgba64(), 1.0f , 0);
        bgfx::setViewRect(id, 0, 0, width, height);


        bgfx::setViewTransform(id, viewMatrix, projMatrix);
        bgfx::touch(id);

        view.render();

        widget->doneCurrent();
        shared->makeCurrent();
        bgfx::frame();
        widget->makeCurrent();
        view.blit();
        return true;
    }

    QOpenGLWidget *widget;
    std::shared_ptr<BGFXSharedData> shared;
};

BGFXRenderer::BGFXRenderer(QOpenGLWidget *widget)
    :pimpl(new Private(widget))
{
}

BGFXRenderer::~BGFXRenderer()
{
}

bool BGFXRenderer::render(const QColor &col,
                          const void * viewMatrix,
                          const void * projMatrix)
{
    return pimpl->render(col, viewMatrix, projMatrix);
}

bool BGFXRenderer::boundBox(float &xmin, float &ymin, float &zmin,
                            float &xmax, float &ymax, float &zmax)
{
    xmin = ymin = zmin = -1000.f;
    xmax = ymax = zmax = 1000.f;
    return true;
}

#endif // HAVE_BGFX
