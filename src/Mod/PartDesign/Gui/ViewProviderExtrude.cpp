/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <QMenu>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Mod/PartDesign/App/FeatureExtrude.h>
#include <Mod/Part/Gui/ReferenceHighlighter.h>

#include "TaskExtrudeParameters.h"
#include "ViewProviderExtrude.h"


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderExtrude, PartDesignGui::ViewProviderSketchBased)

void PartDesignGui::ViewProviderExtrude::highlightShapeFaces(const std::vector<std::string>& faces)
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    auto base = static_cast<Part::Feature*>(extrude->UpToShape.getValue());

    auto baseViewProvider =
        dynamic_cast<PartGui::ViewProviderPart*>(Gui::Application::Instance->getViewProvider(base));

    if (!baseViewProvider) {
        return;
    }

    baseViewProvider->unsetHighlightedFaces();
    baseViewProvider->updateView();

    if (faces.size() > 0) {
        std::vector<App::Material> materials = baseViewProvider->ShapeAppearance.getValues();

        auto color = baseViewProvider->ShapeAppearance.getDiffuseColor();

        PartGui::ReferenceHighlighter highlighter(base->Shape.getValue(), color);
        highlighter.getFaceMaterials(faces, materials);

        baseViewProvider->setHighlightedFaces(materials);
    }
}
