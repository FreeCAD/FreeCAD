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

#include <QDirIterator>
#include <QFileInfo>
#include <QMetaType>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Gui/MetaTypes.h>

#include "Materials.h"

#include "MaterialConfigLoader.h"
#include "MaterialLibrary.h"
#include "MaterialLoader.h"
#include "Model.h"
#include "ModelManager.h"


using namespace Materials;

MaterialEntry::MaterialEntry()
{}

MaterialEntry::MaterialEntry(std::shared_ptr<MaterialLibrary> library,
                             const QString& modelName,
                             const QString& dir,
                             const QString& modelUuid)
    : _library(library)
    , _name(modelName)
    , _directory(dir)
    , _uuid(modelUuid)
{}

MaterialYamlEntry::MaterialYamlEntry(std::shared_ptr<MaterialLibrary> library,
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

std::shared_ptr<Material2DArray> MaterialYamlEntry::read2DArray(const YAML::Node& node)
{
    // Base::Console().Log("Read 2D Array\n");

    auto array2d = std::make_shared<Material2DArray>();

    if (node.size() == 2) {
        // Get the default
        Base::Quantity defaultValue =
            Base::Quantity::parse(QString::fromStdString(node[0].as<std::string>()));
        array2d->setDefault(QVariant::fromValue(defaultValue));

        auto yamlArray = node[1];
        for (std::size_t i = 0; i < yamlArray.size(); i++) {
            auto yamlRow = yamlArray[i];

            auto row = std::make_shared<std::vector<QVariant>>();
            for (std::size_t j = 0; j < yamlRow.size(); j++) {
                Base::Quantity q =
                    Base::Quantity::parse(QString::fromStdString(yamlRow[j].as<std::string>()));
                row->push_back(QVariant::fromValue(q));
            }
            array2d->addRow(row);
        }
    }

    return array2d;
}

std::shared_ptr<Material3DArray> MaterialYamlEntry::read3DArray(const YAML::Node& node)
{
    Base::Console().Log("Read 3D Array\n");

    auto array3d = std::make_shared<Material3DArray>();

    if (node.size() == 2) {
        // Get the default
        Base::Quantity defaultValue =
            Base::Quantity::parse(QString::fromStdString(node[0].as<std::string>()));
        array3d->setDefault(QVariant::fromValue(defaultValue));

        auto yamlArray = node[1];

        for (std::size_t depth = 0; depth < yamlArray.size(); depth++) {
            auto yamlDepth = yamlArray[depth];
            MaterialLoader::showYaml(yamlDepth);
            for (auto it = yamlDepth.begin(); it != yamlDepth.end(); it++) {
                MaterialLoader::showYaml(it->first);
                MaterialLoader::showYaml(it->second);

                Base::Console().Log("Depth %d '%s'\n", depth, it->first.as<std::string>().c_str());
                auto depthValue =
                    Base::Quantity::parse(QString::fromStdString(it->first.as<std::string>()));

                array3d->addDepth(depth, depthValue);

                auto yamlTable = it->second;
                for (std::size_t i = 0; i < yamlTable.size(); i++) {
                    auto yamlRow = yamlTable[i];

                    auto row = std::make_shared<std::vector<Base::Quantity>>();
                    for (std::size_t j = 0; j < yamlRow.size(); j++) {
                        row->push_back(Base::Quantity::parse(
                            QString::fromStdString(yamlRow[j].as<std::string>())));
                    }
                    array3d->addRow(depth, row);
                }
            }
        }
    }

    return array3d;
}

void MaterialYamlEntry::addToTree(
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> materialMap)
{
    std::set<QString> exclude;
    exclude.insert(QString::fromStdString("General"));
    exclude.insert(QString::fromStdString("Inherits"));

    auto yamlModel = getModel();
    auto library = getLibrary();
    auto name = getName();
    auto directory = getDirectory();
    QString uuid = getUUID();

    QString author = yamlValue(yamlModel["General"], "Author", "");
    QString license = yamlValue(yamlModel["General"], "License", "");
    QString description = yamlValue(yamlModel["General"], "Description", "");

    std::shared_ptr<Material> finalModel =
        std::make_shared<Material>(library, directory, uuid, name);
    finalModel->setAuthor(author);
    finalModel->setLicense(license);
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
                if (finalModel->hasPhysicalProperty(QString::fromStdString(propertyName))) {
                    auto prop =
                        finalModel->getPhysicalProperty(QString::fromStdString(propertyName));
                    auto type = prop->getType();

                    try {
                        if (type == MaterialValue::Array2D) {
                            auto array2d = read2DArray(itp->second);
                            finalModel->setPhysicalValue(QString::fromStdString(propertyName),
                                                         array2d);
                        }
                        else if (type == MaterialValue::Array3D) {
                            auto array3d = read3DArray(itp->second);
                            finalModel->setPhysicalValue(QString::fromStdString(propertyName),
                                                         array3d);
                        }
                        else {
                            std::string propertyValue = (itp->second).as<std::string>();
                            finalModel->setPhysicalValue(QString::fromStdString(propertyName),
                                                         QString::fromStdString(propertyValue));
                        }
                    }
                    catch (const YAML::BadConversion& e) {
                        Base::Console().Log("Exception %s <%s:%s> - ignored\n",
                                            e.what(),
                                            name.toStdString().c_str(),
                                            propertyName.c_str());
                    }
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
                if (finalModel->hasAppearanceProperty(QString::fromStdString(propertyName))) {
                    auto prop =
                        finalModel->getAppearanceProperty(QString::fromStdString(propertyName));
                    auto type = prop->getType();

                    try {
                        if (type == MaterialValue::Array2D) {
                            auto array2d = read2DArray(itp->second);
                            finalModel->setAppearanceValue(QString::fromStdString(propertyName),
                                                           array2d);
                        }
                        else if (type == MaterialValue::Array3D) {
                            Base::Console().Log("Read 3D Array\n");
                        }
                        else {
                            std::string propertyValue = (itp->second).as<std::string>();
                            finalModel->setAppearanceValue(QString::fromStdString(propertyName),
                                                           QString::fromStdString(propertyValue));
                        }
                    }
                    catch (const YAML::BadConversion& e) {
                        Base::Console().Log("Exception %s <%s:%s> - ignored\n",
                                            e.what(),
                                            name.toStdString().c_str(),
                                            propertyName.c_str());
                    }
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
    (*materialMap)[uuid] = library->addMaterial(finalModel, path);
}

std::unique_ptr<std::map<QString, std::shared_ptr<MaterialEntry>>>
    MaterialLoader::_materialEntryMap = nullptr;

MaterialLoader::MaterialLoader(
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> materialMap,
    std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> libraryList)
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

void MaterialLoader::addLibrary(std::shared_ptr<MaterialLibrary> model)
{
    _libraryList->push_back(model);
}

std::shared_ptr<MaterialEntry>
MaterialLoader::getMaterialFromYAML(std::shared_ptr<MaterialLibrary> library,
                                    YAML::Node& yamlroot,
                                    const QString& path) const
{
    std::shared_ptr<MaterialEntry> model = nullptr;

    try {
        const std::string uuid = yamlroot["General"]["UUID"].as<std::string>();

        // Always get the name from the filename
        // QString name = QString::fromStdString(yamlroot["General"]["Name"].as<std::string>());
        QFileInfo filepath(path);
        QString name =
            filepath.fileName().remove(QString::fromStdString(".FCMat"), Qt::CaseInsensitive);

        model = std::make_shared<MaterialYamlEntry>(library,
                                                    name,
                                                    path,
                                                    QString::fromStdString(uuid),
                                                    yamlroot);
        // showYaml(yamlroot);
    }
    catch (YAML::Exception const& e) {
        Base::Console().Error("YAML parsing error: '%s'\n", path.toStdString().c_str());
        Base::Console().Error("\t'%s'\n", e.what());
        showYaml(yamlroot);
    }


    return model;
}

std::shared_ptr<MaterialEntry>
MaterialLoader::getMaterialFromPath(std::shared_ptr<MaterialLibrary> library,
                                    const QString& path) const
{
    std::shared_ptr<MaterialEntry> model = nullptr;

    // Used for debugging
    std::string pathName = path.toStdString();

    if (MaterialConfigLoader::isConfigStyle(path)) {
        // Base::Console().Log("Old format .FCMat file: '%s'\n", pathName.c_str());
        auto material = MaterialConfigLoader::getMaterialFromPath(library, path);
        if (material) {
            (*_materialMap)[material->getUUID()] = library->addMaterial(material, path);
        }

        // Return the nullptr as there are no intermediate steps to take, such
        // as checking inheritance
        return model;
    }

    YAML::Node yamlroot;
    try {
        yamlroot = YAML::LoadFile(pathName);

        model = getMaterialFromYAML(library, yamlroot, path);
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


void MaterialLoader::dereference(
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> materialMap,
    std::shared_ptr<Material> material)
{
    // Avoid recursion
    if (material->getDereferenced()) {
        return;
    }

    // Base::Console().Log("Dereferencing material '%s'.\n",
    //                     material->getName().toStdString().c_str());

    auto parentUUID = material->getParentUUID();
    if (parentUUID.size() > 0) {
        std::shared_ptr<Material> parent;
        try {
            parent = materialMap->at(parentUUID);
        }
        catch (std::out_of_range&) {
            Base::Console().Log(
                "Unable to apply inheritance for material '%s', parent '%s' not found.\n",
                material->getName().toStdString().c_str(),
                parentUUID.toStdString().c_str());
            return;
        }

        // Ensure the parent has been dereferenced
        dereference(materialMap, parent);

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
            auto property = itp->second;

            if (material->getPhysicalProperty(name)->isNull()) {
                material->getPhysicalProperty(name)->setValue(property->getValue());
            }
        }

        properties = parent->getAppearanceProperties();
        for (auto itp = properties.begin(); itp != properties.end(); itp++) {
            auto name = itp->first;
            auto property = itp->second;

            if (material->getAppearanceProperty(name)->isNull()) {
                material->getAppearanceProperty(name)->setValue(property->getValue());
            }
        }
    }

    material->markDereferenced();
}

void MaterialLoader::dereference(std::shared_ptr<Material> material)
{
    dereference(_materialMap, material);
}

void MaterialLoader::loadLibrary(std::shared_ptr<MaterialLibrary> library)
{
    if (_materialEntryMap == nullptr) {
        _materialEntryMap = std::make_unique<std::map<QString, std::shared_ptr<MaterialEntry>>>();
    }

    QDirIterator it(library->getDirectory(), QDirIterator::Subdirectories);
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
            loadLibrary(*it);
        }
    }

    for (auto it = _materialMap->begin(); it != _materialMap->end(); it++) {
        dereference(it->second);
    }
}

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialLoader::getMaterialLibraries()
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
        auto libData =
            std::make_shared<MaterialLibrary>(QString::fromStdString("System"),
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
                    auto libData = std::make_shared<MaterialLibrary>(moduleName,
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
            if (!materialDir.exists()) {
                // Try creating the user dir if it doesn't exist
                if (!materialDir.mkpath(resourceDir)) {
                    Base::Console().Log("Unable to create user library '%s'\n",
                                        resourceDir.toStdString().c_str());
                }
            }
            if (materialDir.exists()) {
                auto libData = std::make_shared<MaterialLibrary>(
                    QString::fromStdString("User"),
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
                auto libData =
                    std::make_shared<MaterialLibrary>(QString::fromStdString("Custom"),
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
