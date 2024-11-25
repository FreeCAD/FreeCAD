/***************************************************************************
 *   Copyright (c) 2015 Stefan Tr�ger <stefantroeger@gmx.net>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
 *   Copyright (c) 2024 Ondsel (PL Boyer) <development@ondsel.com>         *
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
# include <string>
#endif

#include <App/Document.h>
#include <Base/Exception.h>
#include <Base/Placement.h>

#include "Datums.h"

#ifndef M_PI
# define M_PI       3.14159265358979323846
#endif

using namespace App;

PROPERTY_SOURCE(App::DatumElement, App::GeoFeature)
PROPERTY_SOURCE(App::Plane, App::DatumElement)
PROPERTY_SOURCE(App::Line, App::DatumElement)
PROPERTY_SOURCE(App::Point, App::DatumElement)
PROPERTY_SOURCE(App::LocalCoordinateSystem, App::GeoFeature)

DatumElement::DatumElement(bool hideRole)
{
    ADD_PROPERTY_TYPE ( Role, (""), 0, App::Prop_ReadOnly, "Role of the datum in the local coordinate system." ) ;

    // The role is hidden by default. It is visible only when the datum is in a LCS
    if (hideRole) {
        Role.setStatus(Property::Hidden, true);
    }
}

DatumElement::~DatumElement() = default;

App::LocalCoordinateSystem* DatumElement::getLCS ()
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

bool DatumElement::isOriginFeature()
{
    return getLCS()->isOrigin();
}

// ----------------------------------------------------------------------------

LocalCoordinateSystem::LocalCoordinateSystem() : extension(this) {
    ADD_PROPERTY_TYPE(OriginFeatures, (nullptr), 0, App::Prop_Hidden,
        "Axis and baseplanes controlled by the LCS");

    setStatus(App::NoAutoExpand, true);
    extension.initExtension(this);
}


LocalCoordinateSystem::~LocalCoordinateSystem() = default;

App::DatumElement* LocalCoordinateSystem::getDatumElement(const char* role) const {
    const auto& features = OriginFeatures.getValues();
    auto featIt = std::find_if(features.begin(), features.end(),
        [role](App::DocumentObject* obj) {
        return obj->isDerivedFrom(App::DatumElement::getClassTypeId()) &&
            strcmp(static_cast<App::DatumElement*>(obj)->Role.getValue(), role) == 0;
    });
    if (featIt != features.end()) {
        return static_cast<App::DatumElement*>(*featIt);
    }
    else {

        std::stringstream err;
        err << "LocalCoordinateSystem \"" << getFullName() << "\" doesn't contain feature with role \""
            << role << '"';
        throw Base::RuntimeError(err.str().c_str());
    }
}

App::Line* LocalCoordinateSystem::getAxis(const char* role) const {
    App::DatumElement* feat = getDatumElement(role);
    if (feat->isDerivedFrom(App::Line::getClassTypeId())) {
        return static_cast<App::Line*> (feat);
    }
    else {
        std::stringstream err;
        err << "LocalCoordinateSystem \"" << getFullName() << "\" contains bad Axis object for role \""
            << role << '"';
        throw Base::RuntimeError(err.str().c_str());
    }
}

App::Plane* LocalCoordinateSystem::getPlane(const char* role) const {
    App::DatumElement* feat = getDatumElement(role);
    if (feat->isDerivedFrom(App::Plane::getClassTypeId())) {
        return static_cast<App::Plane*> (feat);
    }
    else {
        std::stringstream err;
        err << "LocalCoordinateSystem \"" << getFullName() << "\" contains bad Plane object for role \""
            << role << '"';
        throw Base::RuntimeError(err.str().c_str());
    }
}

App::Point* LocalCoordinateSystem::getPoint(const char* role) const {
    App::DatumElement* feat = getDatumElement(role);
    if (feat->isDerivedFrom(App::Point::getClassTypeId())) {
        return static_cast<App::Point*> (feat);
    }
    else {
        std::stringstream err;
        err << "LocalCoordinateSystem \"" << getFullName() << "\" contains bad Point object for role \""
            << role << '"';
        throw Base::RuntimeError(err.str().c_str());
    }
}

bool LocalCoordinateSystem::hasObject(const DocumentObject * obj) const {
    const auto& features = OriginFeatures.getValues();
    return std::find(features.begin(), features.end(), obj) != features.end();
}

short LocalCoordinateSystem::mustExecute() const {
    if (OriginFeatures.isTouched()) {
        return 1;
    }
    else {
        return DocumentObject::mustExecute();
    }
}

App::DocumentObjectExecReturn* LocalCoordinateSystem::execute() {
    try { // try to find all base axis and planes in the origin
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

void LocalCoordinateSystem::setupObject() {
    const static struct {
        const Base::Type type;
        const char* role;
        const QString label;
        Base::Rotation rot;
    }
    setupData[] = {
        // clang-format off
        {App::Line::getClassTypeId(),  AxisRoles[0],  tr("X-axis"),   Base::Rotation()},
        {App::Line::getClassTypeId(),  AxisRoles[1],  tr("Y-axis"),   Base::Rotation(Base::Vector3d(1,1,1), M_PI * 2 / 3)},
        {App::Line::getClassTypeId(),  AxisRoles[2],  tr("Z-axis"),   Base::Rotation(Base::Vector3d(1,-1,1), M_PI * 2 / 3)},
        {App::Plane::getClassTypeId(), PlaneRoles[0], tr("XY-plane"), Base::Rotation()},
        {App::Plane::getClassTypeId(), PlaneRoles[1], tr("XZ-plane"), Base::Rotation(1.0, 0.0, 0.0, 1.0)},
        {App::Plane::getClassTypeId(), PlaneRoles[2], tr("YZ-plane"), Base::Rotation(Base::Vector3d(1,1,1), M_PI * 2 / 3)},
        {App::Point::getClassTypeId(), PointRoles[0], tr("Origin"),   Base::Rotation()}
        // clang-format on
    };

    App::Document* doc = getDocument();

    std::vector<App::DocumentObject*> links;
    for (auto data : setupData) {
        std::string objName = doc->getUniqueObjectName(data.role);
        App::DocumentObject* featureObj = doc->addObject(data.type.getName(), objName.c_str());

        assert(featureObj && featureObj->isDerivedFrom(App::DatumElement::getClassTypeId()));

        QByteArray byteArray = data.label.toUtf8();
        featureObj->Label.setValue(byteArray.constData());

        App::DatumElement* feature = static_cast <App::DatumElement*> (featureObj);
        feature->Placement.setValue(Base::Placement(Base::Vector3d(), data.rot));
        feature->Role.setValue(data.role);

        feature->Placement.setStatus(Property::Hidden, true);

        links.push_back(feature);
    }

    OriginFeatures.setValues(links);
}

void LocalCoordinateSystem::unsetupObject() {
    const auto& objsLnk = OriginFeatures.getValues();
    // Copy to set to assert we won't call methode more then one time for each object
    std::set<App::DocumentObject*> objs(objsLnk.begin(), objsLnk.end());
    // Remove all controlled objects
    for (auto obj : objs) {
        // Check that previous deletes wasn't inderectly removed one of our objects
        const auto& objsLnk = OriginFeatures.getValues();
        if (std::find(objsLnk.begin(), objsLnk.end(), obj) != objsLnk.end()) {
            if (!obj->isRemoving()) {
                obj->getDocument()->removeObject(obj->getNameInDocument());
            }
        }
    }
}

// ----------------------------------------------------------------------------

LocalCoordinateSystem::LCSExtension::LCSExtension(LocalCoordinateSystem * obj)
    : obj(obj)
{
    Group.setStatus(Property::Transient, true);
}

void LocalCoordinateSystem::LCSExtension::initExtension(ExtensionContainer * obj) {
    App::GroupExtension::initExtension(obj);
}

bool LocalCoordinateSystem::LCSExtension::extensionGetSubObject(DocumentObject * &ret, const char* subname,
    PyObject * *pyobj, Base::Matrix4D * mat, bool, int depth) const {
    if (!subname || subname[0] == '\0') {
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
        ret = obj->getDatumElement(name.c_str());
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
        e.ReportException();
        return false;
    }
}
