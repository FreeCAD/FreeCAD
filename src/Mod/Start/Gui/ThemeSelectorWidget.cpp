// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QToolButton>
#endif

#include "ThemeSelectorWidget.h"
#include <gsl/pointers>
#include <App/Application.h>
#include <Gui/Command.h>
#include <Gui/PreferencePackManager.h>

using namespace StartGui;

ThemeSelectorWidget::ThemeSelectorWidget(QWidget* parent)
    : QWidget(parent)
    , _titleLabel {nullptr}
    , _descriptionLabel {nullptr}
    , _buttons {nullptr, nullptr, nullptr}
{
    setObjectName(QLatin1String("ThemeSelectorWidget"));
    setupUi();
    qApp->installEventFilter(this);
}


void ThemeSelectorWidget::setupButtons(QBoxLayout* layout)
{
    if (!layout) {
        return;
    }
    std::map<Theme, QString> themeMap {{Theme::Classic, tr("FreeCAD Classic")},
                                       {Theme::Dark, tr("FreeCAD Dark")},
                                       {Theme::Light, tr("FreeCAD Light")}};
    std::map<Theme, QIcon> iconMap {
        {Theme::Classic, QIcon(QLatin1String(":/thumbnails/Theme_thumbnail_classic.png"))},
        {Theme::Light, QIcon(QLatin1String(":/thumbnails/Theme_thumbnail_light.png"))},
        {Theme::Dark, QIcon(QLatin1String(":/thumbnails/Theme_thumbnail_dark.png"))}};
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/MainWindow");
    auto styleSheetName = QString::fromStdString(hGrp->GetASCII("StyleSheet"));
    for (const auto& theme : themeMap) {
        auto button = gsl::owner<QToolButton*>(new QToolButton());
        button->setCheckable(true);
        button->setAutoExclusive(true);
        button->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextUnderIcon);
        button->setText(theme.second);
        button->setIcon(iconMap[theme.first]);
        button->setIconSize(iconMap[theme.first].actualSize(QSize(256, 256)));
        if (theme.first == Theme::Classic && styleSheetName.isEmpty()) {
            button->setChecked(true);
        }
        else if (theme.first == Theme::Light
                 && styleSheetName.contains(QLatin1String("FreeCAD Light"),
                                            Qt::CaseSensitivity::CaseInsensitive)) {
            button->setChecked(true);
        }
        else if (theme.first == Theme::Dark
                 && styleSheetName.contains(QLatin1String("FreeCAD Dark"),
                                            Qt::CaseSensitivity::CaseInsensitive)) {
            button->setChecked(true);
        }
        connect(button, &QToolButton::clicked, this, [this, theme] {
            themeChanged(theme.first);
        });
        layout->addWidget(button);
        _buttons[static_cast<int>(theme.first)] = button;
    }
}

void ThemeSelectorWidget::setupUi()
{
    auto* outerLayout = gsl::owner<QVBoxLayout*>(new QVBoxLayout(this));
    auto* buttonLayout = gsl::owner<QHBoxLayout*>(new QHBoxLayout);
    _titleLabel = gsl::owner<QLabel*>(new QLabel);
    _descriptionLabel = gsl::owner<QLabel*>(new QLabel);
    outerLayout->addWidget(_titleLabel);
    outerLayout->addLayout(buttonLayout);
    outerLayout->addWidget(_descriptionLabel);
    setupButtons(buttonLayout);
    retranslateUi();
    connect(_descriptionLabel, &QLabel::linkActivated, this, &ThemeSelectorWidget::onLinkActivated);
}

void ThemeSelectorWidget::onLinkActivated(const QString& link)
{
    auto const addonManagerLink = QStringLiteral("freecad:Std_AddonMgr");

    if (link != addonManagerLink) {
        return;
    }

    // Set the user preferences to include only preference packs.
    // This is a quick and dirty way to open Addon Manager with only themes.
    auto pref =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Addons");
    pref->SetInt("PackageTypeSelection", 3);  // 3 stands for Preference Packs
    pref->SetInt("StatusSelection", 0);       // 0 stands for any installation status

    Gui::Application::Instance->commandManager().runCommandByName("Std_AddonMgr");
}

void ThemeSelectorWidget::themeChanged(Theme newTheme)
{
    // Run the appropriate preference pack:
    auto prefPackManager = Gui::Application::Instance->prefPackManager();
    switch (newTheme) {
        case Theme::Classic:
            prefPackManager->apply("FreeCAD Classic");
            break;
        case Theme::Dark:
            prefPackManager->apply("FreeCAD Dark");
            break;
        case Theme::Light:
            prefPackManager->apply("FreeCAD Light");
            break;
    }
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Themes");
    const unsigned long nonExistentColor = -1434171135;
    const unsigned long defaultAccentColor = 1434171135;
    unsigned long longAccentColor1 = hGrp->GetUnsigned("ThemeAccentColor1", nonExistentColor);
    if (longAccentColor1 == nonExistentColor) {
        hGrp->SetUnsigned("ThemeAccentColor1", defaultAccentColor);
        hGrp->SetUnsigned("ThemeAccentColor2", defaultAccentColor);
        hGrp->SetUnsigned("ThemeAccentColor3", defaultAccentColor);
    }
}

bool ThemeSelectorWidget::eventFilter(QObject* object, QEvent* event)
{
    if (object == this && event->type() == QEvent::LanguageChange) {
        this->retranslateUi();
    }
    return QWidget::eventFilter(object, event);
}

void ThemeSelectorWidget::retranslateUi()
{
    _titleLabel->setText(QLatin1String("<h2>") + tr("Theme") + QLatin1String("</h2>"));
    _descriptionLabel->setText(tr("Looking for more themes? You can obtain them using "
                                  "<a href=\"freecad:Std_AddonMgr\">Addon Manager</a>."));
    _buttons[static_cast<int>(Theme::Dark)]->setText(tr("FreeCAD Dark", "Visual theme name"));
    _buttons[static_cast<int>(Theme::Light)]->setText(tr("FreeCAD Light", "Visual theme name"));
    _buttons[static_cast<int>(Theme::Classic)]->setText(tr("FreeCAD Classic", "Visual theme name"));
}
