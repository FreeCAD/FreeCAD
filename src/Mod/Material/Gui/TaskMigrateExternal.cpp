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

#include <QFlags>

#include <Base/Console.h>
#include <Gui/WaitCursor.h>

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
    auto materialLibraries = Materials::MaterialManager::getManager().getLocalLibraries();
    for (auto library : *materialLibraries) {
        if (library->getName() != QLatin1String("User")) {
            auto item = new QListWidgetItem(library->getName());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
            item->setData(Qt::UserRole, QVariant::fromValue(library));
            ui->listMaterialLibraries->addItem(item);
        }
    }

    auto modelLibraries = Materials::ModelManager::getManager().getLocalLibraries();
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

void DlgMigrateExternal::migrate()
{
    try {
        statusUpdate(tr("Migrating models…"));
        for (int row = 0; row < ui->listModelLibraries->count(); row++) {
            auto item = ui->listModelLibraries->item(row);
            if (item->checkState() == Qt::Checked) {
                auto library =
                    item->data(Qt::UserRole).value<std::shared_ptr<Materials::ModelLibrary>>();
                statusUpdate(tr("  Library: ") + library->getName());
                Materials::ModelManager::getManager().migrateToExternal(library);
            }
        }
        statusUpdate(tr("done"));

        statusUpdate(tr("Validating models…"));
        for (int row = 0; row < ui->listModelLibraries->count(); row++) {
            auto item = ui->listModelLibraries->item(row);
            if (item->checkState() == Qt::Checked) {
                auto library =
                    item->data(Qt::UserRole).value<std::shared_ptr<Materials::ModelLibrary>>();
                statusUpdate(tr("  Library: ") + library->getName());
                Materials::ModelManager::getManager().validateMigration(library);
            }
        }
        statusUpdate(tr("done"));

        statusUpdate(tr("Migrating materials…"));
        for (int row = 0; row < ui->listMaterialLibraries->count(); row++) {
            auto item = ui->listMaterialLibraries->item(row);
            if (item->checkState() == Qt::Checked) {
                auto library =
                    item->data(Qt::UserRole).value<std::shared_ptr<Materials::MaterialLibrary>>();
                statusUpdate(tr("  Library: ") + library->getName());
                Materials::MaterialManager::getManager().migrateToExternal(library);
            }
        }
        statusUpdate(tr("done"));

        statusUpdate(tr("Validating materials…"));
        for (int row = 0; row < ui->listMaterialLibraries->count(); row++) {
            auto item = ui->listMaterialLibraries->item(row);
            if (item->checkState() == Qt::Checked) {
                auto library =
                    item->data(Qt::UserRole).value<std::shared_ptr<Materials::MaterialLibrary>>();
                statusUpdate(tr("  Library: ") + library->getName());
                Materials::MaterialManager::getManager().validateMigration(library);
            }
        }
        statusUpdate(tr("done"));
    }
    catch (const Materials::ConnectionError& e) {
        statusUpdate(QString::fromStdString(e.what()));
        statusUpdate(tr("Aborted"));
    }
    catch (const Materials::CreationError& e) {
        statusUpdate(QString::fromStdString(e.what()));
        statusUpdate(tr("Aborted"));
    }
    catch (const Materials::InvalidModel& e) {
        statusUpdate(QString::fromStdString(e.what()));
        statusUpdate(tr("Aborted"));
    }
    catch (const Materials::InvalidMaterial& e) {
        statusUpdate(QString::fromStdString(e.what()));
        statusUpdate(tr("Aborted"));
    }
    catch (const Materials::InvalidProperty& e) {
        statusUpdate(QString::fromStdString(e.what()));
        statusUpdate(tr("Aborted"));
    }
    catch (const Base::Exception& e) {
        statusUpdate(QString::fromStdString(e.what()));
        statusUpdate(tr("Unknown exception - aborted"));
    }
    catch (...) {
        statusUpdate(tr("Unknown exception - aborted"));
    }
}

void DlgMigrateExternal::statusUpdate(const QString& status)
{
    Base::Console().log("%s\n", status.toStdString().c_str());
    ui->textStatus->append(status);

    // This is required to update in real time
    QCoreApplication::processEvents();
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
    _migrateButton = box->button(QDialogButtonBox::Ok);
    _closeButton = box->button(QDialogButtonBox::Close);
    _migrateButton->setText(QApplication::translate("MatGui::TaskMigrateExternal", "&Migrate"));
    // connect(btn, &QPushButton::clicked, this, &TaskMigrateExternal::onMigrate);
}

void TaskMigrateExternal::onMigrate(bool checked)
{
    Q_UNUSED(checked)

    Gui::WaitCursor wc;
    QCoreApplication::processEvents();

    // Disable the buttons during migration
    _migrateButton->setEnabled(false);
    _closeButton->setEnabled(false);

    _widget->migrate();

    _migrateButton->setEnabled(true);
    _closeButton->setEnabled(true);
}

bool TaskMigrateExternal::accept()
{
    _widget->migrate();
    return false;
}

bool TaskMigrateExternal::reject()
{
    return true;
}

#include "moc_TaskMigrateExternal.cpp"
