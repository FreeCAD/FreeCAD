/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
 *   Copyright (c) 2024 Ondsel (PL Boyer) <development@ondsel.com>         *
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
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <string>
#endif

#include <App/Document.h>
#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Tools.h>

#include "Datums.h"

using namespace App;

PROPERTY_SOURCE(App::DatumElement, App::GeoFeature)
PROPERTY_SOURCE(App::Plane, App::DatumElement)
PROPERTY_SOURCE(App::Line, App::DatumElement)
PROPERTY_SOURCE(App::Point, App::DatumElement)
PROPERTY_SOURCE_WITH_EXTENSIONS(App::LocalCoordinateSystem, App::GeoFeature)

DatumElement::DatumElement(bool hideRole)
    : baseDir{0.0, 0.0, 1.0}
{
    ADD_PROPERTY_TYPE(Role,
                      (""),
                      0,
                      App::Prop_ReadOnly,
                      "Role of the datum in the local coordinate system.");

    // The role is hidden by default. It is visible only when the datum is in a LCS
    if (hideRole) {
        Role.setStatus(Property::Hidden, true);
    }
}

DatumElement::~DatumElement() = default;

bool DatumElement::getCameraAlignmentDirection(Base::Vector3d& directionZ, Base::Vector3d& directionX, const char* subname) const
{
    Q_UNUSED(subname);
    
    Placement.getValue().getRotation().multVec(baseDir, directionZ);

    if (baseDir == Base::Vector3d::UnitZ) {
        Placement.getValue().getRotation().multVec(Base::Vector3d::UnitX, directionX);
    }

    return true;
}

App::LocalCoordinateSystem* DatumElement::getLCS() const
{
    auto inList = getInList();
    for (auto* obj : inList) {
        auto* lcs = dynamic_cast<App::LocalCoordinateSystem*>(obj);
        if (lcs) {
            return lcs;
        }
    }

    return nullptr;
}

bool DatumElement::isOriginFeature() const
{
    const auto* lcs = getLCS();
    return lcs ? lcs->isOrigin() : false;
}

Base::Vector3d DatumElement::getBasePoint() const
{
    Base::Placement plc = Placement.getValue();
    return plc.getPosition();
}

Base::Vector3d DatumElement::getDirection() const
{
    Base::Vector3d dir(baseDir);
    Base::Placement plc = Placement.getValue();
    Base::Rotation rot = plc.getRotation();
    rot.multVec(dir, dir);
    return dir;
}

Base::Vector3d DatumElement::getBaseDirection() const
{
    return baseDir;
}

void DatumElement::setBaseDirection(const Base::Vector3d& dir)
{
    baseDir = dir;
}

// ----------------------------------------------------------------------------

Line::Line()
{
    setBaseDirection(Base::Vector3d(1, 0, 0));
}

Point::Point()
{
    setBaseDirection(Base::Vector3d(0, 0, 0));
}

// ----------------------------------------------------------------------------

LocalCoordinateSystem::LocalCoordinateSystem()
{
    ADD_PROPERTY_TYPE(OriginFeatures,
                      (nullptr),
                      0,
                      App::Prop_Hidden,
                      "Axis and baseplanes controlled by the LCS");

    Group.setStatus(Property::Transient, true);

    setStatus(App::NoAutoExpand, true);

    GroupExtension::initExtension(this);
}


LocalCoordinateSystem::~LocalCoordinateSystem() = default;

bool LocalCoordinateSystem::getCameraAlignmentDirection(Base::Vector3d& directionZ,
                                                        Base::Vector3d& directionX,
                                                        const char* subname) const
{
    Q_UNUSED(subname);

    Placement.getValue().getRotation().multVec(Base::Vector3d::UnitZ, directionZ);
    Placement.getValue().getRotation().multVec(Base::Vector3d::UnitX, directionX);

    return true;
}

App::DatumElement* LocalCoordinateSystem::getDatumElement(const char* role) const
{
    const auto& features = OriginFeatures.getValues();
    auto featIt = std::find_if(features.begin(), features.end(), [role](App::DocumentObject* obj) {
        return obj->isDerivedFrom<App::DatumElement>()
            && strcmp(static_cast<App::DatumElement*>(obj)->Role.getValue(), role) == 0;
    });
    if (featIt != features.end()) {
        return static_cast<App::DatumElement*>(*featIt);
    }
    std::stringstream err;
    err << "LocalCoordinateSystem \"" << getFullName() << "\" doesn't contain feature with role \""
        << role << '"';
    throw Base::RuntimeError(err.str().c_str());
}

App::Line* LocalCoordinateSystem::getAxis(const char* role) const
{
    App::DatumElement* feat = getDatumElement(role);
    if (feat->isDerivedFrom<App::Line>()) {
        return static_cast<App::Line*>(feat);
    }
    std::stringstream err;
    err << "LocalCoordinateSystem \"" << getFullName() << "\" contains bad Axis object for role \""
        << role << '"';
    throw Base::RuntimeError(err.str().c_str());
}

App::Plane* LocalCoordinateSystem::getPlane(const char* role) const
{
    App::DatumElement* feat = getDatumElement(role);
    if (feat->isDerivedFrom<App::Plane>()) {
        return static_cast<App::Plane*>(feat);
    }
    std::stringstream err;
    err << "LocalCoordinateSystem \"" << getFullName() << "\" contains bad Plane object for role \""
        << role << '"';
    throw Base::RuntimeError(err.str().c_str());
}

App::Point* LocalCoordinateSystem::getPoint(const char* role) const
{
    App::DatumElement* feat = getDatumElement(role);
    if (feat->isDerivedFrom<App::Point>()) {
        return static_cast<App::Point*>(feat);
    }
    std::stringstream err;
    err << "LocalCoordinateSystem \"" << getFullName() << "\" contains bad Point object for role \""
        << role << '"';
    throw Base::RuntimeError(err.str().c_str());
}

short LocalCoordinateSystem::mustExecute() const
{
    if (OriginFeatures.isTouched()) {
        return 1;
    }
    return DocumentObject::mustExecute();
}

App::DocumentObjectExecReturn* LocalCoordinateSystem::execute()
{
    try {  // try to find all base axis and planes in the origin
        for (const char* role : AxisRoles) {
            App::Line* axis = getAxis(role);
            assert(axis);
            (void)axis;
        }
        for (const char* role : PlaneRoles) {
            App::Plane* plane = getPlane(role);
            assert(plane);
            (void)plane;
        }
    }
    catch (const Base::Exception& ex) {
        setError();
        return new App::DocumentObjectExecReturn(ex.what());
    }

    return DocumentObject::execute();
}

const std::vector<LocalCoordinateSystem::SetupData>& LocalCoordinateSystem::getSetupData()
{
    using std::numbers::pi;

    static const std::vector<SetupData> setupData = {
        // clang-format off
        {App::Line::getClassTypeId(),  AxisRoles[0],  tr("X-axis"),   Base::Rotation()},
        {App::Line::getClassTypeId(),  AxisRoles[1],  tr("Y-axis"),   Base::Rotation(Base::Vector3d(1, 1, 1), pi * 2 / 3)},
        {App::Line::getClassTypeId(),  AxisRoles[2],  tr("Z-axis"),   Base::Rotation(Base::Vector3d(1,-1, 1), pi * 2 / 3)},
        {App::Plane::getClassTypeId(), PlaneRoles[0], tr("XY-plane"), Base::Rotation()},
        {App::Plane::getClassTypeId(), PlaneRoles[1], tr("XZ-plane"), Base::Rotation(1.0, 0.0, 0.0, 1.0)},
        {App::Plane::getClassTypeId(), PlaneRoles[2], tr("YZ-plane"), Base::Rotation(Base::Vector3d(1, 1, 1), pi * 2 / 3)},
        {App::Point::getClassTypeId(), PointRoles[0], tr("Origin"),   Base::Rotation()}
        // clang-format on
    };
    return setupData;
}

DatumElement* LocalCoordinateSystem::createDatum(const SetupData& data)
{
    App::Document* doc = getDocument();
    std::string objName = doc->getUniqueObjectName(data.role);
    App::DocumentObject* featureObj = doc->addObject(data.type.getName(), objName.c_str());

    assert(featureObj && featureObj->isDerivedFrom<App::DatumElement>());

    QByteArray byteArray = data.label.toUtf8();
    featureObj->Label.setValue(byteArray.constData());

    auto* feature = static_cast<App::DatumElement*>(featureObj);
    feature->Placement.setValue(Base::Placement(Base::Vector3d(), data.rot));
    feature->Role.setValue(data.role);

    feature->Placement.setStatus(Property::Hidden, true);
    return feature;
}

LocalCoordinateSystem::SetupData LocalCoordinateSystem::getData(const char* role)
{
    const auto& setupData = getSetupData();
    for (auto data : setupData) {
        if (std::strcmp(role, data.role) == 0) {
            return data;
        }
    }
    return LocalCoordinateSystem::SetupData();
}

void LocalCoordinateSystem::setupObject()
{
    std::vector<App::DocumentObject*> links;
    const auto& setupData = getSetupData();
    for (auto data : setupData) {
        links.push_back(createDatum(data));
    }

    OriginFeatures.setValues(links);
}

void LocalCoordinateSystem::unsetupObject()
{
    const auto& objsLnk = OriginFeatures.getValues();
    // Copy to set to assert we won't call method more then one time for each object
    const std::set<App::DocumentObject*> objs(objsLnk.begin(), objsLnk.end());
    // Remove all controlled objects
    for (auto obj : objs) {
        // Check that previous deletes didn't indirectly remove one of our objects
        const auto& objsLnk2 = OriginFeatures.getValues();
        if (std::ranges::find(objsLnk2, obj) != objsLnk2.end()) {
            if (!obj->isRemoving()) {
                obj->getDocument()->removeObject(obj->getNameInDocument());
            }
        }
    }
}

void LocalCoordinateSystem::onDocumentRestored()
{
    GeoFeature::onDocumentRestored();

    // In 0.22 origins did not have point.
    migrateOriginPoint();
}

void LocalCoordinateSystem::migrateOriginPoint()
{
    auto features = OriginFeatures.getValues();

    auto isOrigin = [](App::DocumentObject* obj) {
        return obj->isDerivedFrom<App::DatumElement>() &&
            strcmp(static_cast<App::DatumElement*>(obj)->Role.getValue(), PointRoles[0]) == 0;
    };
    if (std::ranges::none_of(features, isOrigin)) {
        auto data = getData(PointRoles[0]);
        auto* origin = createDatum(data);
        origin->purgeTouched();
        features.push_back(origin);
        OriginFeatures.setValues(features);
    }
}

// ----------------------------------------------------------------------------

bool LocalCoordinateSystem::extensionGetSubObject(DocumentObject*& ret,
                                                                const char* subname,
                                                                PyObject** pyobj,
                                                                Base::Matrix4D* mat,
                                                                bool,
                                                                int depth) const
{
    if (Base::Tools::isNullOrEmpty(subname)) {
        return false;
    }

    // mapping of object name to role name
    std::string name(subname);
    for (int i = 0; i < 3; i++) {
        if (name.rfind(LocalCoordinateSystem::AxisRoles[i], 0) == 0) {
            name = LocalCoordinateSystem::AxisRoles[i];
            break;
        }
        if (name.rfind(LocalCoordinateSystem::PlaneRoles[i], 0) == 0) {
            name = LocalCoordinateSystem::PlaneRoles[i];
            break;
        }
    }

    if (name.rfind(LocalCoordinateSystem::PointRoles[0], 0) == 0) {
        name = LocalCoordinateSystem::PointRoles[0];
    }

    try {
        ret = getDatumElement(name.c_str());
        if (!ret) {
            return false;
        }
        const char* dot = strchr(subname, '.');
        if (dot) {
            subname = dot + 1;
        }
        else {
            subname = "";
        }
        ret = ret->getSubObject(subname, pyobj, mat, true, depth + 1);
        return true;
    }
    catch (const Base::Exception& e) {
        e.reportException();
        return false;
    }
}

bool LocalCoordinateSystem::hasObject(const DocumentObject* obj, [[maybe_unused]] bool recursive) const
{
    const auto& features = OriginFeatures.getValues();
    return std::ranges::find(features, obj) != features.end();
}
