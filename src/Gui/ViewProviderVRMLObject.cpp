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
# include <Inventor/SoDB.h>
# include <Inventor/SoInput.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTransform.h>
# include <QFile>
#endif

#include "ViewProviderVRMLObject.h"
#include "SoFCSelection.h"
#include <App/VRMLObject.h>
#include <App/Document.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <sstream>

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

std::vector<std::string> ViewProviderVRMLObject::getDisplayModes(void) const
{
    std::vector<std::string> StrList;
    StrList.push_back("VRML");
    return StrList;
}

void ViewProviderVRMLObject::updateData(const App::Property* prop)
{
    App::VRMLObject* ivObj = static_cast<App::VRMLObject*>(pcObject);
    if (prop == &ivObj->VrmlFile) {
        // read also from file
        const char* filename = ivObj->VrmlFile.getValue();
        QString fn = QString::fromUtf8(filename);
        QFile file(fn);
        SoInput in;
        pcVRML->removeAllChildren();
        if (!fn.isEmpty() && file.open(QFile::ReadOnly)) {
            QByteArray buffer = file.readAll();
            in.setBuffer((void *)buffer.constData(), buffer.length());
            SoSeparator * node = SoDB::readAll(&in);
            if (node) pcVRML->addChild(node);
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
        float q0 = (float)p.getRotation().getValue()[0];
        float q1 = (float)p.getRotation().getValue()[1];
        float q2 = (float)p.getRotation().getValue()[2];
        float q3 = (float)p.getRotation().getValue()[3];
        float px = (float)p.getPosition().x;
        float py = (float)p.getPosition().y;
        float pz = (float)p.getPosition().z;
        pcTransform->rotation.setValue(q0,q1,q2,q3);
        pcTransform->translation.setValue(px,py,pz);
        pcTransform->center.setValue(0.0f,0.0f,0.0f);
        pcTransform->scaleFactor.setValue(1.0f,1.0f,1.0f);
    }
}
