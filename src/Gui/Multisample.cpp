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
#ifndef _PreComp_
#include <QCoreApplication>
#include <QOpenGLFramebufferObjectFormat>
#endif

#include "Multisample.h"
#include <App/Application.h>
#include <Base/Parameter.h>

using namespace Gui;

// clang-format off
static constexpr auto numMSAA {6};
static constexpr auto idStr {0};
static constexpr auto idEnum {1};
static constexpr auto idMSAA {2};
static constexpr std::array<std::tuple<std::string_view, AntiAliasing, int>, numMSAA> textMSAA {{
    {QT_TRANSLATE_NOOP("Gui::Dialog::DlgSettings3DView", "None"), AntiAliasing::None, 0},
    {QT_TRANSLATE_NOOP("Gui::Dialog::DlgSettings3DView", "Line Smoothing"), AntiAliasing::MSAA1x, 1},
    {QT_TRANSLATE_NOOP("Gui::Dialog::DlgSettings3DView", "MSAA 2x"), AntiAliasing::MSAA2x, 2},
    {QT_TRANSLATE_NOOP("Gui::Dialog::DlgSettings3DView", "MSAA 4x"), AntiAliasing::MSAA4x, 4},
    {QT_TRANSLATE_NOOP("Gui::Dialog::DlgSettings3DView", "MSAA 6x"), AntiAliasing::MSAA6x, 6},
    {QT_TRANSLATE_NOOP("Gui::Dialog::DlgSettings3DView", "MSAA 8x"), AntiAliasing::MSAA8x, 8},
}};
// clang-format on

Multisample::Multisample()
{
    context.setFormat(format);
    context.create();
    offscreen.setFormat(format);
    offscreen.create();
    context.makeCurrent(&offscreen);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool Multisample::testSamples(int num) const
{
    // This is always true
    if (num == 0 || num == 1) {
        return true;
    }

    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
    fboFormat.setSamples(num);
    QOpenGLFramebufferObject fbo(100, 100, fboFormat);  // NOLINT
    return fbo.format().samples() == num;
}

std::vector<std::pair<QString, AntiAliasing>> Multisample::supported() const
{
    std::vector<std::pair<QString, AntiAliasing>> modes;
    std::for_each(textMSAA.begin(), textMSAA.end(), [&modes, this](const auto& mode) {
        if (testSamples(std::get<idMSAA>(mode))) {
            const char* context = "Gui::Dialog::DlgSettings3DView";
            QString str = QCoreApplication::translate(context, std::get<idStr>(mode).data());
            modes.emplace_back(str, std::get<idEnum>(mode));
        }
    });
    return modes;
}

int Multisample::toSamples(AntiAliasing msaa)
{
    auto it = std::find_if(textMSAA.begin(), textMSAA.end(), [msaa](const auto& mode) {
        return std::get<idEnum>(mode) == msaa;
    });
    if (it != textMSAA.end()) {
        return std::get<idMSAA>(*it);
    }
    return 0;
}

AntiAliasing Multisample::toAntiAliasing(int samples)
{
    auto it = std::find_if(textMSAA.begin(), textMSAA.end(), [samples](const auto& mode) {
        return std::get<idMSAA>(mode) == samples;
    });
    if (it != textMSAA.end()) {
        return std::get<idEnum>(*it);
    }
    return AntiAliasing::None;
}

AntiAliasing Multisample::readMSAAFromSettings()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    return AntiAliasing(hGrp->GetInt("AntiAliasing", int(Gui::AntiAliasing::None)));
}

void Multisample::writeMSAAToSettings(AntiAliasing msaa)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    hGrp->SetInt("AntiAliasing", long(msaa));
}
