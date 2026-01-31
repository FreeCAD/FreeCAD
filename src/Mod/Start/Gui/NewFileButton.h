// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Alfredo Monclus <alfredomonclus@gmail.com>          *
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

#include <QLabel>
#include <QString>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <App/Application.h>

namespace StartGui
{

struct NewButton
{
    QString heading;
    QString description;
    QString iconPath;
};

class NewFileButton: public QPushButton
{
public:
    explicit NewFileButton(const NewButton& newButton);

private:
    int iconSize;
    int labelWidth;
    QHBoxLayout* mainLayout;
    QVBoxLayout* textLayout;
    QLabel* headingLabel;
    QLabel* descriptionLabel;

protected:
    QSize minimumSizeHint() const override;
};

}  // namespace StartGui
