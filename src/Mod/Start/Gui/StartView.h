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

#ifndef FREECAD_STARTVIEW_H
#define FREECAD_STARTVIEW_H

#include <Mod/Start/StartGlobal.h>
#include <Base/Type.h>
#include <Gui/MDIView.h>

#include "../App/DisplayedFilesModel.h"
#include "../App/RecentFilesModel.h"
#include "../App/ExamplesModel.h"

class QCheckBox;
class QEvent;
class QGridLayout;
class QLabel;
class QListView;
class QScrollArea;

namespace Gui
{
class Document;
}

namespace StartGui
{

class StartGuiExport StartView: public Gui::MDIView
{
    Q_OBJECT

    TYPESYSTEM_HEADER_WITH_OVERRIDE();  // NOLINT

public:
    StartView(Gui::Document* pcDocument, QWidget* parent);

    const char* getName() const override
    {
        return "StartView";
    }

    void newEmptyFile() const;
    void newPartDesignFile() const;
    void openExistingFile() const;
    void newAssemblyFile() const;
    void newDraftFile() const;
    void newArchFile() const;

public:
    enum class PostStartBehavior
    {
        switchWorkbench,
        doNotSwitchWorkbench
    };

protected:
    void changeEvent(QEvent* e) override;

    void configureNewFileButtons(QLayout* layout) const;
    static void configureFileCardWidget(QListView* fileCardWidget);
    void configureRecentFilesListWidget(QListView* recentFilesListWidget, QLabel* recentFilesLabel);
    void configureExamplesListWidget(QListView* examplesListWidget);

    void postStart(PostStartBehavior behavior) const;

    void fileCardSelected(const QModelIndex& index);

    void showOnStartupChanged(bool checked);
    QString fileCardStyle() const;

private:
    void retranslateUi();

    QScrollArea* _contents = nullptr;
    Start::RecentFilesModel _recentFilesModel;
    Start::ExamplesModel _examplesModel;

    QLabel* _newFileLabel;
    QLabel* _examplesLabel;
    QLabel* _recentFilesLabel;
    QCheckBox* _showOnStartupCheckBox;


};  // namespace StartGui

}  // namespace StartGui

#endif  // FREECAD_STARTVIEW_H
