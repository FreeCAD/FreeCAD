/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DLGACTIVATEWINDOW_IMP_H
#define GUI_DIALOG_DLGACTIVATEWINDOW_IMP_H

#include <QDialog>

namespace Gui {
namespace Dialog {
class Ui_DlgActivateWindow;
/**
 * The DlgActivateWindowImp class provides a dialog to activate the MDI window
 * of the main window you wish. Since there could be a lot of MDI windows in
 * an application you cannot put all of them into the "Windows" popup menu.
 * \author Werner Mayer
 */
class DlgActivateWindowImp : public QDialog
{
    Q_OBJECT

public:
    explicit DlgActivateWindowImp(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgActivateWindowImp() override;

protected:
    void accept() override;

private:
    Ui_DlgActivateWindow* ui;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGACTIVATEWINDOW_IMP_H
