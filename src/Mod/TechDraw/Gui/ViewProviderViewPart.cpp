/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
//#include <Base/Exception.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewMulti.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/LineGroup.h>

#include<Mod/TechDraw/App/DrawPage.h>
#include "ViewProviderViewPart.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderViewPart, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderViewPart::ViewProviderViewPart()
{
    sPixmap = "TechDraw_Tree_View";

    static const char *group = "Lines";
    static const char *dgroup = "Decoration";
    static const char *hgroup = "Highlight";

    //default line weights
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                                    GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    std::string lgName = hGrp->GetASCII("LineGroup","FC 0.70mm");
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgName);

    double weight = lg->getWeight("Thick");
    ADD_PROPERTY_TYPE(LineWidth,(weight),group,App::Prop_None,"The thickness of visible lines (line groups xx.2");

    weight = lg->getWeight("Thin");
    ADD_PROPERTY_TYPE(HiddenWidth,(weight),group,App::Prop_None,"The thickness of hidden lines, if enabled (line groups xx.1)");

    weight = lg->getWeight("Graphic");
    ADD_PROPERTY_TYPE(IsoWidth,(weight),group,App::Prop_None,"The thickness of isoparameter lines, if enabled");

    weight = lg->getWeight("Extra");
    ADD_PROPERTY_TYPE(ExtraWidth,(weight),group,App::Prop_None,"The thickness of LineGroup Extra lines, if enabled");
    delete lg;                            //Coverity CID 174664

    //decorations
    ADD_PROPERTY_TYPE(HorizCenterLine ,(false),dgroup,App::Prop_None,"Show a horizontal centerline through view");
    ADD_PROPERTY_TYPE(VertCenterLine ,(false),dgroup,App::Prop_None,"Show a vertical centerline through view");
    ADD_PROPERTY_TYPE(ArcCenterMarks ,(true),dgroup,App::Prop_None,"Center marks on/off");
    ADD_PROPERTY_TYPE(CenterScale,(2.0),dgroup,App::Prop_None,"Center mark size adjustment, if enabled");

    //properties that affect Section Line
    ADD_PROPERTY_TYPE(ShowSectionLine ,(true)    ,dgroup,App::Prop_None,"Show/hide section line if applicable");
    
    //properties that affect Detail Highlights
    ADD_PROPERTY_TYPE(HighlightAdjust,(0.0),hgroup,App::Prop_None,"Adjusts the rotation of the Detail highlight");
}

ViewProviderViewPart::~ViewProviderViewPart()
{

}

void ViewProviderViewPart::updateData(const App::Property* prop)
{
    ViewProviderDrawingView::updateData(prop);
}

void ViewProviderViewPart::onChanged(const App::Property* prop)
{
    if (prop == &(LineWidth)   ||
        prop == &(HiddenWidth) ||
        prop == &(IsoWidth) ||
        prop == &(ExtraWidth) ||
        prop == &(HighlightAdjust) ||
        prop == &(ArcCenterMarks) ||
        prop == &(CenterScale) ||
        prop == &(ShowSectionLine)  ||
        prop == &(HorizCenterLine)  ||
        prop == &(VertCenterLine) ) {
        // redraw QGIVP
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
     }

    ViewProviderDrawingView::onChanged(prop);
}


void ViewProviderViewPart::attach(App::DocumentObject *pcFeat)
{
    TechDraw::DrawViewMulti* dvm = dynamic_cast<TechDraw::DrawViewMulti*>(pcFeat);
    if (dvm != nullptr) {
        sPixmap = "TechDraw_Tree_Multi";
    }

    ViewProviderDrawingView::attach(pcFeat);
}

void ViewProviderViewPart::setDisplayMode(const char* ModeName)
{
    ViewProviderDrawingView::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderViewPart::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDrawingView::getDisplayModes();

    return StrList;
}


std::vector<App::DocumentObject*> ViewProviderViewPart::claimChildren(void) const
{
    // Collect any child Document Objects and put them in the right place in the Feature tree
    // valid children of a ViewPart are:
    //    - Dimensions
    //    - Hatches
    //    - GeomHatches
    std::vector<App::DocumentObject*> temp;
    const std::vector<App::DocumentObject *> &views = getViewPart()->getInList();
    try {
      for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {
          if((*it)->getTypeId().isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())) {
              //TODO: make a list, then prune it.  should be faster?
              bool skip = false;
              std::string dimName = (*it)->getNameInDocument();
              for (auto& t: temp) {                              //only add dim once even if it references 2 geometries
                  std::string tName = t->getNameInDocument();
                  if (dimName == tName) {
                      skip = true;
                      break;
                  }
              }
              if (!skip) {
                  temp.push_back(*it);
              }
          } else if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawHatch::getClassTypeId())) {
              temp.push_back((*it));
          } else if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawGeomHatch::getClassTypeId())) {
              temp.push_back((*it));
          }
      }
      return temp;
    } catch (...) {
        std::vector<App::DocumentObject*> tmp;
        return tmp;
    }
}

TechDraw::DrawViewPart* ViewProviderViewPart::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewPart*>(pcObject);
}

TechDraw::DrawViewPart* ViewProviderViewPart::getViewPart() const
{
    return getViewObject();
}
