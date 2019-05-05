/******************************************************************************
 *   Copyright (c)2012 Konstantinos Poulios <logari81@gmail.com>              *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <QDialog>
#endif

#include <App/OriginFeature.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Origin.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumPoint.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>

#include "Utils.h"

#include "ReferenceSelection.h"
#include "TaskFeaturePick.h"
#include <ui_DlgReference.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::ReferenceSelection.cpp */

bool ReferenceSelection::allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName)
{
    // TODO review this function (2015-09-04, Fat-Zer)
    PartDesign::Body *body;
    App::DocumentObject* originGroupObject = nullptr;

    if ( support ) {
        body = PartDesign::Body::findBodyOf (support);
    } else {
        body = PartDesignGui::getBody (false);
    }

    if ( body ) { // Search for Part of the body
        originGroupObject = App::OriginGroupExtension::getGroupOfObject ( body ) ;
    } else if ( support ) { // if no body search part for support
        originGroupObject = App::OriginGroupExtension::getGroupOfObject ( support ) ;
    } else { // fallback to active part
        originGroupObject = PartDesignGui::getActivePart ( );
    }
    
    App::OriginGroupExtension* originGroup = nullptr;
    if(originGroupObject)
        originGroup = originGroupObject->getExtensionByType<App::OriginGroupExtension>();

    // Don't allow selection in other document
    if ( support && pDoc != support->getDocument() ) {
        return false;
    }

    // Enable selection from origin of current part/
    if ( pObj->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId()) ) {
        bool fits = false;
        if ( plane && pObj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId()) ) {
            fits = true;
        } else if ( edge && pObj->getTypeId().isDerivedFrom(App::Line::getClassTypeId()) ) {
            fits = true;
        }

        if (fits) { // check that it is actually belongs to the chosen body or part
            try { // here are some throwers
                if (body) {
                    if (body->getOrigin ()->hasObject (pObj) ) {
                        return true;
                    }
                } else if (originGroup ) {
                    if ( originGroup->getOrigin ()->hasObject (pObj) ) {
                        return true;
                    }
                }
            } catch (const Base::Exception&)
            { }
        }
        return false; // The Plane/Axis doesn't fits our needs
    }

    if (pObj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {

        if (!body) { // Allow selecting Part::Datum features from the active Body
            return false;
        } else if (!allowOtherBody && !body->hasObject(pObj)) {
            return false;
        }

        if (plane && (pObj->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())))
            return true;
        if (edge && (pObj->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())))
            return true;
        if (point && (pObj->getTypeId().isDerivedFrom(PartDesign::Point::getClassTypeId())))
            return true;

        return false;
    }

    if (!allowOtherBody) {
        if (support == NULL)
            return false;
        if (pObj != support)
            return false;
    }
    // Handle selection of geometry elements
    if (!sSubName || sSubName[0] == '\0')
        return whole;
    std::string subName(sSubName);
    if (edge && subName.size() > 4 && subName.substr(0,4) == "Edge") {
        const Part::TopoShape &shape = static_cast<const Part::Feature*>(pObj)->Shape.getValue();
        TopoDS_Shape sh = shape.getSubShape(subName.c_str());
        const TopoDS_Edge& edgeShape = TopoDS::Edge(sh);
        if (!edgeShape.IsNull()) {
            if (planar) {
                BRepAdaptor_Curve adapt(edgeShape);
                if (adapt.GetType() == GeomAbs_Line)
                    return true;
            } else {
                return true;
            }
        }
    }
    if (plane && subName.size() > 4 && subName.substr(0,4) == "Face") {
        const Part::TopoShape &shape = static_cast<const Part::Feature*>(pObj)->Shape.getValue();
        TopoDS_Shape sh = shape.getSubShape(subName.c_str());
        const TopoDS_Face& face = TopoDS::Face(sh);
        if (!face.IsNull()) {
            if (planar) {
                BRepAdaptor_Surface adapt(face);
                if (adapt.GetType() == GeomAbs_Plane)
                    return true;
            } else {
                return true;
            }
        }
    }
    if (point && subName.size() > 6 && subName.substr(0,6) == "Vertex") {
        return true;
    }
    return false;
}

bool NoDependentsSelection::allow(App::Document* /*pDoc*/, App::DocumentObject* pObj, const char* /*sSubName*/)
{
    if (support && support->testIfLinkDAGCompatible(pObj)) {
        return true;
    }
    else {
        this->notAllowedReason = QT_TR_NOOP("Selecting this will cause circular dependency.");
        return false;
    }
}

bool CombineSelectionFilterGates::allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName)
{
    return filter1->allow(pDoc, pObj, sSubName) && filter2->allow(pDoc, pObj, sSubName);
}


namespace PartDesignGui
{

void getReferencedSelection(const App::DocumentObject* thisObj, const Gui::SelectionChanges& msg,
                            App::DocumentObject*& selObj, std::vector<std::string>& selSub)
{
    if (!thisObj)
        return;

    if (strcmp(thisObj->getDocument()->getName(), msg.pDocName) != 0)
        return;
    
    selObj = thisObj->getDocument()->getObject(msg.pObjectName);
    if (selObj == thisObj)
        return;
    
    std::string subname = msg.pSubName;
    
    //check if the selection is an external reference and ask the user what to do
    //of course only if thisObj is in a body, as otherwise the old workflow would not 
    //be supportet
    PartDesign::Body* body = PartDesignGui::getBodyFor(thisObj, false);
    bool originfeature = selObj->isDerivedFrom(App::OriginFeature::getClassTypeId());
    if (!originfeature && body) {
        PartDesign::Body* selBody = PartDesignGui::getBodyFor(selObj, false);
        if(!selBody || body != selBody) {
            
            auto* pcActivePart = PartDesignGui::getPartFor(body, false);

            QDialog dia(Gui::getMainWindow());
            Ui_DlgReference dlg;
            dlg.setupUi(&dia);
            dia.setModal(true);
            int result = dia.exec();
            if (result == QDialog::DialogCode::Rejected) {
                selObj = NULL;
                return;
            }
            else if(!dlg.radioXRef->isChecked()) {
                App::Document* document = thisObj->getDocument();
                document->openTransaction("Make copy");
                auto copy = PartDesignGui::TaskFeaturePick::makeCopy(selObj, subname, dlg.radioIndependent->isChecked());
                if (body)
                    body->addObject(copy);
                else if (pcActivePart)
                    pcActivePart->addObject(copy);

                selObj = copy;
                subname.erase(std::remove_if(subname.begin(), subname.end(), &isdigit), subname.end());
                subname.append("1");
            }
        
        }
    }

    // Remove subname for planes and datum features
    if (PartDesign::Feature::isDatum(selObj)) {
        subname = "";
    }

    selSub = std::vector<std::string>(1,subname);
}

QString getRefStr(const App::DocumentObject* obj, const std::vector<std::string>& sub)
{
    if (obj == NULL)
        return QString::fromLatin1("");

    if (PartDesign::Feature::isDatum(obj))
        return QString::fromLatin1(obj->getNameInDocument());
    else if (sub.size()>0)
        return QString::fromLatin1(obj->getNameInDocument()) + QString::fromLatin1(":") +
               QString::fromLatin1(sub.front().c_str());
    else
        return QString();
}

std::string buildLinkSubPythonStr(const App::DocumentObject* obj, const std::vector<std::string>& subs)
{
    if ( obj == NULL)
        return "None";

    std::string result("[");

    for (const auto & sub : subs)
        result += "\"" + sub + "\",";
    result += "]";

    return result;
}

std::string buildLinkSingleSubPythonStr(const App::DocumentObject* obj,
        const std::vector<std::string>& subs)
{
    if (obj == NULL)
        return "None";

    if (PartDesign::Feature::isDatum(obj))
        return Gui::Command::getObjectCmd(obj,"(",", [''])");
    else
        return Gui::Command::getObjectCmd(obj,"(",", ['") + subs.front() + "'])";
}

std::string buildLinkListPythonStr(const std::vector<App::DocumentObject*> & objs)
{
    if ( objs.empty() ) {
        return "None";
    }

    std::string result("[");

    for (std::vector<App::DocumentObject*>::const_iterator o = objs.begin(); o != objs.end(); o++)
        result += Gui::Command::getObjectCmd(*o,0,",");
    result += "]";

    return result;
}

std::string buildLinkSubListPythonStr(const std::vector<App::DocumentObject*> & objs,
        const std::vector<std::string>& subs)
{
    if ( objs.empty() ) {
        return "None";
    }

    std::string result("[");

    assert (objs.size () == subs.size () );

    for (size_t i=0, objs_sz=objs.size(); i < objs_sz; i++) {
        if (objs[i] ) {
            result += '(';
            result += Gui::Command::getObjectCmd(objs[i]);
            result += ",\"";
            result += subs[i];
            result += "\"),";
        }
    }

    result += "]";

    return result;
}
}
