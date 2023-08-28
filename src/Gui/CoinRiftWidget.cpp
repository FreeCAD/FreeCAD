/**************************************************************************
* Copyright (c) 2014 Bastiaan Veelo <Bastiaan a_t Veelo d_o_t net>        *
* Copyright (c) 2014 Jürgen Riegel <FreeCAD@juergen-riegel.net>           *
*                                                                         *
* All rights reserved. Contact me if the below is too restrictive for you.*
*                                                                         *
* Redistribution and use in source and binary forms, with or without      *
* modification, are permitted provided that the following conditions are  *
* met:                                                                    *
*                                                                         *
* Redistributions of source code must retain the above copyright notice,  *
* this list of conditions and the following disclaimer.                   *
*                                                                         *
* Redistributions in binary form must reproduce the above copyright       *
* notice, this list of conditions and the following disclaimer in the     *
* documentation and/or other materials provided with the distribution.    *
*                                                                         *
* Neither the name of the copyright holder nor the names of its           *
* contributors may be used to endorse or promote products derived from    *
* this software without specific prior written permission.                *
*                                                                         *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     *
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT       *
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   *
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT    *
* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  *
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        *
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   *
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY   *
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT     *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   *
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    *
\**************************************************************************/

#include "PreCompiled.h"
#include "CoinRiftWidget.h"

#include <Base/Console.h>

#if BUILD_VR



#undef max




CoinRiftWidget::CoinRiftWidget() : QGLWidget()
{
    for (int eye = 0; eye < 2; eye++) {
        reinterpret_cast<ovrGLTextureData*>(&eyeTexture[eye])->TexId = 0;
#ifdef USE_FRAMEBUFFER
        frameBufferID[eye] = 0;
        depthBufferID[eye] = 0;
#endif
    }

    // OVR will do the swapping.
    setAutoBufferSwap(false);

    hmd = ovrHmd_Create(0);
    if (!hmd) {
        qDebug() << "Could not find Rift device.";
        throw;
    }

    if (!ovrHmd_ConfigureTracking (hmd, ovrTrackingCap_Orientation |
                                        ovrTrackingCap_MagYawCorrection |
                                        ovrTrackingCap_Position,
                                        ovrTrackingCap_Orientation |
                                        ovrTrackingCap_MagYawCorrection |
                                        ovrTrackingCap_Position
                                        )) { // Capabilities we require.
        qDebug() << "Could not start Rift motion sensor.";
        throw;
    }

    resize(hmd->Resolution.w, hmd->Resolution.h);

    // Configure stereo settings.
    ovrSizei recommenedTex0Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left,
                                                           hmd->DefaultEyeFov[0], 1.0f);
    ovrSizei recommenedTex1Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right,
                                                           hmd->DefaultEyeFov[1], 1.0f);

#ifdef USE_SO_OFFSCREEN_RENDERER
    renderer = new SoOffscreenRenderer(SbViewportRegion(std::max(recommenedTex0Size.w, recommenedTex0Size.w),
                                                        std::max(recommenedTex1Size.h, recommenedTex1Size.h)));
    renderer->setComponents(SoOffscreenRenderer::RGB_TRANSPARENCY);
    BackgroundColor = SbColor(.0f, .0f, .8f);
    renderer->setBackgroundColor(BackgroundColor);
#endif
#ifdef USE_FRAMEBUFFER
    m_sceneManager = new SoSceneManager();
    m_sceneManager->setViewportRegion(SbViewportRegion(std::max(recommenedTex0Size.w, recommenedTex0Size.w),
                                                       std::max(recommenedTex1Size.h, recommenedTex1Size.h)));
    m_sceneManager->setBackgroundColor(SbColor(.0f, .0f, .8f));
#endif
    basePosition = SbVec3f(0.0f, 0.0f, -2.0f);

    // light handling
     SoDirectionalLight *light = new SoDirectionalLight();
    light->direction.setValue(1,-1,-1);

    SoDirectionalLight *light2 = new SoDirectionalLight();
    light2->direction.setValue(-1,-1,-1);
    light2->intensity.setValue(0.6);
    light2->color.setValue(0.8,0.8,1);


    scene = new SoSeparator(0); // Placeholder.
    for (int eye = 0; eye < 2; eye++) {
        rootScene[eye] = new SoSeparator();
        rootScene[eye]->ref();
        camera[eye] = new SoFrustumCamera();
        camera[eye]->position.setValue(basePosition);
        camera[eye]->focalDistance.setValue(5.0f);
        camera[eye]->viewportMapping.setValue(SoCamera::LEAVE_ALONE);
        rootScene[eye]->addChild(camera[eye]);
        rootScene[eye]->addChild(light);
        rootScene[eye]->addChild(light2);
        rootScene[eye]->addChild(scene);
    }

    // Populate ovrEyeDesc[2].
    eyeRenderDesc[0].Eye = ovrEye_Left;
    eyeRenderDesc[1].Eye = ovrEye_Right;
    eyeRenderDesc[0].Fov = hmd->DefaultEyeFov[0];
    eyeRenderDesc[1].Fov = hmd->DefaultEyeFov[1];
#ifdef USE_SO_OFFSCREEN_RENDERER
    eyeTexture[0].Header.TextureSize.w = renderer->getViewportRegion().getViewportSizePixels().getValue()[0];
    eyeTexture[0].Header.TextureSize.h = renderer->getViewportRegion().getViewportSizePixels().getValue()[1];
    eyeTexture[1].Header.TextureSize = eyeTexture[0].Header.TextureSize;
#endif
#ifdef USE_FRAMEBUFFER
    eyeTexture[0].Header.TextureSize = recommenedTex0Size;
    eyeTexture[1].Header.TextureSize = recommenedTex1Size;
#endif
    eyeTexture[0].Header.RenderViewport.Pos.x = 0;
    eyeTexture[0].Header.RenderViewport.Pos.y = 0;
    eyeTexture[0].Header.RenderViewport.Size = eyeTexture[0].Header.TextureSize;
    eyeTexture[1].Header.RenderViewport.Pos = eyeTexture[0].Header.RenderViewport.Pos;
    eyeTexture[1].Header.RenderViewport.Size = eyeTexture[1].Header.TextureSize;

    const int backBufferMultisample = 0; // TODO This is a guess?
    ovrGLConfig cfg;
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.RTSize = hmd->Resolution;
    cfg.OGL.Header.Multisample = backBufferMultisample;
    cfg.OGL.Window = reinterpret_cast<HWND>(winId());
    makeCurrent();
    //cfg.OGL.WglContext = wglGetCurrentContext(); // http://stackoverflow.com/questions/17532033/qglwidget-get-gl-contextes-for-windows
    cfg.OGL.DC = wglGetCurrentDC();
    qDebug() << "Window:" << cfg.OGL.Window;
    //qDebug() << "Context:" << cfg.OGL.WglContext;
    qDebug() << "DC:" << cfg.OGL.DC;

    int DistortionCaps = 0;
    DistortionCaps |= ovrDistortionCap_Chromatic;
// DistortionCaps |= ovrDistortionCap_TimeWarp; // Produces black screen...
    DistortionCaps |= ovrDistortionCap_Vignette;
    DistortionCaps |= ovrDistortionCap_HqDistortion;

    bool VSyncEnabled(false); // TODO This is a guess.
    if (!ovrHmd_ConfigureRendering( hmd,
                                    &cfg.Config,
                                    /*(VSyncEnabled ? 0 : ovrHmdCap_NoVSync),*/
                                    DistortionCaps,
                                    hmd->DefaultEyeFov,//eyes,
                                    eyeRenderDesc)) {
        qDebug() << "Could not configure OVR rendering.";
        throw;
    }
    static const float nearPlane = 0.01;

    for (int eye = 0; eye < 2; eye++) {
        camera[eye]->aspectRatio.setValue((eyeRenderDesc[eye].Fov.LeftTan + eyeRenderDesc[eye].Fov.RightTan) /
                (eyeRenderDesc[eye].Fov.UpTan + eyeRenderDesc[eye].Fov.DownTan));
        camera[eye]->nearDistance.setValue(nearPlane);
        camera[eye]->farDistance.setValue(10000.0f);
        camera[eye]->left.setValue(-eyeRenderDesc[eye].Fov.LeftTan * nearPlane);
        camera[eye]->right.setValue(eyeRenderDesc[eye].Fov.RightTan * nearPlane);
        camera[eye]->top.setValue(eyeRenderDesc[eye].Fov.UpTan * nearPlane);
        camera[eye]->bottom.setValue(-eyeRenderDesc[eye].Fov.DownTan * nearPlane);
    }
}


CoinRiftWidget::~CoinRiftWidget()
{
#ifdef USE_SO_OFFSCREEN_RENDERER
    delete renderer;
#endif
    for (int eye = 0; eye < 2; eye++) {
        rootScene[eye]->unref();
        ovrGLTextureData *texData = reinterpret_cast<ovrGLTextureData*>(&eyeTexture[eye]);
        if (texData->TexId) {
            glDeleteTextures(1, &texData->TexId);
            texData->TexId = 0;
        }
#ifdef USE_FRAMEBUFFER
        if (frameBufferID[eye] != 0) {
// OVR::CAPI::GL::glDeleteFramebuffersExt(1, &frameBufferID[eye]); // TODO
            frameBufferID[eye] = 0;
        }
        if (depthBufferID[eye] != 0) {
// OVR::CAPI::GL::glDeleteRenderbuffersExt(1, &depthBufferID[eye]); // TODO
            depthBufferID[eye] = 0;
        }
#endif
    }
    scene = 0;
    //ovrHmd_StopSensor(hmd);
    ovrHmd_Destroy(hmd);
}


void CoinRiftWidget::setBackgroundColor(const SbColor &Col)
{
    BackgroundColor = Col;
    renderer->setBackgroundColor(BackgroundColor);
}


void CoinRiftWidget::setSceneGraph(SoNode *sceneGraph)
{
    rootScene[0]->replaceChild(scene, sceneGraph);
    rootScene[1]->replaceChild(scene, sceneGraph);
    scene = sceneGraph;
}


void CoinRiftWidget::resizeGL(int width, int height) {
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, 0.0, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

void CoinRiftWidget::initializeGL()
{
    makeCurrent();
    // Infer hardware capabilities.
#ifdef USE_FRAMEBUFFER
    OVR::CAPI::GL::InitGLExtensions();
    if (OVR::CAPI::GL::glBindFramebuffer == NULL) {
        qDebug() << "No GL extensions found.";
        exit(4);
    }

    // Store old framebuffer.
    GLint oldfb;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &oldfb);
#endif

    // Create rendering target textures.
    glEnable(GL_TEXTURE_2D);
    for (int eye = 0; eye < 2; eye++) {
#ifdef USE_FRAMEBUFFER
        OVR::CAPI::GL::glGenFramebuffers(1, &frameBufferID[eye]);
        OVR::CAPI::GL::glBindFramebuffer(GL_FRAMEBUFFER_EXT, frameBufferID[eye]);
        // Create the render buffer.
        // TODO: need to check for OpenGl 3 or higher and load the functions JR 2014
        /*OVR::CAPI::GL::*/glGenRenderbuffers(1, &depthBufferID[eye]);
        /*OVR::CAPI::GL::*/glBindRenderbuffer(GL_RENDERBUFFER_EXT, depthBufferID[eye]);
        /*OVR::CAPI::GL::*/glRenderbufferStorage(GL_RENDERBUFFER_EXT,
                                                GL_DEPTH_COMPONENT16,
                                                eyeTexture[eye].Header.TextureSize.w,
                                                eyeTexture[eye].Header.TextureSize.h);
        // Attach renderbuffer to framebuffer.
        OVR::CAPI::GL::glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT,
                                                    GL_DEPTH_ATTACHMENT_EXT,
                                                    GL_RENDERBUFFER_EXT,
                                                    depthBufferID[eye]);
#endif
        ovrGLTextureData *texData = reinterpret_cast<ovrGLTextureData*>(&eyeTexture[eye]);
        texData->Header.API = ovrRenderAPI_OpenGL;
        texData->Header.TextureSize = eyeTexture[eye].Header.TextureSize;
        texData->Header.RenderViewport = eyeTexture[eye].Header.RenderViewport;
        glGenTextures(1, &texData->TexId);
        glBindTexture(GL_TEXTURE_2D, texData->TexId);
        Q_ASSERT(!glGetError());
        // Allocate storage for the texture.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, eyeTexture[eye].Header.TextureSize.w, eyeTexture[eye].Header.TextureSize.h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        Q_ASSERT(!glGetError());
#ifdef USE_FRAMEBUFFER
        // Attach texture to framebuffer color object.
        OVR::CAPI::GL::glFramebufferTexture2D(GL_FRAMEBUFFER_EXT,
                                                 GL_COLOR_ATTACHMENT0_EXT,
                                                 GL_TEXTURE_2D, texData->TexId, 0);
        if (OVR::CAPI::GL::glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
                GL_FRAMEBUFFER_COMPLETE)
            qDebug() << "ERROR: FrameBuffer is not operational!";
#endif
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

#ifdef USE_FRAMEBUFFER
    // Continue rendering to the original frame buffer (likely 0, the onscreen buffer).
    OVR::CAPI::GL::glBindFramebuffer(GL_FRAMEBUFFER_EXT, oldfb);
#endif
    doneCurrent();
}


void CoinRiftWidget::paintGL()
{
    const int ms(1000 / 75 /*fps*/);
    QTimer::singleShot(ms, this, &CoinRiftWidget::updateGL);

    // handling the safety warning
    handlingSafetyWarning();

    makeCurrent();

    ovrPosef eyePose[2];

    glEnable(GL_TEXTURE_2D);

    ovrFrameTiming hmdFrameTiming = ovrHmd_BeginFrame(hmd, 0);
    for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++) {
        ovrEyeType eye = hmd->EyeRenderOrder[eyeIndex];
        eyePose[eye] = ovrHmd_GetEyePose(hmd, eye);


        SbRotation    riftOrientation(  eyePose[eye].Orientation.x,
                                        eyePose[eye].Orientation.y,
                                        eyePose[eye].Orientation.z,
                                        eyePose[eye].Orientation.w);

        camera[eye]->orientation.setValue(riftOrientation);

        SbVec3f riftPosition =   SbVec3f(eyePose[eye].Position.x,
                                         eyePose[eye].Position.y,
                                         eyePose[eye].Position.z);


        //SbVec3f originalPosition(camera[eye]->position.getValue());
        SbVec3f viewAdjust(eyeRenderDesc[eye].ViewAdjust.x,
                                                              eyeRenderDesc[eye].ViewAdjust.y,
                                                              eyeRenderDesc[eye].ViewAdjust.z);

        riftOrientation.multVec(viewAdjust,viewAdjust);

        camera[eye]->position.setValue(basePosition - viewAdjust + riftPosition);

        //Base::Console().Log("Eye(%d) Pos: %f, %f, %f  ViewAdjust:  %f, %f, %f \n",eye, eyePose[eye].Position.x,
        //                                                eyePose[eye].Position.y,
        //                                 eyePose[eye].Position.z,
        //                                 eyeRenderDesc[eye].ViewAdjust.x,
        //                                                      eyeRenderDesc[eye].ViewAdjust.y,
        //                                                      eyeRenderDesc[eye].ViewAdjust.z);

#ifdef USE_SO_OFFSCREEN_RENDERER
        ovrGLTextureData *texData = reinterpret_cast<ovrGLTextureData*>(&eyeTexture[eye]);
        glBindTexture(GL_TEXTURE_2D, texData->TexId);
        renderer->render(rootScene[eye]);
        Q_ASSERT(!glGetError());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     eyeTexture[eye].Header.TextureSize.w,
                     eyeTexture[eye].Header.TextureSize.h,
                     0, GL_RGBA /*GL_BGRA*/, GL_UNSIGNED_BYTE, renderer->getBuffer());
        Q_ASSERT(!glGetError());
        glBindTexture(GL_TEXTURE_2D, 0);
#endif
#ifdef USE_FRAMEBUFFER
        // Clear state pollution from OVR SDK.
        glBindTexture(GL_TEXTURE_2D, 0); // You need this, at least if (hmdDesc.DistortionCaps & ovrDistortion_Chromatic).
        OVR::CAPI::GL::glUseProgram(0); // You need this even more.

        GLint oldfb;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &oldfb);
        // Set up framebuffer for rendering.
        OVR::CAPI::GL::glBindFramebuffer(GL_FRAMEBUFFER_EXT, frameBufferID[eye]);

        m_sceneManager->setSceneGraph(rootScene[eye]);
// m_sceneManager->setCamera(camera[eye]); // SoSceneManager does this implicitly.
        m_sceneManager->render();

        // Continue rendering to the original frame buffer (likely 0, the onscreen buffer).
        OVR::CAPI::GL::glBindFramebuffer(GL_FRAMEBUFFER_EXT, oldfb);
        Q_ASSERT(!glGetError());
#endif

        //camera[eye]->position.setValue(originalPosition);

    }

    // Submit the texture for distortion.
    ovrHmd_EndFrame(hmd, eyePose, eyeTexture);

    // Swap buffers.
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    //ovrHmd_EndFrame(hmd);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0);

    doneCurrent();
}

void CoinRiftWidget::handlingSafetyWarning(void)
{
    // Health and Safety Warning display state.
    ovrHSWDisplayState hswDisplayState;
    ovrHmd_GetHSWDisplayState(hmd, &hswDisplayState);
    if (hswDisplayState.Displayed)
    {
        // Dismiss the warning if the user pressed the appropriate key or if the user
        // is tapping the side of the HMD.
        // If the user has requested to dismiss the warning via keyboard or controller input...
        //if (Util_GetAndResetHSWDismissedState())
            ovrHmd_DismissHSWDisplay(hmd);
        //else
        //{
        //    // Detect a moderate tap on the side of the HMD.
        //    ovrTrackingState ts = ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds());
        //    if (ts.StatusFlags & ovrStatus_OrientationTracked)
        //    {
        //        const OVR::Vector3f v(ts.RawSensorData.Accelerometer.x,
        //                              ts.RawSensorData.Accelerometer.y,
        //                              ts.RawSensorData.Accelerometer.z);
        //        // Arbitrary value and representing moderate tap on the side of the DK2 Rift.
        //        if (v.LengthSq() > 250.f)
        //            ovrHmd_DismissHSWDisplay(hmd);
        //    }
        //}
    }

}


#ifdef BUILD_RIFT_TEST_MAIN

int main(int argc, char *argv[])
{
    SoDB::init();

    QApplication app(argc, argv);
    qAddPostRoutine(cleanup);

    // Moved here because of https://developer.oculusvr.com/forums/viewtopic.php?f=17&t=7915&p=108503#p108503
    // Init libovr.
    if (!ovr_Initialize()) {
        qDebug() << "Could not initialize Oculus SDK.";
        exit(1);
    }

    CoinRiftWidget window;
    window.show();

    // An example scene.
    static const char * inlineSceneGraph[] = {
        "#Inventor V2.1 ascii\n",
        "\n",
        "Separator {\n",
        "  Rotation { rotation 1 0 0  0.3 }\n",
        "  Cone { }\n",
        "  BaseColor { rgb 1 0 0 }\n",
        "  Scale { scaleFactor .7 .7 .7 }\n",
        "  Cube { }\n",
        "\n",
        "  DrawStyle { style LINES }\n",
        "  ShapeHints { vertexOrdering COUNTERCLOCKWISE }\n",
        "  Coordinate3 {\n",
        "    point [\n",
        "       -2 -2 1.1,  -2 -1 1.1,  -2  1 1.1,  -2  2 1.1,\n",
        "       -1 -2 1.1,  -1 -1 1.1,  -1  1 1.1,  -1  2 1.1\n",
        "        1 -2 1.1,   1 -1 1.1,   1  1 1.1,   1  2 1.1\n",
        "        2 -2 1.1,   2 -1 1.1,   2  1 1.1,   2  2 1.1\n",
        "      ]\n",
        "  }\n",
        "\n",
        "  Complexity { value 0.7 }\n",
        "  NurbsSurface {\n",
        "     numUControlPoints 4\n",
        "     numVControlPoints 4\n",
        "     uKnotVector [ 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 ]\n",
        "     vKnotVector [ 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 ]\n",
        "  }\n",
        "}\n",
        NULL
    };

    SoInput in;
    in.setStringArray(inlineSceneGraph);

    window.setSceneGraph(SoDB::readAll(&in));

    return app.exec();
}

#endif //BUILD_RIFT_TEST_MAIN

#endif //BUILD_VR
