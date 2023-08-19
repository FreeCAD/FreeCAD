/******************************************************************************
 *   Copyright (c) 2012 Konstantinos Poulios <logari81@gmail.com>             *
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
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <QDialog>
#endif

#include <App/Document.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <App/Part.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/DatumPoint.h>

#include "ui_DlgReference.h"
#include "ReferenceSelection.h"
#include "TaskFeaturePick.h"
#include "Utils.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::ReferenceSelection.cpp */

bool ReferenceSelection::allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName)
{
    PartDesign::Body *body = getBody();
    App::OriginGroupExtension* originGroup = getOriginGroupExtension(body);

    // Don't allow selection in other document
    if (support && pDoc != support->getDocument()) {
        return false;
    }

    // Enable selection from origin of current part/
    if (pObj->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId())) {
        return allowOrigin(body, originGroup, pObj);
    }

    if (pObj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {
        return allowDatum(body, pObj);
    }

    // The flag was used to be set. So, this block will never be treated and really doesn't make sense anyway
#if 0
    if (!type.testFlag(AllowSelection::OTHERBODY)) {
        if (support == NULL)
            return false;
        if (pObj != support)
            return false;
    }
#endif
    // Handle selection of geometry elements
    if (!sSubName || sSubName[0] == '\0')
        return type.testFlag(AllowSelection::WHOLE);

    // resolve links if needed
    if (!pObj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        pObj = Part::Feature::getShapeOwner(pObj, sSubName);
    }

    if (pObj && pObj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        return allowPartFeature(pObj, sSubName);
    }

    return false;
}

PartDesign::Body* ReferenceSelection::getBody() const
{
    PartDesign::Body *body;
    if (support) {
        body = PartDesign::Body::findBodyOf (support);
    }
    else {
        body = PartDesignGui::getBody(false);
    }

    return body;
}

App::OriginGroupExtension* ReferenceSelection::getOriginGroupExtension(PartDesign::Body *body) const
{
    App::DocumentObject* originGroupObject = nullptr;

    if (body) { // Search for Part of the body
        originGroupObject = App::OriginGroupExtension::getGroupOfObject(body);
    }
    else if (support) { // if no body search part for support
        originGroupObject = App::OriginGroupExtension::getGroupOfObject(support);
    }
    else { // fallback to active part
        originGroupObject = PartDesignGui::getActivePart();
    }

    App::OriginGroupExtension* originGroup = nullptr;
    if (originGroupObject)
        originGroup = originGroupObject->getExtensionByType<App::OriginGroupExtension>();

    return originGroup;
}

bool ReferenceSelection::allowOrigin(PartDesign::Body *body, App::OriginGroupExtension* originGroup, App::DocumentObject* pObj) const
{
    bool fits = false;
    if (type.testFlag(AllowSelection::FACE) && pObj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
        fits = true;
    }
    else if (type.testFlag(AllowSelection::EDGE) && pObj->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
        fits = true;
    }

    if (fits) { // check that it actually belongs to the chosen body or part
        try { // here are some throwers
            if (body) {
                if (body->getOrigin ()->hasObject (pObj) ) {
                    return true;
                }
            }
            else if (originGroup ) {
                if (originGroup->getOrigin()->hasObject(pObj)) {
                    return true;
                }
            }
        }
        catch (const Base::Exception&) {
        }
    }
    return false; // The Plane/Axis doesn't fits our needs
}

bool ReferenceSelection::allowDatum(PartDesign::Body *body, App::DocumentObject* pObj) const
{
    if (!body) { // Allow selecting Part::Datum features from the active Body
        return false;
    }
    else if (!type.testFlag(AllowSelection::OTHERBODY) && !body->hasObject(pObj)) {
        return false;
    }

    if (type.testFlag(AllowSelection::FACE) && (pObj->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())))
        return true;
    if (type.testFlag(AllowSelection::EDGE) && (pObj->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())))
        return true;
    if (type.testFlag(AllowSelection::POINT) && (pObj->getTypeId().isDerivedFrom(PartDesign::Point::getClassTypeId())))
        return true;

    return false;
}

bool ReferenceSelection::allowPartFeature(App::DocumentObject* pObj, const char* sSubName) const
{
    std::string subName(sSubName);
    if (type.testFlag(AllowSelection::POINT) && subName.compare(0, 6, "Vertex") == 0) {
        return true;
    }

    if (type.testFlag(AllowSelection::EDGE) && subName.compare(0, 4, "Edge") == 0) {
        if (isEdge(pObj, sSubName))
            return true;
    }

    if (type.testFlag(AllowSelection::CIRCLE) && subName.compare(0, 4, "Edge") == 0) {
        if (isCircle(pObj, sSubName))
            return true;
    }

    if (type.testFlag(AllowSelection::FACE) && subName.compare(0, 4, "Face") == 0) {
        if (isFace(pObj, sSubName))
            return true;
    }

    return false;
}

bool ReferenceSelection::isEdge(App::DocumentObject* pObj, const char* sSubName) const
{
    const Part::TopoShape &shape = static_cast<const Part::Feature*>(pObj)->Shape.getValue();
    TopoDS_Shape sh = shape.getSubShape(sSubName);
    const TopoDS_Edge& edgeShape = TopoDS::Edge(sh);
    if (!edgeShape.IsNull()) {
        if (type.testFlag(AllowSelection::PLANAR)) {
            BRepAdaptor_Curve adapt(edgeShape);
            if (adapt.GetType() == GeomAbs_Line)
                return true;
        }
        else {
            return true;
        }
    }

    return false;
}

bool ReferenceSelection::isFace(App::DocumentObject* pObj, const char* sSubName) const
{
    const Part::TopoShape &shape = static_cast<const Part::Feature*>(pObj)->Shape.getValue();
    TopoDS_Shape sh = shape.getSubShape(sSubName);
    const TopoDS_Face& face = TopoDS::Face(sh);
    if (!face.IsNull()) {
        if (type.testFlag(AllowSelection::PLANAR)) {
            BRepAdaptor_Surface adapt(face);
            if (adapt.GetType() == GeomAbs_Plane)
                return true;
        }
        else {
            return true;
        }
    }

    return false;
}

bool ReferenceSelection::isCircle(App::DocumentObject* pObj, const char* sSubName) const
{
    const Part::TopoShape &shape = static_cast<const Part::Feature*>(pObj)->Shape.getValue();
    TopoDS_Shape sh = shape.getSubShape(sSubName);
    const TopoDS_Edge& edgeShape = TopoDS::Edge(sh);
    BRepAdaptor_Curve adapt(edgeShape);
    if (adapt.GetType() == GeomAbs_Circle) {
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------

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

bool getReferencedSelection(const App::DocumentObject* thisObj, const Gui::SelectionChanges& msg,
                            App::DocumentObject*& selObj, std::vector<std::string>& selSub)
{
    selObj = nullptr;
    if (!thisObj)
        return false;

    if (strcmp(thisObj->getDocument()->getName(), msg.pDocName) != 0)
        return false;

    selObj = thisObj->getDocument()->getObject(msg.pObjectName);
    if (selObj == thisObj)
        return false;

    std::string subname = msg.pSubName;

    //check if the selection is an external reference and ask the user what to do
    //of course only if thisObj is in a body, as otherwise the old workflow would not
    //be supported
    PartDesign::Body* body = PartDesignGui::getBodyFor(thisObj, false);
    bool originfeature = selObj->isDerivedFrom(App::OriginFeature::getClassTypeId());
    if (!originfeature && body) {
        PartDesign::Body* selBody = PartDesignGui::getBodyFor(selObj, false);
        if (!selBody || body != selBody) {
            QDialog dia(Gui::getMainWindow());
            Ui_DlgReference dlg;
            dlg.setupUi(&dia);
            dia.setModal(true);
            int result = dia.exec();
            if (result == QDialog::DialogCode::Rejected) {
                selObj = nullptr;
                return false;
            }

            if (!dlg.radioXRef->isChecked()) {
                App::Document* document = thisObj->getDocument();
                document->openTransaction("Make copy");
                auto copy = PartDesignGui::TaskFeaturePick::makeCopy(selObj, subname, dlg.radioIndependent->isChecked());
                body->addObject(copy);

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

    return true;
}

QString getRefStr(const App::DocumentObject* obj, const std::vector<std::string>& sub)
{
    if (!obj) {
        return {};
    }

    if (PartDesign::Feature::isDatum(obj)) {
        return QString::fromLatin1(obj->getNameInDocument());
    }
    else if (!sub.empty()) {
        return QString::fromLatin1(obj->getNameInDocument()) + QString::fromLatin1(":") +
               QString::fromLatin1(sub.front().c_str());
    }

    return {};
}

std::string buildLinkSubPythonStr(const App::DocumentObject* obj, const std::vector<std::string>& subs)
{
    if (!obj)
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
    if (!obj)
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

    for (auto obj : objs)
        result += Gui::Command::getObjectCmd(obj,nullptr,",");
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
