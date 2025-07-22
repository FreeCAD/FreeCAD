/****************************************************************************
* Copyright (c) 2014 Bastiaan Veelo <Bastiaan a_t Veelo d_o_t net>          *
* Copyright (c) 2014 Jürgen Riegel <FreeCAD@juergen-riegel.net>             *
*                                                                           *
* All rights reserved. Contact me if the below is too restrictive for you.  *
*                                                                           *
* Redistribution and use in source and binary forms, with or without        *
* modification, are permitted provided that the following conditions are    *
* met:                                                                      *
*                                                                           *
* Redistributions of source code must retain the above copyright notice,    *
* this list of conditions and the following disclaimer.                     *
*                                                                           *
* Redistributions in binary form must reproduce the above copyright         *
* notice, this list of conditions and the following disclaimer in the       *
* documentation and/or other materials provided with the distribution.      *
*                                                                           *
* Neither the name of the copyright holder nor the names of its             *
* contributors may be used to endorse or promote products derived from      *
* this software without specific prior written permission.                  *
*                                                                           *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     *
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      *
* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    *
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
\****************************************************************************/
#ifndef GUI_CoinRiftWidget
#define GUI_CoinRiftWidget

#if BUILD_VR

// defines which method to use to render
#define USE_SO_OFFSCREEN_RENDERER

#ifdef USE_SO_OFFSCREEN_RENDERER
# ifdef USE_FRAMEBUFFER
# error "Mutually exclusive options defined."
# endif
#endif

#include <algorithm>
#include <QApplication>
#include <QGLWidget>
#include <QTimer>
#include <QDebug>
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#ifdef USE_SO_OFFSCREEN_RENDERER
# include <Inventor/SoOffscreenRenderer.h>
#endif
#ifdef USE_FRAMEBUFFER
# include <Inventor/SoSceneManager.h>
#endif
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoFrustumCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>

#include <OVR.h>
#include <OVR_Kernel.h>
#include <OVR_Version.h>
#include <../Src/OVR_CAPI_GL.h>
#include <../Src/CAPI/GL/CAPI_GL_Util.h> // For framebuffer functions.



class CoinRiftWidget : public QGLWidget
{
    ovrHmd hmd;
    ovrEyeRenderDesc eyeRenderDesc[2];
    ovrTexture eyeTexture[2];

#ifdef USE_FRAMEBUFFER
    GLuint frameBufferID[2], depthBufferID[2];
    // A SoSceneManager has a SoRenderManager to do the rendering -- should we not use SoRenderManager instead?
    // We are probably not that interested in events. SoSceneManager::setSceneGraph() searches for the camera
    // and sets it in SoRenderManager, but its is actually only used for built-in stereo rendering.
    // FIXME: We should probably eliminate that search...
    SoSceneManager *m_sceneManager;
#endif
#ifdef USE_SO_OFFSCREEN_RENDERER
    SoOffscreenRenderer *renderer;
#endif
    SoSeparator *rootScene[2];
    SoFrustumCamera *camera[2];
    SoNode *scene;
public:
    explicit CoinRiftWidget();
    ~CoinRiftWidget();
    virtual void setSceneGraph(SoNode *sceneGraph);
    void setBase(const SbVec3f &pos){basePosition=pos;}
    void setBackgroundColor(const SbColor &Col);

    SbVec3f    basePosition;
    SbRotation baseOrientation;

protected:
    void handlingSafetyWarning(void);
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

    SbColor BackgroundColor;
    SoTranslation *lightTranslation;
};


#endif //BUILD_VR

#endif // GUI_CoinRiftWidget
