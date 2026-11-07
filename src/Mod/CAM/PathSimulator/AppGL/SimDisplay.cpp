// SPDX-License-Identifier: LGPL-2.1-or-later AND MIT

/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
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
 *                                                                         *
 *   Portions of this code are taken from:                                 *
 *   "OpenGL 4 Shading Language cookbook" Third edition                    *
 *   Written by: David Wolff                                               *
 *   Published by: <packt> www.packt.com                                   *
 *   License: MIT License                                                  *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

#include "SimDisplay.h"

#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <algorithm>
#include <numbers>

// include this last as the defines can mess up other includes
#include "OpenGlWrapper.h"

namespace CAMSimulator
{

constexpr auto pi = std::numbers::pi_v<float>;

void SimDisplay::InitShaders()
{
    // use shaders
    //   standard diffuse shader
    shader3D.CompileShader("StdDiffuse", VertShader3DNorm, FragShaderNorm);
    shader3D.UpdateEnvColor(lightPos, lightColor, ambientCol, 0.0f);

    //   invarted normal diffuse shader for inner mesh
    shaderInv3D.CompileShader("InvertNormal", VertShader3DInvNorm, FragShaderNorm);
    shaderInv3D.UpdateEnvColor(lightPos, lightColor, ambientCol, 0.0f);

    //   null shader to calculate meshes only (simulation stage)
    shaderFlat.CompileShader("Null", VertShader3DNorm, FragShaderFlat);

    //   texture shader to render Simulator FBO
    shaderSimFbo.CompileShader("Texture", VertShader2DFbo, FragShader2dFbo);
    shaderSimFbo.UpdateTextureSlot(0);

    // geometric shader - generate texture with all geometric info for further processing
    shaderGeom.CompileShader("Geometric", VertShaderGeom, FragShaderGeom);
    shaderGeomCloser.CompileShader("GeomCloser", VertShaderGeom, FragShaderGeom);

    // SSAO shader - generate SSAO info and embed in texture buffer
    shaderSSAO.CompileShader("SSAO", VertShader2DFbo, FragShaderSSAO);
    shaderSSAO.UpdateRandomTexSlot(0);
    shaderSSAO.UpdatePositionTexSlot(1);
    shaderSSAO.UpdateNormalTexSlot(2);

    // SSAO blur shader - smooth generated SSAO texture
    shaderSSAOBlur.CompileShader("Blur", VertShader2DFbo, FragShaderSSAOBlur);
    shaderSSAOBlur.UpdateSsaoTexSlot(0);

    // SSAO lighting shader - apply lightig modified by SSAO calculations
    shaderSSAOLighting.CompileShader("SsaoLighting", VertShader2DFbo, FragShaderSSAOLighting);
    shaderSSAOLighting.UpdateColorTexSlot(0);
    shaderSSAOLighting.UpdatePositionTexSlot(1);
    shaderSSAOLighting.UpdateNormalTexSlot(2);
    shaderSSAOLighting.UpdateSsaoTexSlot(3);
    shaderSSAOLighting.UpdateEnvColor(lightPos, lightColor, ambientCol, 0.01f);

    // Mill Path Line Shader
    shaderLinePath.CompileShader("PathLine", VertShader3DLine, FragShader3DLine);

    // Stock cache shader - erases the cached stock where it has been cut through
    shaderClear.CompileShader("Clear", VertShader2DFbo, FragShaderClear);
}

void SimDisplay::CreateFboQuad()
{
    float quadVertices[] = {// a quad that fills the entire screen in Normalized Device Coordinates.
                            // positions   // texCoords
                            -1.0f, 1.0f,  0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                            1.0f,  -1.0f, 1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 1.0f,
                            1.0f,  -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenBuffers(1, &mFboQuadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mFboQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices[0], GL_STATIC_DRAW);
}

void SimDisplay::SetupVertexAttribs() const
{
    glBindBuffer(GL_ARRAY_BUFFER, mFboQuadVBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void SimDisplay::CreateGBufTex(GLenum texUnit, GLint intFormat, GLenum format, GLenum type, GLuint& texid)
{
    glActiveTexture(texUnit);
    glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D, texid);
    glTexImage2D(GL_TEXTURE_2D, 0, intFormat, mWidth, mHeight, 0, format, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
}

void SimDisplay::UniformHemisphere(vec3& randVec)
{
    float x1 = distr01(generator);
    float x2 = distr01(generator);
    float s = sqrt(1.0f - x1 * x1);
    randVec[0] = cosf(pi * 2 * x2) * s;
    randVec[1] = sinf(pi * 2 * x2) * s;
    randVec[2] = x1;
}

void SimDisplay::UniformCircle(vec3& randVec)
{
    float x = distr01(generator);
    randVec[0] = cosf(pi * 2 * x);
    randVec[1] = sinf(pi * 2 * x);
    randVec[2] = 0;
}

void SimDisplay::CreateDisplayFbos()
{
    // setup frame buffer for simulation
    glGenFramebuffers(1, &mFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

    // a color texture for the frame buffer
    CreateGBufTex(GL_TEXTURE0, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, mFboColTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFboColTexture, 0);

    // a position texture for the frame buffer
    CreateGBufTex(GL_TEXTURE1, GL_RGB32F, GL_RGBA, GL_FLOAT, mFboPosTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mFboPosTexture, 0);

    // a normal texture for the frame buffer
    CreateGBufTex(GL_TEXTURE2, GL_RGB32F, GL_RGBA, GL_FLOAT, mFboNormTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, mFboNormTexture, 0);

    unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, attachments);

    glGenRenderbuffers(1, &mRboDepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, mRboDepthStencil);
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        mWidth,
        mHeight
    );  // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        mRboDepthStencil
    );  // now actually attach it

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mViewVersion++;
}

void SimDisplay::CreateSsaoFbos()
{
    mSsaoValid = true;

    // setup framebuffer for SSAO processing
    glGenFramebuffers(1, &mSsaoFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mSsaoFbo);
    // SSAO color buffer
    CreateGBufTex(GL_TEXTURE0, GL_R16F, GL_RED, GL_FLOAT, mFboSsaoTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFboSsaoTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        mSsaoValid = false;
        return;
    }

    // setup framebuffer for SSAO blur processing
    glGenFramebuffers(1, &mSsaoBlurFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mSsaoBlurFbo);
    CreateGBufTex(GL_TEXTURE0, GL_R16F, GL_RED, GL_FLOAT, mFboSsaoBlurTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFboSsaoBlurTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        mSsaoValid = false;
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // generate sample kernel
    int kernSize = 64;
    for (int i = 0; i < kernSize; i++) {
        vec3 sample;
        UniformHemisphere(sample);
        float scale = ((float)(i * i)) / (kernSize * kernSize);
        float interpScale = 0.1f * (1.0f - scale) + scale;
        vec3_scale(sample, sample, interpScale);
        mSsaoKernel.push_back(*(Point3D*)sample);
    }
    shaderSSAO.Activate();
    shaderSSAO.UpdateKernelVals(mSsaoKernel.size(), &mSsaoKernel[0].x);

    // generate random direction texture
    int randSize = 4 * 4;
    std::vector<Point3D> randDirections;
    for (int i = 0; i < randSize; i++) {
        vec3 randvec;
        UniformCircle(randvec);
        randDirections.push_back(*(Point3D*)randvec);
    }

    glGenTextures(1, &mFboRandTexture);
    glBindTexture(GL_TEXTURE_2D, mFboRandTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &randDirections[0].x);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

SimDisplay::~SimDisplay()
{
    CleanGL();
}

void SimDisplay::InitGL()
{
    if (displayInitiated) {
        return;
    }

    // setup light object
    mlightObject.GenerateBoxStock(-0.5f, -0.5f, -0.5f, 1, 1, 1);

    InitShaders();
    CreateFboQuad();

    displayInitiated = true;

    UpdateWindowScale(800, 600);
}

void SimDisplay::CleanFbos()
{
    // cleanup frame buffers
    GLDELETE_FRAMEBUFFER(mFbo);
    GLDELETE_FRAMEBUFFER(mCacheFbo);
    GLDELETE_FRAMEBUFFER(mSsaoFbo);
    GLDELETE_FRAMEBUFFER(mSsaoBlurFbo);

    // cleanup fbo textures
    GLDELETE_TEXTURE(mFboColTexture);
    GLDELETE_TEXTURE(mFboPosTexture);
    GLDELETE_TEXTURE(mFboNormTexture);
    GLDELETE_TEXTURE(mFboSsaoTexture);
    GLDELETE_TEXTURE(mFboSsaoBlurTexture);
    GLDELETE_TEXTURE(mFboRandTexture);
    GLDELETE_RENDERBUFFER(mRboDepthStencil);
    GLDELETE_TEXTURE(mCacheColTexture);
    GLDELETE_TEXTURE(mCachePosTexture);
    GLDELETE_TEXTURE(mCacheNormTexture);
    GLDELETE_RENDERBUFFER(mCacheRboDepthStencil);
}

void SimDisplay::CleanGL()
{
    CleanFbos();

    // cleanup geometry
    GLDELETE_BUFFER(mFboQuadVBO);

    // cleanup shaders
    shader3D.Destroy();
    shaderInv3D.Destroy();
    shaderFlat.Destroy();
    shaderSimFbo.Destroy();
    shaderGeom.Destroy();
    shaderSSAO.Destroy();
    shaderSSAOLighting.Destroy();
    shaderSSAOBlur.Destroy();
    shaderClear.Destroy();

    displayInitiated = false;
}

void SimDisplay::PrepareFrameBuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void SimDisplay::StartDepthPass()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    shaderFlat.Activate();
    shaderFlat.UpdateViewMat(mMatView);
}

void SimDisplay::StartGeometryPass(const vec3& objColor, bool invertNormals)
{
    glBindFramebuffer(GL_FRAMEBUFFER, mDrawToCache ? mCacheFbo : mFbo);
    shaderGeom.Activate();
    shaderGeom.UpdateNormalState(invertNormals);
    shaderGeom.UpdateViewMat(mMatView);
    shaderGeom.UpdateObjColor(objColor);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

// A 'closer' geometry pass is similar to std geometry pass, but render the objects
// slightly closer to the camera. This mitigates overlapping faces artifacts.
void SimDisplay::StartCloserGeometryPass(const vec3& objColor)
{
    glBindFramebuffer(GL_FRAMEBUFFER, mDrawToCache ? mCacheFbo : mFbo);
    shaderGeomCloser.Activate();
    shaderGeomCloser.UpdateNormalState(false);
    shaderGeomCloser.UpdateViewMat(mMatView);
    shaderGeomCloser.UpdateObjColor(objColor);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

bool SimDisplay::MakeCache()
{
    // made on first use, the same attachments as the frame buffer and as large
    if (mCacheFbo != 0 || mCacheFailed) {
        return !mCacheFailed;
    }
    glGenFramebuffers(1, &mCacheFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mCacheFbo);
    CreateGBufTex(GL_TEXTURE0, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, mCacheColTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mCacheColTexture, 0);
    CreateGBufTex(GL_TEXTURE1, GL_RGB32F, GL_RGBA, GL_FLOAT, mCachePosTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mCachePosTexture, 0);
    CreateGBufTex(GL_TEXTURE2, GL_RGB32F, GL_RGBA, GL_FLOAT, mCacheNormTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, mCacheNormTexture, 0);
    const unsigned int attachments[3]
        = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, attachments);
    glGenRenderbuffers(1, &mCacheRboDepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, mCacheRboDepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mWidth, mHeight);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        mCacheRboDepthStencil
    );
    mCacheFailed = glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
    return !mCacheFailed;
}

void SimDisplay::BeginCacheDraw(bool clear)
{
    // route the following geometry passes into the stock cache
    mDrawToCache = true;
    glBindFramebuffer(GL_FRAMEBUFFER, mCacheFbo);
    if (clear) {
        glClearColor(0, 0, 0, 0);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glStencilMask(0xFF);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void SimDisplay::EndCacheDraw()
{
    mDrawToCache = false;
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
}

void SimDisplay::ClearCacheHoles()
{
    // after the back faces, stencil 0 marks pixels cut through; clear the cache there
    glBindFramebuffer(GL_FRAMEBUFFER, mCacheFbo);
    shaderClear.Activate();
    SetupVertexAttribs();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void SimDisplay::CopyCacheToFrame()
{
    // all three attachments plus depth and stencil, so the tool and the rest draw on top
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mCacheFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFbo);
    const unsigned int attachments[3]
        = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    for (unsigned int attachment : attachments) {
        glReadBuffer(attachment);
        glDrawBuffers(1, &attachment);
        glBlitFramebuffer(0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    glBlitFramebuffer(
        0,
        0,
        mWidth,
        mHeight,
        0,
        0,
        mWidth,
        mHeight,
        GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
        GL_NEAREST
    );
    // the loop narrowed the frame buffer's draw buffers (the cache's read buffer is set each time)
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
    glDrawBuffers(3, attachments);
}

void SimDisplay::RenderLightObject()
{
    shaderFlat.Activate();
    shaderFlat.UpdateObjColor(lightColor);
    mlightObject.render();
}

void SimDisplay::ScaleViewToStock(StockObject* obj)
{
    mMaxStockDimension = std::max(std::max(obj->size[0], obj->size[1]), obj->size[2]);
    vec3_dup(mStockCenter, obj->center);
    mStockRadius = 0.5f * vec3_len(obj->size);
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

void SimDisplay::RenderResult(bool recalculate, bool ssao)
{
    if (!displayInitiated) {
        return;
    }

    if (mSsaoValid && ssao) {
        RenderResultSSAO(recalculate);
    }
    else {
        RenderResultStandard();
    }
}

void SimDisplay::RenderResultStandard()
{
    // set default frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // display the sim result within the FBO
    shaderSSAOLighting.Activate();
    shaderSSAOLighting.UpdateColorTexSlot(0);
    shaderSSAOLighting.UpdatePositionTexSlot(1);
    shaderSSAOLighting.UpdateNormalTexSlot(2);
    shaderSSAOLighting.UpdateSsaoActive(false);
    // shaderSimFbo.Activate();
    SetupVertexAttribs();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mFboColTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mFboPosTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mFboNormTexture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void SimDisplay::RenderResultSSAO(bool recalculate)
{
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    if (recalculate) {
        // generate SSAO texture
        glBindFramebuffer(GL_FRAMEBUFFER, mSsaoFbo);
        shaderSSAO.Activate();
        shaderSSAO.UpdateRandomTexSlot(0);
        shaderSSAO.UpdatePositionTexSlot(1);
        shaderSSAO.UpdateNormalTexSlot(2);
        shaderSSAO.UpdateScreenDimension(mWidth, mHeight);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mFboRandTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mFboPosTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mFboNormTexture);
        SetupVertexAttribs();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // blur SSAO texture to remove noise
        glBindFramebuffer(GL_FRAMEBUFFER, mSsaoBlurFbo);
        glClear(GL_COLOR_BUFFER_BIT);
        shaderSSAOBlur.Activate();
        shaderSSAOBlur.UpdateSsaoTexSlot(0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mFboSsaoTexture);
        shaderSSAOBlur.UpdateScreenDimension(mWidth, mHeight);
        SetupVertexAttribs();
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // lighting pass:
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    shaderSSAOLighting.Activate();
    shaderSSAOLighting.UpdateColorTexSlot(0);
    shaderSSAOLighting.UpdatePositionTexSlot(1);
    shaderSSAOLighting.UpdateNormalTexSlot(2);
    shaderSSAOLighting.UpdateSsaoTexSlot(3);
    shaderSSAOLighting.UpdateSsaoActive(true);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mFboColTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mFboPosTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mFboNormTexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mFboSsaoBlurTexture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    SetupVertexAttribs();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void SimDisplay::SetPathColor(const vec3& normal, const vec3& rapid)
{
    pathLineColor[0] = normal[0];
    pathLineColor[1] = normal[1];
    pathLineColor[2] = normal[2];

    // TODO: Different color for rapid moves is not supported for now.

    (void)rapid;
}

void SimDisplay::SetupLinePathPass(int curSegment, bool isHidden)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDepthFunc(isHidden ? GL_GREATER : GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2);
    shaderLinePath.Activate();
    pathLineColor[3] = isHidden ? 0.1f : 1.0f;
    shaderLinePath.UpdateObjColorAlpha(pathLineColor);
    shaderLinePath.UpdateObjColor(pathLineColorPassed);
    shaderLinePath.UpdateCurSegment(curSegment);
    shaderLinePath.UpdateViewMat(mMatView);
}

void SimDisplay::UpdateWindowScale(int width, int height)
{
    if (!displayInitiated || (width == mWidth && height == mHeight)) {
        return;
    }

    mWidth = width;
    mHeight = height;

    if (mFbo != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
        CleanFbos();
    }

    CreateDisplayFbos();
    CreateSsaoFbos();
    UpdateProjectionMatrix();
}

void SimDisplay::UpdateCamera(const SoCamera& camera)
{
    if (!displayInitiated) {
        return;
    }

    // projection first: the view matrix depends on the camera type
    UpdateCameraProjection(camera);
    UpdateCameraView(camera);
}

void SimDisplay::UpdateCameraView(const SoCamera& camera)
{

    const SbVec3f position = camera.position.getValue();
    const SbRotation orientation = camera.orientation.getValue();

    if (position == mCameraPosition && orientation == mCameraOrientation
        && mViewPerspective == mCameraPerspective) {
        return;
    }

    mCameraPosition = position;
    mCameraOrientation = orientation;

    UpdateViewMatrix();
}

void SimDisplay::UpdateCameraProjection(const SoCamera& camera)
{
    float heightAngle = std::numbers::pi / 4;
    float height = 100.0f;

    const auto perspective = dynamic_cast<const SoPerspectiveCamera*>(&camera);
    const auto orthographic = dynamic_cast<const SoOrthographicCamera*>(&camera);

    // TODO: We can't use the values from the camera here because the dummy viewer never actually
    // renders the scene and therefore the nearDistance and farDistance of the camera are never
    // updated. Figure out a way to update those values without rendering the scene.

#if 0

    const float nearDistance = camera.nearDistance.getValue();
    const float farDistance = camera.farDistance.getValue();

#else

    float nearDistance = 0.0;
    float farDistance = 0.0;

#endif

    // Smallest scene depth beyond the stock center, in mm. The tool reaches well past a small
    // stock.
    constexpr float minSceneDepth = 1000.0f;
    const float depth = std::max(mMaxStockDimension * 10.0f, minSceneDepth);

    if (perspective) {
        heightAngle = perspective->heightAngle.getValue();

        // The far plane is the camera's distance to the stock plus the scene depth, otherwise
        // zooming out from a small stock clips it. The near plane scales with that distance to
        // keep the same depth precision.
        const SbVec3f center(mStockCenter[0], mStockCenter[1], mStockCenter[2]);
        const float distance = (camera.position.getValue() - center).length();
        nearDistance = std::max(mMaxStockDimension * 0.001f, distance * 0.001f);
        farDistance = distance + depth;
    }
    else if (orthographic) {
        height = orthographic->height.getValue();

        nearDistance = -depth;
        farDistance = depth;
    }

    if ((bool)perspective == mCameraPerspective && heightAngle == mCameraHeightAngle
        && height == mCameraHeight && nearDistance == mCameraNearDistance
        && farDistance == mCameraFarDistance) {
        return;
    }

    mCameraPerspective = (bool)perspective;
    mCameraHeightAngle = heightAngle;
    mCameraHeight = height;
    mCameraNearDistance = nearDistance;
    mCameraFarDistance = farDistance;

    UpdateProjectionMatrix();
}

void SimDisplay::UpdateViewMatrix()
{
    SbVec3f up(0, 1, 0);
    mCameraOrientation.multVec(up, up);

    SbVec3f dir(0, 0, -1);
    mCameraOrientation.multVec(dir, dir);

    // An orthographic eye can end up near or past the stock, or far from it, and the clip planes
    // are set about the eye. Move it along the view direction to one stock radius in front of
    // the stock center; the image is the same.
    SbVec3f eye = mCameraPosition;
    mViewPerspective = mCameraPerspective;
    if (!mCameraPerspective) {
        const SbVec3f center(mStockCenter[0], mStockCenter[1], mStockCenter[2]);
        eye += dir * ((center - eye).dot(dir) - mStockRadius);
    }

    const auto target = eye + dir;
    mat4x4_look_at(mMatLookAt, eye.getValue(), target.getValue(), up.getValue());
    mat4x4_mul(mMatView, mMatLookAt, mMatScene);
    mViewVersion++;

    updateDisplay = true;
}

namespace
{

// True when any element of the two matrices differs by more than rounding
bool SceneMatrixChanged(const mat4x4 oldScene, const mat4x4 newScene)
{
    constexpr float sceneChangeTolerance = 1e-5f;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            if (std::fabs(oldScene[c][r] - newScene[c][r])
                > sceneChangeTolerance * (1.f + std::fabs(newScene[c][r]))) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace

void SimDisplay::SetSceneMatrix(const mat4x4 scene)
{
    // A moved scene makes the stock cache stale. Compared with the scene the cache was last made
    // stale at, so that small moves cannot add up unseen.
    if (SceneMatrixChanged(mVersionScene, scene)) {
        mViewVersion++;
        mat4x4_dup(mVersionScene, scene);
    }
    mat4x4_dup(mMatScene, scene);
    mat4x4_mul(mMatView, mMatLookAt, mMatScene);
}

void SimDisplay::UpdateProjectionMatrix()
{
    // Setup projection

    const float aspect = (float)mWidth / mHeight;

    mat4x4 projmat;
    mViewVersion++;

    if (mCameraPerspective) {
        mat4x4_perspective(projmat, mCameraHeightAngle, aspect, mCameraNearDistance, mCameraFarDistance);
    }
    else {
        const float h = mCameraHeight;
        const float w = mCameraHeight * aspect;
        mat4x4_ortho(projmat, -w / 2, w / 2, -h / 2, h / 2, mCameraNearDistance, mCameraFarDistance);
    }

    shader3D.Activate();
    shader3D.UpdateProjectionMat(projmat);
    shaderInv3D.Activate();
    shaderInv3D.UpdateProjectionMat(projmat);
    shaderFlat.Activate();
    shaderFlat.UpdateProjectionMat(projmat);
    shaderGeom.Activate();
    shaderGeom.UpdateProjectionMat(projmat);
    shaderSSAO.Activate();
    shaderSSAO.UpdateProjectionMat(projmat);
    shaderLinePath.Activate();
    shaderLinePath.UpdateProjectionMat(projmat);
    shaderSSAOLighting.Activate();
    shaderSSAOLighting.UpdateProjectionMat(projmat);

    projmat[2][2] *= 0.99999F;
    shaderGeomCloser.Activate();
    shaderGeomCloser.UpdateProjectionMat(projmat);

    updateDisplay = true;
}

}  // namespace CAMSimulator
