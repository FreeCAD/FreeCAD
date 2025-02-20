/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "ComplexGeoData.h"
#include "Document.h"
#include "DocumentObject.h"
#include "PropertyComplexGeoData.h"


namespace App {

// ------------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyGeometry, App::Property)

PropertyGeometry::PropertyGeometry() = default;

PropertyGeometry::~PropertyGeometry() = default;

// ------------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyComplexGeoData, App::PropertyGeometry)

PropertyComplexGeoData::PropertyComplexGeoData() = default;

PropertyComplexGeoData::~PropertyComplexGeoData() = default;

std::string PropertyComplexGeoData::getElementMapVersion(bool) const
{
    auto data = getComplexData();
    if (!data) {
        return std::string();
    }
    auto owner = Base::freecad_dynamic_cast<DocumentObject>(getContainer());
    std::ostringstream ss;
    if (owner && owner->getDocument() && owner->getDocument()->getStringHasher() == data->Hasher) {
        ss << "1.";
    }
    else {
        ss << "0.";
    }
    ss << data->getElementMapVersion();
    return ss.str();
}

bool PropertyComplexGeoData::checkElementMapVersion(const char* ver) const
{
    auto data = getComplexData();
    if (!data) {
        return false;
    }
    auto owner = Base::freecad_dynamic_cast<DocumentObject>(getContainer());
    std::ostringstream ss;
    const char* prefix;
    if (owner && owner->getDocument() && owner->getDocument()->getStringHasher() == data->Hasher) {
        prefix = "1.";
    }
    else {
        prefix = "0.";
    }
    if (!boost::starts_with(ver, prefix)) {
        return true;
    }
    return data->checkElementMapVersion(ver + 2);
}


void PropertyComplexGeoData::afterRestore()
{
    auto data = getComplexData();
    if (data && data->isRestoreFailed()) {
        data->resetRestoreFailure();
        auto owner = Base::freecad_dynamic_cast<DocumentObject>(getContainer());
        if (owner && owner->getDocument()
            && !owner->getDocument()->testStatus(App::Document::PartialDoc)) {
            owner->getDocument()->addRecomputeObject(owner);
        }
    }
    PropertyGeometry::afterRestore();
}

}  // namespace App
