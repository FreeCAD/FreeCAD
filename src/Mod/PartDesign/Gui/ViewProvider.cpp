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
#endif

#include <Base/Exception.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>

#include "TaskFeatureParameters.h"

#include "ViewProvider.h"
#include "ViewProviderPy.h"

using namespace PartDesignGui;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesignGui::ViewProvider, PartGui::ViewProviderPart)

ViewProvider::ViewProvider()
{
    PartGui::ViewProviderAttachExtension::initExtension(this);
}

ViewProvider::~ViewProvider() = default;

bool ViewProvider::doubleClicked()
{
    try {
        QString text = QObject::tr("Edit %1").arg(QString::fromUtf8(getObject()->Label.getValue()));
        Gui::Command::openCommand(text.toUtf8());
        FCMD_SET_EDIT(pcObject);
    }
    catch (const Base::Exception&) {
        Gui::Command::abortCommand();
    }
    return true;
}

void ViewProvider::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QIcon iconObject = mergeGreyableOverlayIcons(Gui::BitmapFactory().pixmap("Part_ColorFace.svg"));
    QAction* act = menu->addAction(iconObject, QObject::tr("Set colors..."), receiver, member);
    act->setData(QVariant((int)ViewProvider::Color));
    // Call the extensions
    Gui::ViewProvider::setupContextMenu(menu, receiver, member);
}

bool ViewProvider::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this feature the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgFeatureParameters *featureDlg = qobject_cast<TaskDlgFeatureParameters *>(dlg);
        // NOTE: if the dialog is not partDesigan dialog the featureDlg will be NULL
        if (featureDlg && featureDlg->viewProvider() != this) {
            featureDlg = nullptr; // another feature left open its task panel
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
            if (!featureDlg) { // Shouldn't generally happen
                throw Base::RuntimeError ("Failed to create new edit dialog.");
            }
        }

        Gui::Control().showDialog(featureDlg);
        return true;
    } else {
        return PartGui::ViewProviderPart::setEdit(ModNum);
    }
}


TaskDlgFeatureParameters *ViewProvider::getEditDialog() {
    throw Base::NotImplementedError("getEditDialog() not implemented");
}


void ViewProvider::unsetEdit(int ModNum)
{
    // return to the WB we were in before editing the PartDesign feature
    if (!oldWb.empty())
        Gui::Command::assureWorkbench(oldWb.c_str());

    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
#if 0
        PartDesign::Body* activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
#endif
        Gui::Control().closeDialog();
#if 0
        if ((activeBody != NULL) && (oldTip != NULL)) {
            Gui::Selection().clearSelection();
            Gui::Selection().addSelection(oldTip->getDocument()->getName(), oldTip->getNameInDocument());
            Gui::Command::doCommand(Gui::Command::Gui,"FreeCADGui.runCommand('PartDesign_MoveTip')");
        }
#endif
        oldTip = nullptr;
    }
    else {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
        oldTip = nullptr;
    }
}

void ViewProvider::updateData(const App::Property* prop)
{
    // TODO What's that? (2015-07-24, Fat-Zer)
    if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId() &&
        strcmp(prop->getName(),"AddSubShape") == 0) {
        return;
    }

    inherited::updateData(prop);
}

void ViewProvider::onChanged(const App::Property* prop) {

    //if the object is inside of a body we make sure it is the only visible one on activation
    if(prop == &Visibility && Visibility.getValue()) {

        Part::BodyBase* body = Part::BodyBase::findBodyOf(getObject());
        if(body) {

            //hide all features in the body other than this object
            for(App::DocumentObject* obj : body->Group.getValues()) {

                if(obj->isDerivedFrom(PartDesign::Feature::getClassTypeId()) && obj != getObject()) {
                   auto vpd = Base::freecad_dynamic_cast<Gui::ViewProviderDocumentObject>(
                           Gui::Application::Instance->getViewProvider(obj));
                   if(vpd && vpd->Visibility.getValue())
                       vpd->Visibility.setValue(false);
                }
            }
        }
    }

    PartGui::ViewProviderPartExt::onChanged(prop);
}

void ViewProvider::setTipIcon(bool onoff) {
    isSetTipIcon = onoff;

    signalChangeIcon();
}

QIcon ViewProvider::mergeColorfulOverlayIcons (const QIcon & orig) const
{
    QIcon mergedicon = orig;

    if(isSetTipIcon) {
        QPixmap px;

        static const char * const feature_tip_xpm[]={
            "9 9 3 1",
            ". c None",
            "# c #00cc00",
            "a c #ffffff",
            "...###...",
            ".##aaa##.",
            ".##aaa##.",
            "###aaa###",
            "##aaaaa##",
            "##aaaaa##",
            ".##aaa##.",
            ".##aaa##.",
            "...###..."};
        px = QPixmap(feature_tip_xpm);

        mergedicon = Gui::BitmapFactoryInst::mergePixmap(mergedicon, px, Gui::BitmapFactoryInst::BottomRight);

    }

    return Gui::ViewProvider::mergeColorfulOverlayIcons (mergedicon);
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

    if (body) {
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



namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartDesignGui::ViewProviderPython, PartDesignGui::ViewProvider)
/// @endcond

// explicit template instantiation
template class PartDesignGuiExport ViewProviderPythonFeatureT<PartDesignGui::ViewProvider>;
}

