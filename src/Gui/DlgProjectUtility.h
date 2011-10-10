/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DLGPROJECTUTILITY_H
#define GUI_DIALOG_DLGPROJECTUTILITY_H

#include <QDialog>

namespace Gui { namespace Dialog {

class Ui_DlgProjectUtility;
class DlgProjectUtility : public QDialog
{
    Q_OBJECT

public:
    DlgProjectUtility(QWidget* parent = 0, Qt::WFlags fl = 0);
    ~DlgProjectUtility();

private Q_SLOTS:
    void on_extractButton_clicked();
    void on_createButton_clicked();

protected:
    Ui_DlgProjectUtility* ui;
};

} // namespace Dialog
} // namespace Gui


#endif // GUI_DIALOG_DLGPROJECTUTILITY_H

