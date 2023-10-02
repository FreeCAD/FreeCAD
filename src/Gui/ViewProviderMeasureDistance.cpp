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
# include <Inventor/events/SoKeyboardEvent.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoFontStyle.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoTranslation.h>
#endif

#include <Inventor/MarkerBitmaps.h>

#include <App/Document.h>
#include <App/MeasureDistance.h>
#include <Base/Console.h>
#include <Base/Quantity.h>

#include "ViewProviderMeasureDistance.h"
#include "Application.h"
#include <Command.h>
#include "Document.h"
#include "View3DInventorViewer.h"
#include "ViewParams.h"


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderMeasureDistance, Gui::ViewProviderDocumentObject)


ViewProviderMeasureDistance::ViewProviderMeasureDistance()
{
    ADD_PROPERTY(TextColor,(1.0f,1.0f,1.0f));
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

bool ViewProviderMeasureDistance::isPartOfPhysicalObject() const
{
    return false;
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

std::vector<std::string> ViewProviderMeasureDistance::getDisplayModes() const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.emplace_back("Base");
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

    auto ps = new SoPickStyle();
    ps->style = SoPickStyle::UNPICKABLE;

    auto lineSep = new SoSeparator();
    auto style = new SoDrawStyle();
    style->lineWidth = 2.0f;
    lineSep->addChild(ps);
    lineSep->addChild(style);
    lineSep->addChild(pColor);
    lineSep->addChild(pCoords);
    lineSep->addChild(pLines);
    auto points = new SoMarkerSet();
    points->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CROSS",
            ViewParams::instance()->getMarkerSize());
    points->numPoints=2;
    lineSep->addChild(points);

    auto textsep = new SoSeparator();
    textsep->addChild(pTranslation);
    textsep->addChild(pTextColor);
    textsep->addChild(pFont);
    textsep->addChild(pLabel);

    auto sep = new SoAnnotation();
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

        pLabel->string.setValue((Base::Quantity(dif.length(), Base::Unit::Length)).getUserString().toUtf8().constData());
    }

    ViewProviderDocumentObject::updateData(prop);
}

// ----------------------------------------------------------------------------

PointMarker::PointMarker(View3DInventorViewer* iv) : view(iv),
    vp(new ViewProviderPointMarker)
{
    view->addViewProvider(vp);
    previousSelectionEn = view->isSelectionEnabled();
    view->setSelectionEnabled(false);
}

PointMarker::~PointMarker()
{
    view->removeViewProvider(vp);
    view->setSelectionEnabled(previousSelectionEn);
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

void PointMarker::customEvent(QEvent*)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    doc->openCommand(QT_TRANSLATE_NOOP("Command", "Measure distance"));
    App::DocumentObject* obj = doc->getDocument()->addObject
        (App::MeasureDistance::getClassTypeId().getName(),"Distance");

    auto md = static_cast<App::MeasureDistance*>(obj);
    const SbVec3f& pt1 = vp->pCoords->point[0];
    const SbVec3f& pt2 = vp->pCoords->point[1];
    md->P1.setValue(Base::Vector3d(pt1[0],pt1[1],pt1[2]));
    md->P2.setValue(Base::Vector3d(pt2[0],pt2[1],pt2[2]));

    QString str = QString::fromLatin1("Distance: %1")
        .arg(Base::Quantity(md->Distance.getValue(), Base::Unit::Length).getUserString());
    md->Label.setValue(str.toUtf8().constData());
    doc->commitCommand();

    this->deleteLater();
}

PROPERTY_SOURCE(Gui::ViewProviderPointMarker, Gui::ViewProviderDocumentObject)

ViewProviderPointMarker::ViewProviderPointMarker()
{
    pCoords = new SoCoordinate3();
    pCoords->ref();
    pCoords->point.setNum(0);
    pMarker = new SoMarkerSet();
    pMarker->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CROSS",
            ViewParams::instance()->getMarkerSize());
    pMarker->numPoints=0;
    pMarker->ref();

    auto grp = new SoGroup();
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

bool ViewProviderPointMarker::isPartOfPhysicalObject() const
{
    return false;
}

void ViewProviderMeasureDistance::measureDistanceCallback(void * ud, SoEventCallback * n)
{
    auto view  = static_cast<Gui::View3DInventorViewer*>(n->getUserData());
    auto pm = static_cast<PointMarker*>(ud);
    const SoEvent* ev = n->getEvent();
    if (ev->isOfType(SoKeyboardEvent::getClassTypeId())) {
        const auto ke = static_cast<const SoKeyboardEvent*>(ev);
        const SbBool press = ke->getState() == SoButtonEvent::DOWN ? true : false;
        if (ke->getKey() == SoKeyboardEvent::ESCAPE) {
            n->setHandled();
            // Handle it on key up, because otherwise upper layer will handle it too.
            if (!press) {
                endMeasureDistanceMode(ud, view, n, pm);
            }
        }
    }
    else if (ev->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        const auto mbe = static_cast<const SoMouseButtonEvent*>(ev);

        // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
        n->getAction()->setHandled();

        if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN) {
            const SoPickedPoint * point = n->getPickedPoint();
            if (!point) {
                Base::Console().Message("No point picked.\n");
                return;
            }

            n->setHandled();
            pm->addPoint(point->getPoint());
            if (pm->countPoints() == 2) {
                auto e = new QEvent(QEvent::User);
                QApplication::postEvent(pm, e);
                // leave mode
                view->setEditing(false);
                view->removeEventCallback(SoEvent::getClassTypeId(), measureDistanceCallback, ud);
            }
        }
        else if (mbe->getButton() != SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::UP) {
            endMeasureDistanceMode(ud, view, n, pm);
        }
    }
}

void ViewProviderMeasureDistance::endMeasureDistanceMode(void * ud, Gui::View3DInventorViewer* view, SoEventCallback * n, PointMarker *pm)
{
    n->setHandled();
    view->setEditing(false);
    view->removeEventCallback(SoEvent::getClassTypeId(), ViewProviderMeasureDistance::measureDistanceCallback, ud);
    Application::Instance->commandManager().testActive();
    pm->deleteLater();
}
