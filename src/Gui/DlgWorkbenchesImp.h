/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Przemo Firszt <przemo@firszt.eu>                              *
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

// Based on DlgToolbars.h file


#ifndef GUI_DIALOG_DLGWORKBENCHES_IMP_H
#define GUI_DIALOG_DLGWORKBENCHES_IMP_H

#include "PropertyPage.h"
#include <memory>

class QListWidgetCustom;
class QListWidgetItem;

namespace Gui {
namespace Dialog {
class Ui_DlgWorkbenches;

class DlgWorkbenchesImp : public CustomizeActionPage
{
    Q_OBJECT

public:
    DlgWorkbenchesImp(QWidget* parent = nullptr);
    ~DlgWorkbenchesImp();
    static QStringList load_enabled_workbenches();
    static QStringList load_disabled_workbenches();
    static const QString all_workbenches;

protected:
    void changeEvent(QEvent *e);
    void hideEvent(QHideEvent * event);

protected Q_SLOTS:
    void onAddMacroAction(const QByteArray&);
    void onRemoveMacroAction(const QByteArray&);
    void onModifyMacroAction(const QByteArray&);
    void on_add_to_enabled_workbenches_btn_clicked();
    void on_remove_from_enabled_workbenches_btn_clicked();
    void on_shift_workbench_up_btn_clicked();
    void on_shift_workbench_down_btn_clicked();
    void on_sort_enabled_workbenches_btn_clicked();
    void on_add_all_to_enabled_workbenches_btn_clicked();

private:
    void set_lw_properties(QListWidgetCustom *lw);
    void add_workbench(QListWidgetCustom *lw, const QString& it);
    void move_workbench(QListWidgetCustom *lwc_dest,
                        QListWidgetItem *wi);
    void save_workbenches();
    void shift_workbench(bool up);

private:
    std::unique_ptr<Ui_DlgWorkbenches> ui;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGWORKBENCHES_IMP_H
