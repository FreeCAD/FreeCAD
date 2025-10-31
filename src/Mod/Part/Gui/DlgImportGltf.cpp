// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QDialogButtonBox>
#endif

#include <Mod/Part/App/GLTF/ImportExportSettings.h>

#include "DlgImportGltf.h"
#include "ui_DlgImportGltf.h"
#include <Standard_Version.hxx>

using namespace PartGui;

DlgImportGltf::DlgImportGltf(QWidget* parent)
  : PreferencePage(parent)
  , ui(new Ui_DlgImportGltf)
{
    ui->setupUi(this);
    Part::GLTF::ImportExportSettings settings;
    ui->checkBoxMeshOnly->setChecked(settings.isTessellationOnly());
    ui->checkBoxRefine->setChecked(settings.getRefinement());
    ui->checkBoxPrintDebug->setChecked(settings.getPrintDebugMessages());
    ui->checkBoxDoublePrec->setChecked(settings.getDoublePrecision());
    ui->checkBoxLoadAllScenes->setChecked(settings.getLoadAllScenes());
    ui->checkBoxSkipEmpty->setChecked(settings.getSkipEmptyNodes());
    ui->checkBoxParallel->setChecked(settings.getMultiThreadedImport());
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgImportGltf::~DlgImportGltf() = default;

void DlgImportGltf::saveSettings()
{
    ui->checkBoxMeshOnly->onSave();
    ui->checkBoxRefine->onSave();
    ui->checkBoxPrintDebug->onSave();
    ui->checkBoxDoublePrec->onSave();
    ui->checkBoxLoadAllScenes->onSave();
    ui->checkBoxSkipEmpty->onSave();
    ui->checkBoxParallel->onSave();
}

void DlgImportGltf::loadSettings()
{
    ui->checkBoxMeshOnly->onRestore();
    ui->checkBoxRefine->onRestore();
    ui->checkBoxPrintDebug->onRestore();
    ui->checkBoxDoublePrec->onRestore();
    ui->checkBoxLoadAllScenes->onRestore();
    ui->checkBoxSkipEmpty->onRestore();
    ui->checkBoxParallel->onRestore();
}

GltfImportSettings DlgImportGltf::getSettings() const
{
    GltfImportSettings set;
    Part::GLTF::ImportExportSettings settings;
    set.tessellationOnly = settings.isTessellationOnly();
    set.refinement = settings.getRefinement();
    set.skipEmptyNodes = settings.getSkipEmptyNodes();
    set.doublePrecision = settings.getDoublePrecision();
    set.loadAllScenes = settings.getLoadAllScenes();
    set.multiThreaded = settings.getMultiThreadedImport();
    set.printDebugMessages = settings.getPrintDebugMessages();
    return set;
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgImportGltf::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

// ----------------------------------------------------------------------------

TaskImportGltf::TaskImportGltf(QWidget* parent)
  : QDialog(parent)
  , ui(new DlgImportGltf(this))
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

    connect(buttonBox, &QDialogButtonBox::accepted, this, &TaskImportGltf::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TaskImportGltf::reject);
}

TaskImportGltf::~TaskImportGltf()
{
    QApplication::restoreOverrideCursor();
}

void TaskImportGltf::accept()
{
    QDialog::accept();
    ui->saveSettings();

    Part::GLTF::ImportExportSettings settings;
    settings.setVisibleImportDialog(!showThis->isChecked());
}

bool TaskImportGltf::showDialog() const
{
    Part::GLTF::ImportExportSettings settings;
    return settings.isVisibleImportDialog();
}

GltfImportSettings TaskImportGltf::getSettings() const
{
    return ui->getSettings();
}


#include "moc_DlgImportGltf.cpp"
