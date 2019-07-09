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


#ifndef GUI_DIALOG_DLGMACROEXECUTEIMP_H
#define GUI_DIALOG_DLGMACROEXECUTEIMP_H

#include "ui_DlgMacroExecute.h"
#include "Window.h"

namespace Gui {
namespace Dialog {

/**
 * The DlgMacroExecuteImp class implements a dialog to execute or edit a
 * recorded macro. 
 * \author Jürgen Riegel
 */
class DlgMacroExecuteImp : public QDialog, public Ui_DlgMacroExecute, public Gui::WindowParameter
{ 
    Q_OBJECT

public:
    DlgMacroExecuteImp( QWidget* parent = 0, Qt::WindowFlags fl = 0 );
    ~DlgMacroExecuteImp();

    void accept();

public Q_SLOTS:
    void on_fileChooser_fileNameChanged(const QString&);
    void on_createButton_clicked();
    void on_deleteButton_clicked();
    void on_editButton_clicked();
    void on_renameButton_clicked();
    void on_duplicateButton_clicked();
    void on_toolbarButton_clicked();
    void on_addonsButton_clicked();

protected Q_SLOTS:
    void on_userMacroListBox_currentItemChanged(QTreeWidgetItem*);
    void on_systemMacroListBox_currentItemChanged(QTreeWidgetItem*);
    void on_tabMacroWidget_currentChanged(int index);

protected:
    void fillUpList(void);

protected:
    QString macroPath;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGMACROEXECUTEIMP_H
