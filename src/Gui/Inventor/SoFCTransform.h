// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#pragma once

#include <Inventor/nodes/SoTransform.h>
#include <FCGlobal.h>

namespace Gui
{

/**
 * @class SoFCTransform
 * @brief A temporary workaround for coin3d/coin#534.
 *
 * This class is a workaround for a missing feature to reduce the OpenGL stack size.
 * The issue was reported here: https://github.com/coin3d/coin/issues/534
 * And was merged here: https://github.com/coin3d/coin/pull/535
 *
 * Once this feature is available in all supported versions of Coin3D, this class should
 * be removed and all instances should revert to using SoTransform.
 */
class GuiExport SoFCTransform: public SoTransform
{
    using inherited = SoTransform;

    SO_NODE_HEADER(SoFCTransform);

public:
    static void initClass();
    SoFCTransform();

protected:
    void GLRender(SoGLRenderAction* action) override;
    void callback(SoCallbackAction* action) override;
    void pick(SoPickAction* action) override;
    void getPrimitiveCount(SoGetPrimitiveCountAction* action) override;
    void getBoundingBox(SoGetBoundingBoxAction* action) override;
    void doAction(SoAction* action) override;
};

}  // namespace Gui
