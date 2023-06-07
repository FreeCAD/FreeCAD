/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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
#ifndef _PreComp_
#endif

#include <QString>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Gui/Command.h>
#include <Gui/WaitCursor.h>
#include <Gui/Application.h>

#include <Mod/Material/App/ModelManager.h>
#include "ModelSelect.h"
#include "ui_ModelSelect.h"


using namespace MatGui;

/* TRANSLATOR MatGui::ModelSelect */

ModelSelect::ModelSelect(QWidget* parent)
  : QDialog(parent), ui(new Ui_ModelSelect)
{
    ui->setupUi(this);
    connect(ui->standardButtons, &QDialogButtonBox::accepted,
            this, &ModelSelect::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected,
            this, &ModelSelect::reject);
}

/*
 *  Destroys the object and frees any allocated resources
 */
ModelSelect::~ModelSelect()
{
    // no need to delete child widgets, Qt does it all for us
}

void ModelSelect::accept()
{
    reject();
}

void ModelSelect::reject()
{
    QDialog::reject();
    // auto dw = qobject_cast<QDockWidget*>(parent());
    // if (dw) {
    //     dw->deleteLater();
    // }
}

#include "moc_ModelSelect.cpp"
