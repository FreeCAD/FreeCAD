/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2005     *
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


#ifndef _LuxTools_h_
#define _LuxTools_h_

#include <gp_Vec.hxx>
#include <vector>

#include "PovTools.h"

class TopoDS_Shape;
class TopoDS_Face;

namespace Data { class ComplexGeoData; }

namespace Raytracing
{
    
    class AppRaytracingExport LuxTools
    {
    public:
        /// returns the given camera position as luxray defines
        static std::string getCamera(const CamDef& Cam);
        /// returns the given shape as luxrender material + shape data
        static void writeShape(std::ostream &out, const char *PartName, const TopoDS_Shape& Shape, float fMeshDeviation=0.1);
    };
} // namespace Raytracing

#endif // _LuxTools_h_
