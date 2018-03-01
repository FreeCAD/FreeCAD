/***************************************************************************
 *   Copyright (c) 2004 Juergen Riegel <juergen.riegel@web.de>             *
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


#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Command.h>

#include <Mod/Part/App/PartFeature.h>
#include <Gui/Application.h>

#include "ViewProvider.h"


using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderPart, PartGui::ViewProviderPartExt)


ViewProviderPart::ViewProviderPart()
{
}

ViewProviderPart::~ViewProviderPart()
{
}

bool ViewProviderPart::doubleClicked(void)
{
    std::string Msg("Edit ");
    Msg += this->pcObject->Label.getValue();
    try {
        Gui::Command::openCommand(Msg.c_str());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.setEdit('%s',0)",
                                this->pcObject->getNameInDocument());
        return true;
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        return false;
    }
}

void ViewProviderPart::applyColorAndTransparency(const Part::ShapeHistory& hist,
                                  const std::vector<App::Color>& colBase,
								  const float& transparency,
                                  std::vector<App::Color>& colBool)
{
	float tr = transparency/100; // transparency comes in percent
    std::map<int, std::vector<int> >::const_iterator jt;
    // apply color from modified faces
    for (jt = hist.shapeMap.begin(); jt != hist.shapeMap.end(); ++jt) {
        std::vector<int>::const_iterator kt;
        for (kt = jt->second.begin(); kt != jt->second.end(); ++kt) {
            colBool[*kt] = colBase[jt->first];
			if (tr > 0.0) colBool[*kt].a = tr;
        }
    }
}

void ViewProviderPart::applyColor(const Part::ShapeHistory& hist,
                                 const std::vector<App::Color>& colBase,
                                 std::vector<App::Color>& colBool)
{
    std::map<int, std::vector<int> >::const_iterator jt;
    // apply color from modified faces
    for (jt = hist.shapeMap.begin(); jt != hist.shapeMap.end(); ++jt) {
        std::vector<int>::const_iterator kt;
        for (kt = jt->second.begin(); kt != jt->second.end(); ++kt) {
            colBool[*kt] = colBase[jt->first];
        }
    }
}

void ViewProviderPart::applyTransparency(const float& transparency,
                                 std::vector<App::Color>& colors)
{
	float tr = transparency/100; // transparency comes in percent
    if (transparency != 0.0) {
        // transparency has been set object-wide
        std::vector<App::Color>::iterator j;
        for (j = colors.begin(); j != colors.end(); ++j) {
           j->a = tr; 
        }
    }
}
void ViewProviderPart::updateViewColorAndTransparency() {
	Part::Feature* objPart = static_cast<Part::Feature*>(getObject());
	App::Color shapeColor = ShapeColor.getValue();
	const float curTrans = Transparency.getValue();
	
	if (!objPart  || !objPart->isDerivedPart() || usePartColors()) {
		App::Color nColor = shapeColor;
		nColor.a=curTrans/100.0;
		DiffuseColor.setValue(nColor);
		return;
	}
	
	std::vector<App::DocumentObject*> sources = objPart->getChildren();
	if (sources.size()==0)
		return;
	
	const std::vector<Part::ShapeHistory>& hist = objPart->History.getValues();
	const TopoDS_Shape& partShape = objPart->Shape.getValue();
	TopTools_IndexedMapOfShape partMap;
	TopExp::MapShapes(partShape, TopAbs_FACE, partMap);

	std::vector<App::Color> colPart;
	
	// printf("sizes %ld %ld \n", hist.size(), sources.size());
	
	if (hist.size() != sources.size()) {
		// there is no history that should be calculated while executing the feature
		// objPart->execute();
		// objPart->recompute();
		objPart->touch(); // Temporary before a solution is found. The object will be recomputed by F5
		return;
	} else {
		// get the color of the first face of the first shape for the new faces created by the operation
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
					applyColor(hist[index], colBase, colPart);
				}
				else if (!colBase.empty() && colBase[0] != shapeColor) {
					colBase.resize(baseMap.Extent(), colBase[0]);
					applyColor(hist[index], colBase, colPart);
				}
			}
		}
		if (curTrans > 0.0) // that is the criteria to apply a global transparency
			applyTransparency( curTrans, colPart);
	}
	DiffuseColor.setValues(colPart);

}
bool ViewProviderPart::needHistory(void) { 
	Part::Feature* objPart = static_cast<Part::Feature*>(getObject());
	return objPart->isDerivedPart() && !usePartColors(); 
}

bool ViewProviderPart::usePartColors(void){
	App::Color shapeColor = ShapeColor.getValue();
	// printf("ViewProviderPart::usePartColors(void) %x %s\n",shapeColor.getPackedValue(), (shapeColor != 0x000000) ? "true" : "false");
	return shapeColor != 0x000000;
}

std::vector<App::DocumentObject*> ViewProviderPart::claimChildren(void) const {
        Part::Feature* objPart = static_cast<Part::Feature*>(getObject());
		return objPart->getChildren();
}

void ViewProviderPart::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPartExt::updateData(prop);
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


// ----------------------------------------------------------------------------

void ViewProviderShapeBuilder::buildNodes(const App::Property* , std::vector<SoNode*>& ) const
{
}

void ViewProviderShapeBuilder::createShape(const App::Property* , SoSeparator* ) const
{
}
