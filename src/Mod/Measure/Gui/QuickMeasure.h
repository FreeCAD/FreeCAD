// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Pierre-Louis Boyer <development@Ondsel.com>        *
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

#include <QObject>

#include <Mod/Measure/MeasureGlobal.h>

#include <Gui/Selection/Selection.h>

class QTimer;

namespace Measure
{
class Measurement;
}

namespace MeasureGui
{

class QuickMeasure: public QObject, Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit QuickMeasure(QObject* parent = nullptr);
    ~QuickMeasure() override;
    void print(const QString& message);

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void tryMeasureSelection();

    bool shouldMeasure(const Gui::SelectionChanges& msg) const;
    void addSelectionToMeasurement();
    bool isObjAcceptable(App::DocumentObject* obj);
    void printResult();

    void processSelection();

    Measure::Measurement* measurement;

    QTimer* selectionTimer;
    bool pendingProcessing;
};

}  // namespace MeasureGui
