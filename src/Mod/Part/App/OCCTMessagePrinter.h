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

#pragma once

#include <Message_Printer.hxx>
#include <Standard_Handle.hxx>

#include <Mod/Part/PartGlobal.h>

namespace Part
{

class PartExport OCCTMessagePrinter: public Message_Printer
{
public:
    OCCTMessagePrinter() = default;

protected:
    void send(const TCollection_AsciiString& message, Message_Gravity gravity) const override;
};

DEFINE_STANDARD_HANDLE(OCCTMessagePrinter, Message_Printer)

}  // namespace Part
