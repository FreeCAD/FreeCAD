/***************************************************************************
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMaterialBinding.h>
# include <Inventor/nodes/SoScale.h>
# include <Inventor/nodes/SoSeparator.h>
#endif

#include <App/Document.h>
#include <App/Datums.h>
#include <App/Origin.h>

#include "ViewProviderDatum.h"
#include "Inventor/SoAxisCrossKit.h"
#include "SoFCSelection.h"
#include "ViewProviderCoordinateSystem.h"

#include <Inventor/So3DAnnotation.h>


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderDatum, Gui::ViewProviderGeometryObject)

ViewProviderDatum::ViewProviderDatum() {
    // Set default color for origin (light-blue)
    ShapeAppearance.setDiffuseColor(ViewProviderCoordinateSystem::defaultColor);
    Transparency.setValue(0);
    BoundingBox.setStatus(App::Property::Hidden, true); // Hide Boundingbox from the user due to it doesn't make sense

    // Create node for scaling the origin
    soScale = new SoShapeScale();

    // Create the separator filled by inherited classes
    pRoot = new SoSeparator();
    pRoot->ref();

    lineThickness = 2.0;
}


ViewProviderDatum::~ViewProviderDatum() {
    pRoot->unref();
}


void ViewProviderDatum::attach(App::DocumentObject* pcObject)
{
    ViewProviderGeometryObject::attach(pcObject);


    // Create an external separator
    auto sep = new SoSeparator();

    // Add material from the base class
    sep->addChild(pcShapeMaterial);

    // Bind same material to all part
    auto matBinding = new SoMaterialBinding;
    matBinding->value = SoMaterialBinding::OVERALL;
    sep->addChild(matBinding);

    // Setup font size
    auto font = new SoFont();
    static const float size = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")->GetFloat("DatumFontSize", 15.0);
    font->size.setValue(size);
    sep->addChild(font);

    // Create the selection node
    auto highlight = new SoFCSelection();
    highlight->applySettings();
    if (!Selectable.getValue()) {
        highlight->selectionMode = Gui::SoFCSelection::SEL_OFF;
    }
    highlight->objectName = getObject()->getNameInDocument();
    highlight->documentName = getObject()->getDocument()->getName();
    highlight->style = SoFCSelection::EMISSIVE_DIFFUSE;

    // Visible features
    auto visible = new SoSeparator();
    // Style for normal (visible) lines
    auto style = new SoDrawStyle();
    style->lineWidth = lineThickness;
    visible->addChild(style);

    // Visible lines
    visible->addChild(pRoot);

    // Hidden features
    auto hidden = new So3DAnnotation();

    // Style for hidden lines
    style = new SoDrawStyle();
    style->lineWidth = lineThickness;
    style->linePattern.setValue(0xFF00); // (dash-skip)
    hidden->addChild(style);

    // Hidden lines
    hidden->addChild(pRoot);

    visible->addChild(hidden);

    sep->addChild(visible);

    soScale->setPart("shape", sep);
    resetTemporarySize();

    highlight->addChild(soScale);

    addDisplayMaskMode(highlight, "Base");
}

void ViewProviderDatum::setTemporaryScale(double factor)
{
    soScale->scaleFactor = soScale->scaleFactor.getValue() * factor;
}

void ViewProviderDatum::resetTemporarySize()
{
    float sz = App::GetApplication()
        .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
        ->GetFloat("LocalCoordinateSystemSize", 1.0);  // NOLINT

    soScale->scaleFactor = sz;
}

void ViewProviderDatum::onChanged(const App::Property* prop) {
    ViewProviderGeometryObject::onChanged(prop);
}

std::vector<std::string> ViewProviderDatum::getDisplayModes() const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.emplace_back("Base");
    return StrList;
}

void ViewProviderDatum::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0) {
        setDisplayMaskMode("Base");
    }
    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

bool ViewProviderDatum::onDelete(const std::vector<std::string>&) {
    auto feat = static_cast <App::DatumElement*>(getObject());
    // Forbid deletion if there is an origin this feature belongs to
    return !feat->getLCS();
}
