// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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

#pragma once

#include <Base/Parameter.h>
#include <CXX/Objects.hxx>

#include <Mod/Material/MaterialGlobal.h>

class QMutex;
class QByteArray;

namespace Materials
{

class Library;
class LibraryObject;
class Material;
class Model;
class MaterialFilter;
class MaterialFilterOptions;

class MaterialsExport ExternalManager: public ParameterGrp::ObserverType
{
public:

    static ExternalManager* getManager();

    /// Observer message from the ParameterGrp
    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;

    // Library management
    std::shared_ptr<std::vector<std::shared_ptr<Library>>> libraries();
    std::shared_ptr<std::vector<std::shared_ptr<Library>>> modelLibraries();
    std::shared_ptr<std::vector<std::shared_ptr<Library>>> materialLibraries();
    std::shared_ptr<Library> getLibrary(const std::string& name);
    void createLibrary(const std::string& libraryName,
                       const QByteArray& icon,
                       bool readOnly = true);
    void renameLibrary(const std::string& libraryName, const std::string& newName);
    void changeIcon(const std::string& libraryName, const QByteArray& icon);
    void removeLibrary(const std::string& libraryName);
    std::shared_ptr<std::vector<LibraryObject>> libraryModels(const std::string& libraryName);
    std::shared_ptr<std::vector<LibraryObject>> libraryMaterials(const std::string& libraryName);
    std::shared_ptr<std::vector<LibraryObject>>
    libraryMaterials(const std::string& libraryName,
                     const MaterialFilter& filter,
                     const MaterialFilterOptions& options);
    std::shared_ptr<std::vector<std::string>> libraryFolders(const std::string& libraryName);

    // Folder management
    void createFolder(const std::string& libraryName, const std::string& path);
    void renameFolder(const std::string& libraryName,
                      const std::string& oldPath,
                      const std::string& newPath);
    void deleteRecursive(const std::string& libraryName, const std::string& path);

    // Model management
    std::shared_ptr<Model> getModel(const std::string& uuid);
    void
    addModel(const std::string& libraryName, const std::string& path, const Model& model);
    void
    migrateModel(const std::string& libraryName, const std::string& path, const Model& model);
    void updateModel(const std::string& libraryName,
                     const std::string& path,
                     const Model& model);
    void setModelPath(const std::string& libraryName, const std::string& path, const std::string& uuid);
    void renameModel(const std::string& libraryName, const std::string& name, const std::string& uuid);
    void moveModel(const std::string& libraryName, const std::string& path, const std::string& uuid);
    void removeModel(const std::string& uuid);

    // Material management
    std::shared_ptr<Material> getMaterial(const std::string& uuid);
    void addMaterial(const std::string& libraryName,
                     const std::string& path,
                     const Material& material);
    void migrateMaterial(const std::string& libraryName,
                     const std::string& path,
                     const Material& material);
    void updateMaterial(const std::string& libraryName,
                        const std::string& path,
                        const Material& material);
    void setMaterialPath(const std::string& libraryName, const std::string& path, const std::string& uuid);
    void renameMaterial(const std::string& libraryName, const std::string& name, const std::string& uuid);
    void moveMaterial(const std::string& libraryName, const std::string& path, const std::string& uuid);
    void removeMaterial(const std::string& uuid);

private:
    ExternalManager();
    ~ExternalManager() override;

    static void initManager();
    void getConfiguration();
    void instantiate();
    void connect();
    bool checkMaterialLibraryType(const Py::Object& entry);
    std::shared_ptr<Library> libraryFromObject(const Py::Object& entry);
    bool checkMaterialLibraryObjectType(const Py::Object& entry);
    LibraryObject materialLibraryObjectTypeFromObject(const Py::Object& entry);
    bool checkModelObjectType(const Py::Object& entry);
    std::shared_ptr<Model> modelFromObject(const Py::Object& entry, const std::string& uuid);
    bool checkMaterialObjectType(const Py::Object& entry);
    std::shared_ptr<Material> materialFromObject(const Py::Object& entry, const std::string& uuid);

    static ExternalManager* _manager;
    static QMutex _mutex;

    // COnfiguration
    ParameterGrp::handle _hGrp;
    std::string _moduleName;
    std::string _className;
    bool _instantiated;

    Py::Object _managerObject;
};

}  // namespace Materials