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

#include <Base/Placement.h>
#include <App/PropertyGeo.h>
#include <App/GeoFeature.h>
#include <Inventor/draggers/SoCenterballDragger.h>
#if (COIN_MAJOR_VERSION > 2)
#include <Inventor/nodes/SoDepthBuffer.h>
#endif
#include "SoNavigationDragger.h"
#include "SoFCUnifiedSelection.h"

using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderGeometryObject, Gui::ViewProviderDocumentObject)

const App::PropertyIntegerConstraint::Constraints intPercent = {0,100,1};

ViewProviderGeometryObject::ViewProviderGeometryObject() : pcBoundSwitch(0)
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

    // Create the selection node
    pcHighlight = createFromSettings();
    pcHighlight->ref();
    if (pcHighlight->selectionMode.getValue() == Gui::SoFCSelection::SEL_OFF)
        Selectable.setValue(false);

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
    pcHighlight->unref();
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
    else if (prop == &BoundingBox) {
        showBoundingBox( BoundingBox.getValue() );
    }

    ViewProviderDocumentObject::onChanged(prop);
}

void ViewProviderGeometryObject::attach(App::DocumentObject *pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);
    pcHighlight->objectName = pcObj->getNameInDocument();
    pcHighlight->documentName = pcObj->getDocument()->getName();
    pcHighlight->subElementName = "Main";
}

void ViewProviderGeometryObject::updateData(const App::Property* prop)
{
    if (prop->isDerivedFrom(App::PropertyComplexGeoData::getClassTypeId())) {
        Base::BoundBox3d box = static_cast<const App::PropertyComplexGeoData*>(prop)->getBoundingBox();
        pcBoundingBox->minBounds.setValue(box.MinX, box.MinY, box.MinZ);
        pcBoundingBox->maxBounds.setValue(box.MaxX, box.MaxY, box.MaxZ);
        if (pcBoundSwitch) {
            SoGroup* grp = static_cast<SoGroup*>(pcBoundSwitch->getChild(0));
            SoTransform* trf = static_cast<SoTransform*>(grp->getChild(2));
            SbMatrix m;
            m.setTransform(pcTransform->translation.getValue(),
                           pcTransform->rotation.getValue(),
                           pcTransform->scaleFactor.getValue(),
                           pcTransform->scaleOrientation.getValue(),
                           pcTransform->center.getValue());
            trf->setMatrix(m.inverse());
        }
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
        float q0 = (float)p.getRotation().getValue()[0];
        float q1 = (float)p.getRotation().getValue()[1];
        float q2 = (float)p.getRotation().getValue()[2];
        float q3 = (float)p.getRotation().getValue()[3];
        float px = (float)p.getPosition().x;
        float py = (float)p.getPosition().y;
        float pz = (float)p.getPosition().z;
        pcTransform->rotation.setValue(q0,q1,q2,q3);
        pcTransform->translation.setValue(px,py,pz);
        pcTransform->center.setValue(0.0f,0.0f,0.0f);
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
#if 1
    SoSearchAction sa;
    sa.setInterest(SoSearchAction::FIRST);
    sa.setSearchingAll(FALSE);
    sa.setNode(this->pcTransform);
    sa.apply(pcRoot);
    SoPath * path = sa.getPath();
    if (path) {
        SoCenterballManip * manip = new SoCenterballManip;
        SoDragger* dragger = manip->getDragger();
        dragger->addStartCallback(dragStartCallback, this);
        dragger->addFinishCallback(dragFinishCallback, this);
        // Attach a sensor to the transform manipulator and set it as its user
        // data to delete it when the view provider leaves the edit mode
        SoNodeSensor* sensor = new SoNodeSensor(sensorCallback, this);
        //sensor->setPriority(0);
        sensor->attach(manip);
        manip->setUserData(sensor);
        return manip->replaceNode(path);
    }
    return false;
#else
	// get the size of this viewprovider
	SoGetBoundingBoxAction *boundAction = new SoGetBoundingBoxAction(SbViewportRegion());
	boundAction->apply(pcRoot);
    SbBox3f bdBox = boundAction->getBoundingBox();
	float size = bdBox.getSize().length();
	App::GeoFeature* geometry = static_cast<App::GeoFeature*>(pcObject);
	const Base::Placement pos = geometry->Placement.getValue();
	const Base::Vector3d &vpos = pos.getPosition();
	const Base::Rotation &rrot = pos.getRotation();

	// set up manipulator node
	SoSeparator * draggSep = new SoSeparator();
	SoDepthBuffer *depth = new SoDepthBuffer();
	depth->test = true;
	depth->function = SoDepthBuffer::ALWAYS;
	draggSep->addChild(depth);
	SoScale *scale = new SoScale();
	scale->scaleFactor = SbVec3f  (size,size,size);
	draggSep->addChild(scale);
	RotTransDragger *dragger = new RotTransDragger();
	dragger->translation = SbVec3f  (vpos.x,vpos.y,vpos.z);
	dragger->rotation = SbRotation(rrot[0],rrot[1],rrot[2],rrot[3]);
	draggSep->addChild(dragger);

    // Attach a sensor to the transform manipulator and set it as its user
    // data to delete it when the view provider leaves the edit mode
    SoNodeSensor* sensor = new SoNodeSensor(sensorCallback, this);
    //sensor->setPriority(0);
    sensor->attach(dragger);


	pcRoot->insertChild(draggSep,0);
	return true;

#endif 

}

void ViewProviderGeometryObject::unsetEdit(int ModNum)
{
# if 1
    SoSearchAction sa;
    sa.setType(SoCenterballManip::getClassTypeId());
    sa.setInterest(SoSearchAction::FIRST);
    sa.apply(pcRoot);
    SoPath * path = sa.getPath();

    // No transform manipulator found.
    if (!path)
        return;

    // The manipulator has a sensor as user data and this sensor contains the view provider
    SoCenterballManip * manip = static_cast<SoCenterballManip*>(path->getTail());
    SoNodeSensor* sensor = reinterpret_cast<SoNodeSensor*>(manip->getUserData());
    // #0000939: Pressing Escape while pivoting a box crashes
    // #0000942: Crash when 2xdouble-click on part
    SoDragger* dragger = manip->getDragger();
    if (dragger && dragger->getHandleEventAction())
        dragger->grabEventsCleanup();

    // detach sensor
    sensor->detach();
    delete sensor;

    SoTransform* transform = this->pcTransform;
    manip->replaceManip(path, transform);

    if (this->pcObject->getTypeId().isDerivedFrom(App::GeoFeature::getClassTypeId())) {
        App::GeoFeature* geometry = static_cast<App::GeoFeature*>(this->pcObject);
        this->updateData(&geometry->Placement);
    }
#else
	pcRoot->removeChild(0);
#endif 
}

void ViewProviderGeometryObject::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    if (ModNum == (int)ViewProvider::Transform) {
        SoNode* root = viewer->getSceneGraph();
        static_cast<SoFCUnifiedSelection*>(root)->selectionRole.setValue(FALSE);
    }
}

void ViewProviderGeometryObject::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    int ModNum = this->getEditingMode();
    if (ModNum == (int)ViewProvider::Transform) {
        SoNode* root = viewer->getSceneGraph();
        static_cast<SoFCUnifiedSelection*>(root)->selectionRole.setValue(TRUE);
    }
}

void ViewProviderGeometryObject::sensorCallback(void * data, SoSensor * s)
{
    SoNodeSensor* sensor = static_cast<SoNodeSensor*>(s);
    SoNode* node = sensor->getAttachedNode();

    if (node && node->getTypeId().isDerivedFrom(SoCenterballManip::getClassTypeId())) {
        // apply the transformation data to the placement
        SoCenterballManip* manip = static_cast<SoCenterballManip*>(node);
        float q0, q1, q2, q3;
        SbVec3f move = manip->translation.getValue();
        SbVec3f center = manip->center.getValue();
        manip->rotation.getValue().getValue(q0, q1, q2, q3);
    
        // get the placement
        ViewProviderGeometryObject* view = reinterpret_cast<ViewProviderGeometryObject*>(data);
        if (view && view->pcObject && view->pcObject->getTypeId().isDerivedFrom(App::GeoFeature::getClassTypeId())) {
            App::GeoFeature* geometry = static_cast<App::GeoFeature*>(view->pcObject);
            // Note: If R is the rotation, c the rotation center and t the translation
            // vector then Inventor applies the following transformation: R*(x-c)+c+t
            // In FreeCAD a placement only has a rotation and a translation part but
            // no rotation center. This means that we must divide the transformation
            // in a rotation and translation part.
            // R * (x-c) + c + t = R * x - R * c + c + t
            // The rotation part is R, the translation part t', however, is:
            // t' = t + c - R * c
            Base::Placement p;
            p.setRotation(Base::Rotation(q0,q1,q2,q3));
            Base::Vector3d t(move[0],move[1],move[2]);
            Base::Vector3d c(center[0],center[1],center[2]);
            t += c;
            p.getRotation().multVec(c,c);
            t -= c;
            p.setPosition(t);
            geometry->Placement.setValue(p);
        }
    }
#if 0
	else // RotTransDragger -----------------------------------------------------------	 
    if (node && node->getTypeId().isDerivedFrom(RotTransDragger::getClassTypeId())) {
        // apply the transformation data to the placement
        RotTransDragger* dragger = static_cast<RotTransDragger*>(node);
        float q0, q1, q2, q3;
        SbVec3f pos = dragger->translation.getValue();
        dragger->rotation.getValue().getValue(q0, q1, q2, q3);
    
        // get the placement
        ViewProviderGeometryObject* view = reinterpret_cast<ViewProviderGeometryObject*>(data);
        if (view && view->pcObject && view->pcObject->getTypeId().isDerivedFrom(App::GeoFeature::getClassTypeId())) {
            App::GeoFeature* geometry = static_cast<App::GeoFeature*>(view->pcObject);
            // Note: If R is the rotation, c the rotation center and t the translation
            // vector then Inventor applies the following transformation: R*(x-c)+c+t
            // In FreeCAD a placement only has a rotation and a translation part but
            // no rotation center. This means that we must divide the transformation
            // in a rotation and translation part.
            // R * (x-c) + c + t = R * x - R * c + c + t
            // The rotation part is R, the translation part t', however, is:
            // t' = t + c - R * c
            Base::Placement p;
            p.setRotation(Base::Rotation(q0,q1,q2,q3));
            Base::Vector3d t(pos[0],pos[1],pos[2]);
 /*           Base::Vector3d c(center[0],center[1],center[2]);
            t += c;
            p.getRotation().multVec(c,c);
            t -= c;*/
            p.setPosition(t);
            geometry->Placement.setValue(p);
        }
    } else
    if (node && node->getTypeId().isDerivedFrom(SoCenterballDragger::getClassTypeId())) {
        // apply the transformation data to the placement
        SoCenterballDragger* dragger = static_cast<SoCenterballDragger*>(node);
        float q0, q1, q2, q3;
        SbVec3f pos = dragger->center.getValue();
        dragger->rotation.getValue().getValue(q0, q1, q2, q3);
    
        // get the placement
        ViewProviderGeometryObject* view = reinterpret_cast<ViewProviderGeometryObject*>(data);
        if (view && view->pcObject && view->pcObject->getTypeId().isDerivedFrom(App::GeoFeature::getClassTypeId())) {
            App::GeoFeature* geometry = static_cast<App::GeoFeature*>(view->pcObject);
            // Note: If R is the rotation, c the rotation center and t the translation
            // vector then Inventor applies the following transformation: R*(x-c)+c+t
            // In FreeCAD a placement only has a rotation and a translation part but
            // no rotation center. This means that we must divide the transformation
            // in a rotation and translation part.
            // R * (x-c) + c + t = R * x - R * c + c + t
            // The rotation part is R, the translation part t', however, is:
            // t' = t + c - R * c
            Base::Placement p;
            p.setRotation(Base::Rotation(q0,q1,q2,q3));
            Base::Vector3d t(pos[0],pos[1],pos[2]);
 /*           Base::Vector3d c(center[0],center[1],center[2]);
            t += c;
            p.getRotation().multVec(c,c);
            t -= c;*/
            p.setPosition(t);
            geometry->Placement.setValue(p);
        }
    }
#endif
}

void ViewProviderGeometryObject::dragStartCallback(void *data, SoDragger *)
{
    // This is called when a manipulator is about to manipulating
    Gui::Application::Instance->activeDocument()->openCommand("Transform");
}

void ViewProviderGeometryObject::dragFinishCallback(void *data, SoDragger *)
{
    // This is called when a manipulator has done manipulating
    Gui::Application::Instance->activeDocument()->commitCommand();
}

SoPickedPointList ViewProviderGeometryObject::getPickedPoints(const SbVec2s& pos, const View3DInventorViewer& viewer,bool pickAll) const
{
    SoSeparator* root = new SoSeparator;
    root->ref();
    root->addChild(viewer.getHeadlight());
    root->addChild(viewer.getCamera());
    root->addChild(this->pcHighlight);

    SoRayPickAction rp(viewer.getViewportRegion());
    rp.setPickAll(pickAll);
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
    root->addChild(viewer.getCamera());
    root->addChild(this->pcHighlight);

    SoRayPickAction rp(viewer.getViewportRegion());
    rp.setPoint(pos);
    rp.apply(root);
    root->unref();

    // returns a copy of the point
    SoPickedPoint* pick = rp.getPickedPoint();
    //return (pick ? pick->copy() : 0); // needs the same instance of CRT under MS Windows
    return (pick ? new SoPickedPoint(*pick) : 0);
}

void ViewProviderGeometryObject::showBoundingBox(bool show)
{
    if (!pcBoundSwitch && show) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
        unsigned long bbcol = hGrp->GetUnsigned("BoundingBoxColor",4294967295UL); // white (255,255,255)
        float r,g,b;
        r = ((bbcol >> 24) & 0xff) / 255.0; g = ((bbcol >> 16) & 0xff) / 255.0; b = ((bbcol >> 8) & 0xff) / 255.0;
        pcBoundSwitch = new SoSwitch();
        SoSeparator* pBoundingSep = new SoSeparator();
        SoDrawStyle* lineStyle = new SoDrawStyle;
        lineStyle->lineWidth = 2.0f;
        pBoundingSep->addChild(lineStyle);
        SoBaseColor* color = new SoBaseColor();
        color->rgb.setValue(r, g, b);
        pBoundingSep->addChild(color);

        pBoundingSep->addChild(new SoTransform());
        pBoundingSep->addChild(pcBoundingBox);
        pcBoundingBox->coordsOn.setValue(false);
        pcBoundingBox->dimensionsOn.setValue(true);

        // add to the highlight node
        pcBoundSwitch->addChild(pBoundingSep);
        pcRoot->addChild(pcBoundSwitch);
    }

    if (pcBoundSwitch) {
        pcBoundSwitch->whichChild = (show ? 0 : -1);
    }
}

SoFCSelection* ViewProviderGeometryObject::createFromSettings() const
{
    SoFCSelection* sel = new SoFCSelection();

    float transparency;
    ParameterGrp::handle hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("View");
    bool enablePre = hGrp->GetBool("EnablePreselection", true);
    bool enableSel = hGrp->GetBool("EnableSelection", true);
    if (!enablePre) {
        sel->highlightMode = Gui::SoFCSelection::OFF;
    }
    else {
        // Search for a user defined value with the current color as default
        SbColor highlightColor = sel->colorHighlight.getValue();
        unsigned long highlight = (unsigned long)(highlightColor.getPackedValue());
        highlight = hGrp->GetUnsigned("HighlightColor", highlight);
        highlightColor.setPackedValue((uint32_t)highlight, transparency);
        sel->colorHighlight.setValue(highlightColor);
    }
    if (!enableSel || !Selectable.getValue()) {
        sel->selectionMode = Gui::SoFCSelection::SEL_OFF;
    }
    else {
        // Do the same with the selection color
        SbColor selectionColor = sel->colorSelection.getValue();
        unsigned long selection = (unsigned long)(selectionColor.getPackedValue());
        selection = hGrp->GetUnsigned("SelectionColor", selection);
        selectionColor.setPackedValue((uint32_t)selection, transparency);
        sel->colorSelection.setValue(selectionColor);
    }

    return sel;
}

void ViewProviderGeometryObject::setSelectable(bool selectable)
{
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
