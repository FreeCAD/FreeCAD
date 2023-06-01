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

#include <QDir>
#include <QString>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Gui/Command.h>
#include <Gui/WaitCursor.h>
#include <Gui/Application.h>

#include "MaterialsEditor.h"
#include "ui_MaterialsEditor.h"


using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

MaterialsEditor::MaterialsEditor(QWidget* parent)
  : QDialog(parent), ui(new Ui_MaterialsEditor)
{
    ui->setupUi(this);
    connect(ui->standardButtons, &QDialogButtonBox::accepted,
            this, &MaterialsEditor::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected,
            this, &MaterialsEditor::reject);

    createMaterialTree();
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

bool MaterialsEditor::isCard(const fs::path &p) const
{
    if (!fs::is_regular_file(p))
        return false;
    // check file extension
    if (p.extension() == ".FCMat")
        return true;
    return false;
}

void MaterialsEditor::addCards(QStandardItem &parent, const std::string &top, const std::string &folder, const QIcon &icon)
{
    for (const auto& mod : fs::directory_iterator(folder)) {
        if (fs::is_directory(mod)) {
            auto node = new QStandardItem(QString::fromStdString(mod.path().filename().string()));
            parent.appendRow(node);
            ui->treeMaterials->setExpanded(parent.index(), true);

            addCards(*node, top, mod.path().string(), icon);
        }
        else if (isCard(mod)) {
            auto card = new QStandardItem(QString::fromStdString(mod.path().filename().string()));
            card->setIcon(icon);
            parent.appendRow(card);
            ui->treeMaterials->setExpanded(parent.index(), true);
        }
    }
}

void MaterialsEditor::createMaterialTree()
{
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
    model->appendRow(lib);
    tree->setExpanded(lib->index(), true);
    lib = new QStandardItem(QString::fromStdString("Recent"));
    model->appendRow(lib);
    tree->setExpanded(lib->index(), true);

    auto libraries = getMaterialLibraries();
    for (const auto &value : *libraries)
    {
        lib = new QStandardItem(QString::fromStdString(value->getName()));
        model->appendRow(lib);
        tree->setExpanded(lib->index(), true);

        auto path = QDir::toNativeSeparators(value->getDirectory().absolutePath()).toStdString();
        addCards(*lib, path, path, QIcon(QString::fromStdString(value->getIconPath())));
        //addCards(top, Path(value[0]), Path(value[0]), QtGui.QIcon(value[1]));


        Base::Console().Log(value->getName().c_str());
        Base::Console().Log("\n\t");
        Base::Console().Log(value->getDirectory().absolutePath().toStdString().c_str());
        Base::Console().Log("\n\t");
        Base::Console().Log(value->getIconPath().c_str());
        Base::Console().Log("\n");

    }
}

std::list<LibraryData *> *MaterialsEditor::getMaterialLibraries()
{
    auto param =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources");
    bool useBuiltInMaterials = param->GetBool("UseBuiltInMaterials", true);
    bool useMatFromModules = param->GetBool("UseMaterialsFromWorkbenches", true);
    bool useMatFromConfigDir = param->GetBool("UseMaterialsFromConfigDir", true);
    bool useMatFromCustomDir = param->GetBool("UseMaterialsFromCustomDir", true);

    std::list<LibraryData *> *libraries = new std::list<LibraryData *>();
    if (useBuiltInMaterials)
    {
        QString resourceDir = QString::fromStdString(App::Application::getResourceDir() + "/Mod/Material/Resources/Materials");
        QDir *materialDir = new QDir(resourceDir);
        Base::Console().Log(materialDir->absolutePath().toStdString().c_str());
        Base::Console().Log("\n");
        auto libData = new LibraryData("System", materialDir, ":/icons/freecad.svg");
        libraries->push_back(libData);
    }

    if (useBuiltInMaterials)
    {
        auto moduleParam = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules");
        for (auto &group : moduleParam->GetGroups()) {
            // auto module = moduleParam->GetGroup(group->GetGroupName());
            auto moduleName = group->GetGroupName();
            auto materialDir = group->GetASCII("ModuleDir", "");
            auto materialIcon = group->GetASCII("ModuleIcon", "");

            Base::Console().Log("Module ");
            Base::Console().Log(moduleName);
            Base::Console().Log("\n");

            Base::Console().Log("\tModuleDir ");
            Base::Console().Log(materialDir.c_str());
            Base::Console().Log("\n");

            Base::Console().Log("\tModuleIcon ");
            Base::Console().Log(materialIcon.c_str());
            Base::Console().Log("\n");

            QDir *dir = new QDir(QString::fromStdString(materialDir));
            auto libData = new LibraryData(moduleName, dir, materialIcon);
            libraries->push_back(libData);
        }
    }

    if (useMatFromConfigDir)
    {
        QString resourceDir = QString::fromStdString(App::Application::getUserAppDataDir() + "/Material");
        QDir *materialDir = new QDir(resourceDir);
        Base::Console().Log(materialDir->absolutePath().toStdString().c_str());
        Base::Console().Log("\n");
        auto libData = new LibraryData("User", materialDir, ":/icons/preferences-general.svg");
        libraries->push_back(libData);
    }

    if (useMatFromCustomDir)
    {
        QString resourceDir = QString::fromStdString(param->GetASCII("CustomMaterialsDir", ""));
        QDir *materialDir = new QDir(resourceDir);
        if (materialDir->exists())
        {
            auto libData = new LibraryData("Custom", materialDir, ":/icons/user.svg");
            libraries->push_back(libData);
        }
    }

    return libraries;
}

LibraryData::LibraryData()
{}

LibraryData::LibraryData(const std::string &libraryName, QDir *dir, const std::string &icon) :
    name(libraryName), directory(dir), iconPath(icon)
{}

LibraryData::~LibraryData()
{
    delete directory;
}


#include "moc_MaterialsEditor.cpp"
