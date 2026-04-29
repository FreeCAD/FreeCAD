// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Juergen Riegel <juergen.riegel@web.de>
// SPDX-FileCopyrightText: 2011 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 FreeCAD Project Association
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

#include <string>

#include <FCGlobal.h>

namespace App
{
class Document;
class DocumentObject;
}  // namespace App

namespace Gui
{

/** SelectionGate
 * The selection gate allows or disallows selection of certain types.
 * It has to be registered to the selection.
 */
class GuiExport SelectionGate
{
public:
    virtual ~SelectionGate() = default;
    virtual bool allow(App::Document*, App::DocumentObject*, const char*) = 0;

    /**
     * @brief notAllowedReason is a string that sets the message to be
     * displayed in statusbar for cluing the user on why is the selection not
     * allowed. Set this variable in allow() implementation. Enclose the
     * literal into QT_TR_NOOP() for translatability.
     */
    std::string notAllowedReason;
};

}  // namespace Gui
