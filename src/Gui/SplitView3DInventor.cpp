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
# include <qfileinfo.h>
# include <qsplitter.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoPerspectiveCamera.h>
# include <Inventor/nodes/SoDirectionalLight.h>
#endif

#include "SplitView3DInventor.h"
#include "View3DInventorViewer.h"
#include "SoFCSelectionAction.h"
#include "Document.h"
#include "Application.h"
#include "NavigationStyle.h"


using namespace Gui;

TYPESYSTEM_SOURCE_ABSTRACT(Gui::AbstractSplitView,Gui::MDIView);

AbstractSplitView::AbstractSplitView(Gui::Document* pcDocument, QWidget* parent, Qt::WFlags wflags)
  : MDIView(pcDocument,parent, wflags)
{
    // important for highlighting 
    setMouseTracking(true);
}

AbstractSplitView::~AbstractSplitView()
{
    hGrp->Detach(this);
    for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
        delete *it;
    }
}

void AbstractSplitView::setupSettings()
{
    // attach Parameter Observer
    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Attach(this);

    // apply the user settings
    OnChange(*hGrp,"EyeDistance");
    OnChange(*hGrp,"CornerCoordSystem");
    OnChange(*hGrp,"UseAutoRotation");
    OnChange(*hGrp,"Gradient");
    OnChange(*hGrp,"BackgroundColor");
    OnChange(*hGrp,"BackgroundColor2");
    OnChange(*hGrp,"BackgroundColor3");
    OnChange(*hGrp,"BackgroundColor4");
    OnChange(*hGrp,"UseBackgroundColorMid");
    OnChange(*hGrp,"UseAntialiasing");
    OnChange(*hGrp,"ShowFPS");
    OnChange(*hGrp,"Orthographic");
    OnChange(*hGrp,"HeadlightColor");
    OnChange(*hGrp,"HeadlightDirection");
    OnChange(*hGrp,"HeadlightIntensity");
    OnChange(*hGrp,"EnableBacklight");
    OnChange(*hGrp,"BacklightColor");
    OnChange(*hGrp,"BacklightDirection");
    OnChange(*hGrp,"BacklightIntensity");
    OnChange(*hGrp,"NavigationStyle");
}

View3DInventorViewer* AbstractSplitView::getViewer(unsigned int n) const
{
    return (_viewer.size() > n ? _viewer[n] : 0);
}

/// Observer message from the ParameterGrp
void AbstractSplitView::OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason)
{
    const ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);
    if (strcmp(Reason,"HeadlightColor") == 0) {
        unsigned long headlight = rGrp.GetUnsigned("HeadlightColor",ULONG_MAX); // default color (white)
        float transparency;
        SbColor headlightColor;
        headlightColor.setPackedValue((uint32_t)headlight, transparency);
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            (*it)->getHeadlight()->color.setValue(headlightColor);
    }
    else if (strcmp(Reason,"HeadlightDirection") == 0) {
        std::string pos = rGrp.GetASCII("HeadlightDirection");
        QString flt = QString::fromAscii("([-+]?[0-9]+\\.?[0-9]+)");
        QRegExp rx(QString::fromAscii("^\\(%1,%1,%1\\)$").arg(flt));
        if (rx.indexIn(QLatin1String(pos.c_str())) > -1) {
            float x = rx.cap(1).toFloat();
            float y = rx.cap(2).toFloat();
            float z = rx.cap(3).toFloat();
            for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
                (*it)->getHeadlight()->direction.setValue(x,y,z);
        }
    }
    else if (strcmp(Reason,"HeadlightIntensity") == 0) {
        long value = rGrp.GetInt("HeadlightIntensity", 100);
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            (*it)->getHeadlight()->intensity.setValue((float)value/100.0f);
    }
    else if (strcmp(Reason,"EnableBacklight") == 0) {
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            (*it)->setBacklight(rGrp.GetBool("EnableBacklight", false));
    }
    else if (strcmp(Reason,"BacklightColor") == 0) {
        unsigned long backlight = rGrp.GetUnsigned("BacklightColor",ULONG_MAX); // default color (white)
        float transparency;
        SbColor backlightColor;
        backlightColor.setPackedValue((uint32_t)backlight, transparency);
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            (*it)->getBacklight()->color.setValue(backlightColor);
    }
    else if (strcmp(Reason,"BacklightDirection") == 0) {
        std::string pos = rGrp.GetASCII("BacklightDirection");
        QString flt = QString::fromAscii("([-+]?[0-9]+\\.?[0-9]+)");
        QRegExp rx(QString::fromAscii("^\\(%1,%1,%1\\)$").arg(flt));
        if (rx.indexIn(QLatin1String(pos.c_str())) > -1) {
            float x = rx.cap(1).toFloat();
            float y = rx.cap(2).toFloat();
            float z = rx.cap(3).toFloat();
            for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
                (*it)->getBacklight()->direction.setValue(x,y,z);
        }
    }
    else if (strcmp(Reason,"BacklightIntensity") == 0) {
        long value = rGrp.GetInt("BacklightIntensity", 100);
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            (*it)->getBacklight()->intensity.setValue((float)value/100.0f);
    }
    else if (strcmp(Reason,"EnablePreselection") == 0) {
        SoFCEnableHighlightAction cAct(rGrp.GetBool("EnablePreselection", true));
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            cAct.apply((*it)->getSceneGraph());
    }
    else if (strcmp(Reason,"EnableSelection") == 0) {
        SoFCEnableSelectionAction cAct(rGrp.GetBool("EnableSelection", true));
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            cAct.apply((*it)->getSceneGraph());
    }
    else if (strcmp(Reason,"HighlightColor") == 0) {
        float transparency;
        SbColor highlightColor(0.8f, 0.1f, 0.1f);
        unsigned long highlight = (unsigned long)(highlightColor.getPackedValue());
        highlight = rGrp.GetUnsigned("HighlightColor", highlight);
        highlightColor.setPackedValue((uint32_t)highlight, transparency);
        SoSFColor col; col.setValue(highlightColor);
        SoFCHighlightColorAction cAct(col);
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            cAct.apply((*it)->getSceneGraph());
    }
    else if (strcmp(Reason,"SelectionColor") == 0) {
        float transparency;
        SbColor selectionColor(0.1f, 0.8f, 0.1f);
        unsigned long selection = (unsigned long)(selectionColor.getPackedValue());
        selection = rGrp.GetUnsigned("SelectionColor", selection);
        selectionColor.setPackedValue((uint32_t)selection, transparency);
        SoSFColor col; col.setValue(selectionColor);
        SoFCSelectionColorAction cAct(col);
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            cAct.apply((*it)->getSceneGraph());
    }
    else if (strcmp(Reason,"NavigationStyle") == 0) {
        // tmp. disabled will be activated after redesign of 3d viewer
        // check whether the simple or the Full Mouse model is used
        //std::string model = rGrp.GetASCII("NavigationStyle",CADNavigationStyle::getClassTypeId().getName());
        //Base::Type type = Base::Type::fromName(model.c_str());
        //for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
        //    (*it)->setNavigationType(type);
    }
    else if (strcmp(Reason,"EyeDistance") == 0) {
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            (*it)->setStereoOffset(rGrp.GetFloat("EyeDistance",5.0));
    }
    else if (strcmp(Reason,"CornerCoordSystem") == 0) {
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            (*it)->setFeedbackVisibility(rGrp.GetBool("CornerCoordSystem",true));
    }
    else if (strcmp(Reason,"UseAutoRotation") == 0) {
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            (*it)->setAnimationEnabled(rGrp.GetBool("UseAutoRotation",true));
    }
    else if (strcmp(Reason,"Gradient") == 0) {
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            (*it)->setGradientBackground((rGrp.GetBool("Gradient",true)));
    }
    else if (strcmp(Reason,"UseAntialiasing") == 0) {
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            (*it)->getGLRenderAction()->setSmoothing(rGrp.GetBool("UseAntialiasing",false));
    }
    else if (strcmp(Reason,"ShowFPS") == 0) {
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            (*it)->setEnabledFPSCounter(rGrp.GetBool("ShowFPS",false));
    }
    else if (strcmp(Reason,"Orthographic") == 0) {
        // check whether a perspective or orthogrphic camera should be set
        if (rGrp.GetBool("Orthographic", true)) {
            for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
                (*it)->setCameraType(SoOrthographicCamera::getClassTypeId());
        }
        else {
            for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
                (*it)->setCameraType(SoPerspectiveCamera::getClassTypeId());
        }
    }
    else {
        unsigned long col1 = rGrp.GetUnsigned("BackgroundColor",3940932863UL);
        unsigned long col2 = rGrp.GetUnsigned("BackgroundColor2",859006463UL); // default color (dark blue)
        unsigned long col3 = rGrp.GetUnsigned("BackgroundColor3",2880160255UL); // default color (blue/grey)
        unsigned long col4 = rGrp.GetUnsigned("BackgroundColor4",1869583359UL); // default color (blue/grey)
        float r1,g1,b1,r2,g2,b2,r3,g3,b3,r4,g4,b4;
        r1 = ((col1 >> 24) & 0xff) / 255.0; g1 = ((col1 >> 16) & 0xff) / 255.0; b1 = ((col1 >> 8) & 0xff) / 255.0;
        r2 = ((col2 >> 24) & 0xff) / 255.0; g2 = ((col2 >> 16) & 0xff) / 255.0; b2 = ((col2 >> 8) & 0xff) / 255.0;
        r3 = ((col3 >> 24) & 0xff) / 255.0; g3 = ((col3 >> 16) & 0xff) / 255.0; b3 = ((col3 >> 8) & 0xff) / 255.0;
        r4 = ((col4 >> 24) & 0xff) / 255.0; g4 = ((col4 >> 16) & 0xff) / 255.0; b4 = ((col4 >> 8) & 0xff) / 255.0;
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            (*it)->setBackgroundColor(SbColor(r1, g1, b1));
            if (rGrp.GetBool("UseBackgroundColorMid",false) == false)
                (*it)->setGradientBackgroundColor(SbColor(r2, g2, b2), SbColor(r3, g3, b3));
            else
                (*it)->setGradientBackgroundColor(SbColor(r2, g2, b2), SbColor(r3, g3, b3), SbColor(r4, g4, b4));
        }
    }
}

void AbstractSplitView::onUpdate(void)
{
    update();  
}

const char *AbstractSplitView::getName(void) const
{
    return "SplitView3DInventor";
}

bool AbstractSplitView::onMsg(const char* pMsg, const char** ppReturn)
{
    if (strcmp("ViewFit",pMsg) == 0 ) {
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
            (*it)->viewAll();
        return true;
    }
    else if (strcmp("ViewBottom",pMsg) == 0) {
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getCamera();
            cam->orientation.setValue(-1, 0, 0, 0);
            (*it)->viewAll();
        }
        return true;
    }
    else if (strcmp("ViewFront",pMsg) == 0) {
        float root = (float)(sqrt(2.0)/2.0);
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getCamera();
            cam->orientation.setValue(-root, 0, 0, -root);
            (*it)->viewAll();
        }
        return true;
    }
    else if (strcmp("ViewLeft",pMsg) == 0) {
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getCamera();
            cam->orientation.setValue(-0.5, 0.5, 0.5, -0.5);
            (*it)->viewAll();
        }
        return true;
    }
    else if (strcmp("ViewRear",pMsg) == 0) {
        float root = (float)(sqrt(2.0)/2.0);
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getCamera();
            cam->orientation.setValue(0, root, root, 0);
            (*it)->viewAll();
        }
        return true;
    }
    else if (strcmp("ViewRight",pMsg) == 0) {
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getCamera();
            cam->orientation.setValue(0.5, 0.5, 0.5, 0.5);
            (*it)->viewAll();
        }
        return true;
    }
    else if (strcmp("ViewTop",pMsg) == 0) {
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getCamera();
            cam->orientation.setValue(0, 0, 0, 1);
            (*it)->viewAll();
        }
        return true;
    }
    else if (strcmp("ViewAxo",pMsg) == 0) {
        float root = (float)(sqrt(3.0)/4.0);
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getCamera();
            cam->orientation.setValue(-0.333333f, -0.166666f, -0.333333f, -root);
            (*it)->viewAll();
        }
        return true;
    }

    return false;
}

bool AbstractSplitView::onHasMsg(const char* pMsg) const
{
    if (strcmp("ViewFit",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewBottom",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewFront",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewLeft",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewRear",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewRight",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewTop",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewAxo",pMsg) == 0) {
        return true;
    }
    return false;
}

void AbstractSplitView::setOverrideCursor(const QCursor& aCursor)
{
    //_viewer->getWidget()->setCursor(aCursor);
}

// ------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Gui::SplitView3DInventor, Gui::AbstractSplitView);

SplitView3DInventor::SplitView3DInventor(int views, Gui::Document* pcDocument, QWidget* parent, Qt::WFlags wflags)
  : AbstractSplitView(pcDocument,parent, wflags)
{
    QSplitter* mainSplitter=0;

    if (views <= 3) {
        mainSplitter = new QSplitter(Qt::Horizontal, this);
        _viewer.push_back(new View3DInventorViewer(mainSplitter));
        _viewer.push_back(new View3DInventorViewer(mainSplitter));
        if (views==3)
            _viewer.push_back(new View3DInventorViewer(mainSplitter));
    }
    else {
        mainSplitter = new QSplitter(Qt::Vertical, this);
        QSplitter *topSplitter = new QSplitter(Qt::Horizontal, mainSplitter);
        QSplitter *botSplitter = new QSplitter(Qt::Horizontal, mainSplitter);
        _viewer.push_back(new View3DInventorViewer(topSplitter));
        _viewer.push_back(new View3DInventorViewer(topSplitter));
        for (int i=2;i<views;i++)
            _viewer.push_back(new View3DInventorViewer(botSplitter));
        topSplitter->setOpaqueResize( true );
        botSplitter->setOpaqueResize( true );
    }

    mainSplitter->show();
    setCentralWidget(mainSplitter);

    // apply the user settings
    setupSettings();
}

SplitView3DInventor::~SplitView3DInventor()
{
}
