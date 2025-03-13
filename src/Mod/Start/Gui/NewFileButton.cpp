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

#include "NewFileButton.h"
#include <QFont>
#include <QIcon>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <algorithm>

namespace StartGui
{

NewFileButton::NewFileButton(const NewButton& newButton)
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");
    const auto newFileIconSize =
        static_cast<int>(hGrp->GetInt("NewFileIconSize", 48)); // NOLINT

    auto mainLayout = gsl::owner<QHBoxLayout*>(new QHBoxLayout(this));
    mainLayout->setAlignment(Qt::AlignVCenter);
    auto iconLabel = gsl::owner<QLabel*>(new QLabel(this));
    mainLayout->addWidget(iconLabel);
    QIcon baseIcon(newButton.iconPath);
    iconLabel->setPixmap(baseIcon.pixmap(newFileIconSize, newFileIconSize));
    iconLabel->setPixmap(baseIcon.pixmap(newFileIconSize, newFileIconSize));

    auto textLayout = gsl::owner<QVBoxLayout*>(new QVBoxLayout);
    auto textLabelLine1 = gsl::owner<QLabel*>(new QLabel(this));
    textLabelLine1->setText(newButton.heading);
    QFont font = textLabelLine1->font();
    font.setWeight(QFont::Bold);
    textLabelLine1->setFont(font);
    auto textLabelLine2 = gsl::owner<QLabel*>(new QLabel(this));
    textLabelLine2->setText(newButton.description);
    textLabelLine2->setWordWrap(true);
    textLayout->addWidget(textLabelLine1);
    textLayout->addWidget(textLabelLine2);
    textLayout->setSpacing(0);
    mainLayout->addItem(textLayout);
    mainLayout->addStretch();
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

QSize NewFileButton::minimumSizeHint() const
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");
    const auto cardSpacing = static_cast<int>(hGrp->GetInt("FileCardSpacing", 25)); // NOLINT
    const auto newFileIconSize =
        static_cast<int>(hGrp->GetInt("NewFileIconSize", 48)); // NOLINT
    const auto cardLabelWith =
        static_cast<int>(hGrp->GetInt("FileCardLabelWith", 180)); // NOLINT

    auto textLayout = layout()->itemAt(1)->layout();

    auto textLabelLine1 = static_cast<QLabel*>(textLayout->itemAt(0)->widget());
    auto textLabelLine2 = static_cast<QLabel*>(textLayout->itemAt(1)->widget());

    int textHeight =
        textLabelLine1->height() + textLabelLine2->height() + textLayout->spacing();

    int minWidth = newFileIconSize + cardLabelWith + cardSpacing;
    int minHeight = std::max(newFileIconSize, textHeight) + cardSpacing;

    return QSize(minWidth, minHeight);
}

} // namespace StartGui