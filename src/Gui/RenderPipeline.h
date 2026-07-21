// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <optional>
#include <string_view>

#include <FCGlobal.h>

namespace Gui
{

enum class RenderPipeline
{
    LegacyGL,
    DrawList
};

GuiExport std::string_view renderPipelineName(RenderPipeline pipeline) noexcept;
GuiExport std::optional<RenderPipeline> parseRenderPipeline(std::string_view value) noexcept;
GuiExport RenderPipeline parseRenderPipelineOrLegacy(std::string_view value) noexcept;

}  // namespace Gui
