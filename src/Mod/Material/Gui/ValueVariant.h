// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Ladislav Michl
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <QVariant>

#include <Mod/Material/App/MaterialValue.h>
#include <Mod/Material/MaterialGlobal.h>

namespace MatGui
{

MatGuiExport QVariant toQVariant(const Materials::Value& value);
MatGuiExport Materials::Value fromQVariant(const QVariant& variant);

}  // namespace MatGui
