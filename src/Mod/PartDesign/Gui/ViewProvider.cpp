// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <QMessageBox>
#include <QAction>
#include <QMenu>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <BRep_Builder.hxx>

#include <Base/Exception.h>
#include <Base/ServiceProvider.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection/SoFCUnifiedSelection.h>
#include <Gui/Inventor/So3DAnnotation.h>
#include <Gui/MainWindow.h>
#include <Gui/Utilities.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>
#include <Mod/Part/Gui/ViewProvider.h>
#include <Mod/Part/Gui/ViewProviderExt.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>

#include "TaskFeatureParameters.h"
#include "StyleParameters.h"

#include "ViewProvider.h"
#include "ViewProviderPy.h"


using namespace PartDesignGui;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesignGui::ViewProvider, PartGui::ViewProviderPart)

ViewProvider::ViewProvider()
{
    ViewProviderSuppressibleExtension::initExtension(this);
    ViewProviderAttachExtension::initExtension(this);
    ViewProviderPreviewExtension::initExtension(this);
}

ViewProvider::~ViewProvider() = default;

void ViewProvider::beforeDelete()
{
    ViewProviderPart::beforeDelete();
}

void ViewProvider::attach(App::DocumentObject* pcObject)
{
    ViewProviderPart::attach(pcObject);

    auto* styleParameterManager = Base::provideService<Gui::StyleParameters::ParameterManager>();

    if (auto addSubFeature = getObject<PartDesign::FeatureAddSub>()) {
        bool isAdditive = addSubFeature->getAddSubType() == PartDesign::FeatureAddSub::Additive;

        PreviewColor.setValue(
            isAdditive ? styleParameterManager->resolve(StyleParameters::PreviewAdditiveColor)
                       : styleParameterManager->resolve(StyleParameters::PreviewSubtractiveColor)
        );
    }
}

bool ViewProvider::doubleClicked()
{
    try {
        QString text = QObject::tr("Edit %1").arg(QString::fromUtf8(getObject()->Label.getValue()));
        Gui::Command::openCommand(text.toUtf8());
        Gui::cmdSetEdit(pcObject, Gui::Application::Instance->getUserEditMode());
    }
    catch (const Base::Exception&) {
        Gui::Command::abortCommand();
    }
    return true;
}

void ViewProvider::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QIcon iconObject = mergeGreyableOverlayIcons(Gui::BitmapFactory().pixmap("Part_ColorFace.svg"));
    QAction* act = menu->addAction(iconObject, QObject::tr("Set Face Colors"), receiver, member);

    act->setData(QVariant((int)ViewProvider::Color));
    // Call the extensions
    Gui::ViewProvider::setupContextMenu(menu, receiver, member);
}

bool ViewProvider::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Transform) {
        if (forwardToLink()) {
            return true;
        }

        // this is feature so we need to forward the transform to the body
        forwardedViewProvider = getBodyViewProvider();
        return forwardedViewProvider->startEditing(ModNum);
    }
    else if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this feature the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
        TaskDlgFeatureParameters* featureDlg = qobject_cast<TaskDlgFeatureParameters*>(dlg);
        // NOTE: if the dialog is not partDesigan dialog the featureDlg will be NULL
        if (featureDlg && featureDlg->getViewObject() != this) {
            featureDlg = nullptr;  // another feature left open its task panel
        }
        if (dlg && !featureDlg) {
            QMessageBox msgBox(Gui::getMainWindow());
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);

            if (msgBox.exec() == QMessageBox::Yes) {
                Gui::Control().reject();
            }
            else {
                return false;
            }
        }

        // This is handling for an erroneous case where features are for some reason placed outside
        // the body container. That should never happen, but in some cases we find models with a
        // problem like that.
        if (ViewProviderBody* bodyViewProvider = getBodyViewProvider()) {
            PartDesign::Feature* shownFeature = bodyViewProvider->getShownFeature();

            previouslyShownViewProvider = freecad_cast<ViewProvider*>(
                Gui::Application::Instance->getViewProvider(shownFeature)
            );
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // always change to PartDesign WB, remember where we come from
        oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

        // start the edit dialog if
        if (!featureDlg) {
            featureDlg = this->getEditDialog();
            if (!featureDlg) {  // Shouldn't generally happen
                throw Base::RuntimeError("Failed to create new edit dialog.");
            }
        }

        Gui::Control().showDialog(featureDlg);
        return true;
    }
    else {
        return PartGui::ViewProviderPart::setEdit(ModNum);
    }
}


TaskDlgFeatureParameters* ViewProvider::getEditDialog()
{
    throw Base::NotImplementedError("getEditDialog() not implemented");
}


void ViewProvider::unsetEdit(int ModNum)
{
    showPreview(false);

    // return to the WB we were in before editing the PartDesign feature
    if (!oldWb.empty()) {
        Gui::Command::assureWorkbench(oldWb.c_str());
    }

    // ensure that after edit we still show the same feature
    if (previouslyShownViewProvider) {
        previouslyShownViewProvider->show();
    }

    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
    }
}

void ViewProvider::updateData(const App::Property* prop)
{
    if (strcmp(prop->getName(), "PreviewShape") == 0) {
        updatePreview();
    }
    else if (auto* previewExtension = getObject()->getExtensionByType<Part::PreviewExtension>(true)) {
        if (isPreviewEnabled() && !previewExtension->isPreviewFresh() && isEditing()) {
            // Properties can be updated in batches, where some properties trigger other updates.
            // We don't need to compute the preview for intermediate steps. Instead of updating
            // the preview immediately (and potentially doing it multiple times in a row), we
            // schedule the update to happen at a more convenient time.
            if (auto* scheduler = Base::provideService<Part::PreviewUpdateScheduler>()) {
                scheduler->schedulePreviewRecompute(getObject());
            }
        }
    }
    inherited::updateData(prop);
}

void ViewProvider::attachPreview()
{
    ViewProviderPreviewExtension::attachPreview();

    auto* styleParameterManager = Base::provideService<Gui::StyleParameters::ParameterManager>();

    const double opacity = styleParameterManager->resolve(StyleParameters::PreviewToolOpacity).value;
    const double lineWidth = styleParameterManager->resolve(StyleParameters::PreviewLineWidth).value;

    pcPreviewShape->lineWidth = static_cast<float>(lineWidth);

    pcToolPreview = new PartGui::SoPreviewShape;
    pcToolPreview->transparency = 1.0F - static_cast<float>(opacity);
    pcToolPreview->color.connectFrom(&pcPreviewShape->color);

    pcPreviewRoot->addChild(pcToolPreview);
}

void ViewProvider::updatePreview()
{
    ViewProviderPreviewExtension::updatePreview();

    if (auto* addSubFeature = getObject<PartDesign::FeatureAddSub>()) {
        // we only want to show the additional tool preview for subtractive features
        if (addSubFeature->getAddSubType() != PartDesign::FeatureAddSub::Subtractive) {
            return;
        }

        Part::TopoShape toolShape = addSubFeature->AddSubShape.getShape();

        updatePreviewShape(toolShape, pcToolPreview);
    }
    else {
        updatePreviewShape({}, pcToolPreview);
    }
}

void ViewProvider::makeChildrenVisible()
{
    for (const auto child : claimChildren()) {
        if (auto vp = Gui::Application::Instance->getViewProvider(child)) {
            vp->show();
        }
    }
}

void ViewProvider::onChanged(const App::Property* prop)
{

    // if the object is inside of a body we make sure it is the only visible one on activation
    if (prop == &Visibility && Visibility.getValue()) {

        Part::BodyBase* body = Part::BodyBase::findBodyOf(getObject());
        if (body) {

            // hide all features in the body other than this object
            for (App::DocumentObject* obj : body->Group.getValues()) {

                if (obj->isDerivedFrom<PartDesign::Feature>() && obj != getObject()) {
                    auto vpd = freecad_cast<Gui::ViewProviderDocumentObject*>(
                        Gui::Application::Instance->getViewProvider(obj)
                    );
                    if (vpd && vpd->Visibility.getValue()) {
                        vpd->Visibility.setValue(false);
                    }
                }
            }
        }
    }

    PartGui::ViewProviderPartExt::onChanged(prop);
}

Gui::ViewProvider* ViewProvider::startEditing(int ModNum)
{
    // in case of transform we forward the request to body
    if (ModNum == Transform) {
        forwardedViewProvider = nullptr;

        if (!ViewProviderPart::startEditing(ModNum)) {
            return nullptr;
        }

        return forwardedViewProvider;
    }

    return ViewProviderPart::startEditing(ModNum);
}

void ViewProvider::setTipIcon(bool onoff)
{
    isSetTipIcon = onoff;

    signalChangeIcon();
}

QIcon ViewProvider::mergeColorfulOverlayIcons(const QIcon& orig) const
{
    QIcon mergedicon = orig;

    if (isSetTipIcon) {
        static QPixmap px(Gui::BitmapFactory().pixmapFromSvg("PartDesign_Overlay_Tip", QSize(10, 10)));
        mergedicon
            = Gui::BitmapFactoryInst::mergePixmap(mergedicon, px, Gui::BitmapFactoryInst::BottomRight);
    }

    return Gui::ViewProvider::mergeColorfulOverlayIcons(mergedicon);
}

bool ViewProvider::onDelete(const std::vector<std::string>&)
{
    PartDesign::Feature* feature = getObject<PartDesign::Feature>();

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
        // Deletion from the tree of a feature is handled by Document.removeObject, which has no
        // clue about what a body is. Therefore, Bodies, although an "activable" container, know
        // nothing about what happens at Document level with the features they contain.
        //
        // The Deletion command StdCmdDelete::activated, however does notify the viewprovider
        // corresponding to the feature (not body) of the imminent deletion (before actually doing
        // it).
        //
        // Consequently, the only way of notifying a body of the imminent deletion of one of its
        // features so as to do the clean up required (moving basefeature references, tip
        // management) is from the viewprovider, so we call it here.
        //
        // fixes (#3084)

        FCMD_OBJ_CMD(body, "removeObject(" << Gui::Command::getObjectCmd(feature) << ')');
    }

    makeChildrenVisible();

    return true;
}

Part::TopoShape ViewProvider::getPreviewShape() const
{
    if (auto feature = getObject()->getExtensionByType<Part::PreviewExtension>(true)) {
        // Feature is responsible for generating proper shape and this ViewProvider
        // is using it instead of more normal `Shape` property.
        return feature->PreviewShape.getShape();
    }

    return {};
}

void ViewProvider::showPreviousFeature(bool enable)
{
    PartDesign::Feature* feature {getObject<PartDesign::Feature>()};
    PartDesign::Feature* baseFeature {nullptr};

    ViewProvider* baseFeatureViewProvider {nullptr};

    if (!feature) {
        return;
    }

    baseFeature = dynamic_cast<PartDesign::Feature*>(feature->BaseFeature.getValue());
    if (baseFeature) {
        baseFeatureViewProvider = freecad_cast<ViewProvider*>(
            Gui::Application::Instance->getViewProvider(baseFeature)
        );
    }

    if (!baseFeatureViewProvider) {
        baseFeatureViewProvider = this;
    }

    if (enable) {
        baseFeatureViewProvider->show();
        hide();
    }
    else {
        baseFeatureViewProvider->hide();
        show();
    }
}

void ViewProvider::setBodyMode(bool bodymode)
{

    std::vector<App::Property*> props;
    getPropertyList(props);

    auto vp = getBodyViewProvider();
    if (!vp) {
        return;
    }

    for (App::Property* prop : props) {

        // we keep visibility and selectibility per object
        if (prop == &Visibility || prop == &Selectable) {
            continue;
        }

        // we hide only properties which are available in the body, not special ones
        if (!vp->getPropertyByName(prop->getName())) {
            continue;
        }

        prop->setStatus(App::Property::Hidden, bodymode);
    }
}

void ViewProvider::makeTemporaryVisible(bool onoff)
{
    // make sure to not use the overridden versions, as they change properties
    if (onoff) {
        if (VisualTouched) {
            updateVisual();
        }
        Gui::ViewProvider::show();
    }
    else {
        Gui::ViewProvider::hide();
    }
}

PyObject* ViewProvider::getPyObject()
{
    if (!pyViewObject) {
        pyViewObject = new ViewProviderPy(this);
    }
    pyViewObject->IncRef();
    return pyViewObject;
}

ViewProviderBody* ViewProvider::getBodyViewProvider()
{

    auto body = PartDesign::Body::findBodyOf(getObject());
    auto doc = getDocument();
    if (body && doc) {
        auto vp = doc->getViewProvider(body);
        if (vp && vp->isDerivedFrom<ViewProviderBody>()) {
            return static_cast<ViewProviderBody*>(vp);
        }
    }

    return nullptr;
}

namespace Gui
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartDesignGui::ViewProviderPython, PartDesignGui::ViewProvider)
/// @endcond

// explicit template instantiation
template class PartDesignGuiExport ViewProviderFeaturePythonT<PartDesignGui::ViewProvider>;
}  // namespace Gui
