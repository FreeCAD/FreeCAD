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
        glUniform1i(mAlbedoPos, noiseSlot);
    }
}

void Shader::UpdateSsaoTexSlot(int ssaoSlot)
{
    if (mSsaoPos >= 0) {
        glUniform1i(mSsaoPos, ssaoSlot);
    }
}

void Shader::UpdateKernelVals(int nVals, float *vals)
{
    glUniform3fv(mSamplesLoc, nVals, vals);
}


bool CheckCompileResult(int shader)
{
#ifdef QT_OPENGL_LIB
    return false;
#else
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
    mTexSlotPos = glGetUniformLocation(shaderId, "texSlot");
    mInvertedNormalsPos = glGetUniformLocation(shaderId, "invertedNormals");
    mSsaoSamplesPos = glGetUniformLocation(shaderId, "ssaoSamples");
    mAlbedoPos = glGetUniformLocation(shaderId, "texAlbedo");
    mPositionPos = glGetUniformLocation(shaderId, "texPosition");
    mNormalPos = glGetUniformLocation(shaderId, "texNormal");
    mSsaoPos = glGetUniformLocation(shaderId, "texSsao");
    mNoisePos = glGetUniformLocation(shaderId, "texNoise");
    mSamplesLoc = glGetUniformLocation(shaderId, "ssaoSamples");
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


const char* VertShader3DNorm =
    "#version 330 core  \n"  // ----->   add long remark for a uniform auto formatting

    "layout(location = 0) in vec3 aPosition;  \n"
    "layout(location = 1) in vec3 aNormal;  \n"

    "out vec3 Normal;  \n"
    "out vec3 FragPos;  \n"

    "uniform mat4 model;  \n"
    "uniform mat4 view;  \n"
    "uniform mat4 projection;  \n"
    "uniform mat4 normalRot;  \n"

    "void main(void)  \n"
    "{  \n"
    "    vec4 viewPos = view * model * vec4(aPosition, 1.0);  \n"
    "    FragPos = vec3(model * vec4(aPosition, 1.0));  \n"
    "    Normal = vec3(normalRot * vec4(aNormal, 1.0));  \n"
    "    gl_Position = projection * viewPos;  \n"
    "}  \n";

const char* VertShader3DInvNorm =
    "#version 330 core  \n"

    "layout(location = 0) in vec3 aPosition;  \n"
    "layout(location = 1) in vec3 aNormal;  \n"

    "out vec3 Normal;  \n"
    "out vec3 FragPos;  \n"

    "uniform mat4 model;  \n"
    "uniform mat4 view;  \n"
    "uniform mat4 projection;  \n"
    "uniform mat4 normalRot;  \n"

    "void main(void)  \n"
    "{  \n"
    "    gl_Position = projection * view * model * vec4(aPosition, 1.0);  \n"
    "    FragPos = vec3(model * vec4(aPosition, 1.0));  \n"
    "    Normal = -vec3(normalRot * vec4(aNormal, 1.0));  \n"
    "}  \n";


const char* VertShader2DTex =
    "#version 330 core  \n"

    "layout(location = 0) in vec2 aPosition;  \n"
    "layout(location = 1) in vec2 aTexCoord;  \n"

    "out vec2 texCoord;  \n"

    "uniform mat4 projection;  \n"
    "uniform mat4 model;  \n"

    "void main(void)  \n"
    "{  \n"
    "    gl_Position = projection * model * vec4(aPosition, 0.0, 1.0);  \n"
    "    texCoord = aTexCoord;  \n"
    "}  \n";

const char* FragShader2dTex =
    "#version 330\n"  // ----->   add long remark for a uniform auto formatting

    "out vec4 FragColor;  \n"
    "in vec2 texCoord;  \n"

    "uniform vec3 objectColor;  \n"
    "uniform sampler2D texSlot;  \n"

    "void main()  \n"
    "{  \n"
    "    vec4 texColor = texture(texSlot, texCoord);  \n"
    "    FragColor = vec4(objectColor, 1.0) * texColor;  \n"
    "}  \n";


const char* FragShaderNorm =
    "#version 330\n"  // ----->   add long remark for a uniform auto formatting

    "out vec4 FragColor;  \n"

    "in vec3 Normal;  \n"
    "in vec3 FragPos;  \n"

    "uniform vec3 lightPos;  \n"
    "uniform vec3 lightColor;  \n"
    "uniform vec3 objectColor;  \n"
    "uniform vec3 lightAmbient;  \n"

    "void main()  \n"
    "{  \n"
    "    vec3 norm = normalize(Normal);  \n"
    "    vec3 lightDir = normalize(lightPos - FragPos);  \n"
    "    float diff = max(dot(norm, lightDir), 0.0);  \n"
    "    vec3 diffuse = diff * lightColor;  \n"
    "    vec3 result = (lightAmbient + diffuse) * objectColor;  \n"
    "    FragColor = vec4(result, 1.0);  \n"
    "}  \n";

const char* FragShaderFlat =
    "#version 330\n"  // ----->   add long remark for a uniform auto formatting

    "out vec4 FragColor;  \n"

    "in vec3 Normal;  \n"
    "in vec3 FragPos;  \n"
    "uniform vec3 objectColor;  \n"

    "void main()  \n"
    "{  \n"
    "    FragColor = vec4(objectColor, 1.0); \n"
    "}  \n";


const char* VertShader2DFbo =
    "#version 330 core  \n"  // ----->   add long remark for a uniform auto formatting

    "layout(location = 0) in vec2 aPosition;  \n"
    "layout(location = 1) in vec2 aTexCoord;  \n"

    "out vec2 texCoord;  \n"

    "void main(void)  \n"
    "{  \n"
    "    gl_Position = vec4(aPosition.x, aPosition.y, 0.0, 1.0);  \n"
    "    texCoord = aTexCoord;  \n"
    "}  \n";

const char* FragShader2dFbo =
    "#version 330\n"  // ----->   add long remark for a uniform auto formatting

    "out vec4 FragColor;  \n"
    "in vec2 texCoord;  \n"

    "uniform sampler2D texSlot;  \n"

    "void main()  \n"
    "{  \n"
    "    vec4 tc = texture(texSlot, texCoord);  \n"
    "    FragColor = tc;  \n"
    "}  \n";


const char* VertShaderGeom =
    "#version 330 core  \n"
    "layout (location = 0) in vec3 aPos;  \n"
    "layout (location = 1) in vec3 aNormal;  \n"

    "out vec3 FragPos;  \n"
    "out vec3 Normal;  \n"

    "uniform bool invertedNormals;  \n"

    "uniform mat4 model;  \n"
    "uniform mat4 view;  \n"
    "uniform mat4 projection;  \n"

    "void main()  \n"
    "{  \n"
    "    vec4 viewPos = view * model * vec4(aPos, 1.0);  \n"
    "    FragPos = viewPos.xyz;   \n"

    "    mat3 normalMatrix = transpose(inverse(mat3(view * model)));  \n"
    "    Normal = normalMatrix * (invertedNormals ? -aNormal : aNormal);  \n"

    "    gl_Position = projection * viewPos;  \n"
    "}  \n";

const char* FragShaderGeom =
    "#version 330 core  \n"  // ----->   add long remark for a uniform auto formatting
    "layout (location = 0) out vec4 texAlbedo;  \n"
    "layout (location = 1) out vec3 texPosition;  \n"
    "layout (location = 2) out vec3 texNormal;  \n"

    "in vec3 FragPos;  \n"
    "in vec3 Normal;  \n"

    "uniform vec3 objectColor;  \n"

    "void main()  \n"
    "{      \n"
    // store the fragment position vector in the first gbuffer texture
    "    texPosition = FragPos;  \n"
    // also store the per-fragment normals into the gbuffer
    "    texNormal = normalize(Normal);  \n"
    // and the diffuse per-fragment color
    "    texAlbedo = vec4(objectColor, 1.0f);  \n"
    "}  \n";

const char* FragShaderSSAO =
    "#version 330 core  \n"  // ----->   add long remark for a uniform auto formatting
    "out vec4 FragColor;  \n"

    "in vec2 texCoord;  \n"

    "uniform sampler2D texNoise;  \n"
    "uniform sampler2D texPosition;  \n"
    "uniform sampler2D texNormal;  \n"

    "uniform vec3 ssaoSamples[64];  \n"

    // parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
    "int kernelSize = 64;  \n"
    "float radius = 2.5f;  \n"
    "float bias = 0.025;  \n"

    // tile noise texture over screen based on screen dimensions divided by noise size
    "const vec2 noiseScale = vec2(800.0/4.0, 600.0/4.0);   \n"

    "uniform mat4 projection;  \n"

    "void main()  \n"
    "{  \n"
    // get input for SSAO algorithm
    "    vec3 fragPos = texture(texPosition, texCoord).xyz;  \n"
    "    vec3 normal = normalize(texture(texNormal, texCoord).rgb);  \n"
    "    vec3 randomVec = normalize(texture(texNoise, texCoord * noiseScale).xyz);  \n"
    // create TBN change-of-basis matrix: from tangent-space to view-space
    "    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));  \n"
    "    vec3 bitangent = cross(normal, tangent);  \n"
    "    mat3 TBN = mat3(tangent, bitangent, normal);  \n"
    // iterate over the sample kernel and calculate occlusion factor
    "    float occlusion = 0.0;  \n"
    "    for(int i = 0; i < kernelSize; ++i)  \n"
    "    {  \n"
    // get sample position
    "        vec3 samplePos = TBN * ssaoSamples[i];  \n"  // from tangent to view-space
    "        samplePos = fragPos + samplePos * radius;   \n"

    // project sample position (to sample texture) (to get position on screen/texture)
    "        vec4 offset = vec4(samplePos, 1.0);  \n"
    "        offset = projection * offset;  \n"         // from view to clip-space
    "        offset.xyz /= offset.w;  \n"               // perspective divide
    "        offset.xyz = offset.xyz * 0.5 + 0.5;  \n"  // transform to range 0.0 - 1.0

    // get sample depth
    "        float sampleDepth = texture(texPosition, offset.xy).z;  \n"  // get depth value of kernel
                                                                        // sample

    // range check & accumulate
    "        float rangeCheck = smoothstep(0.0, 1.0, radius * 0.1f / abs(sampleDepth - fragPos.z));  \n"
    "        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;  \n"
    "    }  \n"
    "    occlusion = 1.0 - (occlusion / kernelSize);  \n"
    "    FragColor = vec4(pow(occlusion, 2), 0, 0, 1);  \n"
    "}  \n";

const char* FragShaderSSAOLighting =
    "#version 330 core  \n"
    "out vec4 FragColor;  \n"

    "in vec2 texCoord;  \n"

    "uniform sampler2D texSsao;  \n"
    "uniform sampler2D texAlbedo;  \n"
    "uniform sampler2D texPosition;  \n"
    "uniform sampler2D texNormal;  \n"

    "uniform vec3 lightPos;  \n"
    "uniform vec3 lightColor;  \n"
    "uniform vec3 lightAmbient;  \n"

    "uniform float lightLinear;  \n"

    "void main()  \n"
    "{               \n"
    // retrieve data from gbuffer
    "    vec4 DiffuseA = texture(texAlbedo, texCoord);  \n"
    "    vec3 Diffuse = DiffuseA.rgb;  \n"
    "    vec3 FragPos = texture(texPosition, texCoord).rgb;  \n"
    "    vec3 Normal = texture(texNormal, texCoord).rgb;  \n"
    "    float AmbientOcclusion = texture(texSsao, texCoord).r;  \n"

    // then calculate lighting as usual
    "    vec3 lighting = lightAmbient * Diffuse * AmbientOcclusion;  \n "
    "    vec3 viewDir  = normalize(-FragPos);  \n"  // viewpos is (0.0.0)
    // diffuse
    "    vec3 lightDir = normalize(lightPos - FragPos);  \n"
    "    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lightColor;  \n"
    // specular
    "    vec3 halfwayDir = normalize(lightDir + viewDir);    \n"
    "    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);  \n"
    "    vec3 specular = lightColor * spec;  \n"
    // attenuation
    "    float distance = length(lightPos - FragPos);  \n"
    "    float attenuation = 1.0 / (1.0 + lightLinear * distance);  \n"
    "    lighting += (diffuse + specular * 0.3) * attenuation;  \n"

    "    FragColor = vec4(lighting, DiffuseA.a);  \n"
    "}  \n";

const char* FragShaderStdLighting =
    "#version 330 core  \n"
    "out vec4 FragColor;  \n"

    "in vec2 texCoord;  \n"

    "uniform sampler2D texAlbedo;  \n"
    "uniform sampler2D texPosition;  \n"
    "uniform sampler2D texNormal;  \n"

    "uniform vec3 lightPos;  \n"
    "uniform vec3 lightColor;  \n"
    "uniform float lightLinear;  \n"
    "uniform vec3 lightAmbient;  \n"

    "void main()  \n"
    "{               \n"
    // retrieve data from gbuffer
    "    vec4 DiffuseA = texture(texAlbedo, texCoord);  \n"
    "    vec3 Diffuse = DiffuseA.rgb;  \n"
    "    vec3 FragPos = texture(texPosition, texCoord).rgb;  \n"
    "    vec3 Normal = texture(texNormal, texCoord).rgb;  \n"

    // then calculate lighting as usual
    "    vec3 lighting  = lightAmbient * Diffuse;   \n"
    "    vec3 viewDir  = normalize(-FragPos); // viewpos is (0.0.0)  \n"
    // diffuse
    "    vec3 lightDir = normalize(lightPos - FragPos);  \n"
    "    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lightColor;  \n"
    // specular
    "    vec3 halfwayDir = normalize(lightDir + viewDir);    \n"
    "    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);  \n"
    "    vec3 specular = lightColor * spec;  \n"
    // attenuation
    "    float distance = length(lightPos - FragPos);  \n"
    "    float attenuation = 1.0 / (1.0 + lightLinear * distance);  \n"
    "    lighting += (diffuse + specular * 0.3) * attenuation;  \n"

    "    FragColor = vec4(lighting, DiffuseA.a);  \n"
    "}  \n";

const char* FragShaderSSAOBlur =
    "#version 330 core  \n"
    "out vec4 FragColor;  \n"

    "in vec2 texCoord;  \n"

    "uniform sampler2D texSsao;  \n"

    "void main()   \n"
    "{  \n"
    "    vec2 texelSize = 1.0 / vec2(textureSize(texSsao, 0));  \n"
    "    float result = 0.0;  \n"
    "    for (int x = -2; x <= 1; ++x)   \n"
    "    {  \n"
    "        for (int y = -2; y <= 1; ++y)   \n"
    "        {  \n"
    "            vec2 offset = vec2(float(x), float(y)) * texelSize;  \n"
    "            result += texture(texSsao, texCoord + offset).r;  \n"
    "        }  \n"
    "    }  \n"
    "    FragColor = vec4(result / (4.0 * 4.0), 0, 0, 1);  \n"
    "}    \n";

}  // namespace MillSim