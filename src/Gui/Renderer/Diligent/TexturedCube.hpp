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

#pragma once

#include <array>

#include "RenderDevice.h"
#include "Buffer.h"
#include "RefCntAutoPtr.hpp"
#include "BasicMath.hpp"

namespace Diligent
{

namespace TexturedCube
{

enum VERTEX_COMPONENT_FLAGS : Uint32
{
    VERTEX_COMPONENT_FLAG_NONE     = 0x00,
    VERTEX_COMPONENT_FLAG_POSITION = 0x01,
    VERTEX_COMPONENT_FLAG_NORMAL   = 0x02,
    VERTEX_COMPONENT_FLAG_TEXCOORD = 0x04,

    VERTEX_COMPONENT_FLAG_POS_UV =
        VERTEX_COMPONENT_FLAG_POSITION |
        VERTEX_COMPONENT_FLAG_TEXCOORD,

    VERTEX_COMPONENT_FLAG_POS_NORM_UV =
        VERTEX_COMPONENT_FLAG_POSITION |
        VERTEX_COMPONENT_FLAG_NORMAL |
        VERTEX_COMPONENT_FLAG_TEXCOORD
};
DEFINE_FLAG_ENUM_OPERATORS(VERTEX_COMPONENT_FLAGS);

RefCntAutoPtr<IBuffer>  CreateVertexBuffer(IRenderDevice*         pDevice,
                                           VERTEX_COMPONENT_FLAGS Components,
                                           BIND_FLAGS             BindFlags = BIND_VERTEX_BUFFER,
                                           BUFFER_MODE            Mode      = BUFFER_MODE_UNDEFINED);
RefCntAutoPtr<IBuffer>  CreateIndexBuffer(IRenderDevice* pDevice,
                                          BIND_FLAGS     BindFlags = BIND_INDEX_BUFFER,
                                          BUFFER_MODE    Mode      = BUFFER_MODE_UNDEFINED);
RefCntAutoPtr<ITexture> LoadTexture(IRenderDevice* pDevice, const char* Path);

struct CreatePSOInfo
{
    IRenderDevice*                   pDevice                = nullptr;
    TEXTURE_FORMAT                   RTVFormat              = TEX_FORMAT_UNKNOWN;
    TEXTURE_FORMAT                   DSVFormat              = TEX_FORMAT_UNKNOWN;
    IShaderSourceInputStreamFactory* pShaderSourceFactory   = nullptr;
    const char*                      VSFilePath             = nullptr;
    const char*                      PSFilePath             = nullptr;
    VERTEX_COMPONENT_FLAGS           Components             = VERTEX_COMPONENT_FLAG_NONE;
    LayoutElement*                   ExtraLayoutElements    = nullptr;
    Uint32                           NumExtraLayoutElements = 0;
    Uint8                            SampleCount            = 1;
};
RefCntAutoPtr<IPipelineState> CreatePipelineState(const CreatePSOInfo& CreateInfo);

static constexpr Uint32 NumVertices = 4 * 6;
static constexpr Uint32 NumIndices  = 3 * 2 * 6;

extern const std::array<float3, NumVertices> Positions;
extern const std::array<float2, NumVertices> Texcoords;
extern const std::array<float3, NumVertices> Normals;

extern const std::array<Uint32, NumIndices> Indices;

} // namespace TexturedCube

} // namespace Diligent
