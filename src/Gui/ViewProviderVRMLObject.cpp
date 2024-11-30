/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <sstream>
# include <Inventor/SoDB.h>
# include <Inventor/SoFullPath.h>
# include <Inventor/SoInput.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTransform.h>
# include <QFile>
# include <QFileInfo>
#endif

#include <Inventor/lists/SbStringList.h>
#include <Inventor/VRMLnodes/SoVRMLAnchor.h>
#include <Inventor/VRMLnodes/SoVRMLAudioClip.h>
#include <Inventor/VRMLnodes/SoVRMLBackground.h>
#include <Inventor/VRMLnodes/SoVRMLImageTexture.h>
#include <Inventor/VRMLnodes/SoVRMLInline.h>
#include <Inventor/VRMLnodes/SoVRMLMovieTexture.h>
#include <Inventor/VRMLnodes/SoVRMLScript.h>

#include <App/Document.h>
#include <App/VRMLObject.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>

#include "ViewProviderVRMLObject.h"
#include "SoFCSelection.h"


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderVRMLObject, Gui::ViewProviderDocumentObject)

ViewProviderVRMLObject::ViewProviderVRMLObject()
{
    pcVRML = new SoFCSelection();
    pcVRML->highlightMode = Gui::SoFCSelection::OFF;
    pcVRML->selectionMode = Gui::SoFCSelection::SEL_OFF;
    //pcVRML->style = Gui::SoFCSelection::BOX;
    pcVRML->ref();
}

ViewProviderVRMLObject::~ViewProviderVRMLObject()
{
    pcVRML->unref();
}

void ViewProviderVRMLObject::attach(App::DocumentObject *pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);
    addDisplayMaskMode(pcVRML, "VRML");
    pcVRML->objectName = pcObj->getNameInDocument();
    pcVRML->documentName = pcObj->getDocument()->getName();
    pcVRML->subElementName = "Main";
}

void ViewProviderVRMLObject::setDisplayMode(const char* ModeName)
{
    if ( strcmp("VRML",ModeName)==0 )
        setDisplayMaskMode("VRML");
    ViewProviderDocumentObject::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderVRMLObject::getDisplayModes() const
{
    std::vector<std::string> StrList;
    StrList.emplace_back("VRML");
    return StrList;
}

template<typename T>
void ViewProviderVRMLObject::getResourceFile(SoNode* node, std::list<std::string>& resources)
{
    SoSearchAction sa;
    sa.setType(T::getClassTypeId());
    sa.setInterest(SoSearchAction::ALL);
    sa.setSearchingAll(true);
    sa.apply(node);
    const SoPathList & pathlist = sa.getPaths();
    for (int i = 0; i < pathlist.getLength(); i++ ) {
        auto path = static_cast<SoFullPath *>(pathlist[i]);
        if (path->getTail()->isOfType(T::getClassTypeId())) {
            T * tex = static_cast<T*>(path->getTail());
            for (int j = 0; j < tex->url.getNum(); j++) {
                this->addResource(tex->url[j], resources);
            }
        }
    }
}

namespace Gui {
// Special handling for SoVRMLBackground
template<>
void ViewProviderVRMLObject::getResourceFile<SoVRMLBackground>(SoNode* node, std::list<std::string>& resources)
{
    SoSearchAction sa;
    sa.setType(SoVRMLBackground::getClassTypeId());
    sa.setInterest(SoSearchAction::ALL);
    sa.setSearchingAll(true);
    sa.apply(node);
    const SoPathList & pathlist = sa.getPaths();
    for (int i = 0; i < pathlist.getLength(); i++ ) {
        auto path = static_cast<SoFullPath *>(pathlist[i]);
        if (path->getTail()->isOfType(SoVRMLBackground::getClassTypeId())) {
            auto vrml = static_cast<SoVRMLBackground*>(path->getTail());
            // backUrl
            for (int j = 0; j < vrml->backUrl.getNum(); j++) {
                addResource(vrml->backUrl[j], resources);
            }
            // bottomUrl
            for (int j = 0; j < vrml->bottomUrl.getNum(); j++) {
                addResource(vrml->bottomUrl[j], resources);
            }
            // frontUrl
            for (int j = 0; j < vrml->frontUrl.getNum(); j++) {
                addResource(vrml->frontUrl[j], resources);
            }
            // leftUrl
            for (int j = 0; j < vrml->leftUrl.getNum(); j++) {
                addResource(vrml->leftUrl[j], resources);
            }
            // rightUrl
            for (int j = 0; j < vrml->rightUrl.getNum(); j++) {
                addResource(vrml->rightUrl[j], resources);
            }
            // topUrl
            for (int j = 0; j < vrml->topUrl.getNum(); j++) {
                addResource(vrml->topUrl[j], resources);
            }
        }
    }
}

}

void ViewProviderVRMLObject::addResource(const SbString& url, std::list<std::string>& resources)
{
    SbString found = SoInput::searchForFile(url, SoInput::getDirectories(), SbStringList());
    Base::FileInfo fi(found.getString());
    if (fi.exists()) {
        // add the resource file if not yet listed
        if (std::find(resources.begin(), resources.end(), found.getString()) == resources.end()) {
            resources.emplace_back(found.getString());
        }
    }
}

void ViewProviderVRMLObject::getLocalResources(SoNode* node, std::list<std::string>& resources)
{
    // search for SoVRMLInline files
    SoSearchAction sa;
    sa.setType(SoVRMLInline::getClassTypeId());
    sa.setInterest(SoSearchAction::ALL);
    sa.setSearchingAll(true);
    sa.apply(node);

    const SoPathList & pathlist = sa.getPaths();
    for (int i = 0; i < pathlist.getLength(); i++ ) {
        SoPath * path = pathlist[i];
        auto vrml = static_cast<SoVRMLInline*>(path->getTail());
        const SbString& url = vrml->getFullURLName();
        if (url.getLength() > 0) {
            // add the resource file if not yet listed
            if (std::find(resources.begin(), resources.end(), url.getString()) == resources.end()) {
                resources.emplace_back(url.getString());
            }

            // if the resource file could be loaded check if it references further resources
            if (vrml->getChildData()) {
                getLocalResources(vrml->getChildData(), resources);
            }
        }
    }

    // search for SoVRMLImageTexture, ... files
    getResourceFile<SoVRMLImageTexture  >(node, resources);
    getResourceFile<SoVRMLMovieTexture  >(node, resources);
    getResourceFile<SoVRMLScript        >(node, resources);
    getResourceFile<SoVRMLBackground    >(node, resources);
    getResourceFile<SoVRMLAudioClip     >(node, resources);
    getResourceFile<SoVRMLAnchor        >(node, resources);
}

void ViewProviderVRMLObject::updateData(const App::Property* prop)
{
    auto ivObj = static_cast<App::VRMLObject*>(pcObject);
    if (prop == &ivObj->VrmlFile) {
        // read also from file
        const char* filename = ivObj->VrmlFile.getValue();
        QString fn = QString::fromUtf8(filename);
        QFile file(fn);
        SoInput in;
        coinRemoveAllChildren(pcVRML);
        if (!fn.isEmpty() && file.open(QFile::ReadOnly)) {
            QFileInfo fi(fn);
            QByteArray filepath = fi.absolutePath().toUtf8();
            QByteArray subpath = filepath + "/" + ivObj->getNameInDocument();

            // Add this to the search path in order to read inline files
            SoInput::addDirectoryFirst(filepath.constData());
            SoInput::addDirectoryFirst(subpath.constData());

            // Read in the file
            QByteArray buffer = file.readAll();
            in.setBuffer((void *)buffer.constData(), buffer.length());
            SoSeparator * node = SoDB::readAll(&in);

            if (node) {
                if (!checkRecursion(node)) {
                    Base::Console().Error("The VRML file causes an infinite recursion!\n");
                    return;
                }
                pcVRML->addChild(node);

                std::list<std::string> urls;
                getLocalResources(node, urls);
                if (!urls.empty() && ivObj->Urls.getSize() == 0) {
                    std::vector<std::string> res;
                    res.insert(res.end(), urls.begin(), urls.end());
                    ivObj->Urls.setValues(res);
                }
            }
            SoInput::removeDirectory(filepath.constData());
            SoInput::removeDirectory(subpath.constData());
        }
    }
    else if (prop->isDerivedFrom(App::PropertyPlacement::getClassTypeId()) &&
             strcmp(prop->getName(), "Placement") == 0) {
        // Note: If R is the rotation, c the rotation center and t the translation
        // vector then Inventor applies the following transformation: R*(x-c)+c+t
        // In FreeCAD a placement only has a rotation and a translation part but
        // no rotation center. This means that the following equation must be ful-
        // filled: R * (x-c) + c + t = R * x + t
        //    <==> R * x + t - R * c + c = R * x + t
        //    <==> (I-R) * c = 0 ==> c = 0
        // This means that the center point must be the origin!
        Base::Placement p = static_cast<const App::PropertyPlacement*>(prop)->getValue();
        auto q0 = (float)p.getRotation().getValue()[0];
        auto q1 = (float)p.getRotation().getValue()[1];
        auto q2 = (float)p.getRotation().getValue()[2];
        auto q3 = (float)p.getRotation().getValue()[3];
        auto px = (float)p.getPosition().x;
        auto py = (float)p.getPosition().y;
        auto pz = (float)p.getPosition().z;
        pcTransform->rotation.setValue(q0,q1,q2,q3);
        pcTransform->translation.setValue(px,py,pz);
        pcTransform->center.setValue(0.0f,0.0f,0.0f);
        pcTransform->scaleFactor.setValue(1.0f,1.0f,1.0f);
    }
}
