/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <string>
# include <QAction>
# include <QMenu>
# include <Inventor/draggers/SoDragger.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoTransform.h>
#endif

#include <App/GeoFeature.h>
#include <Base/Placement.h>
#include "Gui/ViewParams.h"

#include "Application.h"
#include "BitmapFactory.h"
#include "Control.h"
#include "Document.h"
#include "SoFCCSysDragger.h"
#include "SoFCUnifiedSelection.h"
#include "TaskCSysDragger.h"
#include "View3DInventorViewer.h"
#include "ViewProviderDragger.h"


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderDragger, Gui::ViewProviderDocumentObject)

ViewProviderDragger::ViewProviderDragger() = default;

ViewProviderDragger::~ViewProviderDragger() = default;

void ViewProviderDragger::updateData(const App::Property* prop)
{
    if (prop->isDerivedFrom(App::PropertyPlacement::getClassTypeId()) &&
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
        updateTransform(p, pcTransform);
    }

    ViewProviderDocumentObject::updateData(prop);
}

bool ViewProviderDragger::doubleClicked()
{
    Gui::Application::Instance->activeDocument()->setEdit(this, (int)ViewProvider::Default);
    return true;
}

void ViewProviderDragger::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QIcon iconObject = mergeGreyableOverlayIcons(Gui::BitmapFactory().pixmap("Std_TransformManip.svg"));
    QAction* act = menu->addAction(iconObject, QObject::tr("Transform"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Transform));
    ViewProviderDocumentObject::setupContextMenu(menu, receiver, member);
}

ViewProvider *ViewProviderDragger::startEditing(int mode) {
    _linkDragger = nullptr;
    auto ret = ViewProviderDocumentObject::startEditing(mode);
    if(!ret)
        return ret;
    return _linkDragger?_linkDragger:ret;
}

bool ViewProviderDragger::checkLink() {
    // Trying to detect if the editing request is forwarded by a link object,
    // usually by doubleClicked(). If so, we route the request back. There shall
    // be no risk of infinite recursion, as ViewProviderLink handles
    // ViewProvider::Transform request by itself.
    ViewProviderDocumentObject *vpParent = nullptr;
    std::string subname;
    auto doc = Application::Instance->editDocument();
    if(!doc)
        return false;
    doc->getInEdit(&vpParent,&subname);
    if(!vpParent)
        return false;
    auto sobj = vpParent->getObject()->getSubObject(subname.c_str());
    if(!sobj || sobj==getObject() || sobj->getLinkedObject(true)!=getObject())
        return false;
    auto vp = Application::Instance->getViewProvider(sobj);
    if(!vp)
        return false;
    _linkDragger = vp->startEditing(ViewProvider::Transform);
    if(_linkDragger)
        return true;
    return false;
}

bool ViewProviderDragger::setEdit(int ModNum)
{
  Q_UNUSED(ModNum);

  if(checkLink())
      return true;

  App::DocumentObject *genericObject = this->getObject();
  if (genericObject->isDerivedFrom(App::GeoFeature::getClassTypeId()))
  {
    auto geoFeature = static_cast<App::GeoFeature *>(genericObject);
    const Base::Placement &placement = geoFeature->Placement.getValue();
    auto tempTransform = new SoTransform();
    tempTransform->ref();
    updateTransform(placement, tempTransform);

    assert(!csysDragger);
    csysDragger = new SoFCCSysDragger();
    csysDragger->setAxisColors(
      Gui::ViewParams::instance()->getAxisXColor(),
      Gui::ViewParams::instance()->getAxisYColor(),
      Gui::ViewParams::instance()->getAxisZColor()
    );
    csysDragger->draggerSize.setValue(0.05f);
    csysDragger->translation.setValue(tempTransform->translation.getValue());
    csysDragger->rotation.setValue(tempTransform->rotation.getValue());

    tempTransform->unref();

    pcTransform->translation.connectFrom(&csysDragger->translation);
    pcTransform->rotation.connectFrom(&csysDragger->rotation);

    csysDragger->addFinishCallback(dragFinishCallback, this);

    // dragger node is added to viewer's editing root in setEditViewer
    // pcRoot->insertChild(csysDragger, 0);
    csysDragger->ref();

    auto task = new TaskCSysDragger(this, csysDragger);
    Gui::Control().showDialog(task);
  }

  return true;
}

void ViewProviderDragger::unsetEdit(int ModNum)
{
  Q_UNUSED(ModNum);

  if(csysDragger)
  {
    pcTransform->translation.disconnect(&csysDragger->translation);
    pcTransform->rotation.disconnect(&csysDragger->rotation);

    // dragger node is added to viewer's editing root in setEditViewer
    // pcRoot->removeChild(csysDragger); //should delete csysDragger
    csysDragger->unref();
    csysDragger = nullptr;
  }
  Gui::Control().closeDialog();
}

void ViewProviderDragger::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Q_UNUSED(ModNum);

    if (csysDragger && viewer)
    {
      auto rootPickStyle = new SoPickStyle();
      rootPickStyle->style = SoPickStyle::UNPICKABLE;
      auto selection = static_cast<SoGroup*>(viewer->getSceneGraph());
      selection->insertChild(rootPickStyle, 0);
      viewer->setSelectionEnabled(false);
      csysDragger->setUpAutoScale(viewer->getSoRenderManager()->getCamera());

      auto mat = viewer->getDocument()->getEditingTransform();
      viewer->getDocument()->setEditingTransform(mat);
      auto feat = dynamic_cast<App::GeoFeature *>(getObject());
      if(feat) {
          auto matInverse = feat->Placement.getValue().toMatrix();
          matInverse.inverse();
          mat *= matInverse;
      }
      viewer->setupEditingRoot(csysDragger,&mat);
    }
}

void ViewProviderDragger::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    auto selection = static_cast<SoGroup*>(viewer->getSceneGraph());
    SoNode *child = selection->getChild(0);
    if (child && child->isOfType(SoPickStyle::getClassTypeId())) {
        selection->removeChild(child);
        viewer->setSelectionEnabled(true);
    }
}

void ViewProviderDragger::dragFinishCallback(void *data, SoDragger *d)
{
    // This is called when a manipulator has done manipulating

    auto sudoThis = static_cast<ViewProviderDragger *>(data);
    auto dragger = static_cast<SoFCCSysDragger *>(d);
    updatePlacementFromDragger(sudoThis, dragger);

    //Gui::Application::Instance->activeDocument()->commitCommand();
}

void ViewProviderDragger::updatePlacementFromDragger(ViewProviderDragger* sudoThis, SoFCCSysDragger* draggerIn)
{
  App::DocumentObject *genericObject = sudoThis->getObject();
  if (!genericObject->isDerivedFrom(App::GeoFeature::getClassTypeId()))
    return;
  auto geoFeature = static_cast<App::GeoFeature *>(genericObject);
  Base::Placement originalPlacement = geoFeature->Placement.getValue();
  double pMatrix[16];
  originalPlacement.toMatrix().getMatrix(pMatrix);
  Base::Placement freshPlacement = originalPlacement;

  //local cache for brevity.
  double translationIncrement = draggerIn->translationIncrement.getValue();
  double rotationIncrement = draggerIn->rotationIncrement.getValue();
  int tCountX = draggerIn->translationIncrementCountX.getValue();
  int tCountY = draggerIn->translationIncrementCountY.getValue();
  int tCountZ = draggerIn->translationIncrementCountZ.getValue();
  int rCountX = draggerIn->rotationIncrementCountX.getValue();
  int rCountY = draggerIn->rotationIncrementCountY.getValue();
  int rCountZ = draggerIn->rotationIncrementCountZ.getValue();

  //just as a little sanity check make sure only 1 or 2 fields has changed.
  int numberOfFieldChanged = 0;
  if (tCountX) numberOfFieldChanged++;
  if (tCountY) numberOfFieldChanged++;
  if (tCountZ) numberOfFieldChanged++;
  if (rCountX) numberOfFieldChanged++;
  if (rCountY) numberOfFieldChanged++;
  if (rCountZ) numberOfFieldChanged++;
  if (numberOfFieldChanged == 0)
    return;
  assert(numberOfFieldChanged == 1 || numberOfFieldChanged == 2);

  //helper lambdas.
  auto getVectorX = [&pMatrix]() {return Base::Vector3d(pMatrix[0], pMatrix[4], pMatrix[8]);};
  auto getVectorY = [&pMatrix]() {return Base::Vector3d(pMatrix[1], pMatrix[5], pMatrix[9]);};
  auto getVectorZ = [&pMatrix]() {return Base::Vector3d(pMatrix[2], pMatrix[6], pMatrix[10]);};

  if (tCountX)
  {
    Base::Vector3d movementVector(getVectorX());
    movementVector *= (tCountX * translationIncrement);
    freshPlacement.move(movementVector);
    geoFeature->Placement.setValue(freshPlacement);
  }
  if (tCountY)
  {
    Base::Vector3d movementVector(getVectorY());
    movementVector *= (tCountY * translationIncrement);
    freshPlacement.move(movementVector);
    geoFeature->Placement.setValue(freshPlacement);
  }
  if (tCountZ)
  {
    Base::Vector3d movementVector(getVectorZ());
    movementVector *= (tCountZ * translationIncrement);
    freshPlacement.move(movementVector);
    geoFeature->Placement.setValue(freshPlacement);
  }
  if (rCountX)
  {
    Base::Vector3d rotationVector(getVectorX());
    Base::Rotation rotation(rotationVector, rCountX * rotationIncrement);
    freshPlacement.setRotation(rotation * freshPlacement.getRotation());
    geoFeature->Placement.setValue(freshPlacement);
  }
  if (rCountY)
  {
    Base::Vector3d rotationVector(getVectorY());
    Base::Rotation rotation(rotationVector, rCountY * rotationIncrement);
    freshPlacement.setRotation(rotation * freshPlacement.getRotation());
    geoFeature->Placement.setValue(freshPlacement);
  }
  if (rCountZ)
  {
    Base::Vector3d rotationVector(getVectorZ());
    Base::Rotation rotation(rotationVector, rCountZ * rotationIncrement);
    freshPlacement.setRotation(rotation * freshPlacement.getRotation());
    geoFeature->Placement.setValue(freshPlacement);
  }

  draggerIn->clearIncrementCounts();
}

void ViewProviderDragger::updateTransform(const Base::Placement& from, SoTransform* to)
{
    auto q0 = (float)from.getRotation().getValue()[0];
    auto q1 = (float)from.getRotation().getValue()[1];
    auto q2 = (float)from.getRotation().getValue()[2];
    auto q3 = (float)from.getRotation().getValue()[3];
    auto px = (float)from.getPosition().x;
    auto py = (float)from.getPosition().y;
    auto pz = (float)from.getPosition().z;
  to->rotation.setValue(q0,q1,q2,q3);
  to->translation.setValue(px,py,pz);
  to->center.setValue(0.0f,0.0f,0.0f);
  to->scaleFactor.setValue(1.0f,1.0f,1.0f);
}
