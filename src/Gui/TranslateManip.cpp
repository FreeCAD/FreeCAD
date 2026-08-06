// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2009 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#include <Inventor/draggers/SoTranslate2Dragger.h>

#include "TranslateManip.h"

using namespace Gui;

SO_NODE_SOURCE(TranslateManip)

void TranslateManip::initClass()
{
    SO_NODE_INIT_CLASS(TranslateManip, SoTransformManip, "TransformManip");
}

TranslateManip::TranslateManip()
{
    SO_NODE_CONSTRUCTOR(TranslateManip);

    auto myDrag = new SoTranslate2Dragger;
    setDragger(myDrag);
}

TranslateManip::~TranslateManip() = default;
