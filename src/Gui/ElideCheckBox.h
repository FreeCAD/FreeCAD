/***************************************************************************
 *   Copyright (c) 2025 Alfredo Monclus <alfredomonclus@gmail.com>         *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

// This custom widget adds the missing ellipsize functionality in QT5

#ifndef ELIDECHECKBOX_H
#define ELIDECHECKBOX_H

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QCheckBox>
#include <QPainter>
#include <QFontMetrics>
#include <QStyleOptionButton>
#endif


class ElideCheckBox : public QCheckBox {
    Q_OBJECT

public:
    explicit ElideCheckBox(QWidget *parent = nullptr);
    ~ElideCheckBox() override = default;

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
};

#endif // ELIDECHECKBOX_H

