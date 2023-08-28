/***************************************************************************
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>

#include "OriginGroupExtension.h"
#include "GeoFeature.h"
#include "Origin.h"


FC_LOG_LEVEL_INIT("App", true, true)

using namespace App;

EXTENSION_PROPERTY_SOURCE(App::OriginGroupExtension, App::GeoFeatureGroupExtension)

OriginGroupExtension::OriginGroupExtension () {

    initExtensionType(OriginGroupExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY_TYPE ( Origin, (nullptr), 0, App::Prop_Hidden, "Origin linked to the group" );
    Origin.setScope(LinkScope::Child);
}

OriginGroupExtension::~OriginGroupExtension () = default;

App::Origin *OriginGroupExtension::getOrigin () const {
    App::DocumentObject *originObj = Origin.getValue ();

    if ( !originObj ) {
        std::stringstream err;
        err << "Can't find Origin for \"" << getExtendedObject()->getFullName () << "\"";
        throw Base::RuntimeError ( err.str().c_str () );

    } else if (! originObj->isDerivedFrom ( App::Origin::getClassTypeId() ) ) {
        std::stringstream err;
        err << "Bad object \"" << originObj->getFullName () << "\"(" << originObj->getTypeId().getName()
            << ") linked to the Origin of \"" << getExtendedObject()->getFullName () << "\"";
        throw Base::RuntimeError ( err.str().c_str () );
    } else {
            return static_cast<App::Origin *> ( originObj );
    }
}

bool OriginGroupExtension::extensionGetSubObject(DocumentObject *&ret, const char *subname,
        PyObject **pyObj, Base::Matrix4D *mat, bool transform, int depth) const
{
    App::DocumentObject *originObj = Origin.getValue ();
    const char *dot;
    if(originObj && originObj->getNameInDocument() &&
       subname && (dot=strchr(subname,'.')))
    {
        bool found;
        if(subname[0] == '$')
            found = std::string(subname+1,dot)==originObj->Label.getValue();
        else
            found = std::string(subname,dot)==originObj->getNameInDocument();
        if(found) {
            if(mat && transform)
                *mat *= const_cast<OriginGroupExtension*>(this)->placement().getValue().toMatrix();
            ret = originObj->getSubObject(dot+1,pyObj,mat,true,depth+1);
            return true;
        }
    }
    return GeoFeatureGroupExtension::extensionGetSubObject(ret,subname,pyObj,mat,transform,depth);
}

App::DocumentObject *OriginGroupExtension::getGroupOfObject (const DocumentObject* obj) {

    if(!obj)
        return nullptr;

    bool isOriginFeature = obj->isDerivedFrom(App::OriginFeature::getClassTypeId());

    auto list = obj->getInList();
    for (auto o : list) {
        if(o->hasExtension(App::OriginGroupExtension::getExtensionClassTypeId()))
            return o;
        else if (isOriginFeature && o->isDerivedFrom(App::Origin::getClassTypeId())) {
            auto result = getGroupOfObject(o);
            if(result)
                return result;
        }
    }

    return nullptr;
}

short OriginGroupExtension::extensionMustExecute() {
    if (Origin.isTouched ()) {
        return 1;
    } else {
        return GeoFeatureGroupExtension::extensionMustExecute();
    }
}

App::DocumentObjectExecReturn *OriginGroupExtension::extensionExecute() {
    try { // try to find all base axis and planes in the origin
        getOrigin ();
    } catch (const Base::Exception &ex) {
        //getExtendedObject()->setError ();
        return new App::DocumentObjectExecReturn ( ex.what () );
    }

    return GeoFeatureGroupExtension::extensionExecute ();
}

App::DocumentObject *OriginGroupExtension::getLocalizedOrigin(App::Document *doc) {
    App::DocumentObject *originObject = doc->addObject ( "App::Origin", "Origin" );
    QByteArray byteArray = tr("Origin").toUtf8();
    originObject->Label.setValue(byteArray.constData());
    return originObject;
}

void OriginGroupExtension::onExtendedSetupObject () {
    App::Document *doc = getExtendedObject()->getDocument ();

    App::DocumentObject *originObj = getLocalizedOrigin(doc);

    assert ( originObj && originObj->isDerivedFrom ( App::Origin::getClassTypeId () ) );
    Origin.setValue (originObj);

    GeoFeatureGroupExtension::onExtendedSetupObject ();
}

void OriginGroupExtension::onExtendedUnsetupObject () {
    App::DocumentObject *origin = Origin.getValue ();
    if (origin && !origin->isRemoving ()) {
        origin->getDocument ()->removeObject (origin->getNameInDocument());
    }

    GeoFeatureGroupExtension::onExtendedUnsetupObject ();
}

void OriginGroupExtension::extensionOnChanged(const Property* p) {
    if(p == &Origin) {
        App::DocumentObject *owner = getExtendedObject();
        App::DocumentObject *origin = Origin.getValue();
        // Document::Importing indicates the object is being imported (i.e.
        // copied). So check the Origin ownership here to prevent copy without
        // dependency
        if (origin && owner && owner->getDocument()
                   && owner->getDocument()->testStatus(Document::Importing)) {
            for (auto o : origin->getInList()) {
                if(o != owner && o->hasExtension(App::OriginGroupExtension::getExtensionClassTypeId())) {
                    App::Document *document = owner->getDocument();
                    // Temporarily reset 'Restoring' status to allow document to auto label new objects
                    Base::ObjectStatusLocker<Document::Status, Document> guard(
                            Document::Restoring, document, false);
                    Origin.setValue(getLocalizedOrigin(document));
                    FC_WARN("Reset origin in " << owner->getFullName());
                    return;
                }
            }
        }
    }
    GeoFeatureGroupExtension::extensionOnChanged(p);
}

void OriginGroupExtension::relinkToOrigin(App::DocumentObject* obj)
{
    //we get all links and replace the origin objects if needed (subnames need not to change, they
    //would stay the same)
    std::vector< App::DocumentObject* > result;
    std::vector<App::Property*> list;
    obj->getPropertyList(list);
    for(App::Property* prop : list) {
        if(prop->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId())) {

            auto p = static_cast<App::PropertyLink*>(prop);
            if(!p->getValue() || !p->getValue()->isDerivedFrom(App::OriginFeature::getClassTypeId()))
                continue;

            p->setValue(getOrigin()->getOriginFeature(static_cast<OriginFeature*>(p->getValue())->Role.getValue()));
        }
        else if(prop->getTypeId().isDerivedFrom(App::PropertyLinkList::getClassTypeId())) {
            auto p = static_cast<App::PropertyLinkList*>(prop);
            auto vec = p->getValues();
            std::vector<App::DocumentObject*> result;
            bool changed = false;
            for(App::DocumentObject* o : vec) {
                if(!o || !o->isDerivedFrom(App::OriginFeature::getClassTypeId()))
                    result.push_back(o);
                else {
                    result.push_back(getOrigin()->getOriginFeature(static_cast<OriginFeature*>(o)->Role.getValue()));
                    changed = true;
                }
            }
            if(changed)
                static_cast<App::PropertyLinkList*>(prop)->setValues(result);
        }
        else if(prop->getTypeId().isDerivedFrom(App::PropertyLinkSub::getClassTypeId())) {
            auto p = static_cast<App::PropertyLinkSub*>(prop);
            if(!p->getValue() || !p->getValue()->isDerivedFrom(App::OriginFeature::getClassTypeId()))
                continue;

            std::vector<std::string> subValues = p->getSubValues();
            p->setValue(getOrigin()->getOriginFeature(static_cast<OriginFeature*>(p->getValue())->Role.getValue()), subValues);
        }
        else if(prop->getTypeId().isDerivedFrom(App::PropertyLinkSubList::getClassTypeId())) {
            auto p = static_cast<App::PropertyLinkSubList*>(prop);
            auto vec = p->getSubListValues();
            bool changed = false;
            for(auto &v : vec) {
                if(v.first && v.first->isDerivedFrom(App::OriginFeature::getClassTypeId())) {
                    v.first = getOrigin()->getOriginFeature(static_cast<OriginFeature*>(v.first)->Role.getValue());
                    changed = true;
                }
            }
            if(changed)
                p->setSubListValues(vec);
        }
    }
}

std::vector< DocumentObject* > OriginGroupExtension::addObjects(std::vector<DocumentObject*> objs) {

    for(auto obj : objs)
        relinkToOrigin(obj);

    return App::GeoFeatureGroupExtension::addObjects(objs);
}

bool OriginGroupExtension::hasObject(const DocumentObject* obj, bool recursive) const {

    if(Origin.getValue() && (obj == getOrigin() || getOrigin()->hasObject(obj)))
        return true;

    return App::GroupExtension::hasObject(obj, recursive);
}



// Python feature ---------------------------------------------------------

namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::OriginGroupExtensionPython, App::OriginGroupExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<GroupExtensionPythonT<OriginGroupExtension>>;
}
