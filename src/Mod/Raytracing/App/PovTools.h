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


#ifndef _PovTools_h_
#define _PovTools_h_

#include <gp_Vec.hxx>
#include <vector>

class TopoDS_Shape;
class TopoDS_Face;

namespace Data { class ComplexGeoData; }

namespace Raytracing
{

/// helper class to store a complete camera position
class CamDef
{
public:
    CamDef(const gp_Vec& cCamPos,
           const gp_Vec& cCamDir,
           const gp_Vec& cLookAt,
           const gp_Vec& cUp)
            :
            CamPos(cCamPos),
            CamDir(cCamDir),
            LookAt(cLookAt),
            Up(cUp)
    {}

    CamDef(const CamDef& copyMe) {
        this->operator=(copyMe);
    }

    const CamDef& operator=(const CamDef& copyMe)
    {
        CamPos = copyMe.CamPos;
        CamDir = copyMe.CamDir;
        LookAt = copyMe.LookAt;
        Up     = copyMe.Up;

        return *this;
    }

    gp_Vec CamPos;
    gp_Vec CamDir;
    gp_Vec LookAt;
    gp_Vec Up;
};


class AppRaytracingExport PovTools
{
public:
    /// returns the given camera position as povray defines in a file
    static std::string getCamera(const CamDef& Cam);

    /// writes the given camera position as povray defines in a file
    static void writeCamera(const char*   FileName,
                            const CamDef& Cam);

    /// writes the given camera positions as povray defines in a file
    static void writeCameraVec(const char*                FileName,
                               const std::vector<CamDef>& CamVec);

    /// write a given shape as povray file to disk
    static void writeData(const char *FileName,
                          const char *PartName,
                          const Data::ComplexGeoData*,
                          float fMeshDeviation=0.1);

    /// write a given shape as povray file to disk
    static void writeShape(const char *FileName,
                           const char *PartName,
                           const TopoDS_Shape& Shape,
                           float fMeshDeviation=0.1);

    /// write a given shape as povray in a stream
    static void writeShape(std::ostream &out,
                           const char *PartName,
                           const TopoDS_Shape& Shape,
                           float fMeshDeviation=0.1);

    /// write a given shape as points and normal Vectors in a coma separeted format
    static void writeShapeCSV(const char *FileName,
                              const TopoDS_Shape& Shape,
                              float fMeshDeviation,
                              float fLength);


    static void transferToArray(const TopoDS_Face& aFace,gp_Vec** vertices,gp_Vec** vertexnormals, long** cons,int &nbNodesInFace,int &nbTriInFace );
};


} // namespace Raytracing

#endif // _PovTools_h_
