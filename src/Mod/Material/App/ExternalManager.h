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

class MaterialsExport ExternalManager
{
public:

    static ExternalManager* getManager();

    // Library management
    std::shared_ptr<std::vector<std::tuple<QString, QString, bool>>> libraries();
    void createLibrary(const QString& libraryName,
                       const QString& icon,
                       bool readOnly = true);

    // Model management
    void
    addModel(const QString& libraryName, const QString& path, const std::shared_ptr<Model>& model);

    // Material management
    void addMaterial(const QString& libraryName,
                     const QString& path,
                     const std::shared_ptr<Material>& material);


private:
    ExternalManager();
    ~ExternalManager();

    static void initManager();
    void getConfiguration();
    void instantiate();

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