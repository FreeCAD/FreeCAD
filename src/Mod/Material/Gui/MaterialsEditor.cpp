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

#include <QItemSelectionModel>

#include <Mod/Material/App/ModelManager.h>
#include "MaterialsEditor.h"
#include "ui_MaterialsEditor.h"


using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

MaterialsEditor::MaterialsEditor(QWidget* parent)
  : QDialog(parent), ui(new Ui_MaterialsEditor)
{
    ui->setupUi(this);

    createMaterialTree();

    connect(ui->standardButtons, &QDialogButtonBox::accepted,
            this, &MaterialsEditor::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected,
            this, &MaterialsEditor::reject);

    QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            this, &MaterialsEditor::onSelectMaterial);
    // connect(selectionModel, &QItemSelectionModel::currentChanged,
    //         this, &MaterialsEditor::onCurrentMaterial);
}

/*
 *  Destroys the object and frees any allocated resources
 */
MaterialsEditor::~MaterialsEditor()
{
    // no need to delete child widgets, Qt does it all for us
}

void MaterialsEditor::tryPython()
{
    Base::Console().Log("MaterialsEditor::tryPython()\n");

    // from materialtools.cardutils import get_material_template
    // template_data = get_material_template(True)
    Gui::WaitCursor wc;
    App::GetApplication().setActiveTransaction(QT_TRANSLATE_NOOP("Command", "Python test"));

    Base::PyGILStateLocker lock;
    try {
        PyObject* module = PyImport_ImportModule("materialtools.cardutils");
        if (!module) {
            throw Py::Exception();
        }
        Py::Module utils(module, true);

        // Py::Tuple args(2);
        // args.setItem(0, Py::asObject(it->getPyObject()));
        // args.setItem(1, Py::Float(distance));
        auto ret = utils.callMemberFunction("get_material_template");
        // if (ret != nullptr)
        //     Base::Console().Log("MaterialsEditor::tryPython() - null return\n");
        // else
        //     Base::Console().Log("MaterialsEditor::tryPython() - data!\n");

        Base::Console().Log("MaterialsEditor::tryPython() - call complete\n");

    }
    catch (Py::Exception&) {
        Base::PyException e;
        e.ReportException();
    }

    App::GetApplication().closeActiveTransaction();
    Base::Console().Log("MaterialsEditor::tryPython() - finished\n");
}

void MaterialsEditor::accept()
{
    reject();
}

void MaterialsEditor::reject()
{
    QDialog::reject();
    // auto dw = qobject_cast<QDockWidget*>(parent());
    // if (dw) {
    //     dw->deleteLater();
    // }
}

void MaterialsEditor::addCards(QStandardItem &parent, const std::string &top, const std::string &folder, const QIcon &icon)
{
    auto tree = ui->treeMaterials;
    for (const auto& mod : fs::directory_iterator(folder)) {
        if (fs::is_directory(mod)) {
            auto node = new QStandardItem(QString::fromStdString(mod.path().filename().string()));
            addExpanded(tree, &parent, node);
            node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);

            addCards(*node, top, mod.path().string(), icon);
        }
        else if (isCard(mod)) {
            auto card = new QStandardItem(icon, QString::fromStdString(mod.path().filename().string()));
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                           | Qt::ItemIsDropEnabled);
            card->setData(QVariant(QString::fromStdString(mod.path().string())), Qt::UserRole);
            addExpanded(tree, &parent, card);
        }
    }
}

void MaterialsEditor::addExpanded(QTreeView *tree, QStandardItem *parent, QStandardItem *child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void MaterialsEditor::addExpanded(QTreeView *tree, QStandardItemModel *parent, QStandardItem *child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void MaterialsEditor::createMaterialTree()
{
    Base::Console().Log("MaterialsEditor::createMaterialTree()\n");
    Material::ModelManager modelManager;

    auto param =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material");
    if (!param)
        Base::Console().Log("Unable to find material parameter group\n");
    else
        Base::Console().Log("Found material parameter group\n");

    auto tree = ui->treeMaterials;
    auto model = new QStandardItemModel();
    tree->setModel(model);

    tree->setHeaderHidden(true);

    auto lib = new QStandardItem(QString::fromStdString("Favorites"));
    lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    addExpanded(tree, model, lib);

    lib = new QStandardItem(QString::fromStdString("Recent"));
    lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    addExpanded(tree, model, lib);

    auto libraries = Material::Materials::getMaterialLibraries();
    for (const auto &value : *libraries)
    {
        lib = new QStandardItem(QString::fromStdString(value->getName()));
        lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        addExpanded(tree, model, lib);

        auto path = value->getDirectory().string();
        addCards(*lib, path, path, QIcon(QString::fromStdString(value->getIconPath())));

        // Base::Console().Log(value->getName().c_str());
        // Base::Console().Log("\n\t");
        // Base::Console().Log(value->getDirectory().string().c_str());
        // Base::Console().Log("\n\t");
        // Base::Console().Log(value->getIconPath().c_str());
        // Base::Console().Log("\n");

    }
}

void MaterialsEditor::onSelectMaterial(const QItemSelection& selected, const QItemSelection& deselected)
{
    Base::Console().Log("MaterialsEditor::onSelectMaterial()\n");

    QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(ui->treeMaterials->model());
    QModelIndexList indexes = selected.indexes();
    for (auto it = indexes.begin(); it != indexes.end(); it++)
    {
        QStandardItem* item = model->itemFromIndex(*it);
        Base::Console().Log("%s\n", item->text().toStdString().c_str());
        Base::Console().Log("\t%s\n", item->data(Qt::UserRole).toString().toStdString().c_str());
    }
}

void MaterialsEditor::onCurrentMaterial(const QModelIndex& selected, const QModelIndex& deselected)
{
    Base::Console().Log("MaterialsEditor::onCurrentMaterial()\n");
}

#include "moc_MaterialsEditor.cpp"
