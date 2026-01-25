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
#include <array>

class QBoxLayout;
class QLabel;
class QToolButton;

namespace StartGui
{

enum class Theme
{
    Classic,
    Light,
    Dark
};

/// A widget to allow selection of the UI theme (color scheme).
class ThemeSelectorWidget: public QWidget
{
    Q_OBJECT
public:
    explicit ThemeSelectorWidget(QWidget* parent = nullptr);
    bool eventFilter(QObject* object, QEvent* event) override;

protected:
    void themeChanged(Theme newTheme);

private:
    void retranslateUi();
    void setupUi();
    void setupButtons(QBoxLayout* layout);
    void onLinkActivated(const QString& link);
    void preselectThemeFromSystemSettings();

    QLabel* _titleLabel;
    QLabel* _descriptionLabel;
    std::array<QToolButton*, 3> _buttons;
};

}  // namespace StartGui
