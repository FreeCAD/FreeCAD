// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Pieter Hijma <info@pieterhijma.net>
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

#include <Message_Gravity.hxx>
#include <TCollection_AsciiString.hxx>

#include <Base/Console.h>

#include "OCCTMessagePrinter.h"

namespace Part
{

void OCCTMessagePrinter::send(const TCollection_AsciiString& message, const Message_Gravity gravity) const
{
    const char* txt = message.ToCString();

    switch (gravity) {
        case Message_Fail:
        case Message_Alarm:
            Base::Console().error("OCCT: %s\n", txt);
            break;
        case Message_Warning:
            Base::Console().warning("OCCT: %s\n", txt);
            break;
        case Message_Info:
            Base::Console().message("OCCT: %s\n", txt);
            break;
        case Message_Trace:
        default:
            Base::Console().message("OCCT(trace): %s\n", txt);
            break;
    }
}

}  // namespace Part
