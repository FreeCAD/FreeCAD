/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QCheckBox>
# include <QDialogButtonBox>
# include <QRegularExpression>
# include <QRegularExpressionValidator>
# include <QVBoxLayout>
# include <Interface_Static.hxx>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Mod/Part/App/OCAF/ImportExportSettings.h>
#include <Mod/Part/App/STEP/ImportExportSettings.h>

#include "DlgExportStep.h"
#include "ui_DlgExportStep.h"
#include "ui_DlgExportHeaderStep.h"


using namespace PartGui;

DlgExportStep::DlgExportStep(QWidget* parent)
  : PreferencePage(parent)
  , ui(new Ui_DlgExportStep)
{
    ui->setupUi(this);

    ui->comboBoxSchema->setItemData(0, QByteArray("AP203"));
    ui->comboBoxSchema->setItemData(1, QByteArray("AP214CD"));
    ui->comboBoxSchema->setItemData(2, QByteArray("AP214DIS"));
    ui->comboBoxSchema->setItemData(3, QByteArray("AP214IS"));
    ui->comboBoxSchema->setItemData(4, QByteArray("AP242DIS"));

    // https://tracker.dev.opencascade.org/view.php?id=25654
    ui->checkBoxPcurves->setToolTip(tr("This parameter indicates whether parametric curves (curves in parametric space of surface)\n"
                                       "should be written into the STEP file. This parameter can be set to off in order to minimize\n"
                                       "the size of the resulting STEP file."));

    Part::OCAF::ImportExportSettings settings;
    ui->checkBoxExportHiddenObj->setChecked(settings.getExportHiddenObject());
    ui->checkBoxExportLegacy->setChecked(settings.getExportLegacy());
    ui->checkBoxKeepPlacement->setChecked(settings.getExportKeepPlacement());
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgExportStep::~DlgExportStep() = default;

void DlgExportStep::saveSettings()
{
    // General
    Part::STEP::ImportExportSettings settings;
    settings.setWriteSurfaceCurveMode(ui->checkBoxPcurves->isChecked());

    // STEP
    int unit = ui->comboBoxUnits->currentIndex();
    settings.setUnit(static_cast<Part::Interface::Unit>(unit));

    // scheme
    // possible values: AP203, AP214CD (1996), AP214DIS (1998), AP214IS (2002), AP242DIS
    QByteArray schema = ui->comboBoxSchema->itemData(ui->comboBoxSchema->currentIndex()).toByteArray();
    settings.setScheme(schema);

    // (h)STEP of Import module
    ui->checkBoxExportHiddenObj->onSave();
    ui->checkBoxExportLegacy->onSave();
    ui->checkBoxKeepPlacement->onSave();
}

void DlgExportStep::loadSettings()
{
    // General
    Part::STEP::ImportExportSettings settings;
    ui->checkBoxPcurves->setChecked(settings.getWriteSurfaceCurveMode());

    // STEP
    ui->comboBoxUnits->setCurrentIndex(static_cast<int>(settings.getUnit()));

    // scheme
    QByteArray ap(settings.getScheme().c_str());
    int index = ui->comboBoxSchema->findData(QVariant(ap));
    if (index >= 0)
        ui->comboBoxSchema->setCurrentIndex(index);

    // (h)STEP of Import module
    ui->checkBoxExportHiddenObj->onRestore();
    ui->checkBoxExportLegacy->onRestore();
    ui->checkBoxKeepPlacement->onRestore();
}

StepSettings DlgExportStep::getSettings() const
{
    StepSettings set;
    set.exportLegacy = ui->checkBoxExportLegacy->isChecked();
    set.exportHidden = ui->checkBoxExportHiddenObj->isChecked();
    set.keepPlacement = ui->checkBoxKeepPlacement->isChecked();
    return set;
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgExportStep::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

// ----------------------------------------------------------------------------

DlgExportHeaderStep::DlgExportHeaderStep(QWidget* parent)
  : PreferencePage(parent)
  , ui(new Ui_DlgExportHeaderStep)
{
    ui->setupUi(this);

    ui->lineEditProduct->setReadOnly(true);

    QRegularExpression rx;
    rx.setPattern(QString::fromLatin1("[\\x00-\\x7F]+"));
    QRegularExpressionValidator* companyValidator = new QRegularExpressionValidator(ui->lineEditCompany);
    companyValidator->setRegularExpression(rx);
    ui->lineEditCompany->setValidator(companyValidator);
    QRegularExpressionValidator* authorValidator = new QRegularExpressionValidator(ui->lineEditAuthor);
    authorValidator->setRegularExpression(rx);
    ui->lineEditAuthor->setValidator(authorValidator);
}

DlgExportHeaderStep::~DlgExportHeaderStep() = default;

void DlgExportHeaderStep::saveSettings()
{
    Part::STEP::ImportExportSettings settings;

    // header info
    settings.setCompany(ui->lineEditCompany->text().toLatin1());
    settings.setAuthor(ui->lineEditAuthor->text().toLatin1());
}

void DlgExportHeaderStep::loadSettings()
{
    Part::STEP::ImportExportSettings settings;

    // header info
    ui->lineEditCompany->setText(QString::fromStdString(settings.getCompany()));
    ui->lineEditAuthor->setText(QString::fromStdString(settings.getAuthor()));
    ui->lineEditProduct->setText(QString::fromStdString(settings.getProductName()));
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgExportHeaderStep::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

// ----------------------------------------------------------------------------

TaskExportStep::TaskExportStep(QWidget* parent)
  : QDialog(parent)
  , ui(new DlgExportStep(this))
{
    QApplication::setOverrideCursor(Qt::ArrowCursor);

    ui->loadSettings();
    setWindowTitle(ui->windowTitle());

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(ui.get());
    setLayout(layout);

    showThis = new QCheckBox(this);
    showThis->setText(tr("Don't show this dialog again"));
    layout->addWidget(showThis);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &TaskExportStep::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TaskExportStep::reject);
}

TaskExportStep::~TaskExportStep()
{
    QApplication::restoreOverrideCursor();
}

void TaskExportStep::accept()
{
    QDialog::accept();
    ui->saveSettings();

    Part::STEP::ImportExportSettings settings;
    settings.setVisibleExportDialog(!showThis->isChecked());
}

bool TaskExportStep::showDialog() const
{
    Part::STEP::ImportExportSettings settings;
    return settings.isVisibleExportDialog();
}

StepSettings TaskExportStep::getSettings() const
{
    return ui->getSettings();
}


#include "moc_DlgExportStep.cpp"
