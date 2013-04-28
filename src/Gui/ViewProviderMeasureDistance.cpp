/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <sstream>
# include <QApplication>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoFontStyle.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoDrawStyle.h>
#endif

#include "ViewProviderMeasureDistance.h"
#include "SoFCSelection.h"
#include "Application.h"
#include "Document.h"
#include "View3DInventorViewer.h"

#include <App/PropertyGeo.h>
#include <App/PropertyStandard.h>
#include <App/MeasureDistance.h>
#include <Base/Console.h>

using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderMeasureDistance, Gui::ViewProviderDocumentObject)


ViewProviderMeasureDistance::ViewProviderMeasureDistance() 
{
    ADD_PROPERTY(TextColor,(0.0f,0.0f,0.0f));
    ADD_PROPERTY(LineColor,(1.0f,1.0f,1.0f));
    ADD_PROPERTY(FontSize,(18));
    ADD_PROPERTY(DistFactor,(1.0));
    ADD_PROPERTY(Mirror,(false));

    pFont = new SoFontStyle();
    pFont->ref();
    pLabel = new SoText2();
    pLabel->ref();
    pColor = new SoBaseColor();
    pColor->ref();
    pTextColor = new SoBaseColor();
    pTextColor->ref();
    pTranslation = new SoTranslation();
    pTranslation->ref();

    TextColor.touch();
    FontSize.touch();
    LineColor.touch();

    static const SbVec3f verts[4] =
    {
        SbVec3f(0,0,0), SbVec3f(0,0,0),
        SbVec3f(0,0,0), SbVec3f(0,0,0)
    };

    // indexes used to create the edges
    static const int32_t lines[9] =
    {
        0,2,-1,
        1,3,-1,
        2,3,-1
    };

    pCoords = new SoCoordinate3();
    pCoords->ref();
    pCoords->point.setNum(4);
    pCoords->point.setValues(0, 4, verts);

    pLines  = new SoIndexedLineSet();
    pLines->ref();
    pLines->coordIndex.setNum(9);
    pLines->coordIndex.setValues(0, 9, lines);
    sPixmap = "view-measurement";
}

ViewProviderMeasureDistance::~ViewProviderMeasureDistance()
{
    pFont->unref();
    pLabel->unref();
    pColor->unref();
    pTextColor->unref();
    pTranslation->unref();
    pCoords->unref();
    pLines->unref();
}

void ViewProviderMeasureDistance::onChanged(const App::Property* prop)
{
    if (prop == &Mirror || prop == &DistFactor) {
        updateData(prop);
    }
    else if (prop == &TextColor) {
        const App::Color& c = TextColor.getValue();
        pTextColor->rgb.setValue(c.r,c.g,c.b);
    }
    else if (prop == &LineColor) {
        const App::Color& c = LineColor.getValue();
        pColor->rgb.setValue(c.r,c.g,c.b);
    }
    else if (prop == &FontSize) {
        pFont->size = FontSize.getValue();
    }
    else {
        ViewProviderDocumentObject::onChanged(prop);
    }
}

std::vector<std::string> ViewProviderMeasureDistance::getDisplayModes(void) const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.push_back("Base");
    return StrList;
}

void ViewProviderMeasureDistance::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0)
        setDisplayMaskMode("Base");
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

void ViewProviderMeasureDistance::attach(App::DocumentObject* pcObject)
{
    ViewProviderDocumentObject::attach(pcObject);

    SoPickStyle* ps = new SoPickStyle();
    ps->style = SoPickStyle::UNPICKABLE;

    SoSeparator *lineSep = new SoSeparator();
    SoDrawStyle* style = new SoDrawStyle();
    style->lineWidth = 2.0f;
    lineSep->addChild(ps);
    lineSep->addChild(style);
    lineSep->addChild(pColor);
    lineSep->addChild(pCoords);
    lineSep->addChild(pLines);
    SoMarkerSet* points = new SoMarkerSet();
    points->markerIndex = SoMarkerSet::CROSS_9_9;
    points->numPoints=2;
    lineSep->addChild(points);

    // Making the whole measurement object selectable by 3d view can
    // become cumbersome since it has influence to any raypick action.
    // Thus, it's only selectable by its text label
    SoFCSelection* textsep = new SoFCSelection();
    textsep->objectName = pcObject->getNameInDocument();
    textsep->documentName = pcObject->getDocument()->getName();
    textsep->subElementName = "Main";
    //SoSeparator* textsep = new SoSeparator();
    textsep->addChild(pTranslation);
    textsep->addChild(pTextColor);
    textsep->addChild(pFont);
    textsep->addChild(pLabel);

    SoSeparator* sep = new SoSeparator();
    sep->addChild(lineSep);
    sep->addChild(textsep);
    addDisplayMaskMode(sep, "Base");
}

void ViewProviderMeasureDistance::updateData(const App::Property* prop)
{
    if (prop->getTypeId() == App::PropertyVector::getClassTypeId() ||
        prop == &Mirror || prop == &DistFactor) {
        if (strcmp(prop->getName(),"P1") == 0) {
            Base::Vector3d v = static_cast<const App::PropertyVector*>(prop)->getValue();
            pCoords->point.set1Value(0, SbVec3f(v.x,v.y,v.z));
        }
        else if (strcmp(prop->getName(),"P2") == 0) {
            Base::Vector3d v = static_cast<const App::PropertyVector*>(prop)->getValue();
            pCoords->point.set1Value(1, SbVec3f(v.x,v.y,v.z));
        }

        SbVec3f pt1 = pCoords->point[0];
        SbVec3f pt2 = pCoords->point[1];
        SbVec3f dif = pt1-pt2;

        float length = fabs(dif.length())*DistFactor.getValue();
        if (Mirror.getValue())
            length = -length;


        if (dif.sqrLength() < 10.0e-6f) {
            pCoords->point.set1Value(2, pt1+SbVec3f(0.0f,0.0f,length));
            pCoords->point.set1Value(3, pt2+SbVec3f(0.0f,0.0f,length));
        }
        else {
            SbVec3f dir = dif.cross(SbVec3f(1.0f,0.0f,0.0f));
            if (dir.sqrLength() < 10.0e-6f)
                dir = dif.cross(SbVec3f(0.0f,1.0f,0.0f));
            if (dir.sqrLength() < 10.0e-6f)
                dir = dif.cross(SbVec3f(0.0f,0.0f,1.0f));
            dir.normalize();
            if (dir.dot(SbVec3f(0.0f,0.0f,1.0f)) < 0.0f)
                length = -length;
            pCoords->point.set1Value(2, pt1 + length*dir);
            pCoords->point.set1Value(3, pt2 + length*dir);
        }

        SbVec3f pos = (pCoords->point[2]+pCoords->point[3])/2.0f;
        pTranslation->translation.setValue(pos);

        std::stringstream s;
        s.precision(3);
        s.setf(std::ios::fixed | std::ios::showpoint);
        s << dif.length();
        pLabel->string.setValue(s.str().c_str());
    }

    ViewProviderDocumentObject::updateData(prop);
}

// ----------------------------------------------------------------------------

PointMarker::PointMarker(View3DInventorViewer* iv) : view(iv),
    vp(new ViewProviderPointMarker)
{
    view->addViewProvider(vp);
}

PointMarker::~PointMarker()
{
    view->removeViewProvider(vp);
    delete vp;
}

void PointMarker::addPoint(const SbVec3f& pt)
{
    int ct = countPoints();
    vp->pCoords->point.set1Value(ct, pt);
    vp->pMarker->numPoints=ct+1;
}

int PointMarker::countPoints() const
{
    return vp->pCoords->point.getNum();
}

void PointMarker::customEvent(QEvent* e)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    App::DocumentObject* obj = doc->getDocument()->addObject
        (App::MeasureDistance::getClassTypeId().getName(),"Distance");

    App::MeasureDistance* md = static_cast<App::MeasureDistance*>(obj);
    const SbVec3f& pt1 = vp->pCoords->point[0];
    const SbVec3f& pt2 = vp->pCoords->point[1];
    md->P1.setValue(Base::Vector3d(pt1[0],pt1[1],pt1[2]));
    md->P2.setValue(Base::Vector3d(pt2[0],pt2[1],pt2[2]));
    std::stringstream s;
    s.precision(3);
    s.setf(std::ios::fixed | std::ios::showpoint);
    s << "Distance: " << md->Distance.getValue();
    md->Label.setValue(s.str());
}

PROPERTY_SOURCE(Gui::ViewProviderPointMarker, Gui::ViewProviderDocumentObject)

ViewProviderPointMarker::ViewProviderPointMarker()
{
    pCoords = new SoCoordinate3();
    pCoords->ref();
    pCoords->point.setNum(0);
    pMarker = new SoMarkerSet();
    pMarker->markerIndex = SoMarkerSet::CROSS_9_9;
    pMarker->numPoints=0;
    pMarker->ref();

    SoGroup* grp = new SoGroup();
    grp->addChild(pCoords);
    grp->addChild(pMarker);
    addDisplayMaskMode(grp, "Base");
    setDisplayMaskMode("Base");
}

ViewProviderPointMarker::~ViewProviderPointMarker()
{
    pCoords->unref();
    pMarker->unref();
}

void ViewProviderMeasureDistance::measureDistanceCallback(void * ud, SoEventCallback * n)
{
    const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent*>(n->getEvent());
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
    PointMarker *pm = reinterpret_cast<PointMarker*>(ud);

    // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
    n->getAction()->setHandled();
    if (mbe->getButton() == SoMouseButtonEvent::BUTTON2 && mbe->getState() == SoButtonEvent::UP) {
        n->setHandled();
        view->setEditing(false);
        view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), measureDistanceCallback, ud);
        pm->deleteLater();
    }
    else if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN) {
        const SoPickedPoint * point = n->getPickedPoint();
        if (point == NULL) {
            Base::Console().Message("No point picked.\n");
            return;
        }

        n->setHandled();
        pm->addPoint(point->getPoint());
        if (pm->countPoints() == 2) {
            QEvent *e = new QEvent(QEvent::User);
            QApplication::postEvent(pm, e);
            // leave mode
            view->setEditing(false);
            view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), measureDistanceCallback, ud);
            pm->deleteLater();
        }
    }
}
