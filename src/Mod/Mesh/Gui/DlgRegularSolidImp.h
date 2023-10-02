/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESHGUI_DLGREGULARSOLID_IMP_H
#define MESHGUI_DLGREGULARSOLID_IMP_H

#include <QDialog>
#include <memory>

namespace MeshGui
{
class Ui_DlgRegularSolid;
class DlgRegularSolidImp: public QDialog
{
    Q_OBJECT

public:
    explicit DlgRegularSolidImp(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgRegularSolidImp() override;

private:
    void onCreateSolidButtonClicked();

protected:
    void changeEvent(QEvent* e) override;

private:
    std::unique_ptr<Ui_DlgRegularSolid> ui;
};

}  // namespace MeshGui

#endif  // MESHGUI_DLGREGULARSOLID_IMP_H
