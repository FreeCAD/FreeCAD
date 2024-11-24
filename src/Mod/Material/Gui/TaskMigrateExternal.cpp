/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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

#include <QPushButton>

#include <Base/Console.h>

#include "TaskMigrateExternal.h"
#include "ui_TaskMigrateExternal.h"


using namespace MatGui;

/* TRANSLATOR MatGui::DlgMigrateExternal */

DlgMigrateExternal::DlgMigrateExternal(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_TaskMigrateExternal)
{
    ui->setupUi(this);

    showLibraries();
}

void DlgMigrateExternal::showLibraries()
{
    auto materialLibraries = _materialManager.getLocalLibraries();
    for (auto library : *materialLibraries) {
        if (library->getName() != QLatin1String("User")) {
            auto item = new QListWidgetItem(library->getName());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
            item->setData(Qt::UserRole, QVariant::fromValue(library));
            ui->listMaterialLibraries->addItem(item);
        }
    }

    auto modelLibraries = _modelManager.getLocalLibraries();
    for (auto library : *modelLibraries) {
        if (library->getName() != QLatin1String("User")) {
            auto item = new QListWidgetItem(library->getName());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
            item->setData(Qt::UserRole, QVariant::fromValue(library));
            ui->listModelLibraries->addItem(item);
        }
    }
}

void DlgMigrateExternal::accept()
{
    Base::Console().Log("Migrating Models...\n");
    ui->textStatus->append(tr("Migrating Models..."));
    for (int row = 0; row < ui->listModelLibraries->count(); row++) {
        auto item = ui->listModelLibraries->item(row);
        if (item->checkState() == Qt::Checked) {
            auto library = item->data(Qt::UserRole).value<std::shared_ptr<Materials::ModelLibrary>>();
            Base::Console().Log("\tLibrary: %s...", library->getName().toStdString().c_str());
            ui->textStatus->append(tr("\tLibrary: ") + library->getName());
            _modelManager.migrateToExternal(library);
            Base::Console().Log("done\n");
        }
    }
    Base::Console().Log("done\n");
    ui->textStatus->append(tr("done"));

    Base::Console().Log("Migrating Materials...\n");
    ui->textStatus->append(tr("Migrating Materials..."));
    for (int row = 0; row < ui->listMaterialLibraries->count(); row++) {
        auto item = ui->listMaterialLibraries->item(row);
        if (item->checkState() == Qt::Checked) {
            auto library =
                item->data(Qt::UserRole).value<std::shared_ptr<Materials::MaterialLibrary>>();
            Base::Console().Log("\tLibrary: %s...", library->getName().toStdString().c_str());
            _materialManager.migrateToExternal(library);
            Base::Console().Log("done\n");
        }
    }
    Base::Console().Log("done\n");
    ui->textStatus->append(tr("done"));
}

/* TRANSLATOR MatGui::TaskMigrateExternal */

TaskMigrateExternal::TaskMigrateExternal()
{
    _widget = new DlgMigrateExternal();
    addTaskBox(_widget);
}

QDialogButtonBox::StandardButtons TaskMigrateExternal::getStandardButtons() const
{
    return QDialogButtonBox::Close | QDialogButtonBox::Ok;
}

void TaskMigrateExternal::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btn = box->button(QDialogButtonBox::Ok);
    btn->setText(QApplication::translate("MatGui::TaskMigrateExternal", "&Migrate"));
}

bool TaskMigrateExternal::accept()
{
    _widget->accept();
    return true;
}

bool TaskMigrateExternal::reject()
{
    return true;
}

#include "moc_TaskMigrateExternal.cpp"
