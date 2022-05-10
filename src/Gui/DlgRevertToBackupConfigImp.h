/***************************************************************************
 *   Copyright (c) 2022 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
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


#ifndef GUI_DIALOG_DLG_REVERT_TO_BACKUP_CONFIG_IMP
#define GUI_DIALOG_DLG_REVERT_TO_BACKUP_CONFIG_IMP

#include <memory>
#include <QDialog>

namespace Gui {
namespace Dialog {
class Ui_DlgRevertToBackupConfig;

/** The DlgRevertToBackupConfigImp class
 * \author Chris Hennes
 */
class DlgRevertToBackupConfigImp : public QDialog
{
    Q_OBJECT

public:
    DlgRevertToBackupConfigImp( QWidget* parent = nullptr );
    ~DlgRevertToBackupConfigImp();

public Q_SLOTS:
    void accept() override;
    void onItemSelectionChanged();

protected:
    void changeEvent(QEvent *e) override;
    void showEvent(QShowEvent* event) override;

private:
    std::unique_ptr<Ui_DlgRevertToBackupConfig> ui;
};

} // namespace Dialog
} // namespace Gui

#endif //GUI_DIALOG_DLG_REVERT_TO_BACKUP_CONFIG_IMP
