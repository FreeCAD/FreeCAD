// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <string_view>

#include <Inventor/nodes/SoPerspectiveCamera.h>

#include "View3DSettings.h"
#include "ViewCAMSimulator.h"
#include "DlgCAMSimulator.h"
#include "Dummy3DViewer.h"

using namespace std::literals;
using namespace Gui;

namespace CAMSimulator
{

View3DSettings::View3DSettings(ParameterGrp::handle hGrp, Dummy3DViewer& view, DlgCAMSimulator& dlg)
    : Gui::View3DSettings(hGrp, &view)
    , mView(view)
    , mDlg(dlg)
{}

static View3DInventorViewer::Background backgroundType(const ParameterGrp& rGrp)
{
    if (rGrp.GetBool("Gradient", true)) {
        return View3DInventorViewer::Background::LinearGradient;
    }
    else if (rGrp.GetBool("RadialGradient", false)) {
        return View3DInventorViewer::Background::RadialGradient;
    }
    else {
        return View3DInventorViewer::Background::NoGradient;
    }
}

static QColor backgroundColor(const ParameterGrp& rGrp)
{
    // see View3DSettings::OnChange

    unsigned long col1 = rGrp.GetUnsigned("BackgroundColor", 3940932863UL);
    unsigned long col2 = rGrp.GetUnsigned("BackgroundColor2", 859006463UL);  // default color (dark blue)
    unsigned long col3 = rGrp.GetUnsigned("BackgroundColor3", 2880160255UL);  // default color
                                                                              // (blue/grey)
    unsigned long col4 = rGrp.GetUnsigned("BackgroundColor4", 1869583359UL);  // default color
                                                                              // (blue/grey)
    float r1, g1, b1, r2, g2, b2, r3, g3, b3, r4, g4, b4;
    r1 = ((col1 >> 24) & 0xff) / 255.0;
    g1 = ((col1 >> 16) & 0xff) / 255.0;
    b1 = ((col1 >> 8) & 0xff) / 255.0;
    r2 = ((col2 >> 24) & 0xff) / 255.0;
    g2 = ((col2 >> 16) & 0xff) / 255.0;
    b2 = ((col2 >> 8) & 0xff) / 255.0;
    r3 = ((col3 >> 24) & 0xff) / 255.0;
    g3 = ((col3 >> 16) & 0xff) / 255.0;
    b3 = ((col3 >> 8) & 0xff) / 255.0;
    r4 = ((col4 >> 24) & 0xff) / 255.0;
    g4 = ((col4 >> 16) & 0xff) / 255.0;
    b4 = ((col4 >> 8) & 0xff) / 255.0;

    // TODO: The new cam simulator currently doesn't support gradient background colors. For now we
    // pick the color in the middle of the screen.

    (void)r4, (void)g4, (void)b4;

    const View3DInventorViewer::Background type = backgroundType(rGrp);
    const bool useMidColor = rGrp.GetBool("UseBackgroundColorMid", false);

    if (type == View3DInventorViewer::Background::NoGradient) {
        return QColor::fromRgbF(r1, g1, b1);
    }
    else if (useMidColor) {
        return QColor::fromRgbF(r3, g3, b3);
    }
    else {
        return QColor::fromRgbF((r2 + r3) / 2, (g2 + g3) / 2, (b2 + b3) / 2);
    }
}

void View3DSettings::OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason)
{
    // apply/overwrite settings to dummy viewer

    const ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);
    if (Reason == "ShowNaviCube"sv) {
        // always hide the navi cube
        mView.setEnabledNaviCube(false);
    }
    else {
        Gui::View3DSettings::OnChange(rCaller, Reason);
    }

    // apply settings to dlg

    if (std::string_view(Reason).starts_with("BackgroundColor") || Reason == "Gradient"sv
        || Reason == "RadialGradient"sv || Reason == "UseBackgroundColorMid"sv) {

        const QColor bg = backgroundColor(rGrp);
        mDlg.setBackgroundColor(bg);
    }
}

}  // namespace CAMSimulator
