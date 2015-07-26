/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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
# include <QVariant>
# include <QMenu>
# include <QAction>
# include <QMessageBox>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopExp.hxx>
#endif

#include "ViewProviderDressUp.h"
#include "TaskDressUpParameters.h"
#include <Mod/PartDesign/App/FeatureDressUp.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDressUp,PartDesignGui::ViewProvider)


void ViewProviderDressUp::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    // TODO check if this gets a sane translation (2015-07-26, Fat-Zer)
    act = menu->addAction(QObject::tr((std::string("Edit ") + featureName() + " feature").c_str()), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member);
}


const std::string & ViewProviderDressUp::featureName() const {
    static const std::string name = "Undefined";
    return name;
}


void ViewProviderDressUp::highlightReferences(const bool on)
{
    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(getObject());
    Part::Feature* base = pcDressUp->getBaseObject (/*silent =*/ true);
    if (base == NULL) return;
    PartGui::ViewProviderPart* vp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(base));
    if (vp == NULL) return;

    std::vector<std::string> faces = pcDressUp->Base.getSubValuesStartsWith("Face");
    std::vector<std::string> edges = pcDressUp->Base.getSubValuesStartsWith("Edge");

    if (on) {        
        if (!faces.empty() && originalFaceColors.empty()) {
            TopTools_IndexedMapOfShape fMap;
            TopExp::MapShapes(base->Shape.getValue(), TopAbs_FACE, fMap);

            originalFaceColors = vp->DiffuseColor.getValues();
            std::vector<App::Color> colors = originalFaceColors;
            colors.resize(fMap.Extent(), ShapeColor.getValue());

            for (std::vector<std::string>::const_iterator f = faces.begin(); f != faces.end(); ++f) {
                int idx = atoi(f->substr(4).c_str()) - 1;
                if (idx < colors.size())
                    colors[idx] = App::Color(1.0,0.0,1.0); // magenta
            }
            vp->DiffuseColor.setValues(colors);
        } 
        if (!edges.empty() && originalLineColors.empty()) {
            TopTools_IndexedMapOfShape eMap;
            TopExp::MapShapes(base->Shape.getValue(), TopAbs_EDGE, eMap);
            originalLineColors = vp->LineColorArray.getValues();
            std::vector<App::Color> colors = originalLineColors;
            colors.resize(eMap.Extent(), LineColor.getValue());

            for (std::vector<std::string>::const_iterator e = edges.begin(); e != edges.end(); ++e) {
                int idx = atoi(e->substr(4).c_str()) - 1;
                if (idx < colors.size())
                    colors[idx] = App::Color(1.0,0.0,1.0); // magenta
            }
            vp->LineColorArray.setValues(colors);
        }
    } else {
        if (!faces.empty() && !originalFaceColors.empty()) {
            vp->DiffuseColor.setValues(originalFaceColors);
            originalFaceColors.clear();
        } 
        if (!edges.empty() && !originalLineColors.empty()) {
            vp->LineColorArray.setValues(originalLineColors);
            originalLineColors.clear();
        }
    }
}

