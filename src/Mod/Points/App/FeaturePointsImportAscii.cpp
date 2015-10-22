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

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>

#include "PointsAlgos.h"
#include "Points.h"
#include "FeaturePointsImportAscii.h"


using namespace Points;

PROPERTY_SOURCE(Points::ImportAscii, Points::Feature)

ImportAscii::ImportAscii(void)
{
  ADD_PROPERTY(FileName,(""));
}

short ImportAscii::mustExecute() const
{
    if (FileName.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *ImportAscii::execute(void)
{

    // ask for read permisson
    Base::FileInfo fi(FileName.getValue());
    if (!fi.isReadable()) {
        std::string error = std::string("Cannot open file ") + FileName.getValue();
        return new App::DocumentObjectExecReturn(error);
    }

    PointKernel kernel;
    PointsAlgos::Load(kernel,FileName.getValue());
    Points.setValue(kernel);

    return App::DocumentObject::StdReturn;
}

