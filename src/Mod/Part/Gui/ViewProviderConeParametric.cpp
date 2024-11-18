/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "ViewProviderConeParametric.h"


using namespace PartGui;
using namespace std;

//**************************************************************************
// Construction/Destruction

PROPERTY_SOURCE(PartGui::ViewProviderConeParametric, PartGui::ViewProviderPrimitive)


ViewProviderConeParametric::ViewProviderConeParametric()
{
  sPixmap = "Part_Cone_Parametric";
}

ViewProviderConeParametric::~ViewProviderConeParametric() = default;



// **********************************************************************************

std::vector<std::string> ViewProviderConeParametric::getDisplayModes() const
{
  // get the modes of the father
  std::vector<std::string> StrList;

  // add your own modes
  StrList.emplace_back("Flat Lines");
  StrList.emplace_back("Shaded");
  StrList.emplace_back("Wireframe");
  StrList.emplace_back("Points");

  return StrList;
}
