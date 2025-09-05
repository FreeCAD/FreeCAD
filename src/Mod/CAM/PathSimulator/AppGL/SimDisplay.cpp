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
#include "linmath.h"
#include "OpenGlWrapper.h"
#include <cmath>

namespace MillSim
{

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
}

void SimDisplay::CreateFboQuad()
{
    float quadVertices[] = {// a quad that fills the entire screen in Normalized Device Coordinates.
                            // positions   // texCoords
                            -1.0f, 1.0f,  0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                            1.0f,  -1.0f, 1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 1.0f,
                            1.0f,  -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f, 1.0f};

    glGenVertexArrays(1, &mFboQuadVAO);
    glGenBuffers(1, &mFboQuadVBO);
    glBindVertexArray(mFboQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mFboQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void SimDisplay::CreateGBufTex(GLenum texUnit,
                               GLint intFormat,
                               GLenum format,
                               GLenum type,
                               GLuint& texid)
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


    unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0,
                                   GL_COLOR_ATTACHMENT1,
                                   GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, attachments);

    glGenRenderbuffers(1, &mRboDepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, mRboDepthStencil);
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        mWidth,
        mHeight);  // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER,
                              mRboDepthStencil);  // now actually attach it

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           mFboSsaoBlurTexture,
                           0);
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

    mWidth = gWindowSizeW;
    mHeight = gWindowSizeH;
    InitShaders();
    CreateDisplayFbos();
    CreateSsaoFbos();
    CreateFboQuad();

    UpdateProjection();
    displayInitiated = true;
}

void SimDisplay::CleanFbos()
{
    // cleanup frame buffers
    GLDELETE_FRAMEBUFFER(mFbo);
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
}

void SimDisplay::CleanGL()
{
    CleanFbos();

    // cleanup geometry
    GLDELETE_VERTEXARRAY(mFboQuadVAO);
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

    displayInitiated = false;
}

void SimDisplay::PrepareDisplay(vec3 objCenter)
{
    mat4x4_look_at(mMatLookAt, eye, target, upvec);
    mat4x4_translate_in_place(mMatLookAt, mEyeX * mEyeXZFactor, 0, mEyeZ * mEyeXZFactor);
    mat4x4_rotate_X(mMatLookAt, mMatLookAt, mEyeInclination);
    mat4x4_rotate_Z(mMatLookAt, mMatLookAt, mEyeRoration);
    mat4x4_translate_in_place(mMatLookAt, -objCenter[0], -objCenter[1], -objCenter[2]);
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
    shaderFlat.UpdateViewMat(mMatLookAt);
}

void SimDisplay::StartGeometryPass(vec3 objColor, bool invertNormals)
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
    shaderGeom.Activate();
    shaderGeom.UpdateNormalState(invertNormals);
    shaderGeom.UpdateViewMat(mMatLookAt);
    shaderGeom.UpdateObjColor(objColor);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

// A 'closer' geometry pass is similar to std geometry pass, but render the objects
// slightly closer to the camera. This mitigates overlapping faces artifacts.
void SimDisplay::StartCloserGeometryPass(vec3 objColor)
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
    shaderGeomCloser.Activate();
    shaderGeomCloser.UpdateNormalState(false);
    shaderGeomCloser.UpdateViewMat(mMatLookAt);
    shaderGeomCloser.UpdateObjColor(objColor);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

void SimDisplay::RenderLightObject()
{
    shaderFlat.Activate();
    shaderFlat.UpdateObjColor(lightColor);
    mlightObject.render();
}

void SimDisplay::ScaleViewToStock(StockObject* obj)
{
    mMaxStockDim = fmaxf(obj->size[0], obj->size[1]);
    maxFar = mMaxStockDim * 16;
    UpdateProjection();
    vec3_set(eye, 0, 0, 0);
    UpdateEyeFactor(0.1f);
    vec3_set(lightPos, obj->position[0], obj->position[1], obj->position[2] + mMaxStockDim / 3);
    mlightObject.SetPosition(lightPos);
}

void SimDisplay::RenderResult(bool recalculate)
{
    if (mSsaoValid && applySSAO) {
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
    glBindVertexArray(mFboQuadVAO);
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
        glBindVertexArray(mFboQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // blur SSAO texture to remove noise
        glBindFramebuffer(GL_FRAMEBUFFER, mSsaoBlurFbo);
        glClear(GL_COLOR_BUFFER_BIT);
        shaderSSAOBlur.Activate();
        shaderSSAOBlur.UpdateSsaoTexSlot(0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mFboSsaoTexture);
        glBindVertexArray(mFboQuadVAO);
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
    glBindVertexArray(mFboQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
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
    shaderLinePath.UpdateCurSegment(curSegment);
    shaderLinePath.UpdateViewMat(mMatLookAt);
}

void SimDisplay::TiltEye(float tiltStep)
{
    mEyeInclination += tiltStep;
    if (mEyeInclination > pi / 2) {
        mEyeInclination = pi / 2;
    }
    else if (mEyeInclination < -pi / 2) {
        mEyeInclination = -pi / 2;
    }
}

void SimDisplay::RotateEye(float rotStep)
{
    mEyeRoration += rotStep;
    if (mEyeRoration > pi * 2) {
        mEyeRoration -= pi * 2;
    }
    else if (mEyeRoration < 0) {
        mEyeRoration += pi * 2;
    }
    updateDisplay = true;
}

void SimDisplay::MoveEye(float x, float z)
{
    // Exponential calculate maxValue
    // https://forum.freecad.org/viewtopic.php?t=96939
    const float arg1 = 124.938F;
    const float arg2 = 578.754F;
    const float arg3 = -20.7993F;
    float maxValueX = arg1 + arg2 * exp(arg3 * mEyeDistFactor);
    float maxValueZ = maxValueX * 0.4F;

    mEyeX += x;
    if (mEyeX > maxValueX) {
        mEyeX = maxValueX;
    }
    else if (mEyeX < -maxValueX) {
        mEyeX = -maxValueX;
    }
    mEyeZ += z;

    if (mEyeZ > maxValueZ) {
        mEyeZ = maxValueZ;
    }
    else if (mEyeZ < -maxValueZ) {
        mEyeZ = -maxValueZ;
    }
    updateDisplay = true;
}

void SimDisplay::MoveEyeCenter()
{
    mEyeRoration = 0;
    mEyeInclination = pi / 6;
    mEyeX = 0;
    mEyeZ = 0;
    UpdateEyeFactor(0.1f);
}

void SimDisplay::UpdateEyeFactor(float factor)
{
    if (mEyeDistFactor == factor) {
        return;
    }
    updateDisplay = true;
    mEyeDistFactor = factor;
    mEyeXZFactor = factor * maxFar * 0.005f;
    eye[1] = -factor * maxFar;
}

void SimDisplay::UpdateWindowScale()
{
    mWidth = gWindowSizeW;
    mHeight = gWindowSizeH;
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
    CleanFbos();
    CreateDisplayFbos();
    CreateSsaoFbos();
    UpdateProjection();
}

void SimDisplay::UpdateProjection()
{
    // Setup projection
    mat4x4 projmat;
    mat4x4_perspective(projmat, 0.7f, (float)gWindowSizeW / gWindowSizeH, 1.0f, maxFar);
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
    shaderLinePath.UpdateObjColor(pathLineColorPassed);

    projmat[2][2] *= 0.99999F;
    shaderGeomCloser.Activate();
    shaderGeomCloser.UpdateProjectionMat(projmat);
}


}  // namespace MillSim
