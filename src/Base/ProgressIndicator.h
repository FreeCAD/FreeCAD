// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2025 James Stanley <james@incoherency.co.uk>            *
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

#ifndef BASE_PROGRESSINDICATOR_H
#define BASE_PROGRESSINDICATOR_H

#include "FCGlobal.h"

namespace Base
{

class BaseExport ProgressIndicator
{
public:
    ProgressIndicator();
    ProgressIndicator(const ProgressIndicator&);
    ProgressIndicator(ProgressIndicator&&) = delete;
    ProgressIndicator& operator=(const ProgressIndicator&);
    ProgressIndicator& operator=(ProgressIndicator&&) = delete;
    virtual ~ProgressIndicator();

    // Core interface matching Message_ProgressIndicator
    virtual bool UserBreak()
    {
        return false;
    }

    // Show the progress
    virtual void Show([[maybe_unused]] float position, [[maybe_unused]] bool isForce)
    {}
};

}  // namespace Base

#endif
