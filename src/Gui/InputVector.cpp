/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "PreCompiled.h"

#include "InputVector.h"
#include "ui_InputVector.h"

using namespace Gui;

LocationDialog::LocationDialog(QWidget* parent, Qt::WFlags fl)
  : QDialog(parent, fl)
{
}

LocationDialog::~LocationDialog()
{
}

Base::Vector3f LocationDialog::getUserDirection(bool* ok) const
{
    Gui::Dialog::Ui_InputVector iv;
    QDialog dlg(const_cast<LocationDialog*>(this));
    iv.setupUi(&dlg);
    Base::Vector3f dir;
    if (dlg.exec()) {
        dir.x = (float)iv.vectorX->value();
        dir.y = (float)iv.vectorY->value();
        dir.z = (float)iv.vectorZ->value();
        if (ok) *ok = true;
    }
    else {
        if (ok) *ok = false;
    }

    return dir;
}

void LocationDialog::on_direction_activated(int index)
{
    directionActivated(index);
}

#include "moc_InputVector.cpp"
