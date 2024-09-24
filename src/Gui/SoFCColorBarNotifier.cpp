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

#include "PreCompiled.h"

#include "SoFCColorBarNotifier.h"
#include "SoFCColorBar.h"
#include "Window.h"

using namespace Gui;

SoFCColorBarNotifier& SoFCColorBarNotifier::instance()
{
    static SoFCColorBarNotifier instance;
    return instance;
}

SoFCColorBarNotifier::SoFCColorBarNotifier()
{
    group = Gui::WindowParameter::getDefaultParameter()->GetGroup("View");
    group->Attach(this);
}

void SoFCColorBarNotifier::attach(SoFCColorBarBase* bar)
{
    if (bars.insert(bar).second) {
        bar->ref();
        group->Notify("CbLabelTextSize");
    }
}

void SoFCColorBarNotifier::detach(SoFCColorBarBase* bar)
{
    auto pos = bars.find(bar);
    if (pos != bars.end()) {
        bars.erase(pos);
        bar->unref();
    }
}

void SoFCColorBarNotifier::OnChange(ParameterGrp::SubjectType& caller,
                                    ParameterGrp::MessageType reason)
{
    const ParameterGrp& grp = dynamic_cast<ParameterGrp&>(caller);
    if (strcmp(reason, "CbLabelTextSize") == 0 || strcmp(reason, "CbLabelColor") == 0) {
        SoLabelTextFormat format;
        format.textSize = static_cast<int>(grp.GetInt("CbLabelTextSize", format.textSize));
        format.textColor = static_cast<uint32_t>(grp.GetUnsigned("CbLabelColor", format.textColor));

        for (auto bar : bars) {
            bar->setFormat(format);
        }
    }
}
