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

#include "GlUtils.h"
#include "Shader.h"
#include <iostream>

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

void Shader::UpdateTextureSlot(int slot)
{
    if (mTexSlotPos >= 0) {
        glUniform1i(mTexSlotPos, slot);
    }
}

void Shader::UpdateAlbedoTexSlot(int albedoSlot)
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

void Shader::UpdateNoiseTexSlot(int noiseSlot)
{
    if (mNoisePos >= 0) {
        glUniform1i(mNoisePos, noiseSlot);
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


#ifdef QT_OPENGL_LIB
bool CheckCompileResult(int /* shader */)
{
    return false;
#else
bool CheckCompileResult(int shader)
{
    char log[1024];
    int res = 0;
    GLsizei len;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
    if (res != 0) {
        return false;
    }
    glGetShaderInfoLog(shader, 1020, &len, log);
    if (len > 1020) {
        len = 1020;
    }
    log[len] = 0;
    std::cout << log << std::endl;
    return true;
#endif
}

unsigned int Shader::CompileShader(const char* _vertShader, const char* _fragShader)
{
    vertShader = _vertShader;
    fragShader = _fragShader;
    const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLint res = 0;
    glShaderSource(vertex_shader, 1, &vertShader, NULL);
    glCompileShader(vertex_shader);
    if (CheckCompileResult(vertex_shader)) {
        glDeleteShader(vertex_shader);
        return 0xdeadbeef;
    }

    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragShader, NULL);
    glCompileShader(fragment_shader);
    if (CheckCompileResult(fragment_shader)) {
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
    mSsaoSamplesPos = glGetUniformLocation(shaderId, "ssaoSamples");
    mAlbedoPos = glGetUniformLocation(shaderId, "texAlbedo");
    mPositionPos = glGetUniformLocation(shaderId, "texPosition");
    mNormalPos = glGetUniformLocation(shaderId, "texNormal");
    mSsaoPos = glGetUniformLocation(shaderId, "texSsao");
    mNoisePos = glGetUniformLocation(shaderId, "texNoise");
    mSamplesPos = glGetUniformLocation(shaderId, "ssaoSamples");
    mCurSegmentPos = glGetUniformLocation(shaderId, "curSegment");

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
    out vec3 FragPos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 normalRot;

    void main(void)
    {
        vec4 viewPos = view * model * vec4(aPosition, 1.0);
        FragPos = vec3(model * vec4(aPosition, 1.0));
        Normal = vec3(normalRot * vec4(aNormal, 1.0));
        gl_Position = projection * viewPos;
    }
)";

const char* VertShader3DInvNorm = R"(
    #version 330 core

    layout(location = 0) in vec3 aPosition;
    layout(location = 1) in vec3 aNormal;

    out vec3 Normal;
    out vec3 FragPos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 normalRot;

    void main(void)
    {
        gl_Position = projection * view * model * vec4(aPosition, 1.0);
        FragPos = vec3(model * vec4(aPosition, 1.0));
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
    in vec3 FragPos;

    uniform vec3 lightPos;
    uniform vec3 lightColor;
    uniform vec3 objectColor;
    uniform vec3 lightAmbient;

    void main()
    {
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
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
    in vec3 FragPos;
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

    out vec3 FragPos;
    out vec3 Normal;

    uniform bool invertedNormals;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        vec4 viewPos = view * model * vec4(aPos, 1.0);
        FragPos = viewPos.xyz;

        mat3 normalMatrix = transpose(inverse(mat3(view * model)));
        Normal = normalMatrix * (invertedNormals ? -aNormal : aNormal);

        gl_Position = projection * viewPos;
    }
)";

const char* FragShaderGeom = R"(
    #version 330 core
    layout (location = 0) out vec4 texAlbedo;
    layout (location = 1) out vec3 texPosition;
    layout (location = 2) out vec3 texNormal;

    in vec3 FragPos;
    in vec3 Normal;

    uniform vec3 objectColor;

    void main()
    {
        // store the fragment position vector in the first gbuffer texture
        texPosition = FragPos;
        // also store the per-fragment normals into the gbuffer
        texNormal = normalize(Normal);
        // and the diffuse per-fragment color
        texAlbedo = vec4(objectColor, 1.0f);
    }
)";

const char* FragShaderSSAO = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 texCoord;

    uniform sampler2D texNoise;
    uniform sampler2D texPosition;
    uniform sampler2D texNormal;

    uniform vec3 ssaoSamples[64];

    // parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
    int kernelSize = 64;
    float radius = 30f;
    float bias = 0.01;

    // tile noise texture over screen based on screen dimensions divided by noise size
    const vec2 noiseScale = vec2(800.0/4.0, 600.0/4.0);

    uniform mat4 projection;

    void main()
    {
        // get input for SSAO algorithm
        vec3 fragPos = texture(texPosition, texCoord).xyz;
        vec3 normal = normalize(texture(texNormal, texCoord).rgb);
        vec3 randomVec = normalize(texture(texNoise, texCoord * noiseScale).xyz);
        // create TBN change-of-basis matrix: from tangent-space to view-space
        vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
        vec3 bitangent = cross(normal, tangent);
        mat3 TBN = mat3(tangent, bitangent, normal);
        // iterate over the sample kernel and calculate occlusion factor
        float occlusion = 0.0;
        for(int i = 0; i < kernelSize; ++i)
        {
            // get sample position
            vec3 samplePos = TBN * ssaoSamples[i];  // from tangent to view-space
            samplePos = fragPos + samplePos * radius;

            // project sample position (to sample texture) (to get position on screen/texture)
            vec4 offset = vec4(samplePos, 1.0);
            offset = projection * offset;         // from view to clip-space
            offset.xyz /= offset.w;               // perspective divide
            offset.xyz = offset.xyz * 0.5 + 0.5;  // transform to range 0.0 - 1.0

            // get sample depth
            float sampleDepth = texture(texPosition, offset.xy).z;  // get depth value of kernel
                                                                        // sample

            // range check & accumulate
            float rangeCheck = smoothstep(0.0, 1.0, radius * 0.1f / abs(sampleDepth - fragPos.z));
            occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
        }
        occlusion = 1.0 - (occlusion / kernelSize);
        FragColor = vec4(occlusion, 0, 0, 1);
    }
)";

const char* FragShaderSSAOLighting = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 texCoord;

    uniform sampler2D texSsao;
    uniform sampler2D texAlbedo;
    uniform sampler2D texPosition;
    uniform sampler2D texNormal;

    uniform vec3 lightPos;
    uniform vec3 lightColor;
    uniform vec3 lightAmbient;

    uniform float lightLinear;

    void main()
    {
        // retrieve data from gbuffer
        vec4 DiffuseA = texture(texAlbedo, texCoord);
        vec3 Diffuse = DiffuseA.rgb;
        vec3 FragPos = texture(texPosition, texCoord).rgb;
        vec3 Normal = texture(texNormal, texCoord).rgb;
        float AmbientOcclusion = texture(texSsao, texCoord).r;

        // then calculate lighting as usual
        vec3 lighting = lightAmbient * Diffuse * AmbientOcclusion;
        vec3 viewDir  = normalize(-FragPos);  // viewpos is (0.0.0)
        // diffuse
        vec3 lightDir = normalize(lightPos - FragPos);
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lightColor;
        // specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
        vec3 specular = lightColor * spec;
        // attenuation
        float distance = length(lightPos - FragPos);
        float attenuation = 1.0 / (1.0 + lightLinear * distance);
        lighting += (diffuse + specular * 0.3) * attenuation;

        FragColor = vec4(lighting, DiffuseA.a);
    }
)";

const char* FragShaderStdLighting = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 texCoord;

    uniform sampler2D texAlbedo;
    uniform sampler2D texPosition;
    uniform sampler2D texNormal;

    uniform vec3 lightPos;
    uniform vec3 lightColor;
    uniform float lightLinear;
    uniform vec3 lightAmbient;

    void main()
    {
        // retrieve data from gbuffer
        vec4 DiffuseA = texture(texAlbedo, texCoord);
        vec3 Diffuse = DiffuseA.rgb;
        vec3 FragPos = texture(texPosition, texCoord).rgb;
        vec3 Normal = texture(texNormal, texCoord).rgb;

        // then calculate lighting as usual
        vec3 lighting  = lightAmbient * Diffuse;
        vec3 viewDir  = normalize(-FragPos); // viewpos is (0.0.0)
        // diffuse
        vec3 lightDir = normalize(lightPos - FragPos);
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lightColor;
        // specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
        vec3 specular = lightColor * spec;
        // attenuation
        float distance = length(lightPos - FragPos);
        float attenuation = 1.0 / (1.0 + lightLinear * distance);
        lighting += (diffuse + specular * 0.3) * attenuation;

        FragColor = vec4(lighting, DiffuseA.a);
    }
)";

const char* FragShaderSSAOBlur = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 texCoord;

    uniform sampler2D texSsao;

    void main()
    {
        vec2 texelSize = 1.0 / vec2(textureSize(texSsao, 0));
        float result = 0.0;
        for (int x = -2; x <= 1; ++x)
        {
            for (int y = -2; y <= 1; ++y)
            {
                vec2 offset = vec2(float(x), float(y)) * texelSize;
                result += texture(texSsao, texCoord + offset).r;
            }
        }
        FragColor = vec4(result / (4.0 * 4.0), 0, 0, 1);
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
