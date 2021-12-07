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
#include <App/ComplexGeoDataPy.h>
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
    act->setToolTip(QObject::tr("Transform at the origin of the placement"));
    act->setData(QVariant((int)ViewProvider::Transform));
    act = menu->addAction(QObject::tr("Transform at"), receiver, member);
    act->setToolTip(QObject::tr("Transform at the center of the shape"));
    act->setData(QVariant((int)ViewProvider::TransformAt));
    ViewProviderDocumentObject::setupContextMenu(menu, receiver, member);
}

ViewProvider *ViewProviderDragger::startEditing(int mode) {
    _linkDragger = 0;
    auto ret = ViewProviderDocumentObject::startEditing(mode);
    if(!ret)
        return ret;
    return _linkDragger?_linkDragger:ret;
}

bool ViewProviderDragger::checkLink(int mode) {
    // Trying to detect if the editing request is forwarded by a link object,
    // usually by doubleClicked(). If so, we route the request back. There shall
    // be no risk of infinite recursion, as ViewProviderLink handles
    // ViewProvider::Transform request by itself.
    ViewProviderDocumentObject *vpParent = 0;
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
    _linkDragger = vp->startEditing(mode);
    if(_linkDragger)
        return true;
    return false;
}

Base::Matrix4D ViewProviderDragger::getDragOffset(const ViewProviderDocumentObject *vp)
{
    Base::Matrix4D res;
    if (!vp || !vp->getObject())
        return res;
    auto selctx = Gui::Selection().getExtendedContext(vp->getObject());
    auto objs = selctx.getSubObjectList();
    auto it = std::find(objs.begin(), objs.end(), vp->getObject());
    if (it != objs.end()) {
        objs.erase(objs.begin(), it+1);
        std::string element = selctx.getElementName();
        selctx = App::SubObjectT(objs);
        selctx.setSubName(selctx.getSubName() + element);
    }

    Base::Rotation rot;
    Base::BoundBox3d bbox;

    PyObject *pyobj = nullptr;
    vp->getObject()->getSubObject(selctx.getSubName().c_str(), &pyobj, nullptr, false);
    if (pyobj) {
        Base::PyGILStateLocker lock;
        Py::Object pyObj(pyobj, true);
        try {
            if (PyObject_TypeCheck(pyobj, &Data::ComplexGeoDataPy::Type)) {
                auto geodata = static_cast<Data::ComplexGeoDataPy*>(pyobj)->getComplexGeoDataPtr();
                geodata->getRotation(rot);
                bbox = geodata->getBoundBox();
            }
        } catch (Base::Exception &e) {
            e.ReportException();
        }
    }

    if (!bbox.IsValid())
        bbox = vp->getBoundingBox(selctx.getSubName().c_str(),0,false);
    if (bbox.IsValid()) 
        res = Base::Placement(bbox.GetCenter(), rot).toMatrix();
    return res;
}

Base::Matrix4D ViewProviderDragger::getDragOffset()
{
    return getDragOffset(this);
}

bool ViewProviderDragger::setEdit(int ModNum)
{
  if (ModNum != ViewProvider::Transform 
          && ModNum != ViewProvider::TransformAt
          && ModNum != ViewProvider::Default)
      return ViewProviderDocumentObject::setEdit(ModNum);

  if(checkLink(ModNum))
      return true;

  App::DocumentObject *genericObject = this->getObject();
  if (genericObject->isDerivedFrom(App::GeoFeature::getClassTypeId()))
  {
    App::GeoFeature *geoFeature = static_cast<App::GeoFeature *>(genericObject);

    if (ModNum == TransformAt) {
        this->dragOffset = getDragOffset();
    } else
        this->dragOffset = Base::Matrix4D();
    
    Base::Placement placement =
        geoFeature->Placement.getValue().toMatrix() * this->dragOffset;
    this->dragOffset.inverse();
    SoTransform *tempTransform = new SoTransform();
    tempTransform->ref();
    updateTransform(placement, tempTransform);

    assert(!csysDragger);
    csysDragger = new SoFCCSysDragger();
    csysDragger->draggerSize.setValue(0.05f);
    csysDragger->translation.setValue(tempTransform->translation.getValue());
    csysDragger->rotation.setValue(tempTransform->rotation.getValue());

    tempTransform->unref();

    csysDragger->addStartCallback(dragStartCallback, this);
    csysDragger->addFinishCallback(dragFinishCallback, this);
    csysDragger->addMotionCallback(dragMotionCallback, this);

    // dragger node is added to viewer's editing root in setEditViewer
    // pcRoot->insertChild(csysDragger, 0);
    csysDragger->ref();

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
      SoPickStyle *rootPickStyle = new SoPickStyle();
      rootPickStyle->style = SoPickStyle::UNPICKABLE;
      SoFCUnifiedSelection* selection = static_cast<SoFCUnifiedSelection*>(viewer->getSceneGraph());
      selection->insertChild(rootPickStyle, 0);
      selection->selectionRole.setValue(false);
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
  SoFCUnifiedSelection* selection = static_cast<SoFCUnifiedSelection*>(viewer->getSceneGraph());
  SoNode *child = selection->getChild(0);
  if (child && child->isOfType(SoPickStyle::getClassTypeId())) {
    selection->removeChild(child);
    selection->selectionRole.setValue(true);
  }
}

void ViewProviderDragger::dragStartCallback(void *data, SoDragger *d)
{
    // This is called when a manipulator is about to manipulating
    reinterpret_cast<ViewProviderDragger*>(data)->onDragStart(d);
}

void ViewProviderDragger::dragFinishCallback(void *data, SoDragger *d)
{
    // This is called when a manipulator has done manipulating
    reinterpret_cast<ViewProviderDragger *>(data)->onDragFinish(d);
}

void ViewProviderDragger::dragMotionCallback(void *data, SoDragger *d)
{
    reinterpret_cast<ViewProviderDragger *>(data)->onDragMotion(d);
}

void ViewProviderDragger::onDragStart(SoDragger *)
{
    Gui::Application::Instance->activeDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Transform"));
}

void ViewProviderDragger::onDragFinish(SoDragger *d)
{
    SoFCCSysDragger *dragger = static_cast<SoFCCSysDragger *>(d);
    updatePlacementFromDragger(dragger);

    Gui::Application::Instance->activeDocument()->commitCommand();
}

void ViewProviderDragger::onDragMotion(SoDragger *d)
{
    SoFCCSysDragger *dragger = static_cast<SoFCCSysDragger *>(d);
    SbVec3f v;
    SbRotation r;
    v = dragger->translation.getValue();
    r = dragger->rotation.getValue();
    float q1,q2,q3,q4;
    r.getValue(q1,q2,q3,q4);
    Base::Placement pla(Base::Vector3d(v[0],v[1],v[2]),Base::Rotation(q1,q2,q3,q4));
    updateTransform(pla * this->dragOffset, this->pcTransform);
}

void ViewProviderDragger::updatePlacementFromDragger(SoFCCSysDragger* draggerIn)
{
  App::DocumentObject *genericObject = this->getObject();
  if (!genericObject->isDerivedFrom(App::GeoFeature::getClassTypeId()))
    return;
  App::GeoFeature *geoFeature = static_cast<App::GeoFeature *>(genericObject);
  SbVec3f v;
  SbRotation r;
  v = draggerIn->translation.getValue();
  r = draggerIn->rotation.getValue();
  float q1,q2,q3,q4;
  r.getValue(q1,q2,q3,q4);
  Base::Placement pla(Base::Vector3d(v[0],v[1],v[2]),Base::Rotation(q1,q2,q3,q4));
  geoFeature->Placement.setValue(pla * this->dragOffset);
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
