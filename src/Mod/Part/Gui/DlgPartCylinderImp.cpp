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

#include "PreCompiled.h"

#include "DlgPartCylinderImp.h"
#include "ui_DlgPartCylinder.h"


using namespace PartGui;

DlgPartCylinderImp::DlgPartCylinderImp(QWidget* parent, Qt::WindowFlags fl)
  : Gui::LocationDialogUiImp(new Ui_DlgPartCylinder, parent, fl)
{
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgPartCylinderImp::~DlgPartCylinderImp() = default;

Ui_DlgPartCylinderPtr DlgPartCylinderImp::getUi() const
{
    return boost::any_cast< Ui_DlgPartCylinderPtr >(ui->get());
}

double DlgPartCylinderImp::getRadius() const
{
    return getUi()->radius->value().getValue();
}

double DlgPartCylinderImp::getLength() const
{
    return getUi()->length->value().getValue();
}

#include "moc_DlgPartCylinderImp.cpp"
