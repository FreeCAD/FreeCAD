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
#include <QDirIterator>
#include <QFileInfo>
#include <QList>
#include <QMetaType>
#include <QRegularExpression>
#include <QString>
#endif

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Base/Stream.h>
#include <Gui/MetaTypes.h>

#include "Materials.h"

#include "MaterialConfigLoader.h"
#include "MaterialLibrary.h"
#include "MaterialLoader.h"
#include "Model.h"
#include "ModelManager.h"


using namespace Materials;

MaterialEntry::MaterialEntry(const std::shared_ptr<MaterialLibraryLocal>& library,
                             const QString& modelName,
                             const QString& dir,
                             const QString& modelUuid)
    : _library(library)
    , _name(modelName)
    , _directory(dir)
    , _uuid(modelUuid)
{}

MaterialYamlEntry::MaterialYamlEntry(const std::shared_ptr<MaterialLibraryLocal>& library,
                                     const QString& modelName,
                                     const QString& dir,
                                     const QString& modelUuid,
                                     const YAML::Node& modelData)
    : MaterialEntry(library, modelName, dir, modelUuid)
    , _model(modelData)
{}

QString MaterialYamlEntry::yamlValue(const YAML::Node& node,
                                     const std::string& key,
                                     const std::string& defaultValue)
{
    if (node[key]) {
        return QString::fromStdString(node[key].as<std::string>());
    }
    return QString::fromStdString(defaultValue);
}

std::shared_ptr<QList<QVariant>> MaterialYamlEntry::readList(const YAML::Node& node,
                                                             bool isImageList)
{
    auto list = std::make_shared<QList<QVariant>>();
    for (auto it = node.begin(); it != node.end(); it++) {
        QVariant nodeValue;
        if (isImageList) {
            nodeValue = QString::fromStdString(it->as<std::string>())
                            .remove(QRegularExpression(QStringLiteral("[\r\n]")));
        }
        else {
            nodeValue = QString::fromStdString(it->as<std::string>());
        }
        list->append(nodeValue);
    }

    return list;
}

std::shared_ptr<QList<QVariant>> MaterialYamlEntry::readImageList(const YAML::Node& node)
{
    return readList(node, true);
}

std::shared_ptr<Array2D> MaterialYamlEntry::read2DArray(const YAML::Node& node, int columns)
{
    auto array2d = std::make_shared<Array2D>();
    array2d->setColumns(columns);

    if (node.size() == 1 || node.size() == 2) {
        // There used to be a default value. Ignore it.
        auto yamlArray = node[0];
        if (node.size() == 2) {
            yamlArray = node[1];
        }

        for (std::size_t i = 0; i < yamlArray.size(); i++) {
            auto yamlRow = yamlArray[i];

            auto row = std::make_shared<QList<QVariant>>();
            for (std::size_t j = 0; j < yamlRow.size(); j++) {
                Base::Quantity qq = Base::Quantity::parse(yamlRow[j].as<std::string>());
                qq.setFormat(MaterialValue::getQuantityFormat());
                row->push_back(QVariant::fromValue(qq));
            }
            array2d->addRow(row);
        }
    }

    return array2d;
}

std::shared_ptr<Array3D> MaterialYamlEntry::read3DArray(const YAML::Node& node, int columns)
{
    auto array3d = std::make_shared<Array3D>();
    array3d->setColumns(columns - 1);  // First column is third dimension

    if (node.size() == 1 || node.size() == 2) {
        // There used to be a default value. Ignore it.
        auto yamlArray = node[0];
        if (node.size() == 2) {
            yamlArray = node[1];
        }

        for (std::size_t depth = 0; depth < yamlArray.size(); depth++) {
            auto yamlDepth = yamlArray[depth];
            for (auto it = yamlDepth.begin(); it != yamlDepth.end(); it++) {
                auto depthValue = Base::Quantity::parse(it->first.as<std::string>());
                depthValue.setFormat(MaterialValue::getQuantityFormat());
                array3d->addDepth(depth, depthValue);

                auto yamlTable = it->second;
                for (std::size_t i = 0; i < yamlTable.size(); i++) {
                    auto yamlRow = yamlTable[i];

                    auto row = std::make_shared<QList<Base::Quantity>>();
                    for (std::size_t j = 0; j < yamlRow.size(); j++) {
                        auto qq = Base::Quantity::parse(yamlRow[j].as<std::string>());
                        qq.setFormat(MaterialValue::getQuantityFormat());
                        row->push_back(qq);
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
    exclude.insert(QStringLiteral("General"));
    exclude.insert(QStringLiteral("Inherits"));

    auto yamlModel = getModel();
    auto library = getLibrary();
    auto name = getName();
    auto directory = getDirectory();
    QString uuid = getUUID();

    QString author = yamlValue(yamlModel["General"], "Author", "");
    QString license = yamlValue(yamlModel["General"], "License", "");
    QString description = yamlValue(yamlModel["General"], "Description", "");
    QString sourceReference = yamlValue(yamlModel["General"], "ReferenceSource", "");
    QString sourceURL = yamlValue(yamlModel["General"], "SourceURL", "");

    std::shared_ptr<Material> finalModel =
        std::make_shared<Material>(library, directory, uuid, name);
    finalModel->setAuthor(author);
    finalModel->setLicense(license);
    finalModel->setDescription(description);
    finalModel->setReference(sourceReference);
    finalModel->setURL(sourceURL);

    if (yamlModel["General"]["Tags"]) {
        auto tags = readList(yamlModel["General"]["Tags"]);
        for (auto tag : *tags) {
            finalModel->addTag(tag.toString());
        }
    }

    // Add inheritance list
    if (yamlModel["Inherits"]) {
        auto inherits = yamlModel["Inherits"];
        for (auto it = inherits.begin(); it != inherits.end(); it++) {
            auto nodeName = it->second["UUID"].as<std::string>();

            finalModel->setParentUUID(
                QString::fromStdString(nodeName));  // Should only be one. Need to check
        }
    }

    // Add material models
    if (yamlModel["Models"]) {
        auto models = yamlModel["Models"];
        for (auto it = models.begin(); it != models.end(); it++) {
            auto modelName = (it->first).as<std::string>();

            // Add the model uuid
            auto modelNode = models[modelName];
            auto modelUUID = modelNode["UUID"].as<std::string>();
            finalModel->addPhysical(QString::fromStdString(modelUUID));

            // Add the property values
            auto properties = yamlModel["Models"][modelName];
            for (auto itp = properties.begin(); itp != properties.end(); itp++) {
                auto propertyName = (itp->first).as<std::string>();
                if (finalModel->hasPhysicalProperty(QString::fromStdString(propertyName))) {
                    auto prop =
                        finalModel->getPhysicalProperty(QString::fromStdString(propertyName));
                    auto type = prop->getType();

                    try {
                        if (type == MaterialValue::List || type == MaterialValue::FileList) {
                            auto list = readList(itp->second);
                            finalModel->setPhysicalValue(QString::fromStdString(propertyName),
                                                         list);
                        }
                        else if (type == MaterialValue::ImageList) {
                            auto list = readImageList(itp->second);
                            finalModel->setPhysicalValue(QString::fromStdString(propertyName),
                                                         list);
                        }
                        else if (type == MaterialValue::Array2D) {
                            auto array2d = read2DArray(itp->second, prop->columns());
                            finalModel->setPhysicalValue(QString::fromStdString(propertyName),
                                                         array2d);
                        }
                        else if (type == MaterialValue::Array3D) {
                            auto array3d = read3DArray(itp->second, prop->columns());
                            finalModel->setPhysicalValue(QString::fromStdString(propertyName),
                                                         array3d);
                        }
                        else {
                            QString propertyValue =
                                QString::fromStdString((itp->second).as<std::string>());
                            if (type == MaterialValue::Image) {
                                propertyValue = propertyValue.remove(
                                    QRegularExpression(QStringLiteral("[\r\n]")));
                            }
                            try {
                                finalModel->setPhysicalValue(QString::fromStdString(propertyName),
                                                            propertyValue);
                            }
                            catch (const Base::ValueError&) {
                                // Units mismatch
                                Base::Console().log("Units mismatch in material '%s':'%s' = '%s', "
                                                    "setting to default property units '%s'\n",
                                                    name.toStdString().c_str(),
                                                    propertyName,
                                                    propertyValue.toStdString().c_str(),
                                                    prop->getUnits().toStdString().c_str());
                                auto quantity = Base::Quantity::parse(propertyValue.toStdString());
                                finalModel->setPhysicalValue(
                                    QString::fromStdString(propertyName),
                                    Base::Quantity(quantity.getValue(),
                                                   prop->getUnits().toStdString()));
                            }
                        }
                    }
                    catch (const YAML::BadConversion& e) {
                        Base::Console().log("Exception %s <%s:%s> - ignored\n",
                                            e.what(),
                                            name.toStdString().c_str(),
                                            propertyName.c_str());
                    }
                }
                else if (propertyName != "UUID") {
                    Base::Console().log("\tProperty '%s' is not described by any model. Ignored\n",
                                        propertyName.c_str());
                }
            }
        }
    }

    // Add appearance models
    if (yamlModel["AppearanceModels"]) {
        auto models = yamlModel["AppearanceModels"];
        for (auto it = models.begin(); it != models.end(); it++) {
            auto modelName = (it->first).as<std::string>();

            // Add the model uuid
            auto modelNode = models[modelName];
            auto modelUUID = modelNode["UUID"].as<std::string>();
            finalModel->addAppearance(QString::fromStdString(modelUUID));

            // Add the property values
            auto properties = yamlModel["AppearanceModels"][modelName];
            for (auto itp = properties.begin(); itp != properties.end(); itp++) {
                auto propertyName = (itp->first).as<std::string>();
                if (finalModel->hasAppearanceProperty(QString::fromStdString(propertyName))) {
                    auto prop =
                        finalModel->getAppearanceProperty(QString::fromStdString(propertyName));
                    auto type = prop->getType();

                    try {
                        if (type == MaterialValue::List || type == MaterialValue::FileList) {
                            auto list = readList(itp->second);
                            finalModel->setAppearanceValue(QString::fromStdString(propertyName),
                                                           list);
                        }
                        else if (type == MaterialValue::ImageList) {
                            auto list = readImageList(itp->second);
                            finalModel->setAppearanceValue(QString::fromStdString(propertyName),
                                                           list);
                        }
                        else if (type == MaterialValue::Array2D) {
                            auto array2d = read2DArray(itp->second, prop->columns());
                            finalModel->setAppearanceValue(QString::fromStdString(propertyName),
                                                           array2d);
                        }
                        else if (type == MaterialValue::Array3D) {
                            auto array3d = read3DArray(itp->second, prop->columns());
                            finalModel->setAppearanceValue(QString::fromStdString(propertyName),
                                                           array3d);
                        }
                        else {
                            QString propertyValue =
                                QString::fromStdString((itp->second).as<std::string>());
                            if (type == MaterialValue::Image) {
                                propertyValue = propertyValue.remove(
                                    QRegularExpression(QStringLiteral("[\r\n]")));
                            }
                            finalModel->setAppearanceValue(QString::fromStdString(propertyName),
                                                           propertyValue);
                        }
                    }
                    catch (const YAML::BadConversion& e) {
                        Base::Console().log("Exception %s <%s:%s> - ignored\n",
                                            e.what(),
                                            name.toStdString().c_str(),
                                            propertyName.c_str());
                    }
                }
                else if (propertyName != "UUID") {
                    Base::Console().log("\tProperty '%s' is not described by any model. Ignored\n",
                                        propertyName.c_str());
                }
            }
        }
    }

    QString path = QDir(directory).absolutePath();
    (*materialMap)[uuid] = library->addMaterial(finalModel, path);
}

//===

std::unique_ptr<std::map<QString, std::shared_ptr<MaterialEntry>>>
    MaterialLoader::_materialEntryMap = nullptr;

MaterialLoader::MaterialLoader(
    const std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>& materialMap,
    const std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>& libraryList)
    : _materialMap(materialMap)
    , _libraryList(libraryList)
{
    loadLibraries(libraryList);
}

void MaterialLoader::addLibrary(const std::shared_ptr<MaterialLibraryLocal>& model)
{
    _libraryList->push_back(model);
}

std::shared_ptr<MaterialEntry>
MaterialLoader::getMaterialFromYAML(const std::shared_ptr<MaterialLibraryLocal>& library,
                                    YAML::Node& yamlroot,
                                    const QString& path)
{
    std::shared_ptr<MaterialEntry> model = nullptr;

    try {
        auto uuid = yamlroot["General"]["UUID"].as<std::string>();

        // Always get the name from the filename
        QFileInfo filepath(path);
        QString name =
            filepath.fileName().remove(QStringLiteral(".FCMat"), Qt::CaseInsensitive);

        model = std::make_shared<MaterialYamlEntry>(library,
                                                    name,
                                                    path,
                                                    QString::fromStdString(uuid),
                                                    yamlroot);
    }
    catch (YAML::Exception const& e) {
        Base::Console().error("YAML parsing error: '%s'\n", path.toStdString().c_str());
        Base::Console().error("\t'%s'\n", e.what());
        showYaml(yamlroot);
    }


    return model;
}

std::shared_ptr<MaterialEntry>
MaterialLoader::getMaterialFromPath(const std::shared_ptr<MaterialLibraryLocal>& library,
                                    const QString& path) const
{
    std::shared_ptr<MaterialEntry> model = nullptr;
    auto materialLibrary =
        reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);

    // Used for debugging
    std::string pathName = path.toStdString();

    if (MaterialConfigLoader::isConfigStyle(path)) {
        auto material = MaterialConfigLoader::getMaterialFromPath(materialLibrary, path);
        if (material) {
            (*_materialMap)[material->getUUID()] = materialLibrary->addMaterial(material, path);
        }

        // Return the nullptr as there are no intermediate steps to take, such
        // as checking inheritance
        return model;
    }

    Base::FileInfo info(pathName);
    Base::ifstream fin(info);
    if (!fin) {
        Base::Console().error("YAML file open error: '%s'\n", pathName.c_str());
        return model;
    }

    YAML::Node yamlroot;
    try {
        yamlroot = YAML::Load(fin);

        model = getMaterialFromYAML(materialLibrary, yamlroot, path);
    }
    catch (YAML::Exception const& e) {
        Base::Console().error("YAML parsing error: '%s'\n", pathName.c_str());
        Base::Console().error("\t'%s'\n", e.what());
        showYaml(yamlroot);
    }


    return model;
}

void MaterialLoader::showYaml(const YAML::Node& yaml)
{
    std::stringstream out;

    out << yaml;
    std::string logData = out.str();
    Base::Console().log("%s\n", logData.c_str());
}


void MaterialLoader::dereference(
    const std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>& materialMap,
    const std::shared_ptr<Material>& material)
{
    // Avoid recursion
    if (material->getDereferenced()) {
        return;
    }

    auto parentUUID = material->getParentUUID();
    if (parentUUID.size() > 0) {
        std::shared_ptr<Material> parent;
        try {
            parent = materialMap->at(parentUUID);
        }
        catch (std::out_of_range&) {
            Base::Console().log(
                "Unable to apply inheritance for material '%s', parent '%s' not found.\n",
                material->getName().toStdString().c_str(),
                parentUUID.toStdString().c_str());
            return;
        }

        // Ensure the parent has been dereferenced
        dereference(materialMap, parent);

        // Add physical models
        auto modelVector = parent->getPhysicalModels();
        for (auto& model : *modelVector) {
            if (!material->hasPhysicalModel(model)) {
                material->addPhysical(model);
            }
        }

        // Add appearance models
        modelVector = parent->getAppearanceModels();
        for (auto& model : *modelVector) {
            if (!material->hasAppearanceModel(model)) {
                material->addAppearance(model);
            }
        }

        // Add values
        auto properties = parent->getPhysicalProperties();
        for (auto& itp : properties) {
            auto name = itp.first;
            auto property = itp.second;

            if (material->getPhysicalProperty(name)->isNull()) {
                material->getPhysicalProperty(name)->setValue(property->getValue());
            }
        }

        properties = parent->getAppearanceProperties();
        for (auto& itp : properties) {
            auto name = itp.first;
            auto property = itp.second;

            if (material->getAppearanceProperty(name)->isNull()) {
                material->getAppearanceProperty(name)->setValue(property->getValue());
            }
        }
    }

    material->markDereferenced();
}

void MaterialLoader::dereference(const std::shared_ptr<Material>& material)
{
    dereference(_materialMap, material);
}

void MaterialLoader::loadLibrary(const std::shared_ptr<MaterialLibraryLocal>& library)
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
                try {
                    auto model = getMaterialFromPath(library, file.canonicalFilePath());
                    if (model) {
                        (*_materialEntryMap)[model->getUUID()] = model;
                    }
                }
                catch (const MaterialReadError&) {
                    // Ignore the file. Error messages should have already been logged
                }
            }
        }
    }

    for (auto& it : *_materialEntryMap) {
        it.second->addToTree(_materialMap);
    }
}

void MaterialLoader::loadLibraries(
    const std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>& libraryList)
{
    if (libraryList) {
        for (auto& it : *libraryList) {
            if (it->isLocal()) {
                auto materialLibrary =
                    reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(it);
                loadLibrary(materialLibrary);
            }
        }
    }

    for (auto& it : *_materialMap) {
        dereference(it.second);
    }
}

std::shared_ptr<std::list<QString>>
MaterialLoader::getMaterialFolders(const MaterialLibraryLocal& library)
{
    std::shared_ptr<std::list<QString>> pathList = std::make_shared<std::list<QString>>();
    QDirIterator it(library.getDirectory(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto pathname = it.next();
        QFileInfo file(pathname);
        if (file.isDir()) {
            QString path = QDir(library.getDirectory()).relativeFilePath(file.absoluteFilePath());
            if (!path.startsWith(QStringLiteral("."))) {
                pathList->push_back(path);
            }
        }
    }

    return pathList;
}
