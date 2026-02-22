// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include "OpenGlWrapper.h"
#include "linmath.h"

namespace MillSim
{
class Shader
{
public:
    Shader()
    {}
    ~Shader();

public:
    unsigned int shaderId = 0;
    void UpdateModelMat(const mat4x4& transformMat, const mat4x4& normalMat);
    void UpdateProjectionMat(const mat4x4& mat);
    void UpdateViewMat(const mat4x4& mat);
    void UpdateEnvColor(const vec3& lightPos, const vec3& lightColor, const vec3& ambient, float linearity);
    void UpdateScreenDimension(int width, int height);
    void UpdateObjColor(const vec3& objColor);
    void UpdateObjColorAlpha(const vec4& objColor);
    void UpdateNormalState(bool isInverted);
    void UpdateSsaoActive(bool isInverted);
    void UpdateTextureSlot(int slot);
    void UpdateColorTexSlot(int albedoSlot);
    void UpdatePositionTexSlot(int positionSlot);
    void UpdateNormalTexSlot(int normalSlot);
    void UpdateRandomTexSlot(int noiseSlot);
    void UpdateSsaoTexSlot(int ssaoSlot);
    void UpdateKernelVals(int nVals, float* vals);
    void UpdateCurSegment(int curSeg);
    unsigned int CompileShader(const char* name, const char* vertShader, const char* fragShader);
    void Activate();
    void Destroy();
    bool IsValid()
    {
        return shaderId > 0;
    }


protected:
    int mModelPos = -1;
    int mNormalRotPos = -1;
    int mProjectionPos = -1;
    int mViewPos = -1;
    int mLightPosPos = -1;
    int mLightColorPos = -1;
    int mLightLinearPos = -1;
    int mLightAmbientPos = -1;
    int mObjectColorPos = -1;
    int mObjectColorAlphaPos = -1;
    int mTexSlotPos = -1;
    int mInvertedNormalsPos = -1;
    int mSsaoActivePos = -1;
    int mAlbedoPos = -1;
    int mPositionPos = -1;
    int mNormalPos = -1;
    int mSsaoPos = -1;
    int mRandTexPos = -1;
    int mSamplesPos = -1;
    int mCurSegmentPos = -1;
    int mScreenWidthPos = -1;
    int mScreenHeightPos = -1;

    const char* vertShader = nullptr;
    const char* fragShader = nullptr;
};

extern Shader* CurrentShader;

extern const char* FragShaderNorm;
extern const char* FragShaderFlat;
extern const char* VertShader3DNorm;
extern const char* VertShader3DInvNorm;
extern const char* VertShader2DTex;
extern const char* FragShader2dTex;
extern const char* VertShader2DFbo;
extern const char* FragShader2dFbo;
extern const char* VertShaderGeom;
extern const char* FragShaderGeom;
extern const char* FragShaderSSAO;
extern const char* FragShaderSSAOLighting;
extern const char* FragShaderStdLighting;
extern const char* FragShaderSSAOBlur;
extern const char* VertShader3DLine;
extern const char* FragShader3DLine;


}  // namespace MillSim
