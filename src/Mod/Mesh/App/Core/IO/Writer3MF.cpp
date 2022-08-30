/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <ostream>
#endif


#include "Writer3MF.h"
#include "Core/MeshKernel.h"
#include <Base/Tools.h>
#include <zipios++/gzipoutputstream.h>
#include <zipios++/zipoutputstream.h>

using namespace MeshCore;


void  Writer3MF::SetTransform(const Base::Matrix4D& mat)
{
    transform = mat;
    if (mat != Base::Matrix4D())
        applyTransform = true;
}

bool Writer3MF::Save(std::ostream &str) const
{
    zipios::ZipOutputStream zip(str);
    zip.putNextEntry("3D/3dmodel.model");
    if (!SaveModel(zip))
        return false;
    zip.closeEntry();

    zip.putNextEntry("_rels/.rels");
    if (!SaveRels(zip))
        return false;
    zip.closeEntry();

    zip.putNextEntry("[Content_Types].xml");
    if (!SaveContent(zip))
        return false;
    zip.closeEntry();
    return true;
}

bool Writer3MF::SaveModel(std::ostream &str) const
{
    const MeshPointArray& rPoints = kernel.GetPoints();
    const MeshFacetArray& rFacets = kernel.GetFacets();

    if (!str || str.bad())
        return false;

    str << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<model unit=\"millimeter\"\n"
        << "       xml:lang=\"en-US\"\n"
        << "       xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">\n"
        << "<metadata name=\"Application\">FreeCAD</metadata>\n";
    str << Base::blanks(2) << "<resources>\n";
    str << Base::blanks(4) << "<object id=\"1\" type=\"model\">\n";
    str << Base::blanks(6) << "<mesh>\n";

    // vertices
    str << Base::blanks(8) << "<vertices>\n";
    Base::Vector3f pt;
    std::size_t index = 0;
    for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it, ++index) {
        pt.Set(it->x, it->y, it->z);
        if (this->applyTransform) {
            this->transform.multVec(pt, pt);
        }
        str << Base::blanks(10) << "<vertex x=\"" << pt.x
                                     << "\" y=\"" << pt.y
                                     << "\" z=\"" << pt.z
                                     << "\" />\n";
    }
    str << Base::blanks(8) << "</vertices>\n";

    // facet indices
    str << Base::blanks(8) << "<triangles>\n";
    for (MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it) {
        str << Base::blanks(10) << "<triangle v1=\"" << it->_aulPoints[0]
                                       << "\" v2=\"" << it->_aulPoints[1]
                                       << "\" v3=\"" << it->_aulPoints[2]
                                       << "\" />\n";
    }
    str << Base::blanks(8) << "</triangles>\n";

    str << Base::blanks(6) << "</mesh>\n";
    str << Base::blanks(4) << "</object>\n";
    str << Base::blanks(2) << "</resources>\n";
    str << Base::blanks(2) << "<build>\n";
    str << Base::blanks(4) << "<item objectid=\"1\" />\n";
    str << Base::blanks(2) << "</build>\n";
    str << "</model>\n";
    return true;
}

bool Writer3MF::SaveRels(std::ostream &str) const
{
    str << "<?xml version='1.0' encoding='UTF-8'?>\n"
        << "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
           "<Relationship Id=\"rel0\" Target=\"/3D/3dmodel.model\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" />"
           "</Relationships>";
    return true;
}

bool Writer3MF::SaveContent(std::ostream &str) const
{
    str << "<?xml version='1.0' encoding='UTF-8'?>\n"
        << "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
           "<Default ContentType=\"application/vnd.openxmlformats-package.relationships+xml\" Extension=\"rels\" />"
           "<Default ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\" Extension=\"model\" />"
           "</Types>";
    return true;
}
