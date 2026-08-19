// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include "PreCompiled.h"

#include <cassert>
#include <exception>

#include "RenderPipeline.h"

namespace Gui
{

std::string_view renderPipelineName(RenderPipeline pipeline) noexcept
{
    switch (pipeline) {
        case RenderPipeline::DrawList:
            return "DrawList";
        case RenderPipeline::LegacyGL:
            return "LegacyGL";
    }

    assert(false && "Unhandled RenderPipeline value");
    std::terminate();
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

}  // namespace Gui
