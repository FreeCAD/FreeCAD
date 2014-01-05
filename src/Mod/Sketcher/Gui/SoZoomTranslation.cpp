/***************************************************************************
 *   Copyright (c) 2011                                                    *
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
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/misc/SoState.h>
# include <cmath>
# include <cfloat>
#endif

#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/nodes/SoCamera.h>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>


#include "SoZoomTranslation.h"

// *************************************************************************

using namespace SketcherGui;

// ------------------------------------------------------

SO_NODE_SOURCE(SoZoomTranslation);

void SoZoomTranslation::initClass()
{
    SO_NODE_INIT_CLASS(SoZoomTranslation, SoTranslation, "Translation");
}

float SoZoomTranslation::getScaleFactor()
{
    // Dividing by 5 seems to work well

    Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    if (mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();
        this->scale = viewer->getCamera()->getViewVolume(viewer->getCamera()->aspectRatio.getValue()).getWorldToScreenScale(SbVec3f(0.f, 0.f, 0.f), 0.1f) / 5;
        return this->scale;
    } else {
        return this->scale;
    }
}

SoZoomTranslation::SoZoomTranslation()
{
    SO_NODE_CONSTRUCTOR(SoZoomTranslation);
    SO_NODE_ADD_FIELD(abPos, (SbVec3f(0.f,0.f,0.f)));
    this->scale = -1;
}

void SoZoomTranslation::GLRender(SoGLRenderAction * action)
{   
    SoZoomTranslation::doAction((SoAction *)action);
}

// Doc in superclass.
void SoZoomTranslation::doAction(SoAction * action)
{
    SbVec3f v;
    if(this->translation.getValue() == SbVec3f(0.0f, 0.0f, 0.0f) && this->abPos.getValue() == SbVec3f(0.0f, 0.0f, 0.0f)) {
        return;
    } else {
        SbVec3f absVtr = this->abPos.getValue();
        SbVec3f relVtr = this->translation.getValue();

        float sf = this->getScaleFactor();
        // For Sketcher Keep Z value the same
        relVtr[0] = (relVtr[0] != 0) ? sf * relVtr[0] : 0;
        relVtr[1] = (relVtr[1] != 0) ? sf * relVtr[1] : 0;

        v = absVtr + relVtr;
    }
    
    SoModelMatrixElement::translateBy(action->getState(), this, v);
}

void SoZoomTranslation::getMatrix(SoGetMatrixAction * action)
{
    SbVec3f v;
    if(this->translation.getValue() == SbVec3f(0.0f, 0.0f, 0.0f) && this->abPos.getValue() == SbVec3f(0.0f, 0.0f, 0.0f)) {
        return;
    } else {
        SbVec3f absVtr = this->abPos.getValue();
        SbVec3f relVtr = this->translation.getValue();

        float sf = this->getScaleFactor();
        // For Sketcher Keep Z value the same
        relVtr[0] = (relVtr[0] != 0) ? sf  * relVtr[0] : 0;
        relVtr[1] = (relVtr[1] != 0) ? sf  * relVtr[1] : 0;

        v = absVtr + relVtr;
    }
    
    SbMatrix m;
    m.setTranslate(v);
    action->getMatrix().multLeft(m);
    m.setTranslate(-v);
    action->getInverse().multRight(m);
  
}

void SoZoomTranslation::callback(SoCallbackAction * action)
{
  SoZoomTranslation::doAction((SoAction *)action);
}

void SoZoomTranslation::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoZoomTranslation::doAction((SoAction *)action);
}

void SoZoomTranslation::pick(SoPickAction * action)
{
  SoZoomTranslation::doAction((SoAction *)action);
}

// Doc in superclass.
void SoZoomTranslation::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoZoomTranslation::doAction((SoAction *)action);
}