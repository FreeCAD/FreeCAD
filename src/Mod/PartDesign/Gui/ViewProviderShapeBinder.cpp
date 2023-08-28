/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <QApplication>
# include <QMenu>
# include <QMessageBox>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <App/Document.h>
#include <Gui/ActionFunction.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/ViewParams.h>
#include <Mod/PartDesign/App/ShapeBinder.h>

#include "ViewProviderShapeBinder.h"
#include "TaskShapeBinder.h"

FC_LOG_LEVEL_INIT("ShapeBinder",true,true)

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderShapeBinder,PartGui::ViewProviderPart)

ViewProviderShapeBinder::ViewProviderShapeBinder()
{
    sPixmap = "PartDesign_ShapeBinder.svg";

    //make the viewprovider more datum like
    AngularDeflection.setStatus(App::Property::Hidden, true);
    Deviation.setStatus(App::Property::Hidden, true);
    DrawStyle.setStatus(App::Property::Hidden, true);
    Lighting.setStatus(App::Property::Hidden, true);
    LineColor.setStatus(App::Property::Hidden, true);
    LineWidth.setStatus(App::Property::Hidden, true);
    PointColor.setStatus(App::Property::Hidden, true);
    PointSize.setStatus(App::Property::Hidden, true);
    DisplayMode.setStatus(App::Property::Hidden, true);

    //get the datum coloring scheme
    // set default color for datums (golden yellow with 60% transparency)
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/PartDesign");
    unsigned long shcol = hGrp->GetUnsigned("DefaultDatumColor", 0xFFD70099);
    App::Color col((uint32_t)shcol);

    ShapeColor.setValue(col);
    LineColor.setValue(col);
    PointColor.setValue(col);
    Transparency.setValue(60);
    LineWidth.setValue(1);
}

ViewProviderShapeBinder::~ViewProviderShapeBinder() = default;

bool ViewProviderShapeBinder::setEdit(int ModNum) {
    // TODO Share code with other view providers (2015-09-11, Fat-Zer)

    if (ModNum == ViewProvider::Default || ModNum == 1) {
        // When double-clicking on the item for this pad the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
        TaskDlgShapeBinder* sbDlg = qobject_cast<TaskDlgShapeBinder*>(dlg);
        if (dlg && !sbDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().reject();
            else
                return false;
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // start the edit dialog
        // another pad left open its task panel
        if (sbDlg)
            Gui::Control().showDialog(sbDlg);
        else
            Gui::Control().showDialog(new TaskDlgShapeBinder(this, ModNum == 1));

        return true;
    }
    else {
        return ViewProviderPart::setEdit(ModNum);
    }
}

void ViewProviderShapeBinder::unsetEdit(int ModNum) {

    PartGui::ViewProviderPart::unsetEdit(ModNum);
}

void ViewProviderShapeBinder::highlightReferences(bool on)
{
    App::GeoFeature* obj = nullptr;
    std::vector<std::string> subs;

    if (getObject()->isDerivedFrom(PartDesign::ShapeBinder::getClassTypeId()))
        PartDesign::ShapeBinder::getFilteredReferences(&static_cast<PartDesign::ShapeBinder*>(getObject())->Support, obj, subs);
    else
        return;

    // stop if not a Part feature was found
    if (!obj || !obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return;

    PartGui::ViewProviderPart* svp = dynamic_cast<PartGui::ViewProviderPart*>(
        Gui::Application::Instance->getViewProvider(obj));
    if (!svp)
        return;

    if (on) {
        if (!subs.empty() && originalLineColors.empty()) {
            TopTools_IndexedMapOfShape eMap;
            TopExp::MapShapes(static_cast<Part::Feature*>(obj)->Shape.getValue(), TopAbs_EDGE, eMap);
            originalLineColors = svp->LineColorArray.getValues();
            std::vector<App::Color> lcolors = originalLineColors;
            lcolors.resize(eMap.Extent(), svp->LineColor.getValue());

            TopExp::MapShapes(static_cast<Part::Feature*>(obj)->Shape.getValue(), TopAbs_FACE, eMap);
            originalFaceColors = svp->DiffuseColor.getValues();
            std::vector<App::Color> fcolors = originalFaceColors;
            fcolors.resize(eMap.Extent(), svp->ShapeColor.getValue());

            for (const std::string& e : subs) {
                // Note: stoi may throw, but it strictly shouldn't happen
                if (e.compare(0, 4, "Edge") == 0) {
                    int idx = std::stoi(e.substr(4)) - 1;
                    assert(idx >= 0);
                    if (idx < static_cast<int>(lcolors.size()))
                        lcolors[idx] = App::Color(1.0, 0.0, 1.0); // magenta
                }
                else if (e.compare(0, 4, "Face") == 0) {
                    int idx = std::stoi(e.substr(4)) - 1;
                    assert(idx >= 0);
                    if (idx < static_cast<int>(fcolors.size()))
                        fcolors[idx] = App::Color(1.0, 0.0, 1.0); // magenta
                }
            }
            svp->LineColorArray.setValues(lcolors);
            svp->DiffuseColor.setValues(fcolors);
        }
    }
    else {
        if (!subs.empty() && !originalLineColors.empty()) {
            svp->LineColorArray.setValues(originalLineColors);
            originalLineColors.clear();

            svp->DiffuseColor.setValues(originalFaceColors);
            originalFaceColors.clear();
        }
    }
}

void ViewProviderShapeBinder::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Q_UNUSED(receiver)
    Q_UNUSED(member)

        QAction* act;
    act = menu->addAction(QObject::tr("Edit shape binder"));
    act->setData(QVariant((int)ViewProvider::Default));

    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    func->trigger(act, [this]() {
        QString text = QObject::tr("Edit %1").arg(QString::fromUtf8(getObject()->Label.getValue()));
        Gui::Command::openCommand(text.toUtf8());

        Gui::Document* document = this->getDocument();
        if (document) {
            document->setEdit(this, ViewProvider::Default);
        }
    });
}

//=====================================================================================

PROPERTY_SOURCE(PartDesignGui::ViewProviderSubShapeBinder, PartGui::ViewProviderPart)

ViewProviderSubShapeBinder::ViewProviderSubShapeBinder() {
    sPixmap = "PartDesign_SubShapeBinder.svg";

    ADD_PROPERTY_TYPE(UseBinderStyle, (false), "", (App::PropertyType)(App::Prop_None), "");
}

void ViewProviderSubShapeBinder::attach(App::DocumentObject* obj) {

    UseBinderStyle.setValue(boost::istarts_with(obj->getNameInDocument(), "binder"));
    ViewProviderPart::attach(obj);
}

void ViewProviderSubShapeBinder::onChanged(const App::Property* prop) {
    if (prop == &UseBinderStyle
        && (!getObject() || !getObject()->isRestoring()))
    {
        App::Color shapeColor, lineColor, pointColor;
        int transparency, linewidth;
        if (UseBinderStyle.getValue()) {
            //get the datum coloring scheme
            // set default color for datums (golden yellow with 60% transparency)
            static ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/PartDesign");
            shapeColor.setPackedValue(hGrp->GetUnsigned("DefaultDatumColor", 0xFFD70099));
            lineColor = shapeColor;
            pointColor = shapeColor;
            transparency = 60;
            linewidth = 1;
        }
        else {
            shapeColor.setPackedValue(Gui::ViewParams::instance()->getDefaultShapeColor());
            lineColor.setPackedValue(Gui::ViewParams::instance()->getDefaultShapeLineColor());
            pointColor = lineColor;
            transparency = Gui::ViewParams::instance()->getDefaultShapeTransparency();
            linewidth = Gui::ViewParams::instance()->getDefaultShapeLineWidth();
        }
        ShapeColor.setValue(shapeColor);
        LineColor.setValue(lineColor);
        PointColor.setValue(pointColor);
        Transparency.setValue(transparency);
        LineWidth.setValue(linewidth);
    }

    ViewProviderPart::onChanged(prop);
}

bool ViewProviderSubShapeBinder::canDropObjectEx(App::DocumentObject*,
    App::DocumentObject*, const char*, const std::vector<std::string>&) const
{
    return true;
}

std::string ViewProviderSubShapeBinder::dropObjectEx(App::DocumentObject* obj, App::DocumentObject* owner,
    const char* subname, const std::vector<std::string>& elements)
{
    auto self = dynamic_cast<PartDesign::SubShapeBinder*>(getObject());
    if (!self)
        return {};
    std::map<App::DocumentObject*, std::vector<std::string> > values;
    if (!subname) subname = "";
    std::string sub(subname);
    if (sub.empty())
        values[owner ? owner : obj] = elements;
    else {
        std::vector<std::string> subs;
        if (!elements.empty()) {
            subs.reserve(elements.size());
            for (auto& element : elements)
                subs.push_back(sub + element);
        }
        else
            subs.push_back(sub);
        values[owner ? owner : obj] = std::move(subs);
    }

    self->setLinks(std::move(values), QApplication::keyboardModifiers() == Qt::ControlModifier);
    if (self->Relative.getValue())
        updatePlacement(false);
    return {};
}


bool ViewProviderSubShapeBinder::doubleClicked() {
    updatePlacement(true);
    return true;
}

void ViewProviderSubShapeBinder::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Synchronize"), receiver, member);
    act->setData(QVariant((int)Synchronize));
    act = menu->addAction(QObject::tr("Select bound object"), receiver, member);
    act->setData(QVariant((int)SelectObject));
    ViewProviderPart::setupContextMenu(menu, receiver, member);
}

bool ViewProviderSubShapeBinder::setEdit(int ModNum) {

    switch (ModNum) {
    case Synchronize:
        updatePlacement(true);
        break;
    case SelectObject: {
        auto self = dynamic_cast<PartDesign::SubShapeBinder*>(getObject());
        if (!self || !self->Support.getValue())
            break;

        Gui::Selection().selStackPush();
        Gui::Selection().clearSelection();
        for (auto& link : self->Support.getSubListValues()) {
            auto obj = link.getValue();
            if (!obj || !obj->getNameInDocument())
                continue;
            const auto& subs = link.getSubValues();
            if (!subs.empty())
                Gui::Selection().addSelections(obj->getDocument()->getName(),
                    obj->getNameInDocument(), subs);
            else
                Gui::Selection().addSelection(obj->getDocument()->getName(),
                    obj->getNameInDocument());
        }
        Gui::Selection().selStackPush();
        break;
    }
    default:
        return ViewProviderPart::setEdit(ModNum);
    }
    return false;
}

void ViewProviderSubShapeBinder::updatePlacement(bool transaction) {
    auto self = dynamic_cast<PartDesign::SubShapeBinder*>(getObject());
    if (!self || !self->Support.getValue())
        return;

    std::vector<Base::Matrix4D> mats;
    bool relative = self->Relative.getValue();
    App::DocumentObject* parent = nullptr;
    std::string parentSub;
    if (relative && !self->getParents().empty()) {
        const auto& sel = Gui::Selection().getSelection("", Gui::ResolveMode::NoResolve);
        if (sel.size() != 1 || !sel[0].pObject ||
            sel[0].pObject->getSubObject(sel[0].SubName) != self)
        {
            FC_WARN("invalid selection");
        }
        else {
            parent = sel[0].pObject;
            parentSub = sel[0].SubName;
        }
    }

    if (!transaction) {
        if (relative)
            self->Context.setValue(parent, parentSub.c_str());
        try {
            self->update(PartDesign::SubShapeBinder::UpdateForced);
        }
        catch (Base::Exception& e) {
            e.ReportException();
        }
        return;
    }

    App::GetApplication().setActiveTransaction("Sync binder");
    try {
        if (relative)
            self->Context.setValue(parent, parentSub.c_str());
        self->update(PartDesign::SubShapeBinder::UpdateForced);
        App::GetApplication().closeActiveTransaction();
        return;
    }
    catch (Base::Exception& e) {
        e.ReportException();
    }
    catch (Standard_Failure& e) {
        std::ostringstream str;
        Standard_CString msg = e.GetMessageString();
        str << typeid(e).name() << " ";
        if (msg) { str << msg; }
        else { str << "No OCCT Exception Message"; }
        FC_ERR(str.str());
    }
    App::GetApplication().closeActiveTransaction(true);
}

std::vector<App::DocumentObject*> ViewProviderSubShapeBinder::claimChildren() const {
    std::vector<App::DocumentObject*> ret;
    auto self = Base::freecad_dynamic_cast<PartDesign::SubShapeBinder>(getObject());
    if (self && self->ClaimChildren.getValue() && self->Support.getValue()) {
        std::set<App::DocumentObject*> objSet;
        for (auto& l : self->Support.getSubListValues()) {
            auto obj = l.getValue();
            if (!obj)
                continue;
            const auto& subs = l.getSubValues();
            if (subs.empty()) {
                if (objSet.insert(obj).second)
                    ret.push_back(obj);
                continue;
            }
            for (auto& sub : subs) {
                auto sobj = obj->getSubObject(sub.c_str());
                if (sobj && objSet.insert(sobj).second)
                    ret.push_back(sobj);
            }
        }
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////

namespace Gui {
PROPERTY_SOURCE_TEMPLATE(PartDesignGui::ViewProviderSubShapeBinderPython,
                         PartDesignGui::ViewProviderSubShapeBinder)
template class PartDesignGuiExport ViewProviderPythonFeatureT<ViewProviderSubShapeBinder>;
}
