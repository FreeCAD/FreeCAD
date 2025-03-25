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

#ifndef MATERIAL_EXTERNALMANAGER_H
#define MATERIAL_EXTERNALMANAGER_H

#include <Base/Parameter.h>
#include <CXX/Objects.hxx>

#include <Mod/Material/MaterialGlobal.h>

class QMutex;

namespace Materials
{

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
    std::shared_ptr<std::vector<std::tuple<QString, QString, bool>>> libraries();
    std::shared_ptr<std::vector<std::tuple<QString, QString, bool>>> modelLibraries();
    std::shared_ptr<std::vector<std::tuple<QString, QString, bool>>> materialLibraries();
    std::tuple<QString, QString, bool> getLibrary(const QString& name) const;
    void createLibrary(const QString& libraryName, const QString& icon, bool readOnly = true);
    void renameLibrary(const QString& libraryName, const QString& newName);
    void changeIcon(const QString& libraryName, const QString& icon);
    void removeLibrary(const QString& libraryName);
    std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
    libraryModels(const QString& libraryName);
    std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
    libraryMaterials(const QString& libraryName);
    std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
    libraryMaterials(const QString& libraryName,
                     const std::shared_ptr<MaterialFilter>& filter,
                     const MaterialFilterOptions& options);

    // Model management
    std::shared_ptr<Model> getModel(const QString& uuid);
    void
    addModel(const QString& libraryName, const QString& path, const std::shared_ptr<Model>& model);
    void
    migrateModel(const QString& libraryName, const QString& path, const std::shared_ptr<Model>& model);

    // Material management
    std::shared_ptr<Material> getMaterial(const QString& uuid);
    void addMaterial(const QString& libraryName,
                     const QString& path,
                     const std::shared_ptr<Material>& material);
    void migrateMaterial(const QString& libraryName,
                     const QString& path,
                     const std::shared_ptr<Material>& material);

private:
    ExternalManager();
    ~ExternalManager();

    static void initManager();
    void getConfiguration();
    void instantiate();
    void connect();

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

#endif  // MATERIAL_EXTERNALMANAGER_H