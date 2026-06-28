// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <QDialog>
#include <memory>
#include "PythonTracing.h"
#include "Window.h"

class QTreeWidgetItem;

namespace Gui
{
namespace Dialog
{
class Ui_DlgMacroExecute;

/**
 * The DlgMacroExecuteImp class implements a dialog to execute or edit a
 * recorded macro.
 * \author Jürgen Riegel
 */
class DlgMacroExecuteImp: public QDialog, public Gui::WindowParameter
{
    Q_OBJECT

public:
    explicit DlgMacroExecuteImp(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgMacroExecuteImp() override;

    void accept() override;

private:
    void setupConnections();
    void onFileChooserFileNameChanged(const QString&);
    void onCreateButtonClicked();
    void onDeleteButtonClicked();
    void onEditButtonClicked();
    void onRenameButtonClicked();
    void onDuplicateButtonClicked();
    void onToolbarButtonClicked();
    void onAddonsButtonClicked();
    void onFolderButtonClicked();

    void onUserMacroListBoxCurrentItemChanged(QTreeWidgetItem*);
    void onSystemMacroListBoxCurrentItemChanged(QTreeWidgetItem*);
    void onTabMacroWidgetCurrentChanged(int index);
    void onLineEditFindTextChanged(const QString&);
    void onLineEditFindInFilesTextChanged(const QString&);

protected:
    void fillUpList();
    void fillUpListForDir(const QString& dirPath, bool systemWide);
    QStringList filterFiles(const QString&);

protected:
    QString macroPath;

private:
    std::unique_ptr<PythonTracingWatcher> watcher;
    std::unique_ptr<Ui_DlgMacroExecute> ui;
};

}  // namespace Dialog
}  // namespace Gui
