/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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
#include <Inventor/SoRenderManager.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/nodes/SoCamera.h>

#include <QMenu>
#include <QMessageBox>
#endif

#include <Base/Vector3D.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Utilities.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Text/App/ShapeText.h>

#include "TaskTextEditor.h"
#include "ViewProviderShapeText.h"
#include "Workbench.h"


FC_LOG_LEVEL_INIT("Text", true, true)

using namespace TextGui;
using namespace Text;
namespace bp = boost::placeholders;


PROPERTY_SOURCE_WITH_EXTENSIONS(TextGui::ViewProviderShapeText, PartGui::ViewProvider2DObject)


ViewProviderShapeText::ViewProviderShapeText()
    : SelectionObserver(false)
    , viewOrientationFactor(1)
{
    PartGui::ViewProviderAttachExtension::initExtension(this);

    ADD_PROPERTY_TYPE(
        TempoVis,
        (Py::None()),
        "Visibility automation",
        (App::PropertyType)(App::Prop_ReadOnly),
        "Object that handles hiding and showing other objects when entering/leaving text.");
    ADD_PROPERTY_TYPE(
        HideDependent,
        (true),
        "Visibility automation",
        (App::PropertyType)(App::Prop_ReadOnly),
        "If true, all objects that depend on the text are hidden when opening editing.");
    ADD_PROPERTY_TYPE(
        ShowLinks,
        (true),
        "Visibility automation",
        (App::PropertyType)(App::Prop_ReadOnly),
        "If true, all objects used in links to external geometry are shown when opening text.");
    ADD_PROPERTY_TYPE(
        ShowSupport,
        (true),
        "Visibility automation",
        (App::PropertyType)(App::Prop_ReadOnly),
        "If true, all objects this text is attached to are shown when opening text.");
    ADD_PROPERTY_TYPE(RestoreCamera,
                      (true),
                      "Visibility automation",
                      (App::PropertyType)(App::Prop_ReadOnly),
                      "If true, camera position before entering sketch is remembered, and restored "
                      "after closing it.");
    ADD_PROPERTY_TYPE(
        ForceOrtho,
        (false),
        "Visibility automation",
        (App::PropertyType)(App::Prop_ReadOnly),
        "If true, camera type will be forced to orthographic view when entering editing mode.");
    ADD_PROPERTY_TYPE(
        SectionView,
        (false),
        "Visibility automation",
        (App::PropertyType)(App::Prop_ReadOnly),
        "If true, only objects (or part of) located behind the text plane are visible.");
    ADD_PROPERTY_TYPE(EditingWorkbench,
                      ("TextWorkbench"),
                      "Visibility automation",
                      (App::PropertyType)(App::Prop_ReadOnly),
                      "Name of the workbench to activate when editing this Text.");

    sPixmap = "Text_ShapeText";

    cameraSensor.setFunction(&ViewProviderShapeText::camSensCB);
}

ViewProviderShapeText::~ViewProviderShapeText()
{}

Text::ShapeText* ViewProviderShapeText::getShapeText() const
{
    return dynamic_cast<Text::ShapeText*>(pcObject);
}

bool ViewProviderShapeText::isSelectable() const
{
    return !isEditing() && PartGui::ViewProvider2DObject::isSelectable();
}

void ViewProviderShapeText::onSelectionChanged(const Gui::SelectionChanges& msg)
{}

int ViewProviderShapeText::getViewOrientationFactor() const
{
    return viewOrientationFactor;
}

void ViewProviderShapeText::attach(App::DocumentObject* pcFeat)
{
    ViewProviderPart::attach(pcFeat);
}

void ViewProviderShapeText::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    menu->addAction(tr("Edit text"), receiver, member);
    // Call the extensions
    Gui::ViewProvider::setupContextMenu(menu, receiver, member);
}

bool ViewProviderShapeText::doubleClicked()
{
    Gui::Application::Instance->activeDocument()->setEdit(this);
    return true;
}

bool ViewProviderShapeText::setEdit(int ModNum)
{
    Q_UNUSED(ModNum)
    // When double-clicking on the item for this sketch the
    // object unsets and sets its edit mode without closing
    // the task panel
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    TaskTextEditor* editor = qobject_cast<TaskTextEditor*>(dlg);
    if (editor && editor->getShapeTextView() != this)
        editor = nullptr;// another sketch left open its task panel
    if (dlg && !editor) {
        QMessageBox msgBox;
        msgBox.setText(tr("A dialog is already open in the task panel"));
        msgBox.setInformativeText(tr("Do you want to close this dialog?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Yes)
            Gui::Control().closeDialog();
        else
            return false;
    }

    // clear the selection (convenience)
    Gui::Selection().clearSelection();
    Gui::Selection().rmvPreselect();

    this->attachSelection();

    auto editDoc = Gui::Application::Instance->editDocument();
    App::DocumentObject* editObj = getShapeText();
    std::string editSubName;
    ViewProviderDocumentObject* editVp = nullptr;
    if (editDoc) {
        editDoc->getInEdit(&editVp, &editSubName);
        if (editVp)
            editObj = editVp->getObject();
    }

    // visibility automation
    try {
        Gui::Command::addModule(Gui::Command::Gui, "Show");
        try {
            QString cmdstr =
                QString::fromLatin1(
                    "ActiveText = App.getDocument('%1').getObject('%2')\n"
                    "tv = Show.TempoVis(App.ActiveDocument, tag= ActiveText.ViewObject.TypeId)\n"
                    "ActiveText.ViewObject.TempoVis = tv\n"
                    "if ActiveText.ViewObject.EditingWorkbench:\n"
                    "  tv.activateWorkbench(ActiveText.ViewObject.EditingWorkbench)\n"
                    "if ActiveText.ViewObject.HideDependent:\n"
                    "  tv.hide(tv.get_all_dependent(%3, '%4'))\n"
                    "if ActiveText.ViewObject.ShowSupport:\n"
                    "  tv.show([ref[0] for ref in ActiveText.AttachmentSupport if not "
                    "ref[0].isDerivedFrom(\"PartDesign::Plane\")])\n"
                    "tv.sketchClipPlane(ActiveText, ActiveText.ViewObject.SectionView)\n"
                    "tv.hide(ActiveText)\n"
                    "del(tv)\n"
                    "del(ActiveText)\n")
                    .arg(QString::fromLatin1(getDocument()->getDocument()->getName()),
                         QString::fromLatin1(getShapeText()->getNameInDocument()),
                         QString::fromLatin1(Gui::Command::getObjectCmd(editObj).c_str()),
                         QString::fromLatin1(editSubName.c_str()));
            QByteArray cmdstr_bytearray = cmdstr.toLatin1();
            Gui::Command::runCommand(Gui::Command::Gui, cmdstr_bytearray);
        }
        catch (Base::PyException& e) {
            Base::Console().DeveloperError(
                "ViewProviderShapeText", "setEdit: visibility automation failed with an error: \n");
            e.ReportException();
        }
    }
    catch (Base::PyException&) {
        Base::Console().DeveloperWarning(
            "ViewProviderShapeText",
            "setEdit: could not import Show module. Visibility automation will not work.\n");
    }

    // start the edit dialog
    if (editor)
        Gui::Control().showDialog(editor);
    else
        Gui::Control().showDialog(new TaskTextEditor(this));

    connectUndoDocument = getDocument()->signalUndoDocument.connect(
        boost::bind(&ViewProviderShapeText::slotUndoDocument, this, bp::_1));
    connectRedoDocument = getDocument()->signalRedoDocument.connect(
        boost::bind(&ViewProviderShapeText::slotRedoDocument, this, bp::_1));

    Workbench::enterEditMode();

    return true;
}

void ViewProviderShapeText::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);

    Workbench::leaveEditMode();

    // if (isShapeTextInEditMode()) {
        this->detachSelection();

        App::AutoTransaction trans("Text recompute");
        try {
            // and update the sketch
            getShapeText()->getDocument()->recompute();
            Gui::Command::updateActive();
        }
        catch (...) {
        }
    // }

    // clear the selection and set the new/edited sketch(convenience)
    Gui::Selection().clearSelection();
    Gui::Selection().addSelection(editDocName.c_str(), editObjName.c_str(), editSubName.c_str());

    connectUndoDocument.disconnect();
    connectRedoDocument.disconnect();

    // when pressing ESC make sure to close the dialog
    Gui::Control().closeDialog();

    // visibility automation
    try {
        QString cmdstr =
            QString::fromLatin1("ActiveText = App.getDocument('%1').getObject('%2')\n"
                                "tv = ActiveText.ViewObject.TempoVis\n"
                                "if tv:\n"
                                "  tv.restore()\n"
                                "ActiveText.ViewObject.TempoVis = None\n"
                                "del(tv)\n"
                                "del(ActiveText)\n")
                .arg(QString::fromLatin1(getDocument()->getDocument()->getName()),
                     QString::fromLatin1(getShapeText()->getNameInDocument()));
        QByteArray cmdstr_bytearray = cmdstr.toLatin1();
        Gui::Command::runCommand(Gui::Command::Gui, cmdstr_bytearray);
    }
    catch (Base::PyException& e) {
        Base::Console().DeveloperError(
            "ViewProviderShapeText",
            "unsetEdit: visibility automation failed with an error: %s \n",
            e.what());
    }
}

void ViewProviderShapeText::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Q_UNUSED(ModNum);
    // visibility automation: save camera
    if (!this->TempoVis.getValue().isNone()) {
        try {
            QString cmdstr =
                QString::fromLatin1(
                    "ActiveText = App.getDocument('%1').getObject('%2')\n"
                    "if ActiveText.ViewObject.RestoreCamera:\n"
                    "  ActiveText.ViewObject.TempoVis.saveCamera()\n"
                    "  if ActiveText.ViewObject.ForceOrtho:\n"
                    "    "
                    "ActiveText.ViewObject.Document.ActiveView.setCameraType('Orthographic')\n")
                    .arg(QString::fromLatin1(getDocument()->getDocument()->getName()),
                         QString::fromLatin1(getShapeText()->getNameInDocument()));
            QByteArray cmdstr_bytearray = cmdstr.toLatin1();
            Gui::Command::runCommand(Gui::Command::Gui, cmdstr_bytearray);
        }
        catch (Base::PyException& e) {
            Base::Console().DeveloperError(
                "ViewProviderShapeText",
                "setEdit: visibility automation failed with an error: %s \n",
                e.what());
        }
    }

    auto editDoc = Gui::Application::Instance->editDocument();
    editDocName.clear();
    if (editDoc) {
        ViewProviderDocumentObject* parent = nullptr;
        editDoc->getInEdit(&parent, &editSubName);
        if (parent) {
            editDocName = editDoc->getDocument()->getName();
            editObjName = parent->getObject()->getNameInDocument();
        }
    }
    if (editDocName.empty()) {
        editDocName = getObject()->getDocument()->getName();
        editObjName = getObject()->getNameInDocument();
        editSubName.clear();
    }
    const char* dot = strrchr(editSubName.c_str(), '.');
    if (!dot)
        editSubName.clear();
    else
        editSubName.resize(dot - editSubName.c_str() + 1);

    Base::Placement plm = getEditingPlacement();
    Base::Rotation tmp(plm.getRotation());

    SbRotation rot((float)tmp[0], (float)tmp[1], (float)tmp[2], (float)tmp[3]);

    // Will the sketch be visible from the new position (#0000957)?
    //
    SoCamera* camera = viewer->getSoRenderManager()->getCamera();
    SbVec3f curdir;// current view direction
    camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), curdir);
    SbVec3f focal = camera->position.getValue() + camera->focalDistance.getValue() * curdir;

    SbVec3f newdir;// future view direction
    rot.multVec(SbVec3f(0, 0, -1), newdir);
    SbVec3f newpos = focal - camera->focalDistance.getValue() * newdir;

    SbVec3f plnpos = Base::convertTo<SbVec3f>(plm.getPosition());
    double dist = (plnpos - newpos).dot(newdir);
    if (dist < 0) {
        float focalLength = camera->focalDistance.getValue() - dist + 5;
        camera->position = focal - focalLength * curdir;
        camera->focalDistance.setValue(focalLength);
    }

    viewer->setCameraOrientation(rot);

    viewer->setEditing(true);
    viewer->setSelectionEnabled(false);

    viewer->setupEditingRoot();

    cameraSensor.setData(new VPRender {this, viewer->getSoRenderManager()});
    cameraSensor.attach(viewer->getSoRenderManager()->getSceneGraph());
}

void ViewProviderShapeText::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    auto dataPtr = static_cast<VPRender*>(cameraSensor.getData());
    delete dataPtr;
    cameraSensor.setData(nullptr);
    cameraSensor.detach();

    viewer->setEditing(false);
    viewer->setSelectionEnabled(true);
}

void ViewProviderShapeText::camSensCB(void* data, SoSensor*)
{
    VPRender* proxyVPrdr = static_cast<VPRender*>(data);
    if (!proxyVPrdr)
        return;

    auto vp = proxyVPrdr->vp;
    auto cam = proxyVPrdr->renderMgr->getCamera();

    vp->onCameraChanged(cam);
}

void ViewProviderShapeText::onCameraChanged(SoCamera* cam)
{
    auto rotSk = Base::Rotation(getDocument()->getEditingTransform());// sketch orientation
    auto rotc = cam->orientation.getValue().getValue();
    auto rotCam =
        Base::Rotation(rotc[0],
                       rotc[1],
                       rotc[2],
                       rotc[3]);// camera orientation (needed because float to double conversion)

    // Is camera in the same hemisphere as positive sketch normal ?
    auto orientation = (rotCam.invert() * rotSk).multVec(Base::Vector3d(0, 0, 1));
    auto tmpFactor = orientation.z < 0 ? -1 : 1;

    if (tmpFactor != viewOrientationFactor) {// redraw only if viewing side changed
        Base::Console().Log("Switching side, now %s, redrawing\n",
                            tmpFactor < 0 ? "back" : "front");
        viewOrientationFactor = tmpFactor;
        // draw();

        QString cmdStr = QStringLiteral("ActiveText.ViewObject.TempoVis.sketchClipPlane("
                                        "ActiveText, ActiveText.ViewObject.SectionView, %1)\n")
                             .arg(tmpFactor < 0 ? QLatin1String("True") : QLatin1String("False"));
        Base::Interpreter().runStringObject(cmdStr.toLatin1());
    }
}

void ViewProviderShapeText::slotUndoDocument(const Gui::Document& /*doc*/)
{
    // Note 1: this slot is only operative during edit mode (see signal connection/disconnection)
    // Note 2: ViewProviderSketch::UpdateData does not generate updates during undo/redo
    //         transactions as mid-transaction data may not be in a valid state (e.g. constraints
    //         may reference invalid geometry). However undo/redo notify SketchObject after the
    //         undo/redo and before this slot is called.
    // Note 3: Note that recomputes are no longer inhibited during the call to this slot.
    forceUpdateData();
}

void ViewProviderShapeText::slotRedoDocument(const Gui::Document& /*doc*/)
{
    // Note 1: this slot is only operative during edit mode (see signal connection/disconnection)
    // Note 2: ViewProviderSketch::UpdateData does not generate updates during undo/redo
    //         transactions as mid-transaction data may not be in a valid state (e.g. constraints
    //         may reference invalid geometry). However undo/redo notify SketchObject after the
    //         undo/redo and before this slot is called.
    // Note 3: Note that recomputes are no longer inhibited during the call to this slot.
    forceUpdateData();
}

void ViewProviderShapeText::forceUpdateData()
{
    // if (!getShapeText()->noRecomputes) {// the sketch was already solved in SketchObject in onUndoRedoFinished
        Gui::Command::updateActive();
    // }
}

Base::Placement ViewProviderShapeText::getEditingPlacement() const
{
    auto doc = Gui::Application::Instance->editDocument();
    if (!doc || doc->getInEdit() != this)
        return getShapeText()->globalPlacement();

    // TODO: won't work if there is scale. Hmm... what to do...
    return Base::Placement(doc->getEditingTransform());
}

namespace Gui
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TextGui::ViewProviderPython, TextGui::ViewProviderShapeText)
/// @endcond

// explicit template instantiation
template class TextGuiExport ViewProviderFeaturePythonT<TextGui::ViewProviderShapeText>;
}// namespace Gui
