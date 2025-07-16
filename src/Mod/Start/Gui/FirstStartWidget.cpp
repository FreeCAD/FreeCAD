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
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWidget>
#endif

#include "FirstStartWidget.h"
#include "ThemeSelectorWidget.h"
#include "GeneralSettingsWidget.h"

#include <App/Application.h>
#include <gsl/pointers>

using namespace StartGui;

FirstStartWidget::FirstStartWidget(QWidget* parent)
    : QGroupBox(parent)
    , _themeSelectorWidget {nullptr}
    , _generalSettingsWidget {nullptr}
    , _welcomeLabel {nullptr}
    , _descriptionLabel {nullptr}
    , _doneButton {nullptr}
{
    setObjectName(QLatin1String("FirstStartWidget"));
    setupUi();
    qApp->installEventFilter(this);
}

void FirstStartWidget::setupUi()
{
    auto outerLayout = gsl::owner<QVBoxLayout*>(new QVBoxLayout(this));
    outerLayout->setAlignment(Qt::AlignCenter);
    QString application = QString::fromUtf8(App::Application::Config()["ExeName"].c_str());
    _welcomeLabel = gsl::owner<QLabel*>(new QLabel);
    outerLayout->addWidget(_welcomeLabel);
    _descriptionLabel = gsl::owner<QLabel*>(new QLabel);
    outerLayout->addWidget(_descriptionLabel);

    _themeSelectorWidget = gsl::owner<ThemeSelectorWidget*>(new ThemeSelectorWidget(this));
    _generalSettingsWidget = gsl::owner<GeneralSettingsWidget*>(new GeneralSettingsWidget(this));

    outerLayout->addWidget(_generalSettingsWidget);
    outerLayout->addWidget(_themeSelectorWidget);

    _doneButton = gsl::owner<QPushButton*>(new QPushButton);
    connect(_doneButton, &QPushButton::clicked, this, &FirstStartWidget::dismissed);
    auto buttonBar = gsl::owner<QHBoxLayout*>(new QHBoxLayout);
    buttonBar->setAlignment(Qt::AlignRight);
    buttonBar->addWidget(_doneButton);
    outerLayout->addLayout(buttonBar);

    retranslateUi();
}

bool FirstStartWidget::eventFilter(QObject* object, QEvent* event)
{
    if (object == this && event->type() == QEvent::LanguageChange) {
        this->retranslateUi();
    }
    return QWidget::eventFilter(object, event);
}

void FirstStartWidget::retranslateUi()
{
    _doneButton->setText(tr("Done"));
    QString application = QString::fromUtf8(App::Application::Config()["ExeName"].c_str());
    _welcomeLabel->setText(QLatin1String("<h1>") + tr("Welcome to %1").arg(application)
                           + QLatin1String("</h1>"));
    _descriptionLabel->setText(
        tr("Set your basic configuration options below.") + QLatin1String(" ")
        + tr("These options (and many more) can be changed later in the preferences."));
}
