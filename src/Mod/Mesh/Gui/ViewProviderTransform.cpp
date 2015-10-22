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
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoNormalBinding.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/manips/SoTransformerManip.h>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <App/Application.h>
#include <Gui/Selection.h>
#include <Gui/SoFCSelection.h>
#include <Base/Sequencer.h>

#include "ViewProvider.h"
#include "ViewProviderTransform.h"

#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/Core/Iterator.h>

using namespace MeshGui;
using Mesh::Feature;


PROPERTY_SOURCE(MeshGui::ViewProviderMeshTransform, MeshGui::ViewProviderMesh)

 
ViewProviderMeshTransform::ViewProviderMeshTransform()
{
  pcTransformerDragger = new SoTransformerManip();
  pcTransformerDragger->ref();
}

ViewProviderMeshTransform::~ViewProviderMeshTransform()
{
  pcTransformerDragger->unref();
}

void ViewProviderMeshTransform::attach(App::DocumentObject *pcFeat)
{
  // creats the standard viewing modes
  ViewProviderMesh::attach(pcFeat);

  SoSeparator* pcEditRoot = new SoSeparator();

  // flat shaded (Normal) ------------------------------------------
  SoDrawStyle *pcFlatStyle = new SoDrawStyle();
  pcFlatStyle->style = SoDrawStyle::FILLED;
  SoNormalBinding* pcBinding = new SoNormalBinding();
	pcBinding->value=SoNormalBinding::PER_FACE;

  pcEditRoot->addChild(pcTransformerDragger);
  pcEditRoot->addChild(pcFlatStyle);
  pcEditRoot->addChild(pcShapeMaterial);
  pcEditRoot->addChild(pcBinding);
  pcEditRoot->addChild(pcHighlight);

  // adding to the switch
  addDisplayMaskMode(pcEditRoot, "Edit");
}

void ViewProviderMeshTransform::updateData(const App::Property* prop)
{
  ViewProviderMesh::updateData(prop);
}

void ViewProviderMeshTransform::setDisplayMode(const char* ModeName)
{
  if ( strcmp("Transform",ModeName) == 0 )
    setDisplayMaskMode("Edit");
  ViewProviderMesh::setDisplayMode(ModeName);
}

const char* ViewProviderMeshTransform::getDefaultDisplayMode() const
{
  return "Transform";
}

std::vector<std::string> ViewProviderMeshTransform::getDisplayModes(void) const
{
  std::vector<std::string> StrList = ViewProviderMesh::getDisplayModes();
  StrList.push_back("Transform");
  return StrList;
}


