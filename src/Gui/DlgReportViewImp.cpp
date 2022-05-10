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

#include "PreCompiled.h"

#include "DlgReportViewImp.h"
#include "ui_DlgReportView.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgReportViewImp */

/**
 *  Constructs a DlgReportViewImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgReportViewImp::DlgReportViewImp( QWidget* parent )
  : PreferencePage(parent)
  , ui(new Ui_DlgReportView)
{
    ui->setupUi(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgReportViewImp::~DlgReportViewImp()
{
}

void DlgReportViewImp::saveSettings()
{
    ui->checkMessage->onSave();
    ui->checkLogging->onSave();
    ui->checkWarning->onSave();
    ui->checkError->onSave();
    ui->checkShowReportViewOnWarning->onSave();
    ui->checkShowReportViewOnError->onSave();
    ui->checkShowReportViewOnNormalMessage->onSave();
    ui->checkShowReportViewOnLogMessage->onSave();
    ui->checkShowReportTimecode->onSave();
    ui->colorText->onSave();
    ui->colorLogging->onSave();
    ui->colorWarning->onSave();
    ui->colorError->onSave();
    ui->pythonOutput->onSave();
    ui->pythonError->onSave();
}

void DlgReportViewImp::loadSettings()
{
    ui->checkMessage->onRestore();
    ui->checkLogging->onRestore();
    ui->checkWarning->onRestore();
    ui->checkError->onRestore();
    ui->checkShowReportViewOnWarning->onRestore();
    ui->checkShowReportViewOnError->onRestore();
    ui->checkShowReportViewOnNormalMessage->onRestore();
    ui->checkShowReportViewOnLogMessage->onRestore();
    ui->checkShowReportTimecode->onRestore();
    ui->colorText->onRestore();
    ui->colorLogging->onRestore();
    ui->colorWarning->onRestore();
    ui->colorError->onRestore();
    ui->pythonOutput->blockSignals(true);
    ui->pythonOutput->onRestore();
    ui->pythonOutput->blockSignals(false);
    ui->pythonError->blockSignals(true);
    ui->pythonError->onRestore();
    ui->pythonError->blockSignals(false);
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgReportViewImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgReportViewImp.cpp"
