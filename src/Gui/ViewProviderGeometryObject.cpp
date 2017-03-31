/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include "ViewProviderGeometryObject.h"
#include "View3DInventorViewer.h"
#include "SoFCSelection.h"
#include "SoFCBoundingBox.h"
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

PROPERTY_SOURCE(Gui::ViewProviderGeometryObject, Gui::ViewProviderDocumentObject)

const App::PropertyIntegerConstraint::Constraints intPercent = {0,100,1};

ViewProviderGeometryObject::ViewProviderGeometryObject() : pcBoundSwitch(0),pcBoundColor(0)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    unsigned long shcol = hGrp->GetUnsigned("DefaultShapeColor",3435973887UL); // light gray (204,204,204)
    float r,g,b;
    r = ((shcol >> 24) & 0xff) / 255.0; g = ((shcol >> 16) & 0xff) / 255.0; b = ((shcol >> 8) & 0xff) / 255.0;
    ADD_PROPERTY(ShapeColor,(r, g, b));
    ADD_PROPERTY(Transparency,(0));
    Transparency.setConstraints(&intPercent);
    App::Material mat(App::Material::DEFAULT);
    ADD_PROPERTY(ShapeMaterial,(mat));
    ADD_PROPERTY(BoundingBox,(false));
    ADD_PROPERTY(Selectable,(true));

    ADD_PROPERTY(SelectionStyle,((long)0));
    static const char *SelectionStyleEnum[] = {"Shape","BoundBox",0};
    SelectionStyle.setEnums(SelectionStyleEnum);

    bool enableSel = hGrp->GetBool("EnableSelection", true);
    Selectable.setValue(enableSel);

    pcShapeMaterial = new SoMaterial;
    pcShapeMaterial->ref();
    //ShapeMaterial.touch(); materials are rarely used, so better to initialize with default shape color
    ShapeColor.touch();

    pcBoundingBox = new Gui::SoFCBoundingBox;
    pcBoundingBox->ref();
    sPixmap = "Feature";
}

ViewProviderGeometryObject::~ViewProviderGeometryObject()
{
    pcShapeMaterial->unref();
    pcBoundingBox->unref();
}

void ViewProviderGeometryObject::onChanged(const App::Property* prop)
{
    // Actually, the properties 'ShapeColor' and 'Transparency' are part of the property 'ShapeMaterial'.
    // Both redundant properties are kept due to more convenience for the user. But we must keep the values
    // consistent of all these properties.
    if (prop == &Selectable) {
        bool Sel = Selectable.getValue();
        setSelectable(Sel);
    }
    else if (prop == &ShapeColor) {
        const App::Color& c = ShapeColor.getValue();
        pcShapeMaterial->diffuseColor.setValue(c.r,c.g,c.b);
        if (c != ShapeMaterial.getValue().diffuseColor)
        ShapeMaterial.setDiffuseColor(c);
    }
    else if (prop == &Transparency) {
        const App::Material& Mat = ShapeMaterial.getValue();
        long value = (long)(100*Mat.transparency);
        if (value != Transparency.getValue()) {
            float trans = Transparency.getValue()/100.0f;
            pcShapeMaterial->transparency = trans;
            ShapeMaterial.setTransparency(trans);
        }
    }
    else if (prop == &ShapeMaterial) {
        const App::Material& Mat = ShapeMaterial.getValue();
        long value = (long)(100*Mat.transparency);
        if (value != Transparency.getValue())
        Transparency.setValue(value);
        const App::Color& color = Mat.diffuseColor;
        if (color != ShapeColor.getValue())
        ShapeColor.setValue(Mat.diffuseColor);
        pcShapeMaterial->ambientColor.setValue(Mat.ambientColor.r,Mat.ambientColor.g,Mat.ambientColor.b);
        pcShapeMaterial->diffuseColor.setValue(Mat.diffuseColor.r,Mat.diffuseColor.g,Mat.diffuseColor.b);
        pcShapeMaterial->specularColor.setValue(Mat.specularColor.r,Mat.specularColor.g,Mat.specularColor.b);
        pcShapeMaterial->emissiveColor.setValue(Mat.emissiveColor.r,Mat.emissiveColor.g,Mat.emissiveColor.b);
        pcShapeMaterial->shininess.setValue(Mat.shininess);
        pcShapeMaterial->transparency.setValue(Mat.transparency);
    }
    else if (prop == &BoundingBox || prop == &SelectionStyle) {
        applyBoundColor();
        if(SelectionStyle.getValue()==0 || !Selectable.getValue())
            showBoundingBox( BoundingBox.getValue() );
    }

    ViewProviderDocumentObject::onChanged(prop);
}

void ViewProviderGeometryObject::attach(App::DocumentObject *pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);
}

void ViewProviderGeometryObject::updateData(const App::Property* prop)
{
    if (prop->isDerivedFrom(App::PropertyComplexGeoData::getClassTypeId())) {
        // Note: When the placement of non-parametric objects changes there is currently no update
        // of the bounding box information.
        Base::BoundBox3d box = static_cast<const App::PropertyComplexGeoData*>(prop)->getBoundingBox();
        pcBoundingBox->minBounds.setValue(box.MinX, box.MinY, box.MinZ);
        pcBoundingBox->maxBounds.setValue(box.MaxX, box.MaxY, box.MaxZ);
    }
    else if (prop->isDerivedFrom(App::PropertyPlacement::getClassTypeId()) &&
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
    else {
        ViewProviderDocumentObject::updateData(prop);
    }
}

bool ViewProviderGeometryObject::doubleClicked(void)
{
    Gui::Application::Instance->activeDocument()->setEdit(this, (int)ViewProvider::Default);
    return true;
}

void ViewProviderGeometryObject::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act = menu->addAction(QObject::tr("Transform"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Transform));
}

bool ViewProviderGeometryObject::setEdit(int ModNum)
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

void ViewProviderGeometryObject::unsetEdit(int ModNum)
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

void ViewProviderGeometryObject::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Q_UNUSED(ModNum);

    if (csysDragger && viewer)
    {
      SoPickStyle *rootPickStyle = new SoPickStyle();
      rootPickStyle->style = SoPickStyle::UNPICKABLE;
      static_cast<SoFCUnifiedSelection*>(viewer->getSceneGraph())->insertChild(rootPickStyle, 0);
      csysDragger->setUpAutoScale(viewer->getSoRenderManager()->getCamera());
    }
}

void ViewProviderGeometryObject::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
  SoNode *child = static_cast<SoFCUnifiedSelection*>(viewer->getSceneGraph())->getChild(0);
  if (child && child->isOfType(SoPickStyle::getClassTypeId()))
    static_cast<SoFCUnifiedSelection*>(viewer->getSceneGraph())->removeChild(child);
}

void ViewProviderGeometryObject::dragStartCallback(void *, SoDragger *)
{
    // This is called when a manipulator is about to manipulating
    Gui::Application::Instance->activeDocument()->openCommand("Transform");
}

void ViewProviderGeometryObject::dragFinishCallback(void *data, SoDragger *d)
{
    // This is called when a manipulator has done manipulating
    
    ViewProviderGeometryObject* sudoThis = reinterpret_cast<ViewProviderGeometryObject *>(data);
    SoFCCSysDragger *dragger = static_cast<SoFCCSysDragger *>(d);
    updatePlacementFromDragger(sudoThis, dragger);
    
    Gui::Application::Instance->activeDocument()->commitCommand();
}

void ViewProviderGeometryObject::updatePlacementFromDragger(ViewProviderGeometryObject* sudoThis, SoFCCSysDragger* draggerIn)
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


SoPickedPointList ViewProviderGeometryObject::getPickedPoints(const SbVec2s& pos, const View3DInventorViewer& viewer,bool pickAll) const
{
    SoSeparator* root = new SoSeparator;
    root->ref();
    root->addChild(viewer.getHeadlight());
    root->addChild(viewer.getSoRenderManager()->getCamera());
    root->addChild(const_cast<ViewProviderGeometryObject*>(this)->getRoot());

    SoRayPickAction rp(viewer.getSoRenderManager()->getViewportRegion());
    rp.setPickAll(pickAll);
    rp.setRadius(viewer.getPickRadius());
    rp.setPoint(pos);
    rp.apply(root);
    root->unref();

    // returns a copy of the list
    return rp.getPickedPointList();
}

SoPickedPoint* ViewProviderGeometryObject::getPickedPoint(const SbVec2s& pos, const View3DInventorViewer& viewer) const
{
    SoSeparator* root = new SoSeparator;
    root->ref();
    root->addChild(viewer.getHeadlight());
    root->addChild(viewer.getSoRenderManager()->getCamera());
    root->addChild(const_cast<ViewProviderGeometryObject*>(this)->getRoot());

    SoRayPickAction rp(viewer.getSoRenderManager()->getViewportRegion());
    rp.setPoint(pos);
    rp.setRadius(viewer.getPickRadius());
    rp.apply(root);
    root->unref();

    // returns a copy of the point
    SoPickedPoint* pick = rp.getPickedPoint();
    //return (pick ? pick->copy() : 0); // needs the same instance of CRT under MS Windows
    return (pick ? new SoPickedPoint(*pick) : 0);
}

unsigned long ViewProviderGeometryObject::getBoundColor() const {
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    if(SelectionStyle.getValue() == 0 || !Selectable.getValue() || !hGrp->GetBool("EnableSelection", true))
        return hGrp->GetUnsigned("BoundingBoxColor",4294967295UL); // white (255,255,255)
    else
        return hGrp->GetUnsigned("SelectionColor",0x00CD00UL); // rgb(0,205,0)
}

void ViewProviderGeometryObject::applyBoundColor() {
    if(!pcBoundColor) return;

    unsigned long bbcol = getBoundColor();
    float r,g,b;
    r = ((bbcol >> 24) & 0xff) / 255.0; g = ((bbcol >> 16) & 0xff) / 255.0; b = ((bbcol >> 8) & 0xff) / 255.0;
    pcBoundColor->rgb.setValue(r, g, b);
}

void ViewProviderGeometryObject::showBoundingBox(bool show)
{
    if (!pcBoundSwitch && show) {
        pcBoundSwitch = new SoSwitch();
        SoSeparator* pBoundingSep = new SoSeparator();
        SoDrawStyle* lineStyle = new SoDrawStyle;
        lineStyle->lineWidth = 2.0f;
        pBoundingSep->addChild(lineStyle);

        pcBoundColor = new SoBaseColor();
        pBoundingSep->addChild(pcBoundColor);
        applyBoundColor();

        pBoundingSep->addChild(new SoResetTransform());
        pBoundingSep->addChild(pcBoundingBox);
        pcBoundingBox->coordsOn.setValue(false);
        pcBoundingBox->dimensionsOn.setValue(true);

        // add to the highlight node
        pcBoundSwitch->addChild(pBoundingSep);
        pcRoot->insertChild(pcBoundSwitch,pcRoot->findChild(pcModeSwitch));
    }

    if (pcBoundSwitch) {
        pcBoundSwitch->whichChild = (show ? 0 : -1);
    }
}

void ViewProviderGeometryObject::setSelectable(bool selectable)
{
    if(SelectionStyle.getValue()) {
        applyBoundColor();
        if(!selectable) 
            showBoundingBox(false);
    }

    SoSearchAction sa;
    sa.setInterest(SoSearchAction::ALL);
    sa.setSearchingAll(TRUE);
    sa.setType(Gui::SoFCSelection::getClassTypeId());
    sa.apply(pcRoot);

    SoPathList & pathList = sa.getPaths();

    for (int i=0;i<pathList.getLength();i++) {
        SoFCSelection *selNode = dynamic_cast<SoFCSelection*>(pathList[i]->getTail());
        if (selectable) {
            selNode->selectionMode = SoFCSelection::SEL_ON;
            selNode->highlightMode = SoFCSelection::AUTO;
        }
        else {
            selNode->selectionMode = SoFCSelection::SEL_OFF;
            selNode->highlightMode = SoFCSelection::OFF;
            selNode->selected = SoFCSelection::NOTSELECTED;
        }
    }
}

void ViewProviderGeometryObject::updateTransform(const Base::Placement& from, SoTransform* to)
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
