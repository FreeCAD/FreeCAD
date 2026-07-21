// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include "PreCompiled.h"

#include <mutex>

#include <Base/Console.h>

#include "RenderPipeline.h"

namespace Gui
{

std::string_view renderPipelineName(RenderPipeline pipeline) noexcept
{
    switch (pipeline) {
        case RenderPipeline::DrawList:
            return "DrawList";
        case RenderPipeline::LegacyGL:
        default:
            return "LegacyGL";
    }
}

std::optional<RenderPipeline> parseRenderPipeline(std::string_view value) noexcept
{
    if (value == "LegacyGL") {
        return RenderPipeline::LegacyGL;
    }
    if (value == "DrawList") {
        return RenderPipeline::DrawList;
    }
    return std::nullopt;
}

RenderPipeline parseRenderPipelineOrLegacy(std::string_view value) noexcept
{
    if (const auto pipeline = parseRenderPipeline(value)) {
        return *pipeline;
    }

    static std::once_flag warned;
    std::call_once(warned, [value]() {
        Base::Console().warning(
            "Unknown CoinRenderPipeline preference '%.*s'; using LegacyGL\n",
            static_cast<int>(value.size()),
            value.data()
        );
    });
    return RenderPipeline::LegacyGL;
}

}  // namespace Gui
