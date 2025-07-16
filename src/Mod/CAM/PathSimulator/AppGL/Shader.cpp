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

#include "GlUtils.h"
#include "Shader.h"
#include <iostream>
#include <Base/Console.h>

namespace MillSim
{

Shader* CurrentShader = nullptr;

Shader::~Shader()
{
    Destroy();
}

void Shader::UpdateModelMat(mat4x4 tmat, mat4x4 nmat)
{
    if (mModelPos >= 0) {
        glUniformMatrix4fv(mModelPos, 1, GL_FALSE, (GLfloat*)tmat);
    }
    if (mNormalRotPos >= 0) {
        glUniformMatrix4fv(mNormalRotPos, 1, GL_FALSE, (GLfloat*)nmat);
    }
}

void Shader::UpdateProjectionMat(mat4x4 mat)
{
    if (mProjectionPos >= 0) {
        glUniformMatrix4fv(mProjectionPos, 1, GL_FALSE, (GLfloat*)mat);
    }
}

void Shader::UpdateViewMat(mat4x4 mat)
{
    if (mViewPos >= 0) {
        glUniformMatrix4fv(mViewPos, 1, GL_FALSE, (GLfloat*)mat);
    }
}

void Shader::UpdateEnvColor(vec3 lightPos, vec3 lightColor, vec3 ambient, float linearity)
{
    if (mLightPosPos >= 0) {
        glUniform3fv(mLightPosPos, 1, lightPos);
    }
    if (mLightColorPos >= 0) {
        glUniform3fv(mLightColorPos, 1, lightColor);
    }
    if (mLightAmbientPos >= 0) {
        glUniform3fv(mLightAmbientPos, 1, ambient);
    }
    if (mLightLinearPos >= 0) {
        glUniform1f(mLightLinearPos, linearity);
    }
}

void Shader::UpdateScreenDimension(int width, int height)
{
    if (mScreenWidthPos >= 0) {
        glUniform1f(mScreenWidthPos, (float)width);
    }
    if (mScreenHeightPos >= 0) {
        glUniform1f(mScreenHeightPos, (float)height);
    }
}

void Shader::UpdateObjColor(vec3 objColor)
{
    if (mObjectColorPos >= 0) {
        glUniform3fv(mObjectColorPos, 1, objColor);
    }
}

void Shader::UpdateObjColorAlpha(vec4 objColor)
{
    if (mObjectColorAlphaPos >= 0) {
        glUniform4fv(mObjectColorAlphaPos, 1, objColor);
    }
}

void Shader::UpdateNormalState(bool isInverted)
{
    if (mInvertedNormalsPos >= 0) {
        glUniform1i(mInvertedNormalsPos, isInverted);
    }
}

void Shader::UpdateSsaoActive(bool isActive)
{
    if (mSsaoActivePos >= 0) {
        glUniform1i(mSsaoActivePos, isActive);
    }
}

void Shader::UpdateTextureSlot(int slot)
{
    if (mTexSlotPos >= 0) {
        glUniform1i(mTexSlotPos, slot);
    }
}

void Shader::UpdateColorTexSlot(int albedoSlot)
{
    if (mAlbedoPos >= 0) {
        glUniform1i(mAlbedoPos, albedoSlot);
    }
}

void Shader::UpdatePositionTexSlot(int positionSlot)
{
    if (mPositionPos >= 0) {
        glUniform1i(mPositionPos, positionSlot);
    }
}

void Shader::UpdateNormalTexSlot(int normalSlot)
{
    if (mNormalPos >= 0) {
        glUniform1i(mNormalPos, normalSlot);
    }
}

void Shader::UpdateRandomTexSlot(int randSlot)
{
    if (mRandTexPos >= 0) {
        glUniform1i(mRandTexPos, randSlot);
    }
}

void Shader::UpdateSsaoTexSlot(int ssaoSlot)
{
    if (mSsaoPos >= 0) {
        glUniform1i(mSsaoPos, ssaoSlot);
    }
}

void Shader::UpdateKernelVals(int nVals, float* vals)
{
    glUniform3fv(mSamplesPos, nVals, vals);
}

void Shader::UpdateCurSegment(int curSeg)
{
    if (mCurSegmentPos >= 0) {
        glUniform1i(mCurSegmentPos, curSeg);
    }
}


bool CheckCompileResult(int shaderId, const char* shaderName, bool isVertex)
{
    char log[1024];
    int res = 0;
    GLsizei len;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &res);
    if (res != 0) {
        return false;
    }
    int headerLen = snprintf(log,
                             48,
                             "Error compiling %s %s shader: ",
                             shaderName,
                             isVertex ? "vertex" : "fragment");
    glGetShaderInfoLog(shaderId, 1020 - headerLen, &len, log + headerLen);
    len += headerLen;
    if (len > 1020) {
        len = 1020;
    }
    log[len] = 0;
    Base::Console().error(log);
    return true;
}

unsigned int
Shader::CompileShader(const char* name, const char* _vertShader, const char* _fragShader)
{
    vertShader = _vertShader;
    fragShader = _fragShader;
    const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLint res = 0;
    glShaderSource(vertex_shader, 1, &vertShader, NULL);
    glCompileShader(vertex_shader);
    if (CheckCompileResult(vertex_shader, name, true)) {
        glDeleteShader(vertex_shader);
        return 0xdeadbeef;
    }

    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragShader, NULL);
    glCompileShader(fragment_shader);
    if (CheckCompileResult(fragment_shader, name, false)) {
        glDeleteShader(fragment_shader);
        glDeleteShader(vertex_shader);
        return 0xdeadbeef;
    }

    shaderId = glCreateProgram();
    glAttachShader(shaderId, vertex_shader);
    glAttachShader(shaderId, fragment_shader);
    glLinkProgram(shaderId);

    glGetProgramiv(shaderId, GL_LINK_STATUS, &res);
    if (res == 0) {
        Destroy();
        return 0xdeadbeef;
    }

    // get all uniform parameters positions
    mModelPos = glGetUniformLocation(shaderId, "model");
    mNormalRotPos = glGetUniformLocation(shaderId, "normalRot");
    mProjectionPos = glGetUniformLocation(shaderId, "projection");
    mViewPos = glGetUniformLocation(shaderId, "view");
    mLightPosPos = glGetUniformLocation(shaderId, "lightPos");
    mLightColorPos = glGetUniformLocation(shaderId, "lightColor");
    mLightLinearPos = glGetUniformLocation(shaderId, "lightLinear");
    mLightAmbientPos = glGetUniformLocation(shaderId, "lightAmbient");
    mObjectColorPos = glGetUniformLocation(shaderId, "objectColor");
    mObjectColorAlphaPos = glGetUniformLocation(shaderId, "objectColorAlpha");
    mTexSlotPos = glGetUniformLocation(shaderId, "texSlot");
    mInvertedNormalsPos = glGetUniformLocation(shaderId, "invertedNormals");
    mSsaoActivePos = glGetUniformLocation(shaderId, "ssaoActive");
    mAlbedoPos = glGetUniformLocation(shaderId, "ColorTex");
    mPositionPos = glGetUniformLocation(shaderId, "PositionTex");
    mNormalPos = glGetUniformLocation(shaderId, "NormalTex");
    mSsaoPos = glGetUniformLocation(shaderId, "AoTex");
    mRandTexPos = glGetUniformLocation(shaderId, "RandTex");
    mSamplesPos = glGetUniformLocation(shaderId, "SampleKernel");
    mCurSegmentPos = glGetUniformLocation(shaderId, "curSegment");
    mScreenWidthPos = glGetUniformLocation(shaderId, "screenWidth");
    mScreenHeightPos = glGetUniformLocation(shaderId, "screenHeight");

    Activate();
    return shaderId;
}

void Shader::Activate()
{
    if (shaderId > 0) {
        glUseProgram(shaderId);
    }
    CurrentShader = this;
}

void Shader::Destroy()
{
    if (shaderId == 0) {
        return;
    }
    glDeleteProgram(shaderId);
    shaderId = 0;
}


const char* VertShader3DNorm = R"(
    #version 330 core

    layout(location = 0) in vec3 aPosition;
    layout(location = 1) in vec3 aNormal;

    out vec3 Normal;
    out vec3 Position;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 normalRot;

    void main(void)
    {
        vec4 viewPos = view * model * vec4(aPosition, 1.0);
        Position = vec3(model * vec4(aPosition, 1.0));
        Normal = vec3(normalRot * vec4(aNormal, 1.0));
        gl_Position = projection * viewPos;
    }
)";

const char* VertShader3DInvNorm = R"(
    #version 330 core

    layout(location = 0) in vec3 aPosition;
    layout(location = 1) in vec3 aNormal;

    out vec3 Normal;
    out vec3 Position;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 normalRot;

    void main(void)
    {
        gl_Position = projection * view * model * vec4(aPosition, 1.0);
        Position = vec3(model * vec4(aPosition, 1.0));
        Normal = -vec3(normalRot * vec4(aNormal, 1.0));
    }
)";


const char* VertShader2DTex = R"(
    #version 330 core

    layout(location = 0) in vec2 aPosition;
    layout(location = 1) in vec2 aTexCoord;

    out vec2 texCoord;

    uniform mat4 projection;
    uniform mat4 model;

    void main(void)
    {
        gl_Position = projection * model * vec4(aPosition, 0.0, 1.0);
        texCoord = aTexCoord;
    }
)";

const char* FragShader2dTex = R"(
    #version 330

    out vec4 FragColor;
    in vec2 texCoord;

    uniform vec3 objectColor;
    uniform sampler2D texSlot;

    void main()
    {
        vec4 texColor = texture(texSlot, texCoord);
        FragColor = vec4(objectColor, 1.0) * texColor;
    }
)";


const char* FragShaderNorm = R"(
    #version 330

    out vec4 FragColor;

    in vec3 Normal;
    in vec3 Position;

    uniform vec3 lightPos;
    uniform vec3 lightColor;
    uniform vec3 objectColor;
    uniform vec3 lightAmbient;

    void main()
    {
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - Position);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        vec3 result = (lightAmbient + diffuse) * objectColor;
        FragColor = vec4(result, 1.0);
    }
)";

const char* FragShaderFlat = R"(
    #version 330

    out vec4 FragColor;

    in vec3 Normal;
    in vec3 Position;
    uniform vec3 objectColor;

    void main()
    {
        FragColor = vec4(objectColor, 1.0);
    }
)";


const char* VertShader2DFbo = R"(
    #version 330 core

    layout(location = 0) in vec2 aPosition;
    layout(location = 1) in vec2 aTexCoord;

    out vec2 texCoord;

    void main(void)
    {
        gl_Position = vec4(aPosition.x, aPosition.y, 0.0, 1.0);
        texCoord = aTexCoord;
    }
)";

const char* FragShader2dFbo = R"(
    #version 330

    out vec4 FragColor;
    in vec2 texCoord;

    uniform sampler2D texSlot;

    void main()
    {
        vec4 tc = texture(texSlot, texCoord);
        FragColor = tc;
    }
)";

const char* VertShaderGeom = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;

    out vec3 Position;
    out vec3 Normal;

    uniform bool invertedNormals;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        vec4 viewPos = view * model * vec4(aPos, 1.0);
        Position = viewPos.xyz;

        mat3 normalMatrix = transpose(inverse(mat3(view * model)));
        Normal = normalMatrix * (invertedNormals ? -aNormal : aNormal);

        gl_Position = projection * viewPos;
    }
)";

const char* FragShaderGeom = R"(
    #version 330 core
    layout (location = 0) out vec4 ColorTex;
    layout (location = 1) out vec3 PositionTex;
    layout (location = 2) out vec3 NormalTex;

    in vec3 Position;
    in vec3 Normal;

    uniform vec3 objectColor;

    void main()
    {
        // Store position, normal, and diffuse color in textures
        PositionTex = Position;
        NormalTex = normalize(Normal);
        ColorTex = vec4(objectColor, 1.0f);
    }
)";

const char* FragShaderSSAO = R"(
    #version 330 core
    layout (location = 0) out float AoData;

    in vec2 texCoord;

    uniform sampler2D RandTex;
    uniform sampler2D PositionTex;
    uniform sampler2D NormalTex;

    const int kernelSize = 64;
    uniform vec3 SampleKernel[kernelSize];
    uniform float Radius = 20.0f;
    uniform float screenWidth = 800.0;
    uniform float screenHeight = 600.0;

    uniform mat4 projection;

    void main()
    {
        // Create the random tangent space matrix
        vec2 randScale = vec2( screenWidth / 4.0, screenHeight / 4.0 );
        vec3 randDir = normalize( texture(RandTex, texCoord.xy * randScale).xyz );
        vec3 n = normalize( texture(NormalTex, texCoord).xyz );
        vec3 biTang = cross( n, randDir );
        if( length(biTang) < 0.0001 )  // If n and randDir are parallel, n is in x-y plane
            biTang = cross( n, vec3(0,0,1));
        biTang = normalize(biTang);
        vec3 tang = cross(biTang, n);
        mat3 toCamSpace = mat3(tang, biTang, n);

        float occlusionSum = 0.0;
        vec3 camPos = texture(PositionTex, texCoord).xyz;
        for( int i = 0; i < kernelSize; i++ ) {
            vec3 samplePos = camPos + Radius * (toCamSpace * SampleKernel[i]);

            // Project point
            vec4 p = projection * vec4(samplePos,1);
            p *= 1.0 / p.w;
            p.xyz = p.xyz * 0.5 + 0.5;

            // Access camera space z-coordinate at that point
            float surfaceZ = texture(PositionTex, p.xy).z;
            float zDist = surfaceZ - camPos.z;

            // Count points that ARE occluded
            if( zDist >= 0.0 && zDist <= Radius && surfaceZ > samplePos.z ) occlusionSum += 1.0;
        }

        float occ = occlusionSum / kernelSize;
        AoData = 1.0 - occ;
    }
)";


const char* FragShaderSSAOBlur = R"(
    #version 330 core
    layout (location = 0) out float AoData;

    in vec2 texCoord;

    uniform sampler2D AoTex;

    void main()
    {
        ivec2 pix = ivec2( gl_FragCoord.xy );
        float sum = 0.0;
        for( int x = -1; x <= 2; ++x ) {
            for( int y = -1; y <= 2; y++ ) {
                sum += texelFetch( AoTex, pix + ivec2(x,y), 0).r;
            }
        }

        float ao = sum * (1.0 / 16.0);
        AoData = ao;
    }
)";

const char* FragShaderSSAOLighting = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 texCoord;

    uniform vec3 lightPos;
    uniform vec3 lightColor;
    uniform vec3 lightAmbient;

    uniform bool ssaoActive;

    uniform sampler2D ColorTex;
    uniform sampler2D PositionTex;
    uniform sampler2D NormalTex;
    uniform sampler2D AoTex;

    vec3 ambAndDiffuse( vec3 pos, vec3 norm, vec3 diff, float ao ) {
        ao = pow(ao, 4);
        vec3 ambient = lightAmbient * diff * ao;
        vec3 s = normalize( lightPos - pos);
        float sDotN = max( dot(s,norm), 0.0 );
        return ambient + lightColor * diff * sDotN;
    }

    void main()
    {
        vec3 pos = texture( PositionTex, texCoord ).xyz;
        vec3 norm = texture( NormalTex, texCoord ).xyz;
        vec4 DiffColorA = texture(ColorTex, texCoord);
        vec3 diffColor = DiffColorA.rgb;
        float aoVal = ssaoActive ? texture( AoTex, texCoord).r : 1.0;

        vec3 col = ambAndDiffuse(pos, norm, diffColor, aoVal);
        col = pow(col, vec3(1.0/2.2));

        FragColor = vec4( col, DiffColorA.a );
    }
)";

const char* VertShader3DLine = R"(
    #version 330 core

    layout(location = 0) in vec3 aPosition;
    layout(location = 1) in int aIndex;
    flat out int Index;

    uniform mat4 view;
    uniform mat4 projection;

    void main(void)
    {
        gl_Position = projection * view * vec4(aPosition, 1.0);
        Index = aIndex;
    }
)";

const char* FragShader3DLine = R"(
    #version 330

    out vec4 FragColor;

    flat in int Index;
    uniform vec4 objectColorAlpha;
    uniform vec3 objectColor;
    uniform int curSegment;

    void main()
    {
        if (Index > curSegment) FragColor = objectColorAlpha;
        else FragColor = vec4(objectColor, objectColorAlpha.a);
    }
)";

}  // namespace MillSim
