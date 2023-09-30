/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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
#include <QString>
#endif

#include <App/Application.h>
#include <Base/Interpreter.h>

#include <QDirIterator>
#include <QFileInfo>

#include "MaterialConfigLoader.h"
#include "MaterialLoader.h"
#include "Model.h"
#include "ModelManager.h"


using namespace Materials;

MaterialEntry::MaterialEntry()
{}

MaterialEntry::MaterialEntry(const MaterialLibrary& library,
                             const QString& modelName,
                             const QString& dir,
                             const QString& modelUuid)
    : _library(library)
    , _name(modelName)
    , _directory(dir)
    , _uuid(modelUuid)
{}

MaterialYamlEntry::MaterialYamlEntry(const MaterialLibrary& library,
                                     const QString& modelName,
                                     const QString& dir,
                                     const QString& modelUuid,
                                     const YAML::Node& modelData)
    : MaterialEntry(library, modelName, dir, modelUuid)
    , _model(modelData)
{}

// MaterialYamlEntry::~MaterialYamlEntry()
// {}

QString MaterialYamlEntry::yamlValue(const YAML::Node& node,
                                     const std::string& key,
                                     const std::string& defaultValue)
{
    if (node[key]) {
        return QString::fromStdString(node[key].as<std::string>());
    }
    return QString::fromStdString(defaultValue);
}

void MaterialYamlEntry::addToTree(std::shared_ptr<std::map<QString, Material*>> materialMap)
{
    std::set<QString> exclude;
    exclude.insert(QString::fromStdString("General"));
    exclude.insert(QString::fromStdString("Inherits"));

    auto yamlModel = getModel();
    auto library = getLibrary();
    auto name = getName();
    auto directory = getDirectory();
    QString uuid = getUUID();

    QString authorAndLicense = yamlValue(yamlModel["General"], "AuthorAndLicense", "");
    QString description = yamlValue(yamlModel["General"], "Description", "");

    Material* finalModel = new Material(library, directory, uuid, name);
    finalModel->setAuthorAndLicense(authorAndLicense);
    finalModel->setDescription(description);

    // Add inheritance list
    if (yamlModel["Inherits"]) {
        auto inherits = yamlModel["Inherits"];
        for (auto it = inherits.begin(); it != inherits.end(); it++) {
            std::string nodeName = it->second["UUID"].as<std::string>();

            finalModel->setParentUUID(
                QString::fromStdString(nodeName));  // Should only be one. Need to check
        }
    }

    // Add material models
    if (yamlModel["Models"]) {
        auto models = yamlModel["Models"];
        for (auto it = models.begin(); it != models.end(); it++) {
            std::string modelName = (it->first).as<std::string>();

            // Add the model uuid
            auto modelNode = models[modelName];
            std::string modelUUID = modelNode["UUID"].as<std::string>();
            finalModel->addPhysical(QString::fromStdString(modelUUID));

            // Add the property values
            auto properties = yamlModel["Models"][modelName];
            for (auto itp = properties.begin(); itp != properties.end(); itp++) {
                std::string propertyName = (itp->first).as<std::string>();
                std::string propertyValue = (itp->second).as<std::string>();

                if (finalModel->hasPhysicalProperty(QString::fromStdString(propertyName))) {
                    finalModel->setPhysicalValue(QString::fromStdString(propertyName),
                                                 QString::fromStdString(propertyValue));
                }
                else if (propertyName != "UUID") {
                    Base::Console().Log("\tProperty '%s' is not described by any model. Ignored\n",
                                        propertyName.c_str());
                }
            }
        }
    }

    // Add appearance models
    if (yamlModel["AppearanceModels"]) {
        auto models = yamlModel["AppearanceModels"];
        for (auto it = models.begin(); it != models.end(); it++) {
            std::string modelName = (it->first).as<std::string>();

            // Add the model uuid
            auto modelNode = models[modelName];
            std::string modelUUID = modelNode["UUID"].as<std::string>();
            finalModel->addAppearance(QString::fromStdString(modelUUID));

            // Add the property values
            auto properties = yamlModel["AppearanceModels"][modelName];
            for (auto itp = properties.begin(); itp != properties.end(); itp++) {
                std::string propertyName = (itp->first).as<std::string>();
                std::string propertyValue = (itp->second).as<std::string>();

                if (finalModel->hasAppearanceProperty(QString::fromStdString(propertyName))) {
                    finalModel->setAppearanceValue(QString::fromStdString(propertyName),
                                                   QString::fromStdString(propertyValue));
                }
                else if (propertyName != "UUID") {
                    Base::Console().Log("\tProperty '%s' is not described by any model. Ignored\n",
                                        propertyName.c_str());
                }
            }
        }
    }

    QString path = QDir(directory).absolutePath();
    // Base::Console().Log("\tPath '%s'\n", path.toStdString().c_str());
    (*materialMap)[uuid] = library.addMaterial(*finalModel, path);
}

std::unique_ptr<std::map<QString, MaterialEntry*>> MaterialLoader::_materialEntryMap = nullptr;

MaterialLoader::MaterialLoader(std::shared_ptr<std::map<QString, Material*>> materialMap,
                               std::shared_ptr<std::list<MaterialLibrary*>> libraryList)
    : _materialMap(materialMap)
    , _libraryList(libraryList)
{
    loadLibraries();
}

/*
 *  Destroys the object and frees any allocated resources
 */
MaterialLoader::~MaterialLoader()
{}

void MaterialLoader::addLibrary(MaterialLibrary* model)
{
    _libraryList->push_back(model);
}

MaterialEntry* MaterialLoader::getMaterialFromPath(MaterialLibrary& library,
                                                   const QString& path) const
{
    MaterialEntry* model = nullptr;

    // Used for debugging
    std::string uuid;
    std::string pathName = path.toStdString();

    if (MaterialConfigLoader::isConfigStyle(path)) {
        Base::Console().Log("Old format .FCMat file: '%s'\n", pathName.c_str());
        Material* material = MaterialConfigLoader::getMaterialFromPath(library, path);
        if (material) {
            (*_materialMap)[material->getUUID()] = library.addMaterial(*material, path);
        }

        // Return the nullptr as there are no intermediate steps to take, such
        // as checking inheritance
        return model;
    }

    YAML::Node yamlroot;
    try {
        yamlroot = YAML::LoadFile(pathName);

        const std::string uuid = yamlroot["General"]["UUID"].as<std::string>();
        // Always get the name from the filename
        // QString name = QString::fromStdString(yamlroot["General"]["Name"].as<std::string>());
        QFileInfo filepath(path);
        QString name =
            filepath.fileName().remove(QString::fromStdString(".FCMat"), Qt::CaseInsensitive);

        model = new MaterialYamlEntry(library, name, path, QString::fromStdString(uuid), yamlroot);
        // showYaml(yamlroot);
    }
    catch (YAML::Exception const& e) {
        Base::Console().Error("YAML parsing error: '%s'\n", pathName.c_str());
        Base::Console().Error("\t'%s'\n", e.what());
        showYaml(yamlroot);
    }


    return model;
}

void MaterialLoader::showYaml(const YAML::Node& yaml)
{
    std::stringstream out;

    out << yaml;
    std::string logData = out.str();
    Base::Console().Log("%s\n", logData.c_str());
}


void MaterialLoader::dereference(Material* material)
{
    // Avoid recursion
    if (material->getDereferenced()) {
        return;
    }

    // Base::Console().Log("Dereferencing material '%s'.\n",
    //                     material->getName().toStdString().c_str());

    auto parentUUID = material->getParentUUID();
    if (parentUUID.size() > 0) {
        Material* parent;
        try {
            parent = (*_materialMap)[parentUUID];
        }
        catch (std::out_of_range& e) {
            Base::Console().Log(
                "Unable to apply inheritance for material '%s', parent '%s' not found.\n",
                material->getName().toStdString().c_str(),
                parentUUID.toStdString().c_str());
        }

        // Ensure the parent has been dereferenced
        dereference(parent);

        // Add physical models
        auto modelVector = parent->getPhysicalModels();
        for (auto model = modelVector->begin(); model != modelVector->end(); model++) {
            if (!material->hasPhysicalModel(*model)) {
                material->addPhysical(*model);
            }
        }

        // Add appearance models
        modelVector = parent->getAppearanceModels();
        for (auto model = modelVector->begin(); model != modelVector->end(); model++) {
            if (!material->hasAppearanceModel(*model)) {
                material->addAppearance(*model);
            }
        }

        // Add values
        auto properties = parent->getPhysicalProperties();
        for (auto itp = properties.begin(); itp != properties.end(); itp++) {
            auto name = itp->first;
            auto property = static_cast<const MaterialProperty>(itp->second);

            if (material->getPhysicalProperty(name).isNull()) {
                material->getPhysicalProperty(name).setValue(property.getValue());
            }
        }

        properties = parent->getAppearanceProperties();
        for (auto itp = properties.begin(); itp != properties.end(); itp++) {
            auto name = itp->first;
            auto property = static_cast<const MaterialProperty>(itp->second);

            if (material->getAppearanceProperty(name).isNull()) {
                material->getAppearanceProperty(name).setValue(property.getValue());
            }
        }
    }

    material->markDereferenced();
}

void MaterialLoader::loadLibrary(MaterialLibrary& library)
{
    if (_materialEntryMap == nullptr) {
        _materialEntryMap = std::make_unique<std::map<QString, MaterialEntry*>>();
    }

    QDirIterator it(library.getDirectory(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto pathname = it.next();
        QFileInfo file(pathname);
        if (file.isFile()) {
            if (file.suffix().toStdString() == "FCMat") {
                QString libraryName = file.baseName();

                auto model = getMaterialFromPath(library, file.canonicalFilePath());
                if (model) {
                    (*_materialEntryMap)[model->getUUID()] = model;
                }
            }
        }
    }

    for (auto it = _materialEntryMap->begin(); it != _materialEntryMap->end(); it++) {
        it->second->addToTree(_materialMap);
    }
}

void MaterialLoader::loadLibraries()
{
    auto _libraryList = getMaterialLibraries();
    if (_libraryList) {
        for (auto it = _libraryList->begin(); it != _libraryList->end(); it++) {
            loadLibrary(**it);
        }
    }

    for (auto it = _materialMap->begin(); it != _materialMap->end(); it++) {
        dereference(it->second);
    }
}

std::shared_ptr<std::list<MaterialLibrary*>> MaterialLoader::getMaterialLibraries()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources");
    bool useBuiltInMaterials = param->GetBool("UseBuiltInMaterials", true);
    bool useMatFromModules = param->GetBool("UseMaterialsFromWorkbenches", true);
    bool useMatFromConfigDir = param->GetBool("UseMaterialsFromConfigDir", true);
    bool useMatFromCustomDir = param->GetBool("UseMaterialsFromCustomDir", true);

    if (useBuiltInMaterials) {
        QString resourceDir = QString::fromStdString(App::Application::getResourceDir()
                                                     + "/Mod/Material/Resources/Materials");
        auto libData = new MaterialLibrary(QString::fromStdString("System"),
                                           resourceDir,
                                           QString::fromStdString(":/icons/freecad.svg"),
                                           true);
        _libraryList->push_back(libData);
    }

    if (useMatFromModules) {
        auto moduleParam = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules");
        for (auto& group : moduleParam->GetGroups()) {
            // auto module = moduleParam->GetGroup(group->GetGroupName());
            auto moduleName = QString::fromStdString(group->GetGroupName());
            auto materialDir = QString::fromStdString(group->GetASCII("ModuleDir", ""));
            auto materialIcon = QString::fromStdString(group->GetASCII("ModuleIcon", ""));
            auto materialReadOnly = group->GetBool("ModuleReadOnly", true);

            if (materialDir.length() > 0) {
                QDir dir(materialDir);
                if (dir.exists()) {
                    auto libData = new MaterialLibrary(moduleName,
                                                       materialDir,
                                                       materialIcon,
                                                       materialReadOnly);
                    _libraryList->push_back(libData);
                }
            }
        }
    }

    if (useMatFromConfigDir) {
        QString resourceDir =
            QString::fromStdString(App::Application::getUserAppDataDir() + "/Material");
        if (!resourceDir.isEmpty()) {
            QDir materialDir(resourceDir);
            if (materialDir.exists()) {
                auto libData =
                    new MaterialLibrary(QString::fromStdString("User"),
                                        resourceDir,
                                        QString::fromStdString(":/icons/preferences-general.svg"),
                                        false);
                _libraryList->push_back(libData);
            }
        }
    }

    if (useMatFromCustomDir) {
        QString resourceDir = QString::fromStdString(param->GetASCII("CustomMaterialsDir", ""));
        if (!resourceDir.isEmpty()) {
            QDir materialDir(resourceDir);
            if (materialDir.exists()) {
                auto libData = new MaterialLibrary(QString::fromStdString("Custom"),
                                                   resourceDir,
                                                   QString::fromStdString(":/icons/user.svg"),
                                                   false);
                _libraryList->push_back(libData);
            }
        }
    }

    return _libraryList;
}

std::shared_ptr<std::list<QString>>
MaterialLoader::getMaterialFolders(const MaterialLibrary& library)
{
    std::shared_ptr<std::list<QString>> pathList = std::make_shared<std::list<QString>>();
    QDirIterator it(library.getDirectory(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto pathname = it.next();
        QFileInfo file(pathname);
        if (file.isDir()) {
            QString path = QDir(library.getDirectory()).relativeFilePath(file.absoluteFilePath());
            if (!path.startsWith(QString::fromStdString("."))) {
                pathList->push_back(path);
            }
        }
    }

    return pathList;
}
