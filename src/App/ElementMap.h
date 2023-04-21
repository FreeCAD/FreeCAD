// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2018-2022 Zheng, Lei (realthunder)                       *
 *   <realthunder.dev@gmail.com>                                            *
 *   Copyright (c) 2023 FreeCAD Project Association                         *
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

#ifndef DATA_ELEMENTMAP_H
#define DATA_ELEMENTMAP_H

#include "FCGlobal.h"
#include "IndexedName.h"

namespace Data {

static constexpr const char *POSTFIX_TAG = ";:H";
static constexpr const char *POSTFIX_DECIMAL_TAG = ";:T";
static constexpr const char *POSTFIX_EXTERNAL_TAG = ";:X";
static constexpr const char *POSTFIX_CHILD = ";:C";
static constexpr const char *POSTFIX_INDEX = ";:I";
static constexpr const char *POSTFIX_UPPER = ";:U";
static constexpr const char *POSTFIX_LOWER = ";:L";
static constexpr const char *POSTFIX_MOD = ";:M";
static constexpr const char *POSTFIX_GEN = ";:G";
static constexpr const char *POSTFIX_MODGEN = ";:MG";
static constexpr const char *POSTFIX_DUPLICATE = ";D";

} // namespace data

#endif // DATA_ELEMENTMAP_H
