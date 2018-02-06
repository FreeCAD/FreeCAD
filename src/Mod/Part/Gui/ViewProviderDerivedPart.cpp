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
# include <BRepAlgoAPI_BooleanOperation.hxx>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_ListOfShape.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include "ViewProviderDerivedPart.h"
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
// #include <Mod/Part/App/FeaturePartBoolean.h>
// #include <Mod/Part/App/FeaturePartFuse.h>
// #include <Mod/Part/App/FeaturePartCommon.h>
#include <Mod/Part/App/FeatureDerivedPart.h>

using namespace PartGui;
PROPERTY_SOURCE(PartGui::ViewProviderDerivedPart,PartGui::ViewProviderPart)

ViewProviderDerivedPart::ViewProviderDerivedPart()
{
}


ViewProviderDerivedPart::~ViewProviderDerivedPart()
{
}

void ViewProviderDerivedPart::updateViewColorAndTransparency () {
	App::Color shapeColor = ShapeColor.getValue();
	const float curTrans = Transparency.getValue();
	if (shapeColor != 0x000000) {  // this is the criteria to choose the color of the part or of it components.
		// set a uniform color and transparency (the default for simple parts)
		ViewProviderPartExt::updateViewColorAndTransparency();
		return;
	}
	
	Part::FeatureDerivedPart* objPart = static_cast<Part::FeatureDerivedPart*>(getObject());
	if (!objPart) 
		return;
	std::vector<App::DocumentObject*> sources = claimChildren();
	if (sources.size()==0)
		return;
	const std::vector<Part::ShapeHistory>& hist = objPart->History.getValues();
	const TopoDS_Shape& partShape = objPart->Shape.getValue();
	TopTools_IndexedMapOfShape partMap;
	TopExp::MapShapes(partShape, TopAbs_FACE, partMap);

	std::vector<App::Color> colPart;
	
	if (hist.size() != sources.size()) {
		// there is no history that should be calcultated while executing the feaure
		// objPart->execute();
		// objPart->recompute();
		objPart->touch();
		return;
	} else {
		// get the color of the first face of the first shape for the new faces created by the opération
		shapeColor=static_cast<PartGui::ViewProviderPart*>(
						Gui::Application::Instance->getViewProvider(dynamic_cast<Part::Feature*>(*sources.begin())) 
					) ->DiffuseColor.getValues()[0];
		colPart.resize(partMap.Extent(), shapeColor);
		int index=0;
		for (std::vector<App::DocumentObject*>::iterator it = sources.begin(); it != sources.end(); ++it, ++index) {
			if (!*it)
				continue;
			Part::Feature* objBase = dynamic_cast<Part::Feature*>(*it);
			
			const TopoDS_Shape& baseShape = objBase->Shape.getValue();
 
			TopTools_IndexedMapOfShape baseMap;
			TopExp::MapShapes(baseShape, TopAbs_FACE, baseMap);

			Gui::ViewProvider* vpBase = Gui::Application::Instance->getViewProvider(objBase);
			if (vpBase) {
				std::vector<App::Color> colBase = static_cast<PartGui::ViewProviderPart*>(vpBase)->DiffuseColor.getValues();
				if (static_cast<int>(colBase.size()) == baseMap.Extent()) {
					// PartGui::ViewProviderPart::applyColorAndTransparency(hist[index], colBase, curTrans, colPart);
					PartGui::ViewProviderPart::applyColor(hist[index], colBase, colPart);
				}
				else if (!colBase.empty() && colBase[0] != shapeColor) {
					colBase.resize(baseMap.Extent(), colBase[0]);
					// PartGui::ViewProviderPart::applyColorAndTransparency(hist[index], colBase, curTrans, colPart);
					PartGui::ViewProviderPart::applyColor(hist[index], colBase, colPart);
				}
			}
		}
		if (curTrans > 0.0) // that is the criteria to apply a global transparency
			PartGui::ViewProviderPart::applyTransparency( curTrans, colPart);
	}
	DiffuseColor.setValues(colPart);
}

void ViewProviderDerivedPart::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    if (prop->getTypeId() == Part::PropertyShapeHistory::getClassTypeId()) {
 		updateViewColorAndTransparency();
    }
    else if (prop->getTypeId().isDerivedFrom(App::PropertyLinkList::getClassTypeId())) {
        const std::vector<App::DocumentObject*>& pShapes = static_cast<const App::PropertyLinkList*>(prop)->getValues();
        for (std::vector<App::DocumentObject*>::const_iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
            if (*it) {
                Gui::Application::Instance->hideViewProvider(*it);
			}
        }
	}
    else if (prop->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId())) {
        App::DocumentObject *pBase = static_cast<const App::PropertyLink*>(prop)->getValue();
        if (pBase)
            Gui::Application::Instance->hideViewProvider(pBase);
    }
}

std::vector<App::DocumentObject*> ViewProviderDerivedPart::claimChildren(void) const {
        Part::FeatureDerivedPart* objPart = static_cast<Part::FeatureDerivedPart*>(getObject());
		return objPart->getChildren();
}