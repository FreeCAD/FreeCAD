// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#pragma once

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

namespace Part
{
class Offset;
}
namespace PartGui
{

class OffsetWidget: public QWidget
{
    Q_OBJECT

public:
    explicit OffsetWidget(Part::Offset*, QWidget* parent = nullptr);
    ~OffsetWidget() override;

    bool accept();
    bool reject();
    Part::Offset* getObject() const;

private:
    void setupConnections();
    void onSpinOffsetValueChanged(double);
    void onModeTypeActivated(int);
    void onJoinTypeActivated(int);
    void onIntersectionToggled(bool);
    void onSelfIntersectionToggled(bool);
    void onFillOffsetToggled(bool);
    void onUpdateViewToggled(bool);

private:
    void changeEvent(QEvent* e) override;

private:
    class Private;
    Private* d;
};

class TaskOffset: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskOffset(Part::Offset*);
    ~TaskOffset() override;

public:
    void open() override;
    bool accept() override;
    bool reject() override;
    void clicked(int) override;
    Part::Offset* getObject() const;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    OffsetWidget* widget;
};

}  // namespace PartGui
