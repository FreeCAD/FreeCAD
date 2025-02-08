// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 The FreeCAD Project Association AISBL               *
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

#ifndef FREECAD_FILEUTILITIES_H
#define FREECAD_FILEUTILITIES_H

class QString;
class QDir;

namespace Start
{

QString getThumbnailsImage();

QString getThumbnailsName();

QDir getThumbnailsParentDir();

QString getThumbnailsDir();

void createThumbnailsDir();

QString getMD5Hash(const QString& path);

QString getUniquePNG(const QString& path);

bool useCachedPNG(const QString& image, const QString& project);

}  // namespace Start

#endif  // FREECAD_FILEUTILITIES_H
