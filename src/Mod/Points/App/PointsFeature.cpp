/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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
#endif
#include <vector>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Stream.h>
#include <Base/Writer.h>


#include "PointsFeature.h"

using namespace Points;


//===========================================================================
// Feature
//===========================================================================

PROPERTY_SOURCE(Points::Feature, App::GeoFeature)

Feature::Feature() 
{
    ADD_PROPERTY(Points, (PointKernel()));
}

Feature::~Feature()
{
}

App::DocumentObjectExecReturn *Feature::execute(void)
{
    return App::DocumentObject::StdReturn;
}

void Feature::Restore(Base::XMLReader &reader)
{
    GeoFeature::Restore(reader);
}

void Feature::RestoreDocFile(Base::Reader &reader)
{
    // This gets only invoked if a points file has been added from Restore()
    Points.RestoreDocFile(reader);
}

void Feature::onChanged(const App::Property* prop)
{
    // if the placement has changed apply the change to the point data as well
    if (prop == &this->Placement) {
        PointKernel& pts = const_cast<PointKernel&>(this->Points.getValue());
        pts.setTransform(this->Placement.getValue().toMatrix());
    }
    // if the point data has changed check and adjust the transformation as well
    else if (prop == &this->Points) {
        Base::Placement p;
        p.fromMatrix(this->Points.getValue().getTransform());
        if (p != this->Placement.getValue())
            this->Placement.setValue(p);
    }
    
    GeoFeature::onChanged(prop);
}

// ------------------------------------------------------------------

PROPERTY_SOURCE(Points::Export, Points::Feature)

Export::Export(void)
{
    ADD_PROPERTY(Sources ,(0));
    ADD_PROPERTY(FileName,(""));
    ADD_PROPERTY(Format  ,(""));
}

App::DocumentObjectExecReturn *Export::execute(void)
{
  // ask for write permission
  Base::FileInfo fi(FileName.getValue());
  Base::FileInfo di(fi.dirPath().c_str());
  if ((fi.exists() && !fi.isWritable()) || !di.exists() || !di.isWritable())
  {
      return new App::DocumentObjectExecReturn("No write permission for file");
  }

  Base::ofstream str(fi, std::ios::out | std::ios::binary);

  if (fi.hasExtension("asc"))
  {
    const std::vector<App::DocumentObject*>& features = Sources.getValues();
    for ( std::vector<App::DocumentObject*>::const_iterator it = features.begin(); it != features.end(); ++it )
    {
      Feature *pcFeat  = dynamic_cast<Feature*>(*it);
      const PointKernel& kernel = pcFeat->Points.getValue();

      str << "# " << pcFeat->getNameInDocument() << " Number of points: " << kernel.size() << std::endl;
      for ( PointKernel::const_iterator it = kernel.begin(); it != kernel.end(); ++it )
        str << it->x << " " << it->y << " " << it->z << std::endl;
    }
  }
  else
  {
      return new App::DocumentObjectExecReturn("File format not supported");
  }

  return App::DocumentObject::StdReturn;
}

// ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Points::FeaturePython, Points::Feature)
template<> const char* Points::FeaturePython::getViewProviderName(void) const {
    return "PointsGui::ViewProviderPython";
}
/// @endcond

// explicit template instantiation
template class PointsExport FeaturePythonT<Points::Feature>;
}

