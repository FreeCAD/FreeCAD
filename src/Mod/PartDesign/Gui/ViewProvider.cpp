/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <QMessageBox>
# include <QAction>
# include <QApplication>
# include <QMenu>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/details/SoFaceDetail.h>
#endif

#include <Gui/Command.h>
#include <Gui/MDIView.h>
#include <Gui/Control.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/CommandT.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/Part/Gui/SoBrepPointSet.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/FeatureExtrusion.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "Utils.h"
#include "TaskFeatureParameters.h"

#include "ViewProvider.h"
#include "ViewProviderPy.h"
#include "ViewProviderBody.h"

using namespace PartDesignGui;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesignGui::ViewProvider, PartGui::ViewProviderPart)

ViewProvider::ViewProvider()
:oldWb(""), isSetTipIcon(false)
{
    PartGui::ViewProviderAttachExtension::initExtension(this);
    ADD_PROPERTY(IconColor,((long)0));
}

ViewProvider::~ViewProvider()
{
}

bool ViewProvider::doubleClicked(void)
{
    auto modifiers = QApplication::queryKeyboardModifiers();
    if (modifiers & Qt::ShiftModifier) {
        auto feat = Base::freecad_dynamic_cast<PartDesign::Feature>(getObject());
        auto bodyVp = Base::freecad_dynamic_cast<ViewProviderBody>(
                Gui::Application::Instance->getViewProvider(
                    PartDesign::Body::findBodyOf(getObject())));

        App::SubObjectT objT;
        if (feat && bodyVp) {
            auto sels = Gui::Selection().getSelectionT("*", 0);
            if (sels.size() && sels[0].getSubObject() == feat) {
                auto parent = sels[0].getParent();
                if (parent.getSubObject() == bodyVp->getObject())
                    objT = sels[0];
                else {
                    parent = parent.getParent();
                    if (parent.getSubObject() == bodyVp->getObject()) {
                        objT = parent;
                        objT.setSubName(objT.getSubName()
                                + feat->getNameInDocument() + ".");
                    }
                }
            }
            Gui::Selection().clearSelection();
            bodyVp->groupSiblings(feat, feat->_Siblings.getSize() == 0, false);
            if (objT.getObjectName().size())
                Gui::Selection().addSelection(objT);
            return true;
        }
        return false;
    }

    std::string Msg("Edit ");
    Msg += this->pcObject->Label.getValue();
    App::AutoTransaction committer(Msg.c_str());
    try {
	    PartDesign::Body* body = PartDesign::Body::findBodyOf(getObject());
        PartDesignGui::setEdit(pcObject,body);
    }
    catch (const Base::Exception &) {
        committer.close(true);
    }
    return true;
}

enum PartDesignEditMode {
    EditToggleSupress = ViewProvider::UserEditMode+1,
    EditSelectSiblings = ViewProvider::UserEditMode+2,
    EditExpandSiblings = ViewProvider::UserEditMode+3,
    EditExpandAll = ViewProvider::UserEditMode+4,
    EditCollapseSiblings = ViewProvider::UserEditMode+5,
    EditCollapseAll = ViewProvider::UserEditMode+6,
    EditSetTip = ViewProvider::UserEditMode+7,
};

void ViewProvider::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    auto feat = Base::freecad_dynamic_cast<PartDesign::Feature>(getObject());
    if(feat) {
        QAction* act = menu->addAction(QObject::tr(feat->Suppress.getValue()?"Unsuppress":"Suppress"),
                receiver, member);
        act->setData(QVariant((int)EditToggleSupress));

        if (feat->_Siblings.getSize()) {
            act = menu->addAction(QObject::tr("Expand siblings"), receiver, member);
            act->setData(QVariant((int)EditExpandSiblings));
            act = menu->addAction(QObject::tr("Expand all"), receiver, member);
            act->setData(QVariant((int)EditExpandAll));
        }

        if (PartDesign::Body::isSolidFeature(feat)) {
            auto body = PartDesign::Body::findBodyOf(feat);
            if (body) {
                auto siblings = body->getSiblings(feat);
                if (siblings.size() > 1) {
                    if (siblings.front() != feat) {
                        act = menu->addAction(QObject::tr("Collapse siblings"), receiver, member);
                        act->setData(QVariant((int)EditCollapseSiblings));
                    }
                    act = menu->addAction(QObject::tr("Collapse all"), receiver, member);
                    act->setData(QVariant((int)EditCollapseAll));
                }
                if (body->Tip.getValue() != feat) {
                    act = menu->addAction(QObject::tr("Set body tip"), receiver, member);
                    act->setData(QVariant((int)EditSetTip));
                }
            }
            act = menu->addAction(QObject::tr("Select siblings"), receiver, member);
            act->setData(QVariant((int)EditSelectSiblings));
        }
    }
    QAction* act = menu->addAction(QObject::tr("Set colors..."), receiver, member);
    act->setData(QVariant((int)ViewProvider::Color));
    // Call the extensions
    Gui::ViewProvider::setupContextMenu(menu, receiver, member);
}

bool ViewProvider::setEdit(int ModNum)
{
    auto feat = Base::freecad_dynamic_cast<PartDesign::Feature>(getObject());
    if (!feat)
        return false;

    switch(ModNum) {
    case ViewProvider::Default: {
        // When double-clicking on the item for this feature the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgFeatureParameters *featureDlg = qobject_cast<TaskDlgFeatureParameters *>(dlg);
        // NOTE: if the dialog is not partDesigan dialog the featureDlg will be NULL
        if (featureDlg && featureDlg->viewProvider() != this) {
            featureDlg = 0; // another feature left open its task panel
        }
        if (dlg && !featureDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes) {
                Gui::Control().reject();
            } else {
                return false;
            }
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // always change to PartDesign WB, remember where we come from
        oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

        // start the edit dialog if
        if (!featureDlg) {
            featureDlg = this->getEditDialog();
            if (!featureDlg) // Shouldn't generally happen
                return false;
        }

        PartDesignGui::beforeEdit(getObject());
        Gui::Control().showDialog(featureDlg);
        return true;
    }
    case EditToggleSupress: {
        std::ostringstream ss;
        ss << (feat->Suppress.getValue()?"Unsuppress":"Suppress") << " " << feat->getNameInDocument();
        App::AutoTransaction committer(ss.str().c_str());
        try {
            if(feat->Suppress.getValue())
                Gui::cmdAppObject(feat, "Suppress = False"); 
            else
                Gui::cmdAppObject(feat, "Suppress = True"); 
            Gui::cmdAppDocument(App::GetApplication().getActiveDocument(), "recompute()");
        } catch (Base::Exception &e) {
            e.ReportException();
        }
        return false;
    }
    case EditCollapseSiblings:
    case EditCollapseAll:
    case EditExpandSiblings:
    case EditExpandAll:
    {
        auto bodyVp = Base::freecad_dynamic_cast<ViewProviderBody>(
                Gui::Application::Instance->getViewProvider(
                    PartDesign::Body::findBodyOf(getObject())));
        if (bodyVp)
            bodyVp->groupSiblings(feat, 
                    ModNum == EditCollapseSiblings || ModNum == EditCollapseAll,
                    ModNum == EditCollapseAll || ModNum == EditExpandAll);
        return false;
    }
    case EditSetTip: {
        auto body = PartDesign::Body::findBodyOf(feat);
        body->Tip.setValue(feat);
        feat->Visibility.setValue(true);
        Gui::Command::updateActive();
        return false;
    }
    case EditSelectSiblings: {
        auto body = PartDesign::Body::findBodyOf(getObject());
        if (body) {
            ViewProviderDocumentObject *vpParent = 0;
            std::string subname;
            auto doc = Gui::Application::Instance->editDocument();
            if(!doc) 
                return false;
            doc->getInEdit(&vpParent,&subname);
            if (!vpParent)
                return false;

            App::SubObjectT objT(vpParent->getObject(), subname.c_str());
            objT = objT.getParent();

            for (auto obj : body->getSiblings(getObject()))
                Gui::Selection().addSelection(objT.getChild(obj));
        }
        return false;
    }
    default:
        return PartGui::ViewProviderPart::setEdit(ModNum);
    }
}


TaskDlgFeatureParameters *ViewProvider::getEditDialog() {
    return nullptr;
}


void ViewProvider::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // return to the WB we were in before editing the PartDesign feature
        if (!oldWb.empty()) {
            Gui::Command::assureWorkbench(oldWb.c_str());
            oldWb.clear();
        }

        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
    }
}

void ViewProvider::updateData(const App::Property* prop)
{
    auto feature = Base::freecad_dynamic_cast<PartDesign::Feature>(getObject());
    if(!feature) {
        inherited::updateData(prop);
        return;
    }

    if(prop == &feature->SuppressedShape) {
        if (feature->SuppressedShape.getShape().isNull()) {
            enableFullSelectionHighlight();
        } else {
            auto node = getDisplayMaskMode("Flat Lines");
            if (!pSuppressedView && node && node->isOfType(SoGroup::getClassTypeId())) {
                pSuppressedView.reset(new PartGui::ViewProviderPart);
                pSuppressedView->setShapePropertyName("SuppressedShape");
                pSuppressedView->forceUpdate();
                pSuppressedView->MapFaceColor.setValue(false);    
                pSuppressedView->MapLineColor.setValue(false);    
                pSuppressedView->MapPointColor.setValue(false);    
                pSuppressedView->MapTransparency.setValue(false);    
                pSuppressedView->ForceMapColors.setValue(false);
                pSuppressedView->ShapeColor.setValue(App::Color(1.0f));
                pSuppressedView->LineColor.setValue(App::Color(1.0f));
                pSuppressedView->PointColor.setValue(App::Color(1.0f));
                pSuppressedView->Selectable.setValue(false);
                pSuppressedView->enableFullSelectionHighlight(false, false, false);
                pSuppressedView->setStatus(Gui::SecondaryView,true);

                auto switchNode = getModeSwitch();
                if(switchNode->isOfType(Gui::SoFCSwitch::getClassTypeId()))
                    static_cast<Gui::SoFCSwitch*>(switchNode)->overrideSwitch = Gui::SoFCSwitch::OverrideVisible;

                pSuppressedView->attach(feature);

                static_cast<SoGroup*>(node)->addChild(pSuppressedView->getRoot());
            }

            if(pSuppressedView)
                enableFullSelectionHighlight(false, false, false);
        }

        if(pSuppressedView)
            pSuppressedView->updateData(prop);

    } else if (prop == &feature->Suppress) {
        signalChangeIcon();
    } else if (prop == &feature->_Siblings) {
        pxTipIcon = QPixmap();
        signalChangeIcon();
    } else if (prop == &feature->_Siblings) {

    } else if (prop == &feature->Shape) {
        if (!getObject()->getDocument()->isPerformingTransaction()
                && !getObject()->getDocument()->testStatus(App::Document::Restoring)
                && !IconColor.getValue().getPackedValue())
        {
            auto body = Base::freecad_dynamic_cast<ViewProviderBody>(
                    Gui::Application::Instance->getViewProvider(
                        PartDesign::Body::findBodyOf(getObject())));
            if (body) {
                unsigned long color = body->generateIconColor(getObject());
                if (color)
                    IconColor.setValue(color);
            }
        }
    } else if (prop == &feature->BaseFeature && !prop->testStatus(App::Property::User3)) {
        PartDesign::Body* body = PartDesign::Body::findBodyOf(getObject());
        auto bodyVp = Base::freecad_dynamic_cast<ViewProviderBody>(
                Gui::Application::Instance->getViewProvider(body));
        if (!getObject()->getDocument()->isPerformingTransaction()
                && !getObject()->getDocument()->testStatus(App::Document::Restoring)
                && bodyVp
                && IconColor.getValue().getPackedValue())
        {
            unsigned long color = 0;
            if (!PartDesign::Body::isSolidFeature(getObject()))
                this->IconColor.setValue(0);
            else {
                if (!feature->BaseFeature.getValue())
                    color = bodyVp->generateIconColor();

                bool first = true;
                for (auto obj : body->getSiblings(feature)) {
                    auto vp = Base::freecad_dynamic_cast<ViewProvider>(
                            Gui::Application::Instance->getViewProvider(obj));
                    if (!vp)
                        continue;
                    if (first) {
                        first = false;
                        if (!color) {
                            color = vp->IconColor.getValue().getPackedValue();
                            if (!color) {
                                color = IconColor.getValue().getPackedValue();
                                if (!color) {
                                    color = bodyVp->generateIconColor();
                                    if (!color)
                                        break;
                                }
                            }
                        }
                    }
                    vp->IconColor.setValue(color);
                    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
                    bool randomColor = hGrp->GetBool("RandomColor", false);
                    if (randomColor){
                        bodyVp->ShapeColor.setValue(color);
                    }
                }
            }
        }
    }

    inherited::updateData(prop);
}

void ViewProvider::updateVisual()
{
    inherited::updateVisual();
    auto feature = Base::freecad_dynamic_cast<PartDesign::Feature>(getObject());
    if (feature && feature->SuppressedShape.getShape().isNull()) {
        std::vector<int> faces;
        std::vector<int> edges;
        std::vector<int> vertices;
        feature->getGeneratedIndices(faces,edges,vertices);

        lineset->highlightIndices.setNum(edges.size());
        lineset->highlightIndices.setValues(0,edges.size(),&edges[0]);
        nodeset->highlightIndices.setNum(vertices.size());
        nodeset->highlightIndices.setValues(0,vertices.size(),&vertices[0]);
        faceset->highlightIndices.setNum(faces.size());
        faceset->highlightIndices.setValues(0,faces.size(),&faces[0]);
    }
}

void ViewProvider::onChanged(const App::Property* prop) {
    if (prop == &IconColor) {
        pxTipIcon = QPixmap();
        signalChangeIcon();

        auto feature = Base::freecad_dynamic_cast<PartDesign::Feature>(getObject());
        auto body = PartDesign::Body::findBodyOf(getObject());
        if (feature && body && feature->_Siblings.getSize()) {
            auto siblings = body->getSiblings(feature);
            if (siblings.size() && siblings.front() == feature)
                feature->_Siblings.setValues();
        }
    }

    PartGui::ViewProviderPartExt::onChanged(prop);
}

void ViewProvider::setTipIcon(bool onoff)
{
    if (isSetTipIcon != onoff) {
        isSetTipIcon = onoff;
        pxTipIcon = QPixmap();
        signalChangeIcon();
    }
}

QPixmap ViewProvider::getTagIcon() const
{
    unsigned long color = IconColor.getValue().getPackedValue();
    if (!color)
        return QPixmap();

    if(pxTipIcon.isNull()) {
        std::map<unsigned long, unsigned long> colormap;
        colormap[0xffffff] = color >> 8;
        if (isSetTipIcon)
            colormap[0xf0f0f0] = 0x00ff00;
        pxTipIcon = Gui::BitmapFactory().pixmapFromSvg("PartDesign_Overlay.svg",
                                                        QSizeF(64,64),
                                                        colormap);
        auto feat = Base::freecad_dynamic_cast<PartDesign::Feature>(getObject());
        if (feat && feat->_Siblings.getSize()) {
            QPixmap px(64, 64);
            QPainter pt;
            px.fill(Qt::transparent);
            pt.begin(&px);
            pt.setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);
            pt.rotate(10);
            pt.drawPixmap(0, 0, pxTipIcon);
            pt.rotate(-20);
            pt.drawPixmap(0, 0, pxTipIcon);
            pt.end();
            pxTipIcon = px;
        }
    }
    return pxTipIcon;
}

void ViewProvider::getExtraIcons(std::vector<QPixmap> &icons) const
{
    auto feat = Base::freecad_dynamic_cast<PartDesign::Feature>(getObject());
    if (!feat) {
        inherited::getExtraIcons(icons);
        return;
    }

    QPixmap px = getTagIcon();
    if (!px.isNull())
        icons.push_back(px);

    if(feat->Suppress.getValue())
        icons.push_back(Gui::BitmapFactory().pixmap("PartDesign_Suppressed.svg"));

    inherited::getExtraIcons(icons);
}

bool ViewProvider::onDelete(const std::vector<std::string> &)
{
    PartDesign::Feature* feature = static_cast<PartDesign::Feature*>(getObject());

    App::DocumentObject* previousfeat = feature->BaseFeature.getValue();

    // Visibility - we want:
    // 1. If the visible object is not the one being deleted, we leave that one visible.
    // 2. If the visible object is the one being deleted, we make the previous object visible.
    if (isShow() && previousfeat && Gui::Application::Instance->getViewProvider(previousfeat)) {
        Gui::Application::Instance->getViewProvider(previousfeat)->show();
    }

    // find surrounding features in the tree
    Part::BodyBase* body = PartDesign::Body::findBodyOf(getObject());

    if (body != NULL) {
        // Deletion from the tree of a feature is handled by Document.removeObject, which has no clue
        // about what a body is. Therefore, Bodies, although an "activable" container, know nothing
        // about what happens at Document level with the features they contain.
        //
        // The Deletion command StdCmdDelete::activated, however does notify the viewprovider corresponding
        // to the feature (not body) of the imminent deletion (before actually doing it).
        //
        // Consequently, the only way of notifying a body of the imminent deletion of one of its features
        // so as to do the clean up required (moving basefeature references, tip management) is from the
        // viewprovider, so we call it here.
        //
        // fixes (#3084)

        FCMD_OBJ_CMD(body,"removeObject(" << Gui::Command::getObjectCmd(feature) << ')');
    }

    return true;
}

void ViewProvider::setBodyMode(bool bodymode) {

    std::vector<App::Property*> props;
    getPropertyList(props);

    auto vp = getBodyViewProvider();
    if(!vp)
        return;
    
#if 1
    // Realthunder: I want to test element color mapping in PartDesign.
    // If it works well, then there is no reason to hide all the properties.
    (void)bodymode;
#else
    for(App::Property* prop : props) {

        //we keep visibility and selectibility per object
        if(prop == &Visibility ||
           prop == &Selectable)
            continue;

        //we hide only properties which are available in the body, not special ones
        if(!vp->getPropertyByName(prop->getName()))
            continue;

        prop->setStatus(App::Property::Hidden, bodymode);
    }
#endif
}

void ViewProvider::makeTemporaryVisible(bool onoff)
{
    //make sure to not use the overridden versions, as they change properties
    if (onoff) {
        if (VisualTouched) {
            updateVisual();
        }
        Gui::ViewProvider::show();
    }
    else
        Gui::ViewProvider::hide();
}

PyObject* ViewProvider::getPyObject()
{
    if (!pyViewObject)
        pyViewObject = new ViewProviderPy(this);
    pyViewObject->IncRef();
    return pyViewObject;
}

ViewProviderBody* ViewProvider::getBodyViewProvider() {

    auto body = PartDesign::Body::findBodyOf(getObject());
    auto doc = getDocument();
    if(body && doc) {
        auto vp = doc->getViewProvider(body);
        if(vp && vp->isDerivedFrom(ViewProviderBody::getClassTypeId()))
           return static_cast<ViewProviderBody*>(vp);
    }

    return nullptr;
}

bool ViewProvider::hasBaseFeature() const{
    auto feature = Base::freecad_dynamic_cast<PartDesign::Feature>(getObject());
    if(feature && feature->getBaseObject(true))
        return true;
    return PartGui::ViewProviderPart::hasBaseFeature();
}

bool ViewProvider::canReplaceObject(App::DocumentObject *oldObj, App::DocumentObject *newObj)
{
    auto vp = Gui::Application::Instance->getViewProvider(
            PartDesign::Body::findBodyOf(getObject()));
    return vp && vp->canReplaceObject(oldObj, newObj);
}

int ViewProvider::replaceObject(App::DocumentObject *oldObj, App::DocumentObject *newObj)
{
    auto vp = Gui::Application::Instance->getViewProvider(
            PartDesign::Body::findBodyOf(getObject()));
    if (vp)
        return vp->replaceObject(oldObj, newObj);
    return 0;
}

std::vector<App::DocumentObject*> ViewProvider::claimChildren(void) const
{
    auto res = inherited::claimChildren();
    auto feature = Base::freecad_dynamic_cast<PartDesign::Feature>(getObject());
    if (feature) {
        auto siblings = feature->_Siblings.getValues();
        res.insert(res.end(), siblings.begin(), siblings.end());
    }
    return res;
}

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartDesignGui::ViewProviderPython, PartDesignGui::ViewProvider)
/// @endcond

// explicit template instantiation
template class PartDesignGuiExport ViewProviderPythonFeatureT<PartDesignGui::ViewProvider>;
}

