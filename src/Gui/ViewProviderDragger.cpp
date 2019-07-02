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
# include <cfloat>
# include <QAction>
# include <QMenu>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/draggers/SoDragger.h>
# include <Inventor/draggers/SoCenterballDragger.h>
# include <Inventor/manips/SoCenterballManip.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoDirectionalLight.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/sensors/SoNodeSensor.h> 
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoRayPickAction.h> 
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "ViewProviderDragger.h"
#include "View3DInventorViewer.h"
#include "Application.h"
#include "Document.h"
#include "Window.h"

#include <Base/Console.h>
#include <Base/Placement.h>
#include <App/PropertyGeo.h>
#include <App/GeoFeature.h>
#include <Inventor/draggers/SoCenterballDragger.h>
#include <Inventor/nodes/SoResetTransform.h>
#if (COIN_MAJOR_VERSION > 2)
#include <Inventor/nodes/SoDepthBuffer.h>
#endif
#include "SoFCUnifiedSelection.h"
#include "SoFCCSysDragger.h"
#include "Control.h"
#include "TaskCSysDragger.h"
#include <boost/math/special_functions/fpclassify.hpp>

using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderDragger, Gui::ViewProviderDocumentObject)

ViewProviderDragger::ViewProviderDragger()
{
}

ViewProviderDragger::~ViewProviderDragger()
{
}

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

bool ViewProviderDragger::doubleClicked(void)
{
    Gui::Application::Instance->activeDocument()->setEdit(this, (int)ViewProvider::Default);
    return true;
}

void ViewProviderDragger::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act = menu->addAction(QObject::tr("Transform"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Transform));
}

bool ViewProviderDragger::setEdit(int ModNum)
{
  Q_UNUSED(ModNum);

  App::DocumentObject *genericObject = this->getObject();
  if (genericObject->isDerivedFrom(App::GeoFeature::getClassTypeId()))
  {
    App::GeoFeature *geoFeature = static_cast<App::GeoFeature *>(genericObject);
    const Base::Placement &placement = geoFeature->Placement.getValue();
    SoTransform *tempTransform = new SoTransform();
    tempTransform->ref();
    updateTransform(placement, tempTransform);
    
    assert(!csysDragger);
    csysDragger = new SoFCCSysDragger();
    csysDragger->draggerSize.setValue(0.05f);
    csysDragger->translation.setValue(tempTransform->translation.getValue());
    csysDragger->rotation.setValue(tempTransform->rotation.getValue());
    
    tempTransform->unref();
    
    pcTransform->translation.connectFrom(&csysDragger->translation);
    pcTransform->rotation.connectFrom(&csysDragger->rotation);
    
    csysDragger->addStartCallback(dragStartCallback, this);
    csysDragger->addFinishCallback(dragFinishCallback, this);
    
    pcRoot->insertChild(csysDragger, 0);
    
    TaskCSysDragger *task = new TaskCSysDragger(this, csysDragger);
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
    
    pcRoot->removeChild(csysDragger); //should delete csysDragger
    csysDragger = nullptr;
  }
  Gui::Control().closeDialog();
}

void ViewProviderDragger::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Q_UNUSED(ModNum);

    if (csysDragger && viewer)
    {
      SoPickStyle *rootPickStyle = new SoPickStyle();
      rootPickStyle->style = SoPickStyle::UNPICKABLE;
      SoFCUnifiedSelection* selection = static_cast<SoFCUnifiedSelection*>(viewer->getSceneGraph());
      selection->insertChild(rootPickStyle, 0);
      selection->selectionRole.setValue(false);
      csysDragger->setUpAutoScale(viewer->getSoRenderManager()->getCamera());
    }
}

void ViewProviderDragger::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
  SoFCUnifiedSelection* selection = static_cast<SoFCUnifiedSelection*>(viewer->getSceneGraph());
  SoNode *child = selection->getChild(0);
  if (child && child->isOfType(SoPickStyle::getClassTypeId())) {
    selection->removeChild(child);
    selection->selectionRole.setValue(true);
  }
}

void ViewProviderDragger::dragStartCallback(void *, SoDragger *)
{
    // This is called when a manipulator is about to manipulating
    Gui::Application::Instance->activeDocument()->openCommand("Transform");
}

void ViewProviderDragger::dragFinishCallback(void *data, SoDragger *d)
{
    // This is called when a manipulator has done manipulating
    
    ViewProviderDragger* sudoThis = reinterpret_cast<ViewProviderDragger *>(data);
    SoFCCSysDragger *dragger = static_cast<SoFCCSysDragger *>(d);
    updatePlacementFromDragger(sudoThis, dragger);
    
    Gui::Application::Instance->activeDocument()->commitCommand();
}

void ViewProviderDragger::updatePlacementFromDragger(ViewProviderDragger* sudoThis, SoFCCSysDragger* draggerIn)
{
  App::DocumentObject *genericObject = sudoThis->getObject();
  if (!genericObject->isDerivedFrom(App::GeoFeature::getClassTypeId()))
    return;
  App::GeoFeature *geoFeature = static_cast<App::GeoFeature *>(genericObject);
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
  
  //just as a little sanity check make sure only 1 field has changed.
  int numberOfFieldChanged = 0;
  if (tCountX) numberOfFieldChanged++;
  if (tCountY) numberOfFieldChanged++;
  if (tCountZ) numberOfFieldChanged++;
  if (rCountX) numberOfFieldChanged++;
  if (rCountY) numberOfFieldChanged++;
  if (rCountZ) numberOfFieldChanged++;
  if (numberOfFieldChanged == 0)
    return;
  assert(numberOfFieldChanged == 1);
  
  //helper lamdas.
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
  else if (tCountY)
  {
    Base::Vector3d movementVector(getVectorY());
    movementVector *= (tCountY * translationIncrement);
    freshPlacement.move(movementVector);
    geoFeature->Placement.setValue(freshPlacement);
  }
  else if (tCountZ)
  {
    Base::Vector3d movementVector(getVectorZ());
    movementVector *= (tCountZ * translationIncrement);
    freshPlacement.move(movementVector);
    geoFeature->Placement.setValue(freshPlacement);
  }
  else if (rCountX)
  {
    Base::Vector3d rotationVector(getVectorX());
    Base::Rotation rotation(rotationVector, rCountX * rotationIncrement);
    freshPlacement.setRotation(rotation * freshPlacement.getRotation());
    geoFeature->Placement.setValue(freshPlacement);
  }
  else if (rCountY)
  {
    Base::Vector3d rotationVector(getVectorY());
    Base::Rotation rotation(rotationVector, rCountY * rotationIncrement);
    freshPlacement.setRotation(rotation * freshPlacement.getRotation());
    geoFeature->Placement.setValue(freshPlacement);
  }
  else if (rCountZ)
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
  float q0 = (float)from.getRotation().getValue()[0];
  float q1 = (float)from.getRotation().getValue()[1];
  float q2 = (float)from.getRotation().getValue()[2];
  float q3 = (float)from.getRotation().getValue()[3];
  float px = (float)from.getPosition().x;
  float py = (float)from.getPosition().y;
  float pz = (float)from.getPosition().z;
  to->rotation.setValue(q0,q1,q2,q3);
  to->translation.setValue(px,py,pz);
  to->center.setValue(0.0f,0.0f,0.0f);
  to->scaleFactor.setValue(1.0f,1.0f,1.0f);
}
