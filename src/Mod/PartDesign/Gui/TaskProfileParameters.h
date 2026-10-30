// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Jesse Trutna <jesse@trutna.com>                    *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                      *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#pragma once

#include <Gui/Selection/Selection.h>
#include <Gui/TaskView/TaskView.h>

class QLabel;
class QToolButton;

namespace App
{
class DocumentObject;
}

namespace PartDesign
{
class ProfileBased;
}

namespace PartDesignGui
{

class ViewProvider;

/// Lets the user re-pick which sketch regions or faces a profile-based feature uses.
class TaskProfileParameters: public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit TaskProfileParameters(ViewProvider* vp, QWidget* parent = nullptr);
    ~TaskProfileParameters() override;

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void setSelecting(bool on);
    void useWholeProfile();
    void pick(App::DocumentObject* obj, const std::string& sub);
    void updateSummary();
    PartDesign::ProfileBased* getFeature() const;

    ViewProvider* vp;
    QLabel* summary;
    QToolButton* selectButton;
    QToolButton* allButton;
    bool selecting = false;

    // Visibility to restore when selection ends
    App::DocumentObject* shownProfile = nullptr;
    App::DocumentObject* shownBase = nullptr;
    bool featureWasVisible = false;
};

}  // namespace PartDesignGui
