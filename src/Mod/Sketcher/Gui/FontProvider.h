// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association                        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2 or         *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QMap>
#include <QString>

namespace SketcherGui
{

/// Availible font files, according to OS-specific logic.
/// @return A map of "Font Name" -> "Font Path".
QMap<QString, QString> findAvailableFontFiles();

}  // namespace SketcherGui
