// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <Gui/StyleParameters/ParameterManager.h>

namespace PartDesignGui::StyleParameters
{
DEFINE_STYLE_PARAMETER(PreviewAdditiveColor, Base::Color(0.0F, 1.0F, 0.6F));
DEFINE_STYLE_PARAMETER(PreviewSubtractiveColor, Base::Color(1.0F, 0.0F, 0.0F));
DEFINE_STYLE_PARAMETER(PreviewCommonColor, Base::Color(1.0F, 1.0F, 0.0F));
DEFINE_STYLE_PARAMETER(PreviewDressUpColor, Base::Color(1.0F, 0.0F, 1.0F));

DEFINE_STYLE_PARAMETER(PreviewProfileLineWidth, Gui::StyleParameters::Numeric(4));
DEFINE_STYLE_PARAMETER(PreviewProfileOpacity, Gui::StyleParameters::Numeric(0.0));

DEFINE_STYLE_PARAMETER(PreviewErrorColor, Base::Color(1.0F, 0.0F, 0.0F));
DEFINE_STYLE_PARAMETER(PreviewErrorOpacity, Gui::StyleParameters::Numeric(0.05));

DEFINE_STYLE_PARAMETER(PreviewToolOpacity, Gui::StyleParameters::Numeric(0.05));
DEFINE_STYLE_PARAMETER(PreviewShapeOpacity, Gui::StyleParameters::Numeric(0.2));

DEFINE_STYLE_PARAMETER(PreviewLineWidth, Gui::StyleParameters::Numeric(2));
}  // namespace PartDesignGui::StyleParameters
