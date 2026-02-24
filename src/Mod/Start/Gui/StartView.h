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

#include <Mod/Start/StartGlobal.h>
#include <Base/Type.h>
#include <Gui/MDIView.h>

#include "../App/DisplayedFilesModel.h"
#include "../App/RecentFilesModel.h"
#include "../App/ExamplesModel.h"
#include "../App/CustomFolderModel.h"

class QCheckBox;
class QEvent;
class QGridLayout;
class QLabel;
class QListView;
class QMdiSubWindow;
class QScrollArea;
class QStackedWidget;
class QPushButton;

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
    StartView(QWidget* parent);

    const char* getName() const override
    {
        return "StartView";
    }

    void newEmptyFile();
    void newPartDesignFile();
    void openExistingFile();
    void newAssemblyFile();
    void newDraftFile();
    void newArchFile();

    bool onHasMsg(const char* pMsg) const override;

public:
    enum class PostStartBehavior
    {
        switchWorkbench,
        doNotSwitchWorkbench
    };

protected:
    void changeEvent(QEvent* e) override;
    void showEvent(QShowEvent* event) override;

    void configureNewFileButtons(QLayout* layout) const;
    static void configureFileCardWidget(QListView* fileCardWidget);
    void configureRecentFilesListWidget(QListView* recentFilesListWidget, QLabel* recentFilesLabel);
    void configureExamplesListWidget(QListView* examplesListWidget);
    void configureCustomFolderListWidget(QListView* customFolderListWidget);

    void postStart(PostStartBehavior behavior);

    void fileCardSelected(const QModelIndex& index);
    void showOnStartupChanged(bool checked);
    void openFirstStartClicked();
    void firstStartWidgetDismissed();

    QString fileCardStyle() const;

private Q_SLOTS:
    void onMdiSubWindowActivated(QMdiSubWindow* subWindow);

private:
    void retranslateUi();
    void setListViewUpdatesEnabled(bool enabled);

    QStackedWidget* _contents = nullptr;
    Start::RecentFilesModel _recentFilesModel;
    Start::ExamplesModel _examplesModel;
    Start::CustomFolderModel _customFolderModel;
    QLabel* _newFileLabel;
    QLabel* _examplesLabel;
    QLabel* _recentFilesLabel;
    QLabel* _customFolderLabel;
    QPushButton* _openFirstStart;
    QCheckBox* _showOnStartupCheckBox;

    bool isInitialized = false;

};  // namespace StartGui

}  // namespace StartGui
