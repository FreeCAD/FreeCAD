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

#include "ui_DlgRegularSolid.h"

namespace MeshGui {
class DlgRegularSolidImp : public QDialog, public Ui_DlgRegularSolid
{
    Q_OBJECT

public:
    DlgRegularSolidImp(QWidget* parent = 0, Qt::WFlags fl = 0);
    ~DlgRegularSolidImp();

public Q_SLOTS:
    void on_createSolidButton_clicked();

protected:
    void changeEvent(QEvent *e);
};

/**
 * The SingleDlgRegularSolidImp class creates a single instance.
 * \author Werner Mayer
 */
class SingleDlgRegularSolidImp : public DlgRegularSolidImp
{ 
protected:
    SingleDlgRegularSolidImp(QWidget* parent = 0, Qt::WFlags fl = 0);
    ~SingleDlgRegularSolidImp();

public:
    static SingleDlgRegularSolidImp* instance();
    static void destruct();
    static bool hasInstance();

private:
    static SingleDlgRegularSolidImp* _instance;
};

}

#endif // MESHGUI_DLGREGULARSOLID_IMP_H
