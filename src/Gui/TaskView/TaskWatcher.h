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

#include <vector>
#include <QObject>

#include <Gui/Selection/SelectionFilter.h>


namespace Gui
{
namespace TaskView
{

class TaskContent;

/// Father class of watcher classes
class GuiExport TaskWatcher: public QObject, public Gui::SelectionFilter
{
    Q_OBJECT

public:
    explicit TaskWatcher(const char* Filter);
    ~TaskWatcher() override;

    QWidget* addTaskBox(QWidget* widget, bool expandable = true, QWidget* parent = nullptr);
    QWidget* addTaskBox(
        const QPixmap& icon,
        QWidget* widget,
        bool expandable = true,
        QWidget* parent = nullptr
    );
    QWidget* addTaskBoxWithoutHeader(QWidget* widget);

    std::vector<QWidget*>& getWatcherContent();

public:
    /// is called wenn the document or the Selection changes.
    virtual bool shouldShow();

protected:
    /// List of TaskBoxes of that dialog
    std::vector<QWidget*> Content;
};

// --------------------------------------------------------------------------

/// Special watcher class for showing commands dependene on the selection
class GuiExport TaskWatcherCommands: public TaskWatcher
{
    Q_OBJECT

public:
    TaskWatcherCommands(const char* Filter, const char* commands[], const char* name, const char* pixmap);

public:
    /// is called wenn the document or the Selection changes.
    bool shouldShow() override;
};

// --------------------------------------------------------------------------

/// Special watcher class for showing commands when active document is empty
class GuiExport TaskWatcherCommandsEmptyDoc: public TaskWatcherCommands
{
    Q_OBJECT

public:
    TaskWatcherCommandsEmptyDoc(const char* commands[], const char* name, const char* pixmap);

public:
    /// is called wenn the document or the Selection changes.
    bool shouldShow() override;
};

// --------------------------------------------------------------------------

/// Special watcher class for showing commands when there is nothing selected
class GuiExport TaskWatcherCommandsEmptySelection: public TaskWatcherCommands
{
    Q_OBJECT

public:
    TaskWatcherCommandsEmptySelection(const char* commands[], const char* name, const char* pixmap);
    ~TaskWatcherCommandsEmptySelection() override;

public:
    /// is called wenn the document or the Selection changes.
    bool shouldShow() override;
};


}  // namespace TaskView
}  // namespace Gui
