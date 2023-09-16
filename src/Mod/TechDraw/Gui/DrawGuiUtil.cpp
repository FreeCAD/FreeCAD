/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

# include <QComboBox>
# include <QMessageBox>
# include <QPointF>
# include <QRectF>
# include <QString>

# include <BRepAdaptor_Surface.hxx>
# include <BRepLProp_SLProps.hxx>
# include <gp_Dir.hxx>
#endif
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyPythonObject.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Type.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Gui/Selection.h>
#include <Inventor/SbVec3f.h>
#include <Mod/TechDraw/App/ArrowPropEnum.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "DlgPageChooser.h"
#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "QGSPage.h"
#include "ViewProviderPage.h"


using namespace TechDrawGui;
using namespace TechDraw;

void DrawGuiUtil::loadArrowBox(QComboBox* qcb)
{
    qcb->clear();
    int i = 0;
    for (; i < ArrowPropEnum::ArrowCount; i++) {
        qcb->addItem(QCoreApplication::translate("ArrowPropEnum", ArrowPropEnum::ArrowTypeEnums[i]));
        QIcon itemIcon(QString::fromUtf8(ArrowPropEnum::ArrowTypeIcons[i].c_str()));
        qcb->setItemIcon(i, itemIcon);
    }
}

//===========================================================================
// validate helper routines
//===========================================================================

//find a page in Selection, Document or CurrentWindow.
TechDraw::DrawPage* DrawGuiUtil::findPage(Gui::Command* cmd, bool findAny)
{
    //    Base::Console().Message("DGU::findPage()\n");
    std::vector<std::string> names;
    std::vector<std::string> labels;
    auto docs = App::GetApplication().getDocuments();

    if (findAny) {
        //find a page in any open document
        std::vector<App::DocumentObject*> foundPageObjects;
        //no page found in the usual places, but we have been asked to search all
        //open documents for a page.
        auto docsAll = App::GetApplication().getDocuments();
        for (auto& doc : docsAll) {
            auto docPages = doc->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
            if (docPages.empty()) {
                //this open document has no TD pages
                continue;
            }
            foundPageObjects.insert(foundPageObjects.end(), docPages.begin(), docPages.end());
        }
        if (foundPageObjects.empty()) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page found"),
                                 QObject::tr("No Drawing Pages available."));
            return nullptr;
        }
        else if (foundPageObjects.size() > 1) {
            //multiple pages available, ask for help
            for (auto obj : foundPageObjects) {
                std::string name = obj->getNameInDocument();
                names.push_back(name);
                std::string label = obj->Label.getValue();
                labels.push_back(label);
            }
            DlgPageChooser dlg(labels, names, Gui::getMainWindow());
            if (dlg.exec() == QDialog::Accepted) {
                std::string selName = dlg.getSelection();
                App::Document* doc = cmd->getDocument();
                return static_cast<TechDraw::DrawPage*>(doc->getObject(selName.c_str()));
            }
        }
        else {
            //only 1 page found
            return static_cast<TechDraw::DrawPage*>(foundPageObjects.front());
        }
    }

    //check Selection for a page
    std::vector<App::DocumentObject*> selPages =
        cmd->getSelection().getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (selPages.empty()) {
        //no page in selection, try this document
        auto docPages = cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
        if (docPages.empty()) {
            //we are only to look in this document, and there is no page in this document
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page found"),
                                 QObject::tr("No Drawing Pages in document."));
            return nullptr;
        }
        else if (docPages.size() > 1) {
            //multiple pages in document, use active page if there is one
            Gui::MainWindow* w = Gui::getMainWindow();
            Gui::MDIView* mv = w->activeWindow();
            MDIViewPage* mvp = dynamic_cast<MDIViewPage*>(mv);
            if (mvp) {
                QGSPage* qp = mvp->getViewProviderPage()->getQGSPage();
                return qp->getDrawPage();
            }
            else {
                // none of pages in document is active, ask for help
                for (auto obj : docPages) {
                    std::string name = obj->getNameInDocument();
                    names.push_back(name);
                    std::string label = obj->Label.getValue();
                    labels.push_back(label);
                }
                DlgPageChooser dlg(labels, names, Gui::getMainWindow());
                if (dlg.exec() == QDialog::Accepted) {
                    std::string selName = dlg.getSelection();
                    App::Document* doc = cmd->getDocument();
                    return static_cast<TechDraw::DrawPage*>(doc->getObject(selName.c_str()));
                }
                return nullptr;
            }
        }
        else {
            //only 1 page in document - use it
            return static_cast<TechDraw::DrawPage*>(docPages.front());
        }
    }
    else if (selPages.size() > 1) {
        //multiple pages in selection
        for (auto obj : selPages) {
            std::string name = obj->getNameInDocument();
            names.push_back(name);
            std::string label = obj->Label.getValue();
            labels.push_back(label);
        }
        DlgPageChooser dlg(labels, names, Gui::getMainWindow());
        if (dlg.exec() == QDialog::Accepted) {
            std::string selName = dlg.getSelection();
            App::Document* doc = cmd->getDocument();
            return static_cast<TechDraw::DrawPage*>(doc->getObject(selName.c_str()));
        }
    }
    else {
        //exactly 1 page in selection, use it
        return static_cast<TechDraw::DrawPage*>(selPages.front());
    }

    //we can not actually reach this point.
    return nullptr;
}

bool DrawGuiUtil::isDraftObject(App::DocumentObject* obj)
{
    bool result = false;
    App::PropertyPythonObject* proxy =
        dynamic_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"));

    if (proxy) {
        //if no proxy, can not be Draft obj
        //if has proxy, might be Draft obj
        std::stringstream ss;
        Py::Object proxyObj = proxy->getValue();
        Base::PyGILStateLocker lock;
        try {
            if (proxyObj.hasAttr("__module__")) {
                Py::String mod(proxyObj.getAttr("__module__"));
                ss << (std::string)mod;
                if (ss.str().find("Draft") != std::string::npos) {
                    result = true;
                }
                else if (ss.str().find("draft") != std::string::npos) {
                    result = true;
                }
            }
        }
        catch (Py::Exception&) {
            Base::PyException e;// extract the Python error text
            e.ReportException();
            result = false;
        }
    }
    return result;
}

bool DrawGuiUtil::isArchObject(App::DocumentObject* obj)
{
    bool result = false;
    App::PropertyPythonObject* proxy =
        dynamic_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"));

    if (proxy) {
        //if no proxy, can not be Arch obj
        //if has proxy, might be Arch obj
        Py::Object proxyObj = proxy->getValue();
        std::stringstream ss;
        Base::PyGILStateLocker lock;
        try {
            if (proxyObj.hasAttr("__module__")) {
                Py::String mod(proxyObj.getAttr("__module__"));
                ss << (std::string)mod;
                //does this have to be an ArchSection, or can it be any Arch object?
                if (ss.str().find("Arch") != std::string::npos) {
                    result = true;
                }
            }
        }
        catch (Py::Exception&) {
            Base::PyException e;// extract the Python error text
            e.ReportException();
            result = false;
        }
    }
    return result;
}

bool DrawGuiUtil::isArchSection(App::DocumentObject* obj)
{
    bool result = false;
    App::PropertyPythonObject* proxy =
        dynamic_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"));

    if (proxy) {
        //if no proxy, can not be Arch obj
        //if has proxy, might be Arch obj
        Py::Object proxyObj = proxy->getValue();
        std::stringstream ss;
        Base::PyGILStateLocker lock;
        try {
            if (proxyObj.hasAttr("__module__")) {
                Py::String mod(proxyObj.getAttr("__module__"));
                ss << (std::string)mod;
                //does this have to be an ArchSection, or can it be other Arch objects?
                if (ss.str().find("ArchSectionPlane") != std::string::npos) {
                    result = true;
                }
            }
        }
        catch (Py::Exception&) {
            Base::PyException e;// extract the Python error text
            e.ReportException();
            result = false;
        }
    }
    return result;
}

bool DrawGuiUtil::needPage(Gui::Command* cmd, bool findAny)
{
    if (findAny) {
        //look for any page in any open document
        auto docsAll = App::GetApplication().getDocuments();
        for (auto& doc : docsAll) {
            auto docPages = doc->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
            if (docPages.empty()) {
                //this open document has no TD pages
                continue;
            }
            else {
                //found at least 1 page
                return true;
            }
        }
        //did not find any pages
        return false;
    }

    //need a Document and a Page
    if (cmd->hasActiveDocument()) {
        auto drawPageType(TechDraw::DrawPage::getClassTypeId());
        auto selPages = cmd->getDocument()->getObjectsOfType(drawPageType);
        return !selPages.empty();
    }
    return false;
}

bool DrawGuiUtil::needView(Gui::Command* cmd, bool partOnly)
{
    bool haveView = false;
    if (cmd->hasActiveDocument()) {
        if (partOnly) {
            auto drawPartType(TechDraw::DrawViewPart::getClassTypeId());
            auto selParts = cmd->getDocument()->getObjectsOfType(drawPartType);
            if (!selParts.empty()) {
                haveView = true;
            }
        }
        else {
            auto drawViewType(TechDraw::DrawView::getClassTypeId());
            auto selParts = cmd->getDocument()->getObjectsOfType(drawViewType);
            if (!selParts.empty()) {
                haveView = true;
            }
        }
    }
    return haveView;
}

void DrawGuiUtil::dumpRectF(const char* text, const QRectF& r)
{
    Base::Console().Message("DUMP - dumpRectF - %s\n", text);
    double left = r.left();
    double right = r.right();
    double top = r.top();
    double bottom = r.bottom();
    Base::Console().Message("Extents: L: %.3f, R: %.3f, T: %.3f, B: %.3f\n", left, right, top,
                            bottom);
    Base::Console().Message("Size: W: %.3f H: %.3f\n", r.width(), r.height());
    Base::Console().Message("Centre: (%.3f, %.3f)\n", r.center().x(), r.center().y());
}

void DrawGuiUtil::dumpPointF(const char* text, const QPointF& p)
{
    Base::Console().Message("DUMP - dumpPointF - %s\n", text);
    Base::Console().Message("Point: (%.3f, %.3f)\n", p.x(), p.y());
}

std::pair<Base::Vector3d, Base::Vector3d> DrawGuiUtil::get3DDirAndRot()
{
    std::pair<Base::Vector3d, Base::Vector3d> result;
    Base::Vector3d viewDir(0.0, -1.0, 0.0); //default to front
    Base::Vector3d viewUp(0.0, 0.0, 1.0);   //default to top
    Base::Vector3d viewRight(1.0, 0.0, 0.0);//default to right
    std::list<Gui::MDIView*> mdis = Gui::Application::Instance->activeDocument()->getMDIViews();
    Gui::View3DInventor* view;
    Gui::View3DInventorViewer* viewer = nullptr;
    for (auto& m : mdis) {//find the 3D viewer
        view = dynamic_cast<Gui::View3DInventor*>(m);
        if (view) {
            viewer = view->getViewer();
            break;
        }
    }
    if (!viewer) {
        return std::make_pair(viewDir, viewRight);
    }

    // Coin is giving us a values like 0.000000134439 instead of 0.000000000000.
    // This small difference caused circles to be projected as ellipses among other
    // problems.
    // Since SbVec3f is single precision floating point, it is only good to 6-9
    // significant decimal digits, and the rest of TechDraw works with doubles
    // that are good to 15-18 significant decimal digits.
    // But. When a float is promoted to double the value is supposed to be unchanged!
    // So where do the garbage digits come from???
    // In any case, if we restrict directions to 6 digits, we avoid the problem.
    int digits(6);
    SbVec3f dvec = viewer->getViewDirection();
    double dvecX = roundToDigits(dvec[0], digits);
    double dvecY = roundToDigits(dvec[1], digits);
    double dvecZ = roundToDigits(dvec[2], digits);
    viewDir = Base::Vector3d(dvecX, dvecY, dvecZ);
    viewDir = viewDir * (-1.0);// Inventor dir is opposite TD projection dir

    SbVec3f upvec = viewer->getUpDirection();
    double upvecX = roundToDigits(upvec[0], digits);
    double upvecY = roundToDigits(upvec[1], digits);
    double upvecZ = roundToDigits(upvec[2], digits);
    viewUp = Base::Vector3d(upvecX, upvecY, upvecZ);

    Base::Vector3d right = viewUp.Cross(viewDir);

    result = std::make_pair(viewDir, right);
    return result;
}

std::pair<Base::Vector3d, Base::Vector3d> DrawGuiUtil::getProjDirFromFace(App::DocumentObject* obj,
                                                                          std::string faceName)
{
    std::pair<Base::Vector3d, Base::Vector3d> d3Dirs = get3DDirAndRot();
    std::pair<Base::Vector3d, Base::Vector3d> dirs;
    dirs.first = Base::Vector3d(0.0, 0.0, 1.0);//set a default
    dirs.second = Base::Vector3d(1.0, 0.0, 0.0);
    Base::Vector3d projDir, rotVec;
    projDir = d3Dirs.first;
    rotVec = d3Dirs.second;

    auto ts = Part::Feature::getShape(obj, faceName.c_str(), true);
    if (ts.IsNull() || ts.ShapeType() != TopAbs_FACE) {
        Base::Console().Warning("getProjDirFromFace(%s) is not a Face\n", faceName.c_str());
        return dirs;
    }

    const TopoDS_Face& face = TopoDS::Face(ts);
    TopAbs_Orientation orient = face.Orientation();
    BRepAdaptor_Surface adapt(face);

    double u1 = adapt.FirstUParameter();
    double u2 = adapt.LastUParameter();
    double v1 = adapt.FirstVParameter();
    double v2 = adapt.LastVParameter();
    double uMid = (u1 + u2) / 2.0;
    double vMid = (v1 + v2) / 2.0;

    BRepLProp_SLProps props(adapt, uMid, vMid, 2, Precision::Confusion());
    if (props.IsNormalDefined()) {
        gp_Dir vec = props.Normal();
        projDir = Base::Vector3d(vec.X(), vec.Y(), vec.Z());
        if (orient != TopAbs_FORWARD) {
            projDir = projDir * (-1.0);
        }
    }

    return std::make_pair(projDir, rotVec);
}

// converts original value to one with only digits significant figures
double DrawGuiUtil::roundToDigits(double original, int digits)
{
    double factor = std::pow(10.0, digits);
    double temp = original * factor;
    double rounded = std::round(temp);
    temp = rounded / factor;
    return temp;
}
