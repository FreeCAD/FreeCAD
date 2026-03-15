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

#pragma once

#include <App/Application.h>
#include <Base/ProgressIndicator.h>
#include <Mod/Part/PartGlobal.h>

#include <Message_ProgressIndicator.hxx>

namespace Part
{

class PartExport OCCTProgressIndicator: public Message_ProgressIndicator
{
    Base::ProgressIndicator& baseIndicator;

public:
    OCCTProgressIndicator(Base::ProgressIndicator& indicator)
        : baseIndicator(indicator)
    {}

    Standard_Boolean UserBreak() override
    {
        return baseIndicator.userBreak();
    }

    void Show(const Message_ProgressScope& scope, const Standard_Boolean isForce) override
    {
        float pos = -1;  // negative means indeterminate
        if (!scope.IsInfinite()) {
            pos = static_cast<float>(GetPosition());
        }
        using ShowFlags = Base::ProgressIndicator::ShowFlags;
        baseIndicator.show(pos, isForce ? ShowFlags::Force : ShowFlags::None);
    }

    static OCCTProgressIndicator getAppIndicator()
    {
        return {App::GetApplication().getProgressIndicator()};
    }
};


#if OCC_VERSION_HEX < 0x070600
// Stubs out OCCT Message_ProgressRange for OCCT versions below 7.5
class Message_ProgressRange
{
public:
    bool UserBreak()
    {
        return false;
    }
    void Show([[maybe_unused]] float position, [[maybe_unused]] bool isForce)
    {}
};
#endif

}  // namespace Part
