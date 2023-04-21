/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
# include <QDialog>
#endif

#include <Gui/MainWindow.h>

#include "DlgProcessorChooser.h"
#include "ui_DlgProcessorChooser.h"


using namespace PathGui;

/* TRANSLATOR PathGui::DlgProcessorChooser */

DlgProcessorChooser::DlgProcessorChooser(std::vector<std::string> &scriptnames, bool withArguments)
  : QDialog(Gui::getMainWindow()), ui(new Ui_DlgProcessorChooser)
{
    ui->setupUi(this);
    ui->comboBox->addItem(tr("None"));
    for (std::vector<std::string>::const_iterator it = scriptnames.begin(); it != scriptnames.end(); ++it)
        ui->comboBox->addItem(QString::fromUtf8((*it).c_str()));
    QMetaObject::connectSlotsByName(this);
    if (withArguments) {
      ui->argsLabel->setEnabled(true);
      ui->argsLineEdit->setEnabled(true);
    }
}

DlgProcessorChooser::~DlgProcessorChooser()
{
    delete ui;
}

std::string DlgProcessorChooser::getProcessor()
{
    return processor;
}

std::string DlgProcessorChooser::getArguments()
{
  return arguments;
}

void DlgProcessorChooser::accept()
{
    if (ui->comboBox->currentText() == tr("None")) {
        processor = "";
        arguments = "";
    } else {
        processor = ui->comboBox->currentText().toUtf8().data();
        arguments = ui->argsLineEdit->text().toUtf8().data();
    }
    QDialog::accept();
}
#include "moc_DlgProcessorChooser.cpp"
