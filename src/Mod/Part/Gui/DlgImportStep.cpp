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
# include <QDialogButtonBox>
#endif

#include <Mod/Part/App/OCAF/ImportExportSettings.h>
#include <Mod/Part/App/STEP/ImportExportSettings.h>

#include "DlgImportStep.h"
#include "ui_DlgImportStep.h"
#include <Standard_Version.hxx>

using namespace PartGui;

DlgImportStep::DlgImportStep(QWidget* parent)
  : PreferencePage(parent)
  , ui(new Ui_DlgImportStep)
{
    ui->setupUi(this);
    Part::OCAF::ImportExportSettings settings;
    ui->checkBoxMergeCompound->setChecked(settings.getReadShapeCompoundMode());
    ui->checkBoxImportHiddenObj->setChecked(settings.getImportHiddenObject());
    ui->checkBoxUseLinkGroup->setChecked(settings.getUseLinkGroup());
    ui->checkBoxUseBaseName->setChecked(settings.getUseBaseName());
    ui->checkBoxReduceObjects->setChecked(settings.getReduceObjects());
    ui->checkBoxExpandCompound->setChecked(settings.getExpandCompound());
    ui->checkBoxShowProgress->setChecked(settings.getShowProgress());
#if OCC_VERSION_HEX >= 0x070800
    std::list<Part::OCAF::ImportExportSettings::CodePage> codepagelist;
    codepagelist = settings.getCodePageList();
    for (const auto& codePage : codepagelist) {
        ui->comboBoxImportCodePage->addItem(QString::fromStdString(codePage.codePageName));
    }
#else
    // hide options that not supported in this OCCT version (7.8.0)
    ui->label_6->hide();
    ui->comboBoxImportCodePage->hide();
#endif
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgImportStep::~DlgImportStep() = default;

void DlgImportStep::saveSettings()
{
    // (h)STEP of Import module
#if OCC_VERSION_HEX >= 0x070800
    ui->comboBoxImportCodePage->onSave();
#endif
    ui->checkBoxMergeCompound->onSave();
    ui->checkBoxImportHiddenObj->onSave();
    ui->checkBoxUseLinkGroup->onSave();
    ui->checkBoxUseBaseName->onSave();
    ui->checkBoxReduceObjects->onSave();
    ui->checkBoxExpandCompound->onSave();
    ui->checkBoxShowProgress->onSave();
    ui->comboBoxImportMode->onSave();
}

void DlgImportStep::loadSettings()
{
    // (h)STEP of Import module
#if OCC_VERSION_HEX >= 0x070800
    ui->comboBoxImportCodePage->onRestore();
#endif
    ui->checkBoxMergeCompound->onRestore();
    ui->checkBoxImportHiddenObj->onRestore();
    ui->checkBoxUseLinkGroup->onRestore();
    ui->checkBoxUseBaseName->onRestore();
    ui->checkBoxReduceObjects->onRestore();
    ui->checkBoxExpandCompound->onRestore();
    ui->checkBoxShowProgress->onRestore();
    ui->comboBoxImportMode->onRestore();
}

StepImportSettings DlgImportStep::getSettings() const
{
    StepImportSettings set;
    Part::OCAF::ImportExportSettings settings;
    set.merge = settings.getReadShapeCompoundMode();
    set.useLinkGroup = settings.getUseLinkGroup();
    set.useBaseName = settings.getUseBaseName();
    set.importHidden = settings.getImportHiddenObject();
    set.reduceObjects = settings.getReduceObjects();
    set.showProgress = settings.getShowProgress();
    set.expandCompound = settings.getExpandCompound();
    set.mode = static_cast<int>(settings.getImportMode());
#if OCC_VERSION_HEX >= 0x070800
    Resource_FormatType cp = settings.getImportCodePage();
    set.codePage = static_cast<int>(cp);
#endif
    return set;
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgImportStep::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

// ----------------------------------------------------------------------------

TaskImportStep::TaskImportStep(QWidget* parent)
  : QDialog(parent)
  , ui(new DlgImportStep(this))
{
    QApplication::setOverrideCursor(Qt::ArrowCursor);

    ui->loadSettings();
    setWindowTitle(ui->windowTitle());

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(ui.get());
    setLayout(layout);

    showThis = new QCheckBox(this);
    showThis->setText(tr("Do not show this dialog again"));
    layout->addWidget(showThis);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &TaskImportStep::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TaskImportStep::reject);
}

TaskImportStep::~TaskImportStep()
{
    QApplication::restoreOverrideCursor();
}

void TaskImportStep::accept()
{
    QDialog::accept();
    ui->saveSettings();

    Part::STEP::ImportExportSettings settings;
    settings.setVisibleImportDialog(!showThis->isChecked());
}

bool TaskImportStep::showDialog() const
{
    Part::STEP::ImportExportSettings settings;
    return settings.isVisibleImportDialog();
}

StepImportSettings TaskImportStep::getSettings() const
{
    return ui->getSettings();
}


#include "moc_DlgImportStep.cpp"
