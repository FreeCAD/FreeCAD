/*
 *  Copyright 2019-2021 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include <vector>

#include "TexturedCube.hpp"
#include "BasicMath.hpp"
#include "TextureUtilities.h"

namespace Diligent
{

namespace TexturedCube
{


//      (-1,+1,+1)________________(+1,+1,+1)                  Z
//               /|              /|                           |      Y
//              / |             / |                           |     /
//             /  |            /  |                           |    /
//            /   |           /   |                           |   /
//(-1,-1,+1) /____|__________/(+1,-1,+1)                      |  /
//           |    |__________|____|                           | /
//           |   /(-1,+1,-1) |    /(+1,+1,-1)                 |----------------> X
//           |  /            |   /
//           | /             |  /
//           |/              | /
//           /_______________|/
//        (-1,-1,-1)       (+1,-1,-1)
//

const std::array<float3, NumVertices> Positions = {
    float3{-1, -1, -1}, float3{-1, +1, -1}, float3{+1, +1, -1}, float3{+1, -1, -1}, // Bottom
    float3{-1, -1, -1}, float3{-1, -1, +1}, float3{+1, -1, +1}, float3{+1, -1, -1}, // Front
    float3{+1, -1, -1}, float3{+1, -1, +1}, float3{+1, +1, +1}, float3{+1, +1, -1}, // Right
    float3{+1, +1, -1}, float3{+1, +1, +1}, float3{-1, +1, +1}, float3{-1, +1, -1}, // Back
    float3{-1, +1, -1}, float3{-1, +1, +1}, float3{-1, -1, +1}, float3{-1, -1, -1}, // Left
    float3{-1, -1, +1}, float3{+1, -1, +1}, float3{+1, +1, +1}, float3{-1, +1, +1}  // Top
};

const std::array<float2, NumVertices> Texcoords = {
    float2{0, 1}, float2{0, 0}, float2{1, 0}, float2{1, 1}, // Bottom
    float2{0, 1}, float2{0, 0}, float2{1, 0}, float2{1, 1}, // Front
    float2{0, 1}, float2{1, 1}, float2{1, 0}, float2{0, 0}, // Right
    float2{0, 1}, float2{0, 0}, float2{1, 0}, float2{1, 1}, // Back
    float2{1, 0}, float2{0, 0}, float2{0, 1}, float2{1, 1}, // Left
    float2{1, 1}, float2{0, 1}, float2{0, 0}, float2{1, 0}  // Top
};

const std::array<float3, NumVertices> Normals = {
    float3{0, 0, -1}, float3{0, 0, -1}, float3{0, 0, -1}, float3{0, 0, -1}, // Bottom
    float3{0, -1, 0}, float3{0, -1, 0}, float3{0, -1, 0}, float3{0, -1, 0}, // Front
    float3{+1, 0, 0}, float3{+1, 0, 0}, float3{+1, 0, 0}, float3{+1, 0, 0}, // Right
    float3{0, +1, 0}, float3{0, +1, 0}, float3{0, +1, 0}, float3{0, +1, 0}, // Back
    float3{-1, 0, 0}, float3{-1, 0, 0}, float3{-1, 0, 0}, float3{-1, 0, 0}, // Left
    float3{0, 0, +1}, float3{0, 0, +1}, float3{0, 0, +1}, float3{0, 0, +1}  // Top
};

// clang-format off
const std::array<Uint32, NumIndices> Indices =
{
    2,0,1,    2,3,0,
    4,6,5,    4,7,6,
    8,10,9,   8,11,10,
    12,14,13, 12,15,14,
    16,18,17, 16,19,18,
    20,21,22, 20,22,23
};
// clang-format on

RefCntAutoPtr<IBuffer> CreateVertexBuffer(IRenderDevice*         pDevice,
                                          VERTEX_COMPONENT_FLAGS Components,
                                          BIND_FLAGS             BindFlags,
                                          BUFFER_MODE            Mode)
{
    VERIFY_EXPR(Components != VERTEX_COMPONENT_FLAG_NONE);
    const Uint32 TotalVertexComponents =
        ((Components & VERTEX_COMPONENT_FLAG_POSITION) ? 3 : 0) +
        ((Components & VERTEX_COMPONENT_FLAG_NORMAL) ? 3 : 0) +
        ((Components & VERTEX_COMPONENT_FLAG_TEXCOORD) ? 2 : 0);

    std::vector<float> VertexData(TotalVertexComponents * NumVertices);

    auto it = VertexData.begin();
    for (Uint32 v = 0; v < NumVertices; ++v)
    {
        if (Components & VERTEX_COMPONENT_FLAG_POSITION)
        {
            const auto& Pos{Positions[v]};
            *(it++) = Pos.x;
            *(it++) = Pos.y;
            *(it++) = Pos.z;
        }
        if (Components & VERTEX_COMPONENT_FLAG_NORMAL)
        {
            const auto& N{Normals[v]};
            *(it++) = N.x;
            *(it++) = N.y;
            *(it++) = N.z;
        }
        if (Components & VERTEX_COMPONENT_FLAG_TEXCOORD)
        {
            const auto& UV{Texcoords[v]};
            *(it++) = UV.x;
            *(it++) = UV.y;
        }
    }
    VERIFY_EXPR(it == VertexData.end());

    BufferDesc VertBuffDesc;
    VertBuffDesc.Name      = "Cube vertex buffer";
    VertBuffDesc.Usage     = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BindFlags;
    VertBuffDesc.Size      = static_cast<Uint64>(VertexData.size() * sizeof(VertexData[0]));
    VertBuffDesc.Mode      = Mode;
    if (Mode != BUFFER_MODE_UNDEFINED)
    {
        VertBuffDesc.ElementByteStride = TotalVertexComponents * sizeof(VertexData[0]);
    }

    BufferData VBData;
    VBData.pData    = VertexData.data();
    VBData.DataSize = VertBuffDesc.Size;
    RefCntAutoPtr<IBuffer> pCubeVertexBuffer;

    pDevice->CreateBuffer(VertBuffDesc, &VBData, &pCubeVertexBuffer);

    return pCubeVertexBuffer;
}

RefCntAutoPtr<IBuffer> CreateIndexBuffer(IRenderDevice* pDevice, BIND_FLAGS BindFlags, BUFFER_MODE Mode)
{
    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = "Cube index buffer";
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BindFlags;
    IndBuffDesc.Size      = sizeof(Indices);
    IndBuffDesc.Mode      = Mode;
    if (Mode != BUFFER_MODE_UNDEFINED)
        IndBuffDesc.ElementByteStride = sizeof(Indices[0]);
    BufferData IBData;
    IBData.pData    = Indices.data();
    IBData.DataSize = NumIndices * sizeof(Indices[0]);
    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(IndBuffDesc, &IBData, &pBuffer);
    return pBuffer;
}

RefCntAutoPtr<ITexture> LoadTexture(IRenderDevice* pDevice, const char* Path)
{
    TextureLoadInfo loadInfo;
    loadInfo.IsSRGB = true;
    RefCntAutoPtr<ITexture> pTex;
    CreateTextureFromFile(Path, loadInfo, pDevice, &pTex);
    return pTex;
}


RefCntAutoPtr<IPipelineState> CreatePipelineState(const CreatePSOInfo& CreateInfo)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
    PipelineResourceLayoutDesc&     ResourceLayout   = PSODesc.ResourceLayout;
    GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    // This is a graphics pipeline
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    PSODesc.Name = "Cube PSO";

    // clang-format off
    // This tutorial will render to a single render target
    GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    GraphicsPipeline.RTVFormats[0]                = CreateInfo.RTVFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    GraphicsPipeline.DSVFormat                    = CreateInfo.DSVFormat;
    // Set the desired number of samples
    GraphicsPipeline.SmplDesc.Count               = CreateInfo.SampleCount;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // Cull back faces
    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
    // Enable depth testing
    GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    // clang-format on
    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.UseCombinedTextureSamplers = true;

    ShaderCI.pShaderSourceStreamFactory = CreateInfo.pShaderSourceFactory;
    // Create a vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube VS";
        ShaderCI.FilePath        = CreateInfo.VSFilePath;
        CreateInfo.pDevice->CreateShader(ShaderCI, &pVS);
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube PS";
        ShaderCI.FilePath        = CreateInfo.PSFilePath;
        CreateInfo.pDevice->CreateShader(ShaderCI, &pPS);
    }

    std::vector<LayoutElement> LayoutElems;

    Uint32 Attrib = 0;
    if (CreateInfo.Components & VERTEX_COMPONENT_FLAG_POSITION)
        LayoutElems.emplace_back(Attrib++, 0, 3, VT_FLOAT32, False);
    if (CreateInfo.Components & VERTEX_COMPONENT_FLAG_NORMAL)
        LayoutElems.emplace_back(Attrib++, 0, 3, VT_FLOAT32, False);
    if (CreateInfo.Components & VERTEX_COMPONENT_FLAG_TEXCOORD)
        LayoutElems.emplace_back(Attrib++, 0, 2, VT_FLOAT32, False);

    for (Uint32 i = 0; i < CreateInfo.NumExtraLayoutElements; ++i)
        LayoutElems.push_back(CreateInfo.ExtraLayoutElements[i]);

    GraphicsPipeline.InputLayout.LayoutElements = LayoutElems.data();
    GraphicsPipeline.InputLayout.NumElements    = static_cast<Uint32>(LayoutElems.size());

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    // Define variable type that will be used by default
    ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    // clang-format off
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    // clang-format on
    ResourceLayout.Variables    = Vars;
    ResourceLayout.NumVariables = _countof(Vars);

    // Define immutable sampler for g_Texture. Immutable samplers should be used whenever possible
    // clang-format off
    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    ImmutableSamplerDesc ImtblSamplers[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SamLinearClampDesc}
    };
    // clang-format on
    ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    RefCntAutoPtr<IPipelineState> pPSO;
    CreateInfo.pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    return pPSO;
}

} // namespace TexturedCube

} // namespace Diligent
