// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD project association AISBL
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <Base/ParameterObserver.h>

namespace Gui
{

/** Convenience class to obtain General parameters
 *
 * The parameters are under group "User parameter:BaseApp/Preferences/General"
 */
class GuiExport GeneralParameter: public Base::ParameterObserver
{
public:
    GeneralParameter();
    static GeneralParameter* instance();

    bool getShowExperimentalFeatures() const;
    void setShowExperimentalFeatures(bool value);

    bool getShowDevelopmentPreviewFeatures() const;
    void setShowDevelopmentPreviewFeatures(bool value);

private:
    void setup();
};

}  // namespace Gui
