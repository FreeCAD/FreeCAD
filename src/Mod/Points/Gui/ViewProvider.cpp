/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoPointSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMaterialBinding.h>
# include <Inventor/nodes/SoNormal.h>
# include <Inventor/errors/SoDebugError.h>
# include <Inventor/events/SoMouseButtonEvent.h>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui,...
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <Base/Tools2D.h>
#include <Base/Vector3D.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/SoFCSelection.h>

#include <Gui/View3DInventorViewer.h>
#include <Mod/Points/App/PointsFeature.h>

#include "ViewProvider.h"
#include "../App/Properties.h"


using namespace PointsGui;
using namespace Points;


PROPERTY_SOURCE(PointsGui::ViewProviderPoints, Gui::ViewProviderGeometryObject)


App::PropertyFloatConstraint::Constraints ViewProviderPoints::floatRange = {1.0,64.0,1.0};

ViewProviderPoints::ViewProviderPoints()
{
    ADD_PROPERTY(PointSize,(2.0f));
    PointSize.setConstraints(&floatRange);

    pcPointsCoord = new SoCoordinate3();
    pcPointsCoord->ref();
    pcPoints = new SoPointSet();
    pcPoints->ref();
    pcPointsNormal = new SoNormal();  
    pcPointsNormal->ref();
    pcColorMat = new SoMaterial;
    pcColorMat->ref();

    pcPointStyle = new SoDrawStyle();
    pcPointStyle->ref();
    pcPointStyle->style = SoDrawStyle::POINTS;
    pcPointStyle->pointSize = PointSize.getValue();
}

ViewProviderPoints::~ViewProviderPoints()
{
    pcPointsCoord->unref();
    pcPoints->unref();
    pcPointsNormal->unref();
    pcColorMat->unref();
    pcPointStyle->unref();
}

void ViewProviderPoints::onChanged(const App::Property* prop)
{
    if ( prop == &PointSize ) {
        pcPointStyle->pointSize = PointSize.getValue();
    }
    else {
        ViewProviderGeometryObject::onChanged(prop);
    }
}

void ViewProviderPoints::setVertexColorMode(App::PropertyColorList* pcProperty)
{
    const std::vector<App::Color>& val = pcProperty->getValues();
    unsigned long i=0;

    pcColorMat->enableNotify(false);
    pcColorMat->diffuseColor.deleteValues(0);
    pcColorMat->diffuseColor.setNum(val.size());

    for ( std::vector<App::Color>::const_iterator it = val.begin(); it != val.end(); ++it ) {
        pcColorMat->diffuseColor.set1Value(i++, SbColor(it->r, it->g, it->b));
    }

    pcColorMat->enableNotify(true);
    pcColorMat->touch();
}

void ViewProviderPoints::setVertexGreyvalueMode(Points::PropertyGreyValueList* pcProperty)
{
    const std::vector<float>& val = pcProperty->getValues();
    unsigned long i=0;

    pcColorMat->enableNotify(false);
    pcColorMat->diffuseColor.deleteValues(0);
    pcColorMat->diffuseColor.setNum(val.size());

    for ( std::vector<float>::const_iterator it = val.begin(); it != val.end(); ++it ) {
        pcColorMat->diffuseColor.set1Value(i++, SbColor(*it, *it, *it));
    }

    pcColorMat->enableNotify(true);
    pcColorMat->touch();
}

void ViewProviderPoints::setVertexNormalMode(Points::PropertyNormalList* pcProperty)
{
    const std::vector<Base::Vector3f>& val = pcProperty->getValues();
    unsigned long i=0;

    pcPointsNormal->enableNotify(false);
    pcPointsNormal->vector.deleteValues(0);
    pcPointsNormal->vector.setNum(val.size());

    for ( std::vector<Base::Vector3f>::const_iterator it = val.begin(); it != val.end(); ++it ) {
        pcPointsNormal->vector.set1Value(i++, it->x, it->y, it->z);
    }

    pcPointsNormal->enableNotify(true);
    pcPointsNormal->touch();
}

void ViewProviderPoints::attach(App::DocumentObject* pcObj)
{
    // call parent's attach to define display modes
    ViewProviderGeometryObject::attach(pcObj);

    SoGroup* pcPointRoot = new SoGroup();
    SoGroup* pcPointShadedRoot = new SoGroup();
    SoGroup* pcColorShadedRoot = new SoGroup();

    // Hilight for selection
    pcHighlight->addChild(pcPointsCoord);
    pcHighlight->addChild(pcPoints);

    // points part ---------------------------------------------
    pcPointRoot->addChild(pcPointStyle);
    pcPointRoot->addChild(pcShapeMaterial);
    pcPointRoot->addChild(pcHighlight);

    // points shaded ---------------------------------------------
    pcPointShadedRoot->addChild(pcPointStyle);
    pcPointShadedRoot->addChild(pcShapeMaterial);
    pcPointShadedRoot->addChild(pcPointsNormal);
    pcPointShadedRoot->addChild(pcHighlight);

    // color shaded  ------------------------------------------
    pcColorShadedRoot->addChild(pcPointStyle);
    SoMaterialBinding* pcMatBinding = new SoMaterialBinding;
    pcMatBinding->value = SoMaterialBinding::PER_VERTEX_INDEXED;
    pcColorShadedRoot->addChild(pcColorMat);
    pcColorShadedRoot->addChild(pcMatBinding);
    pcColorShadedRoot->addChild(pcHighlight);

    // putting all together with a switch
    addDisplayMaskMode(pcPointRoot, "Point");
    addDisplayMaskMode(pcColorShadedRoot, "Color");
    addDisplayMaskMode(pcPointShadedRoot, "Shaded");
}

void ViewProviderPoints::setDisplayMode(const char* ModeName)
{
  int numPoints = pcPointsCoord->point.getNum();

  if ( strcmp("Color",ModeName)==0 )
  {
    std::map<std::string,App::Property*> Map;
    pcObject->getPropertyMap(Map);
    for( std::map<std::string,App::Property*>::iterator it = Map.begin(); it != Map.end(); ++it )
    {
      Base::Type t = it->second->getTypeId();
      if ( t==App::PropertyColorList::getClassTypeId() )
      {
        App::PropertyColorList* colors = (App::PropertyColorList*)it->second;
        if ( numPoints != colors->getSize() ) {
#ifdef FC_DEBUG
          SoDebugError::postWarning("ViewProviderPoints::setDisplayMode",
                                    "The number of points (%d) doesn't match with the number of colors (%d).", numPoints, colors->getSize());
#endif
          // fallback 
          setDisplayMaskMode("Point");
        } else {
          setVertexColorMode(colors);
          setDisplayMaskMode("Color");
        }
        break;
      }
    }
  }
  else if ( strcmp("Intensity",ModeName)==0 )
  {
    std::map<std::string,App::Property*> Map;
    pcObject->getPropertyMap(Map);
    for( std::map<std::string,App::Property*>::iterator it = Map.begin(); it != Map.end(); ++it )
    {
      Base::Type t = it->second->getTypeId();
      if ( t==Points::PropertyGreyValueList::getClassTypeId() )
      {
        Points::PropertyGreyValueList* greyValues = (Points::PropertyGreyValueList*)it->second;
        if ( numPoints != greyValues->getSize() ) {
#ifdef FC_DEBUG
          SoDebugError::postWarning("ViewProviderPoints::setDisplayMode",
                                    "The number of points (%d) doesn't match with the number of grey values (%d).", numPoints, greyValues->getSize());
#endif
          // Intensity mode is not possible then set the default () mode instead.
          setDisplayMaskMode("Point");
        } else {
          setVertexGreyvalueMode((Points::PropertyGreyValueList*)it->second);
          setDisplayMaskMode("Color");
        }
        break;
      }
    }
  }
  else if ( strcmp("Shaded",ModeName)==0 )
  {
    std::map<std::string,App::Property*> Map;
    pcObject->getPropertyMap(Map);
    for( std::map<std::string,App::Property*>::iterator it = Map.begin(); it != Map.end(); ++it )
    {
      Base::Type t = it->second->getTypeId();
      if ( t==Points::PropertyNormalList::getClassTypeId() )
      {
        Points::PropertyNormalList* normals = (Points::PropertyNormalList*)it->second;
        if ( numPoints != normals->getSize() ) {
#ifdef FC_DEBUG
          SoDebugError::postWarning("ViewProviderPoints::setDisplayMode",
                                    "The number of points (%d) doesn't match with the number of normals (%d).", numPoints, normals->getSize());
#endif
          // fallback 
          setDisplayMaskMode("Point");
        } else {
          setVertexNormalMode(normals);
          setDisplayMaskMode("Shaded");
        }
        break;
      }
    }
  }
  else if ( strcmp("Points",ModeName)==0 )
  {
    setDisplayMaskMode("Point");
  }

  ViewProviderGeometryObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderPoints::getDisplayModes(void) const
{
  std::vector<std::string> StrList;
  StrList.push_back("Points");

  if ( pcObject )
  {
    std::map<std::string,App::Property*> Map;
    pcObject->getPropertyMap(Map);

    for( std::map<std::string,App::Property*>::iterator it = Map.begin(); it != Map.end(); ++it )
    {
      Base::Type t = it->second->getTypeId();
      if ( t==Points::PropertyNormalList::getClassTypeId() )
        StrList.push_back("Shaded");
      else if ( t==Points::PropertyGreyValueList::getClassTypeId() )
        StrList.push_back("Intensity");
      else if ( t==App::PropertyColorList::getClassTypeId() )
        StrList.push_back("Color");
    }
  }

  return StrList;
}

void ViewProviderPoints::updateData(const App::Property* prop)
{
    Gui::ViewProviderGeometryObject::updateData(prop);
    if (prop->getTypeId() == Points::PropertyPointKernel::getClassTypeId()) {
        ViewProviderPointsBuilder builder;
        builder.createPoints(prop, pcPointsCoord, pcPoints);

        // The number of points might have changed, so force also a resize of the Inventor internals
        setActiveMode();
    }
}

QIcon ViewProviderPoints::getIcon() const
{
  static const char * const Points_Feature_xpm[] = {
    "16 16 4 1",
    ".	c none",
    "s	c #000000",
    "b	c #FFFF00",
    "r	c #FF0000",
    "ss.....ss.....bb",
    "ss..ss.ss.....bb",
    "....ss..........",
    "...........bb...",
    ".ss..ss....bb...",
    ".ss..ss.........",
    "........bb....bb",
    "ss......bb....bb",
    "ss..rr......bb..",
    "....rr......bb..",
    "........bb......",
    "..rr....bb..bb..",
    "..rr........bb..",
    ".....rr.........",
    "rr...rr..rr..rr.",
    "rr.......rr..rr."};
  QPixmap px(Points_Feature_xpm);
  return px;
}

bool ViewProviderPoints::setEdit(int)
{
    return true;
}

void ViewProviderPoints::unsetEdit(int)
{
}

void ViewProviderPoints::clipPointsCallback(void * ud, SoEventCallback * n)
{
    // When this callback function is invoked we must in either case leave the edit mode
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
    view->setEditing(false);
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), clipPointsCallback);
    n->setHandled();

    std::vector<SbVec2f> clPoly = view->getGLPolygon();
    if (clPoly.size() < 3)
        return;
    if (clPoly.front() != clPoly.back())
        clPoly.push_back(clPoly.front());

    std::vector<Gui::ViewProvider*> views = view->getViewProvidersOfType(ViewProviderPoints::getClassTypeId());
    for (std::vector<Gui::ViewProvider*>::iterator it = views.begin(); it != views.end(); ++it) {
        ViewProviderPoints* that = static_cast<ViewProviderPoints*>(*it);
        if (that->getEditingMode() > -1) {
            that->finishEditing();
            that->cut(clPoly, *view);
        }
    }

    view->render();
}

void ViewProviderPoints::cut(const std::vector<SbVec2f>& picked, Gui::View3DInventorViewer &Viewer)
{
    // create the polygon from the picked points
    Base::Polygon2D cPoly;
    for (std::vector<SbVec2f>::const_iterator it = picked.begin(); it != picked.end(); ++it) {
        cPoly.Add(Base::Vector2D((*it)[0],(*it)[1]));
    }

    // get a reference to the point feature
    Points::Feature* fea = (Points::Feature*)pcObject;
    const Points::PointKernel& points = fea->Points.getValue();

    SoCamera* pCam = Viewer.getCamera();  
    SbViewVolume  vol = pCam->getViewVolume(); 

    // search for all points inside/outside the polygon
    Points::PointKernel newKernel;
    for ( Points::PointKernel::const_iterator jt = points.begin(); jt != points.end(); ++jt ) {
        SbVec3f pt(jt->x,jt->y,jt->z);

        // project from 3d to 2d
        vol.projectToScreen(pt, pt);
        if (!cPoly.Contains(Base::Vector2D(pt[0],pt[1])))
            newKernel.push_back(*jt);
    }

    if (newKernel.size() == points.size())
        return; // nothing needs to be done

    //Remove the points from the cloud and open a transaction object for the undo/redo stuff
    Gui::Application::Instance->activeDocument()->openCommand("Cut points");

    // sets the points outside the polygon to update the Inventor node
    fea->Points.setValue(newKernel);

    // unset the modified flag because we don't need the features' execute() to be called
    Gui::Application::Instance->activeDocument()->commitCommand();
    fea->purgeTouched();
}

// -------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PointsGui::ViewProviderPython, PointsGui::ViewProviderPoints)
/// @endcond

// explicit template instantiation
template class PointsGuiExport ViewProviderPythonFeatureT<PointsGui::ViewProviderPoints>;
}

// -------------------------------------------------

void ViewProviderPointsBuilder::buildNodes(const App::Property* prop, std::vector<SoNode*>& nodes) const
{
    SoCoordinate3 *pcPointsCoord=0;
    SoPointSet *pcPoints=0;

    if (nodes.empty()) {
        pcPointsCoord = new SoCoordinate3();
        nodes.push_back(pcPointsCoord);
        pcPoints = new SoPointSet();
        nodes.push_back(pcPoints);
    }
    else if (nodes.size() == 2) {
        if (nodes[0]->getTypeId() == SoCoordinate3::getClassTypeId())
            pcPointsCoord = static_cast<SoCoordinate3*>(nodes[0]);
        if (nodes[1]->getTypeId() == SoPointSet::getClassTypeId())
            pcPoints = static_cast<SoPointSet*>(nodes[1]);
    }

    if (pcPointsCoord && pcPoints)
        createPoints(prop, pcPointsCoord, pcPoints);
}

void ViewProviderPointsBuilder::createPoints(const App::Property* prop, SoCoordinate3* coords, SoPointSet* points) const
{
    const Points::PropertyPointKernel* prop_points = static_cast<const Points::PropertyPointKernel*>(prop);
    const Points::PointKernel& cPts = prop_points->getValue();

    // disable the notification, otherwise whenever a point is inserted SoPointSet gets notified
    coords->enableNotify(false);
    coords->point.deleteValues(0);
    coords->point.setNum(cPts.size());

    // get all points
    int idx=0;
    const std::vector<Points::PointKernel::value_type>& kernel = cPts.getBasicPoints();
    for (std::vector<Points::PointKernel::value_type>::const_iterator it = kernel.begin(); it != kernel.end(); ++it, idx++) {
        coords->point.set1Value(idx, (float)it->x, (float)it->y, (float)it->z);
    }

    points->numPoints = cPts.size();
    coords->enableNotify(true);
    coords->touch();
}
