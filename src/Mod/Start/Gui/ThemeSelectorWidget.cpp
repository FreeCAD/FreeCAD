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
#include <Gui/Application.h>
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
    std::map<Theme, QString> themeMap {{Theme::Classic, tr("Classic")},
                                       {Theme::Dark, tr("Dark theme")},
                                       {Theme::Light, tr("Light theme")}};
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
                 && styleSheetName.contains(QLatin1String("Light theme"),
                                            Qt::CaseSensitivity::CaseInsensitive)) {
            button->setChecked(true);
        }
        else if (theme.first == Theme::Dark
                 && styleSheetName.contains(QLatin1String("Dark theme"),
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
    outerLayout->addWidget(_descriptionLabel);
    outerLayout->addLayout(buttonLayout);
    setupButtons(buttonLayout);
    retranslateUi();
}

void ThemeSelectorWidget::themeChanged(Theme newTheme)
{
    // Run the appropriate preference pack:
    auto prefPackManager = Gui::Application::Instance->prefPackManager();
    switch (newTheme) {
        case Theme::Classic:
            prefPackManager->apply("Classic");
            break;
        case Theme::Dark:
            prefPackManager->apply("Dark theme");
            break;
        case Theme::Light:
            prefPackManager->apply("Light theme");
            break;
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
    _descriptionLabel->setText(tr("More themes are available online using the Addon Manager"));
    _buttons[static_cast<int>(Theme::Dark)]->setText(tr("Dark theme", "Visual theme name"));
    _buttons[static_cast<int>(Theme::Light)]->setText(tr("Light theme", "Visual theme name"));
    _buttons[static_cast<int>(Theme::Classic)]->setText(tr("Classic", "Visual theme name"));
}
