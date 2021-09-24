/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <QMessageBox>
# include <QMenu>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
#endif

#include "Utils.h"
#include "ViewProviderLoft.h"
//#include "TaskLoftParameters.h"
#include "TaskLoftParameters.h"
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureLoft.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderLoft,PartDesignGui::ViewProvider)

ViewProviderLoft::ViewProviderLoft()
{
}

ViewProviderLoft::~ViewProviderLoft()
{

}

void ViewProviderLoft::cleanup()
{
    for (std::pair<Part::Feature*, int> undo : undoQueue){
        highlightWire(false, undo.first, undo.second);
    }
    undoQueue.clear();
}

std::vector<App::DocumentObject*> ViewProviderLoft::claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp;

    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(getObject());

    App::DocumentObject* sketch = pcLoft->getVerifiedSketch(true);
    if (sketch != NULL)
        temp.push_back(sketch);

    for(App::DocumentObject* obj : pcLoft->Sections.getValues()) {
        if (obj != NULL && obj->isDerivedFrom(Part::Part2DObject::getClassTypeId()))
            temp.push_back(obj);
    }

    return temp;
}

void ViewProviderLoft::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit loft"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

bool ViewProviderLoft::doubleClicked(void)
{
    return PartDesignGui::setEdit(pcObject);
}

bool ViewProviderLoft::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default)
        setPreviewDisplayMode(true);

    return ViewProviderAddSub::setEdit(ModNum);
}

TaskDlgFeatureParameters* ViewProviderLoft::getEditDialog() {
    return new TaskDlgLoftParameters(this);
}


void ViewProviderLoft::unsetEdit(int ModNum) {
    setPreviewDisplayMode(false);
    ViewProviderAddSub::unsetEdit(ModNum);
}


bool ViewProviderLoft::onDelete(const std::vector<std::string> & /*s*/)
{/*
    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(getObject());

    // get the Sketch
    Sketcher::SketchObject *pcSketch = 0;
    if (pcLoft->Sketch.getValue())
        pcSketch = static_cast<Sketcher::SketchObject*>(pcLoft->Sketch.getValue());

    // if abort command deleted the object the sketch is visible again
    if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
        Gui::Application::Instance->getViewProvider(pcSketch)->show();

    return ViewProvider::onDelete(s);*/
    return true;
}

void ViewProviderLoft::highlightReferences(const bool /*on*/, bool /*auxiliary*/)
{
  /*  PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(getObject());
    Part::Feature* base;
    if(!auxiliary)
        base = static_cast<Part::Feature*>(pcLoft->Spine.getValue());
    else
        base = static_cast<Part::Feature*>(pcLoft->AuxillerySpine.getValue());

    if (base == NULL) return;
    PartGui::ViewProviderPart* svp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(base));
    if (svp == NULL) return;

    std::vector<std::string> edges;
    if(!auxiliary)
        edges = pcLoft->Spine.getSubValuesStartsWith("Edge");
    else
        edges = pcLoft->AuxillerySpine.getSubValuesStartsWith("Edge");

    if (on) {
         if (!edges.empty() && originalLineColors.empty()) {
            TopTools_IndexedMapOfShape eMap;
            TopExp::MapShapes(base->Shape.getValue(), TopAbs_EDGE, eMap);
            originalLineColors = svp->LineColorArray.getValues();
            std::vector<App::Color> colors = originalLineColors;
            colors.resize(eMap.Extent(), svp->LineColor.getValue());

            for (std::string e : edges) {
                int idx = atoi(e.substr(4).c_str()) - 1;
                if (idx < colors.size())
                    colors[idx] = App::Color(1.0,0.0,1.0); // magenta
            }
            svp->LineColorArray.setValues(colors);
        }
    } else {
        if (!edges.empty() && !originalLineColors.empty()) {
            svp->LineColorArray.setValues(originalLineColors);
            originalLineColors.clear();
        }
    }*/
}
/** highlights a wire in the sketch object by setting its edges to which_color if on=true
 *  else sets that wire back to original line colors
 */

void ViewProviderLoft::highlightWire(const bool on, Part::Feature* sketch, int which_wire, App::Color which_color)
{
    if (sketch == NULL)
        return;
    if (!sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId()))
        return;

    PartGui::ViewProviderPart* svp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(sketch));
    if (svp == NULL)
        return;

    TopoDS_Wire wire; //find the wire in the sketch
    TopExp_Explorer exp_wire;
    size_t i = 0;
    for (exp_wire.Init(sketch->Shape.getValue(), TopAbs_WIRE); exp_wire.More(); exp_wire.Next(), ++i) {
        if (which_wire == (int)i){
            wire = TopoDS::Wire(exp_wire.Current());
            break;
        }
    }

    TopExp_Explorer exp_edge; //find the edges in this wire
    std::vector<TopoDS_Edge> edges;
    for (exp_edge.Init(wire, TopAbs_EDGE); exp_edge.More(); exp_edge.Next()) {
        edges.push_back(TopoDS::Edge(exp_edge.Current()));
    }

    TopExp_Explorer exp_shape; //name the edges in the wire
    std::vector<std::string> edgeNames;
    int ii=1;
    for (exp_shape.Init(sketch->Shape.getValue(), TopAbs_EDGE); exp_shape.More(); exp_shape.Next()){
        for (auto edge : edges){
            if (edge == TopoDS::Edge(exp_shape.Current())){
                std::ostringstream strm;
                strm << "Edge" << ii;
                edgeNames.push_back(strm.str());
            }
        }
        ii++;
    }

    if (on) {
        if (!edgeNames.empty()) {
           TopTools_IndexedMapOfShape eMap;
           TopExp::MapShapes(sketch->Shape.getValue(), TopAbs_EDGE, eMap);
           if (originalLineColors[sketch].empty()){
               originalLineColors[sketch] = svp->LineColorArray.getValues();
           }
           std::vector<App::Color> colors = svp->LineColorArray.getValues();
           colors.resize(eMap.Extent(), svp->LineColor.getValue());

           for (std::string e : edgeNames) {
               int idx = atoi(e.substr(4).c_str()) - 1;
               if (idx < (int) colors.size())
                   colors[idx] = which_color;
           }
           svp->LineColorArray.setValues(colors);
           undoQueue.push_back(std::pair<Part::Feature*, int>(sketch, which_wire)); //undo when closing dialog
       }
   } else {
       if (!edgeNames.empty() && !originalLineColors[sketch].empty()) {
           svp->LineColorArray.setValues(originalLineColors[sketch]);
           originalLineColors[sketch].clear();
       }
    }
}

QIcon ViewProviderLoft::getIcon(void) const {
    QString str = QString::fromLatin1("PartDesign_");
    auto* prim = static_cast<PartDesign::Loft*>(getObject());
    if(prim->getAddSubType() == PartDesign::FeatureAddSub::Additive)
        str += QString::fromLatin1("Additive");
    else
        str += QString::fromLatin1("Subtractive");

    str += QString::fromLatin1("Loft.svg");
    return PartDesignGui::ViewProvider::mergeGreyableOverlayIcons(Gui::BitmapFactory().pixmap(str.toStdString().c_str()));
}

