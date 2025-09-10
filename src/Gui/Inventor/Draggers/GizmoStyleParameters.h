// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Sayantan Deb <sayantandebin[at]gmail.com>           *
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

#ifndef GUI_GIZMO_STYLE_PARAMETERS_H
#define GUI_GIZMO_STYLE_PARAMETERS_H

#include <Base/Builder3D.h>
#include <Gui/StyleParameters/ParameterManager.h>

namespace Gui::StyleParameters {
	DEFINE_STYLE_PARAMETER(LinearGizmoBaseColor, Base::Color(1.0F, 0.0F, 0.0F));
	DEFINE_STYLE_PARAMETER(LinearGizmoActiveColor, Base::Color(1.0F, 1.0F, 0.0F));

	DEFINE_STYLE_PARAMETER(RotationGizmoBaseColor, Base::Color(1.0F, 0.0F, 0.0F));
	DEFINE_STYLE_PARAMETER(RotationGizmoActiveColor, Base::Color(1.0F, 1.0F, 0.0F));

	DEFINE_STYLE_PARAMETER(DimensionVisualizerColor, Base::Color(0.214F, 0.560F, 0.930F));
}

#endif /* GUI_GIZMO_STYLE_PARAMETERS_H */
