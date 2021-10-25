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

#include "FCConfig.h"
#include "DiligentRenderer.h"

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

#include <unordered_map>
#include <random>
#include <set>

#undef GL_GLEXT_VERSION
#include <QColor>
#include <QVariant>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QOpenGLWidget>
#include <QWindow>
#include <QDebug>

// #if !defined(FC_OS_MACOSX)
// # include <GL/gl.h>
// # include <GL/glu.h>
// # include <GL/glext.h>
// #endif

#include <EngineFactory.h>
#include <EngineFactoryOpenGL.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <DeviceContext.h>
#include <DeviceContextGL.h>
#include <TextureGL.h>
#include <SwapChain.h>
#include <Timer.hpp>
#include <BasicMath.hpp>
#include <MapHelper.hpp>
#include <GraphicsUtilities.h>
#include <TextureUtilities.h>
#include "Diligent/TexturedCube.hpp"

#define _RENDER_LOG(_lvl, _line, _msg) \
    _lvl() << "diligent (" << _line << "): " << _msg

#define _RENDER_ERR(_line, _msg) _RENDER_LOG(qCritical, _line, _msg)
#define RENDER_ERR(_msg) _RENDER_ERR(__LINE__, _msg)
#define _RENDER_WARN(_line, _msg) _RENDER_LOG(qWarn, _line, _msg)
#define RENDER_WARN(_msg) _RENDER_ERR(__LINE__, _msg)

using namespace Render;
using namespace Diligent;

////////////////////////////////////////////////////////

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
        _RENDER_ERR(line, msg << " (" << errors[clamped] << ")");
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
            _RENDER_ERR(line, "Unsupported framebuffer format.");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            _RENDER_ERR(line, "Framebuffer incomplete attachment.");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            _RENDER_ERR(line, "Framebuffer incomplete, missing attachment.");
            break;
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT
        case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT:
            _RENDER_ERR(line, "Framebuffer incomplete, duplicate attachment.");
            break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
            _RENDER_ERR(line, "Framebuffer incomplete, attached images must have same dimensions.");
            break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_FORMATS
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
            _RENDER_ERR(line, "Framebuffer incomplete, attached images must have same format.");
            break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            _RENDER_ERR(line, "Framebuffer incomplete, missing draw buffer.");
            break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            _RENDER_ERR(line, "Framebuffer incomplete, missing read buffer.");
            break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            _RENDER_ERR(line, "Framebuffer incomplete, attachments must have same number of samples per pixel.");
            break;
#endif
        default:
            _RENDER_ERR(line, "An undefined error has occurred: " << status);
            break;
        }
        return false;
    }
    #define checkFramebufferStatus() _checkFramebufferStatus(__LINE__)
}

////////////////////////////////////////////////////////

class DiligentView;

namespace Render {

class DiligentRendererLibP {
public:
    DiligentRendererLibP() {
        for (auto &v : typeMap)
            types.push_back(v.first);
    }

    DiligentView *getView(QOpenGLWidget *widget, RENDER_DEVICE_TYPE type);

    void removeView(QOpenGLWidget *widget);

    bool prepare(QOpenGLWidget *widget, RENDER_DEVICE_TYPE type)
    {
        if (!context) {
            context.reset(new QOpenGLContext);
            auto format = widget->format();
            context->setShareContext(QOpenGLContext::globalShareContext());
            context->setFormat(format);
            context->create();
            offscreen.reset(new QOffscreenSurface);
            offscreen->setFormat(format);
            offscreen->create();
        }

        if (currentType == RENDER_DEVICE_TYPE_UNDEFINED) {
            currentType = type;

            if (currentType == RENDER_DEVICE_TYPE_GL) {
                makeCurrent();

                auto pEngineFactory = GetEngineFactoryOpenGL();
                m_pEngineFactory = pEngineFactory;
                EngineGLCreateInfo Attribs;
                pEngineFactory->AttachToActiveGLContext(Attribs, &m_pDevice, &m_pContext);
                m_pContextGL = RefCntAutoPtr<IDeviceContextGL>(m_pContext, IID_DeviceContextGL);

            } else {
                // window = new QWindow();
                // window->setObjectName(QLatin1String("DiligentOffscreen"));
                // window->setSurfaceType(QSurface::VulkanSurface);
                // auto pEngineFactory = GetEngineFactoryVulkan();
                // m_pEngineFactory = pEngineFactory;
            }
        }
        return true;
    }

    std::string resource(const char *path = nullptr)
    {
        return RendererFactory::resourcePath() + "/Diligent/assets/" + (path?path:"");
    }

    void shutdown();

    void makeCurrent()
    {
        context->makeCurrent(offscreen.get());
        for (auto &v : pendingRemoves)
            v.second(context->functions(), v.first);
        pendingRemoves.clear();
        if (m_pContextGL)
            m_pContextGL->UpdateCurrentGLContext();
    }

    void doneCurrent()
    {
        if (m_pContext)
            m_pContext->InvalidateState();
        context->doneCurrent();
    }

    void freeFBO(GLint fbo)
    {
        pendingRemoves.emplace_back(fbo, freeFramebufferFunc);
    }

    typedef void (*FreeResourceFunc)(QOpenGLFunctions *functions, GLuint id);
    std::vector<std::pair<GLuint, FreeResourceFunc>> pendingRemoves;
    std::unordered_map<QOpenGLWidget *, std::unique_ptr<DiligentView>> views;
    std::set<uint16_t> viewIds;
    std::unique_ptr<QOpenGLContext> context;
    std::unique_ptr<QOffscreenSurface> offscreen;

    std::map<std::string, RENDER_DEVICE_TYPE> typeMap = {
        {"Diligent - OpenGL", RENDER_DEVICE_TYPE_GL},
        // {"Diligent - Vulkan", RENDER_DEVICE_TYPE_VULKAN},
#ifdef FC_OS_WIN32
        // {"Diligent - Direct3D11", RENDER_DEVICE_TYPE_D3D11},
        // {"Diligent - Direct3D12", RENDER_DEVICE_TYPE_D3D12},
#elif defined(FC_OS_MACOSX)
        // {"Diligent - Metal", RENDER_DEVICE_TYPE_METAL},
#endif
    };
    std::vector<std::string> types;
    RENDER_DEVICE_TYPE currentType = RENDER_DEVICE_TYPE_UNDEFINED;
    std::string name = "Diligent";
    std::set<DiligentRenderer::Private *> renderers;
    QWindow *window = nullptr;

    RefCntAutoPtr<IRenderDevice>  m_pDevice;
    RefCntAutoPtr<IDeviceContext> m_pContext;
    RefCntAutoPtr<IDeviceContextGL> m_pContextGL;

    IEngineFactory*  m_pEngineFactory  = nullptr;
};

DiligentRendererLibP _DiligentLib;
DiligentRendererLib DiligentLib;

} // namespace Render

////////////////////////////////////////////////////////

class DiligentView
{
public:
    ~DiligentView()
    {
        destroy();
    }

    void destroy()
    {
        m_pPSO.Release();
        m_CubeVertexBuffer.Release();
        m_CubeIndexBuffer.Release();
        m_InstanceBuffer.Release();
        m_VSConstants.Release();
        m_TextureSRV.Release();
        m_SRB.Release();

        if (hasFBO) {
            _DiligentLib.freeFBO(fbo);
            hasFBO = false;
        }
    }

    void init()
    {
        destroy();
        width = uint16_t(widget->width());
        height = uint16_t(widget->height());

        int samples = widget->format().samples();
        if (samples >= 8)
            samples = 8;
        else if (samples >= 4)
            samples = 4;
        else if (samples >= 2)
            samples = 2;
        else
            samples = 1;

        auto pDevice = _DiligentLib.m_pDevice;
        auto pContext = _DiligentLib.m_pContext;
        auto pEngineFactory = _DiligentLib.m_pEngineFactory;

        // Create window-size offscreen render target
        TextureDesc RTColorDesc;
        RTColorDesc.Name      = "Offscreen render target";
        RTColorDesc.Type      = RESOURCE_DIM_TEX_2D;
        RTColorDesc.Width     = width;
        RTColorDesc.Height    = height;
        RTColorDesc.MipLevels = 1;
        RTColorDesc.Format    = RenderTargetFormat;
        // The render target can be bound as a shader resource and as a render target
        RTColorDesc.BindFlags = BIND_RENDER_TARGET;
        // Define optimal clear value
        RTColorDesc.ClearValue.Format   = RTColorDesc.Format;
        RTColorDesc.ClearValue.Color[0] = 0.350f;
        RTColorDesc.ClearValue.Color[1] = 0.350f;
        RTColorDesc.ClearValue.Color[2] = 0.350f;
        RTColorDesc.ClearValue.Color[3] = 1.f;
        RTColorDesc.SampleCount = samples;
        RefCntAutoPtr<ITexture> pRTColor;
        pDevice->CreateTexture(RTColorDesc, nullptr, &pRTColor);
        // Store the render target view
        m_pColorRTV = pRTColor->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);

        RefCntAutoPtr<ITextureGL> pRTColorGL(pRTColor, IID_TextureGL);
        colorBuffer = pRTColorGL->GetGLTextureHandle();

        // Create window-size depth buffer
        TextureDesc RTDepthDesc = RTColorDesc;
        RTDepthDesc.Name        = "Offscreen depth buffer";
        RTDepthDesc.Format      = DepthBufferFormat;
        RTDepthDesc.BindFlags   = BIND_DEPTH_STENCIL;
        // Define optimal clear value
        RTDepthDesc.ClearValue.Format               = RTDepthDesc.Format;
        RTDepthDesc.ClearValue.DepthStencil.Depth   = 1;
        RTDepthDesc.ClearValue.DepthStencil.Stencil = 0;
        RTColorDesc.SampleCount = 1;
        RefCntAutoPtr<ITexture> pRTDepth;
        pDevice->CreateTexture(RTDepthDesc, nullptr, &pRTDepth);
        // Store the depth-stencil view
        m_pDepthDSV = pRTDepth->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);

        RefCntAutoPtr<ITextureGL> pRTDepthGL(pRTDepth, IID_TextureGL);
        depthBuffer = pRTDepthGL->GetGLTextureHandle();

        if (m_pPSO)
            return;

        // Define vertex shader input layout
        // This tutorial uses two types of input: per-vertex data and per-instance data.
        LayoutElement LayoutElems[] =
        {
            // Per-vertex data - first buffer slot
            // Attribute 0 - vertex position
            LayoutElement{0, 0, 3, VT_FLOAT32, False},
            // Attribute 1 - texture coordinates
            LayoutElement{1, 0, 2, VT_FLOAT32, False},
                
            // Per-instance data - second buffer slot
            // We will use four attributes to encode instance-specific 4x4 transformation matrix
            // Attribute 2 - first row
            LayoutElement{2, 1, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
            // Attribute 3 - second row
            LayoutElement{3, 1, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
            // Attribute 4 - third row
            LayoutElement{4, 1, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
            // Attribute 5 - fourth row
            LayoutElement{5, 1, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE}
        };
        // clang-format on

        // Create a shader source stream factory to load shaders from files.
        RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
        std::string path = _DiligentLib.resource();
        pEngineFactory->CreateDefaultShaderSourceStreamFactory(path.c_str(), &pShaderSourceFactory);

        TexturedCube::CreatePSOInfo CubePsoCI;
        CubePsoCI.pDevice                = pDevice;
        CubePsoCI.RTVFormat              = RenderTargetFormat;
        CubePsoCI.DSVFormat              = DepthBufferFormat;
        CubePsoCI.pShaderSourceFactory   = pShaderSourceFactory;
        CubePsoCI.VSFilePath             = "cube_inst.vsh";
        CubePsoCI.PSFilePath             = "cube_inst.psh";
        CubePsoCI.ExtraLayoutElements    = LayoutElems;
        CubePsoCI.NumExtraLayoutElements = _countof(LayoutElems);

        m_pPSO = TexturedCube::CreatePipelineState(CubePsoCI);
        if (!m_pPSO)
            return;

        // Create dynamic uniform buffer that will store our transformation matrix
        // Dynamic buffers can be frequently updated by the CPU
        CreateUniformBuffer(pDevice, sizeof(float4x4) * 2, "VS constants CB", &m_VSConstants);

        // Since we did not explcitly specify the type for 'Constants' variable, default
        // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables
        // never change and are bound directly to the pipeline state object.
        m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);

        // Since we are using mutable variable, we must create a shader resource binding object
        // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
        m_pPSO->CreateShaderResourceBinding(&m_SRB, true);

        // Load textured cube
        m_CubeVertexBuffer = TexturedCube::CreateVertexBuffer(
                pDevice, TexturedCube::VERTEX_COMPONENT_FLAG_POS_UV);
        m_CubeIndexBuffer  = TexturedCube::CreateIndexBuffer(pDevice);
        m_TextureSRV = TexturedCube::LoadTexture(pDevice,
            _DiligentLib.resource("DGLogo.png").c_str())->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

        // Set render target color texture SRV in the SRB
        m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_TextureSRV);

        // Create instance data buffer that will store transformation matrices
        BufferDesc InstBuffDesc;
        InstBuffDesc.Name = "Instance data buffer";
        // Use default usage as this buffer will only be updated when grid size changes
        InstBuffDesc.Usage     = USAGE_DEFAULT;
        InstBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        InstBuffDesc.Size      = sizeof(float4x4) * MaxInstances;
        pDevice->CreateBuffer(InstBuffDesc, nullptr, &m_InstanceBuffer);

        // Populate instance data buffer
        std::vector<float4x4> InstanceData(m_GridSize * m_GridSize * m_GridSize);

        float fGridSize = static_cast<float>(m_GridSize);

        std::mt19937 gen; // Standard mersenne_twister_engine. Use default seed
        // to generate consistent distribution.

        std::uniform_real_distribution<float> scale_distr(0.3f, 1.0f);
        std::uniform_real_distribution<float> offset_distr(-0.15f, +0.15f);
        std::uniform_real_distribution<float> rot_distr(-PI_F, +PI_F);

        float BaseScale = 0.6f / fGridSize;
        int   instId    = 0;
        for (int x = 0; x < m_GridSize; ++x)
        {
            for (int y = 0; y < m_GridSize; ++y)
            {
                for (int z = 0; z < m_GridSize; ++z)
                {
                    // Add random offset from central position in the grid
                    float xOffset = 2.f * (x + 0.5f + offset_distr(gen)) / fGridSize - 1.f;
                    float yOffset = 2.f * (y + 0.5f + offset_distr(gen)) / fGridSize - 1.f;
                    float zOffset = 2.f * (z + 0.5f + offset_distr(gen)) / fGridSize - 1.f;
                    // Random scale
                    float scale = BaseScale * scale_distr(gen);
                    // Random rotation
                    float4x4 rotation = float4x4::RotationX(rot_distr(gen)) * float4x4::RotationY(rot_distr(gen)) * float4x4::RotationZ(rot_distr(gen));
                    // Combine rotation, scale and translation
                    float4x4 matrix        = rotation * float4x4::Scale(scale, scale, scale) * float4x4::Translation(xOffset, yOffset, zOffset);
                    InstanceData[instId++] = matrix;
                }
            }
        }
        // Update instance data buffer
        Uint32 DataSize = static_cast<Uint32>(sizeof(InstanceData[0]) * InstanceData.size());
        pContext->UpdateBuffer(m_InstanceBuffer, 0,
                DataSize, InstanceData.data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    void render(const QColor &color, const void *viewMat, const void *projMat)
    {
        if (!m_pPSO)
            return;

        auto pContext = _DiligentLib.m_pContext;
        pContext->SetRenderTargets(
                1, &m_pColorRTV, m_pDepthDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        float ClearColor[4] = {(float)color.redF(),
                               (float)color.greenF(),
                               (float)color.blueF(),
                               1.0f};
        pContext->ClearRenderTarget(
                m_pColorRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->ClearDepthStencil(
                m_pDepthDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        auto &vMat = *reinterpret_cast<const float4x4*>(viewMat);
        auto &pMat = *reinterpret_cast<const float4x4*>(projMat);
        m_ViewProjMatrix = pMat * vMat;

        auto CurrTime = m_Timer.GetElapsedTime();
        m_RotationMatrix = float4x4::RotationY(static_cast<float>(CurrTime) * 1.0f) *
                           float4x4::RotationX(-static_cast<float>(CurrTime) * 0.25f);

        {
            // Map the buffer and write current world-view-projection matrix
            MapHelper<float4x4> CBConstants(
                    pContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
            CBConstants[0] = m_ViewProjMatrix.Transpose();
            CBConstants[1] = m_RotationMatrix.Transpose();
        }

        // Bind vertex, instance and index buffers
        const Uint64 offsets[] = {0, 0};
        IBuffer*     pBuffs[]  = {m_CubeVertexBuffer, m_InstanceBuffer};
        pContext->SetVertexBuffers(0, _countof(pBuffs), pBuffs, offsets,
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
        pContext->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // Set the pipeline state
        pContext->SetPipelineState(m_pPSO);
        // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
        // makes sure that resources are transitioned to required states.
        pContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawIndexedAttribs DrawAttrs;       // This is an indexed draw call
        DrawAttrs.IndexType    = VT_UINT32; // Index type
        DrawAttrs.NumIndices   = 36;
        DrawAttrs.NumInstances = m_GridSize * m_GridSize * m_GridSize; // The number of instances
        // Verify the state of vertex and index buffers
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
        pContext->DrawIndexed(DrawAttrs);
    }

    void blit()
    {
        GLint prevFbo;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *) &prevFbo);
        if (!hasFBO) {
            hasFBO = true;
            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorBuffer, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthBuffer, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, depthBuffer, 0);
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

    GLuint fbo = 0;
    bool hasFBO = false;

    Timer m_Timer;

    RefCntAutoPtr<IPipelineState>         m_pPSO;
    RefCntAutoPtr<IBuffer>                m_CubeVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_CubeIndexBuffer;
    RefCntAutoPtr<IBuffer>                m_InstanceBuffer;
    RefCntAutoPtr<IBuffer>                m_VSConstants;
    RefCntAutoPtr<ITextureView>           m_TextureSRV;
    RefCntAutoPtr<IShaderResourceBinding> m_SRB;

    RefCntAutoPtr<ITextureView>           m_pColorRTV;
    RefCntAutoPtr<ITextureView>           m_pDepthDSV;

    GLuint colorBuffer;
    GLuint depthBuffer;

    static constexpr TEXTURE_FORMAT RenderTargetFormat = TEX_FORMAT_RGBA8_UNORM;
    static constexpr TEXTURE_FORMAT DepthBufferFormat  = TEX_FORMAT_D24_UNORM_S8_UINT;

    float4x4             m_ViewProjMatrix;
    float4x4             m_RotationMatrix;
    int                  m_GridSize   = 10;
    static constexpr int MaxGridSize  = 32;
    static constexpr int MaxInstances = MaxGridSize * MaxGridSize * MaxGridSize;
};

class DiligentRenderer::Private 
{
public:
    Private(QOpenGLWidget *widget)
        :widget(widget)
    {
        _DiligentLib.renderers.insert(this);
    }

    ~Private()
    {
        _DiligentLib.renderers.erase(this);
        deinit();
    }

    void deinit()
    {
        _deinit = true;
        _DiligentLib.removeView(widget);
    }

    bool render(const QColor &color,
                const void * viewMatrix,
                const void * projMatrix)
    {
        if (_deinit)
            return false;

        auto view = _DiligentLib.getView(widget, type); 
        if (!view)
            return false;

        if (widget->width() != int(view->width)
                || widget->height() != int(view->height))
            view->init();

        widget->doneCurrent();
        _DiligentLib.makeCurrent();

        view->render(color, viewMatrix, projMatrix);

        widget->makeCurrent();
        view->blit();
        return true;
    }

    QOpenGLWidget *widget;
    bool _deinit = false;
    RENDER_DEVICE_TYPE type;
    std::string typeName;
};

DiligentRenderer::DiligentRenderer(QOpenGLWidget *widget)
    :pimpl(new Private(widget))
{
}

DiligentRenderer::~DiligentRenderer()
{
}

bool DiligentRenderer::render(const QColor &col,
                          const void * viewMatrix,
                          const void * projMatrix)
{
    return pimpl->render(col, viewMatrix, projMatrix);
}

bool DiligentRenderer::boundBox(float &xmin, float &ymin, float &zmin,
                            float &xmax, float &ymax, float &zmax)
{
    xmin = ymin = zmin = -1000.f;
    xmax = ymax = zmax = 1000.f;
    return true;
}

const std::string &DiligentRenderer::type() const
{
    return pimpl->typeName;
}

//////////////////////////////////////////////////////////////////////

DiligentRendererLib::DiligentRendererLib()
{
    RendererFactory::registerLib(this);
}

const std::string &DiligentRendererLib::name() const
{
    return _DiligentLib.name;
}

const std::vector<std::string> &DiligentRendererLib::types() const
{
    return _DiligentLib.types;
}

std::unique_ptr<Renderer> DiligentRendererLib::create(
        const std::string &type, QOpenGLWidget *widget) const
{
    std::unique_ptr<Renderer> res;
    auto it = _DiligentLib.typeMap.find(type);
    if (it == _DiligentLib.typeMap.end()) {
        RENDER_WARN("Unsupported renderer type " << type.c_str());
        return res;
    }
    if (_DiligentLib.currentType != it->second) {
        for (auto renderer : _DiligentLib.renderers) {
            if (renderer->type != it->second)
                renderer->deinit();
        }
        _DiligentLib.shutdown();
    }
    auto renderer = new DiligentRenderer(widget);
    res.reset(renderer);
    renderer->pimpl->typeName = it->first;
    renderer->pimpl->type = it->second;
    return res;
}

/////////////////////////////////////////////////////////
void DiligentRendererLibP::removeView(QOpenGLWidget *widget)
{
    auto it = views.find(widget);
    if (it != views.end()) {
        viewIds.erase(it->second->viewId);
        views.erase(it);
        if (views.empty())
            _DiligentLib.shutdown();
    }
}

DiligentView *DiligentRendererLibP::getView(QOpenGLWidget *widget, RENDER_DEVICE_TYPE type)
{
    if (!prepare(widget, type))
        return nullptr;

    auto &view = views[widget];
    if (!view) {
        view.reset(new DiligentView);
        view->widget = widget;
        view->viewId = 0;
        for (int id : viewIds) {
            if (view->viewId == id)
                ++view->viewId;
            else
                break;
        }
        viewIds.insert(view->viewId);
    } 
    return view.get();
}

void DiligentRendererLibP::shutdown()
{
    if (currentType == RENDER_DEVICE_TYPE_UNDEFINED)
        return;
    if (window) {
        window->deleteLater();
        window = nullptr;
    }

    m_pContext.Release();
    m_pContextGL.Release();
    m_pDevice.Release();

    context.reset();
    offscreen.reset();
    currentType = RENDER_DEVICE_TYPE_UNDEFINED;
}
