// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Gui/Selection/Selection.h>
#include "TaskView.h"


namespace App
{
class Property;
}

namespace Gui
{
class ViewProvider;

namespace TaskView
{

using TaskAppearance_Connection = fastsignals::connection;
class Ui_TaskAppearance;

class TaskAppearance: public TaskBox, public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    explicit TaskAppearance(QWidget* parent = nullptr);
    ~TaskAppearance() override;
    /// Observer message from the Selection
    void OnChange(
        Gui::SelectionSingleton::SubjectType& rCaller,
        Gui::SelectionSingleton::MessageType Reason
    ) override;

private Q_SLOTS:
    void setupConnections();
    void onChangeModeActivated(const QString&);
    void onChangePlotActivated(const QString&);
    void onTransparencyValueChanged(int);
    void onPointSizeValueChanged(int);
    void onLineWidthValueChanged(int);

protected:
    void changeEvent(QEvent* e) override;

private:
    void slotChangedObject(const Gui::ViewProvider&, const App::Property& Prop);
    void setDisplayModes(const std::vector<Gui::ViewProvider*>&);
    void setPointSize(const std::vector<Gui::ViewProvider*>&);
    void setLineWidth(const std::vector<Gui::ViewProvider*>&);
    void setTransparency(const std::vector<Gui::ViewProvider*>&);
    std::vector<Gui::ViewProvider*> getSelection() const;

private:
    QWidget* proxy;
    Ui_TaskAppearance* ui;
    TaskAppearance_Connection connectChangedObject;
};

}  // namespace TaskView
}  // namespace Gui
