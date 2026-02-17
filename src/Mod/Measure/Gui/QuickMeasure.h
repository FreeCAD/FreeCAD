// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2023 Pierre-Louis Boyer <development@Ondsel.com>                       *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#ifndef MEASUREGUI_QUICKMEASURE_H
#define MEASUREGUI_QUICKMEASURE_H

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

#endif  // MEASUREGUI_QUICKMEASURE_H
