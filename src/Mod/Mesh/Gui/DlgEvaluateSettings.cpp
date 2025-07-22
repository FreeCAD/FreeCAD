/***************************************************************************
 *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "DlgEvaluateSettings.h"
#include "ui_DlgEvaluateSettings.h"


using namespace MeshGui;

/* TRANSLATOR MeshGui::DlgEvaluateSettings */

DlgEvaluateSettings::DlgEvaluateSettings(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , ui(new Ui_DlgEvaluateSettings)
{
    ui->setupUi(this);
}

DlgEvaluateSettings::~DlgEvaluateSettings()
{
    delete ui;
}

void DlgEvaluateSettings::setNonmanifoldPointsChecked(bool on)
{
    ui->checkNonmanifoldPoints->setChecked(on);
}

bool DlgEvaluateSettings::isNonmanifoldPointsChecked() const
{
    return ui->checkNonmanifoldPoints->isChecked();
}

void DlgEvaluateSettings::setFoldsChecked(bool on)
{
    ui->checkFolds->setChecked(on);
}

bool DlgEvaluateSettings::isFoldsChecked() const
{
    return ui->checkFolds->isChecked();
}

void DlgEvaluateSettings::setDegeneratedFacetsChecked(bool on)
{
    ui->checkDegenerated->setChecked(on);
}

bool DlgEvaluateSettings::isDegeneratedFacetsChecked() const
{
    return ui->checkDegenerated->isChecked();
}

#include "moc_DlgEvaluateSettings.cpp"
