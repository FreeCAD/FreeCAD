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
 ***************************************************************************/

#ifndef __simdisplay_h__
#define __simdisplay_h__

#include "GlUtils.h"
#include "Shader.h"
#include "StockObject.h"
#include "MillPathLine.h"
#include <vector>
#include <random>
#include <algorithm>

namespace MillSim
{

constexpr auto pi = std::numbers::pi_v<float>;

struct Point3D
{
    float x, y, z;
};

class SimDisplay
{
public:
    ~SimDisplay();
    void InitGL();
    void CleanGL();
    void CleanFbos();
    void PrepareDisplay(vec3 objCenter);
    void PrepareFrameBuffer();
    void StartDepthPass();
    void StartGeometryPass(vec3 objColor, bool invertNormals);
    void StartCloserGeometryPass(vec3 objColor);
    void RenderLightObject();
    void ScaleViewToStock(StockObject* obj);
    void RenderResult(bool recalculate);
    void RenderResultStandard();
    void RenderResultSSAO(bool recalculate);
    void SetupLinePathPass(int curSegment, bool isHidden);
    void TiltEye(float tiltStep);
    void RotateEye(float rotStep);
    void MoveEye(float x, float z);
    void MoveEyeCenter();
    void UpdateEyeFactor(float factor);
    void UpdateWindowScale();

    void UpdateProjection();
    float GetEyeFactor()
    {
        return mEyeDistFactor;
    }

public:
    bool applySSAO = false;
    bool updateDisplay = false;
    float maxFar = 100;
    bool displayInitiated = false;

protected:
    void InitShaders();
    void CreateDisplayFbos();
    void CreateSsaoFbos();
    void CreateFboQuad();
    void CreateGBufTex(GLenum texUnit, GLint intFormat, GLenum format, GLenum type, GLuint& texid);
    void UniformHemisphere(vec3& randVec);
    void UniformCircle(vec3& randVec);

protected:
    // shaders
    Shader shader3D, shaderInv3D, shaderFlat, shaderSimFbo;
    Shader shaderGeom, shaderSSAO, shaderSSAOLighting, shaderSSAOBlur;
    Shader shaderGeomCloser;
    Shader shaderLinePath;
    vec3 lightColor = {0.5f, 0.6f, 0.7f};
    vec3 lightPos = {20.0f, 20.0f, 10.0f};
    vec3 ambientCol = {0.2f, 0.2f, 0.25f};
    vec4 pathLineColor = {0.0f, 0.9f, 0.0f, 1.0};
    vec3 pathLineColorPassed = {0.9f, 0.3f, 0.3f};

    vec3 eye = {0, 100, 40};
    vec3 target = {0, 0, 0};
    vec3 upvec = {0, 0, 1};

    mat4x4 mMatLookAt;
    StockObject mlightObject;

    int mWidth;
    int mHeight;

    std::mt19937 generator;
    std::uniform_real_distribution<float> distr01;


    float mEyeDistance = 30;
    float mEyeRoration = 0;
    float mEyeInclination = pi / 6;  // 30 degree
    float mEyeStep = pi / 36;        // 5 degree

    float mMaxStockDim = 100;
    float mEyeDistFactor = 0.0f;
    float mEyeXZFactor = 0.01f;
    float mEyeXZScale = 0;
    float mEyeX = 0.0f;
    float mEyeZ = 0.0f;

    // base frame buffer
    unsigned int mFbo;
    unsigned int mFboColTexture;
    unsigned int mFboPosTexture;
    unsigned int mFboNormTexture;
    unsigned int mRboDepthStencil;
    unsigned int mFboQuadVAO, mFboQuadVBO;

    // ssao frame buffers
    bool mSsaoValid = false;
    std::vector<Point3D> mSsaoKernel;
    unsigned int mSsaoFbo;
    unsigned int mSsaoBlurFbo;
    unsigned int mFboSsaoTexture;
    unsigned int mFboSsaoBlurTexture;
    unsigned int mFboRandTexture;
};

}  // namespace MillSim
#endif  // !__simdisplay_h__
