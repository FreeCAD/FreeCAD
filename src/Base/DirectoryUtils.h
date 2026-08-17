// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD project association AISBL
// SPDX-FileNotice: Part of the FreeCAD project.
/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2 or           *
 *   (at your option) any later version.                                     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <filesystem>

#ifndef FC_GLOBAL_H
# include <FCGlobal.h>
#endif

namespace Base
{

/// Create the parent directories for a path.
/// Returns false if the directories could not be created.
BaseExport bool createParentDirectories(const std::filesystem::path& path);

}  // namespace Base
