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

#pragma once

#include <QWidget>
#include <QGroupBox>

class QLabel;
class QPushButton;

namespace StartGui
{

class ThemeSelectorWidget;
class GeneralSettingsWidget;

class FirstStartWidget: public QGroupBox
{
    Q_OBJECT
public:
    explicit FirstStartWidget(QWidget* parent = nullptr);
    bool eventFilter(QObject* object, QEvent* event) override;
    Q_SIGNAL void dismissed();

private:
    void retranslateUi();
    void setupUi();

    ThemeSelectorWidget* _themeSelectorWidget;
    GeneralSettingsWidget* _generalSettingsWidget;

    QLabel* _welcomeLabel;
    QLabel* _descriptionLabel;
    QPushButton* _doneButton;
};

}  // namespace StartGui
