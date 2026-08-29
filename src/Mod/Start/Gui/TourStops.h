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

#include <QList>
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <Base/Bitmask.h>

namespace StartGui
{

extern const QString kWelcomeId;
extern const QString kSketcherWorkbenchId;
extern const QString kNewSketchId;
extern const QString kSelectPlaneId;
extern const QString kExternalProjectionId;
extern const QString kSubShapeBinderId;
extern const QString kReadMoreId;

enum class TourStopExitAction : int
{
    None = 0,
    CreateSketchOnXYPlane = 1 << 0,
    LeaveSketchEditMode = 1 << 1
};

struct TourStop
{
    QWidget* widget = nullptr;
    QString id;
    QString chapterLabel;
    QString headline;
    QString description;
    QStringList commandNames;
    bool highlight = true;
    bool isSubchapter = false;
    QString workbenchName;
    Base::Flags<TourStopExitAction> onExit = TourStopExitAction::None;

    TourStop(
        QWidget* widget = nullptr,
        QString id = {},
        QString chapterLabel = {},
        QString headline = {},
        QString description = {},
        QStringList commandNames = {},
        bool highlight = true,
        bool isSubchapter = false,
        QString workbenchName = {},
        Base::Flags<TourStopExitAction> onExit = TourStopExitAction::None
    );
};

QList<TourStop> buildStops(const QMainWindow* mainWindow);

}  // namespace StartGui

template<>
struct enum_traits<StartGui::TourStopExitAction>: enum_traits<>::allow_bitops
{
};
