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

#include <functional>
#include <FCGlobal.h>

namespace App
{

/*!
 * \brief The CleanupProcess class
 */
class AppExport CleanupProcess
{
public:
    /*!
     * \brief registerCleanup
     * \param func
     * This adds a callback function that will be called when the application
     * is about to be shut down.
     * @note A callback function is only about to free resources. Accessing
     * stuff of the application like parameter groups should be avoided.
     */
    static void registerCleanup(const std::function<void()>& func);
    /*!
     * \brief callCleanup
     * Calls the functions that are registered with \a registerCleanup.
     */
    static void callCleanup();
};

}  // namespace App