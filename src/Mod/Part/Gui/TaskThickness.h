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

#include <memory>

#include <Gui/DeferredDialogRejectUtils.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

namespace Gui
{
class LinearGizmo;
class GizmoContainer;
}  // namespace Gui

namespace Part
{
class Thickness;
}
namespace PartGui
{

class ThicknessWidget: public QWidget
{
    Q_OBJECT

public:
    explicit ThicknessWidget(Part::Thickness*, QWidget* parent = nullptr);
    ~ThicknessWidget() override;

    bool accept();
    bool reject();
    void flushPendingRecompute();
    void stopPendingRecompute();
    bool hasOutstandingRecompute() const;
    bool isFaceSelectionActive() const;
    void setDeferredClosePending(bool pending);
    Part::Thickness* getObject() const;

Q_SIGNALS:
    void recomputeSettled();

private:
    void setupConnections();
    void schedulePreviewRecompute();
    void requestPreviewRecompute(bool waitForCompletion);
    void updateRecomputeUi();
    void onSpinOffsetValueChanged(double);
    void onModeTypeActivated(int);
    void onJoinTypeActivated(int);
    void onIntersectionToggled(bool);
    void onSelfIntersectionToggled(bool);
    void onFacesButtonToggled(bool);
    void onUpdateViewToggled(bool);

private:
    void changeEvent(QEvent* e) override;

    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
    Gui::LinearGizmo* linearGizmo = nullptr;
    void setupGizmos();
    void setGizmoPositions();

private:
    class Private;
    Private* d;
};

class TaskThickness: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskThickness(Part::Thickness*);

public:
    void open() override;
    bool accept() override;
    bool reject() override;
    void clicked(int) override;
    Part::Thickness* getObject() const;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    void ensureDeferredRejectConnection();
    void setDeferredRejectPending(bool pending);
    bool rejectNow();

private Q_SLOTS:
    void onRecomputeSettled();

private:
    ThicknessWidget* widget;
    Gui::DeferredDialogRejectState deferredReject;
};

}  // namespace PartGui
