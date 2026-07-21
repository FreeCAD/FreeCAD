// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <optional>
#include <string_view>

#include <FCGlobal.h>

namespace Gui
{

/**
 * Coin's live viewport rendering pipeline.
 *
 * DrawList is experimental and LegacyGL remains the default. If DrawList
 * backend initialization fails, Coin falls back to LegacyGL for the active
 * viewer. Raster capture and save-picture rendering currently use a temporary
 * LegacyGL action; routing them through the selected pipeline is a follow-up.
 * After-main commands are not included in the DrawList GPU ID buffer and use
 * the existing CPU scene-graph picking path instead.
 */
enum class RenderPipeline
{
    LegacyGL,
    DrawList
};

GuiExport std::string_view renderPipelineName(RenderPipeline pipeline) noexcept;
GuiExport std::optional<RenderPipeline> parseRenderPipeline(std::string_view value) noexcept;
GuiExport RenderPipeline parseRenderPipelineOrLegacy(std::string_view value) noexcept;

}  // namespace Gui
