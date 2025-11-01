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
# include <QCheckBox>
# include <QDialogButtonBox>
# include <QRegularExpression>
# include <QRegularExpressionValidator>
# include <QVBoxLayout>
# include <Interface_Static.hxx>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Mod/Part/App/GLTF/ImportExportSettings.h>

#include "DlgExportGltf.h"
#include "ui_DlgExportGltf.h"


using namespace PartGui;

DlgExportGltf::DlgExportGltf(QWidget* parent)
  : PreferencePage(parent)
  , ui(new Ui_DlgExportGltf)
{
    ui->setupUi(this);

    Part::GLTF::ImportExportSettings settings;
    ui->checkBoxExportUV->setChecked(settings.getUVCoords());
    ui->checkBoxMergeFaces->setChecked(settings.getMergeFaces());
    ui->checkBoxParallel->setChecked(settings.getMultiThreadedExport());
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgExportGltf::~DlgExportGltf() = default;

void DlgExportGltf::saveSettings()
{
    ui->checkBoxExportUV->onSave();
    ui->checkBoxMergeFaces->onSave();
    ui->checkBoxParallel->onSave();
}

void DlgExportGltf::loadSettings()
{
    ui->checkBoxExportUV->onRestore();
    ui->checkBoxMergeFaces->onRestore();
    ui->checkBoxParallel->onRestore();
}

GltfExportSettings DlgExportGltf::getSettings() const
{
    GltfExportSettings set;
    set.exportUVCoords = ui->checkBoxExportUV->isChecked();
    set.mergeFaces = ui->checkBoxMergeFaces->isChecked();
    set.multiThreaded = ui->checkBoxParallel->isChecked();
    return set;
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgExportGltf::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

// ----------------------------------------------------------------------------

TaskExportGltf::TaskExportGltf(QWidget* parent)
  : QDialog(parent)
  , ui(new DlgExportGltf(this))
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

    connect(buttonBox, &QDialogButtonBox::accepted, this, &TaskExportGltf::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TaskExportGltf::reject);
}

TaskExportGltf::~TaskExportGltf()
{
    QApplication::restoreOverrideCursor();
}

void TaskExportGltf::accept()
{
    QDialog::accept();
    ui->saveSettings();

    Part::GLTF::ImportExportSettings settings;
    settings.setVisibleExportDialog(!showThis->isChecked());
}

bool TaskExportGltf::showDialog() const
{
    Part::GLTF::ImportExportSettings settings;
    return settings.isVisibleExportDialog();
}

GltfExportSettings TaskExportGltf::getSettings() const
{
    return ui->getSettings();
}


#include "moc_DlgExportGltf.cpp"
