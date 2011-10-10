/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QButtonGroup>
#endif

#include "DlgSmoothing.h"
#include "ui_DlgSmoothing.h"

using namespace MeshGui;

/* TRANSLATOR MeshGui::DlgSmoothing */

DlgSmoothing::DlgSmoothing(QWidget* parent, Qt::WFlags fl)
    : QDialog(parent, fl), ui(new Ui_DlgSmoothing())
{
    ui->setupUi(this);
    bg = new QButtonGroup(this);
    bg->addButton(ui->radioButtonTaubin, 0);
    bg->addButton(ui->radioButtonLaplace, 1);
    connect(bg, SIGNAL(buttonClicked(int)),
            this, SLOT(method_clicked(int)));

    ui->labelLambda->setText(QString::fromUtf8("\xce\xbb"));
    ui->labelMu->setText(QString::fromUtf8("\xce\xbc"));
    this->resize(this->sizeHint());
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgSmoothing::~DlgSmoothing()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgSmoothing::method_clicked(int id)
{
    if (bg->button(id) == ui->radioButtonTaubin) {
        ui->labelMu->setEnabled(true);
        ui->spinMicro->setEnabled(true);
    }
    else {
        ui->labelMu->setEnabled(false);
        ui->spinMicro->setEnabled(false);
    }
}

int DlgSmoothing::iterations() const
{
    return ui->iterations->value();
}

double DlgSmoothing::lambdaStep() const
{
    return ui->spinLambda->value();
}

double DlgSmoothing::microStep() const
{
    return ui->spinMicro->value();
}

DlgSmoothing::Smooth DlgSmoothing::method() const
{
    if (ui->radioButtonTaubin->isChecked())
        return DlgSmoothing::Taubin;
    else if (ui->radioButtonLaplace->isChecked())
        return DlgSmoothing::Laplace;
    return DlgSmoothing::None;
}

#include "moc_DlgSmoothing.cpp"
