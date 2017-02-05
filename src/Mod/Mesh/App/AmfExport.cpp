/***************************************************************************
 *   Copyright (c) Ian Rees                  <ian.rees@gmail.com>          *
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
    #include <map>
    #include <vector>
#endif  //  #ifndef _PreComp_

#include "AmfExport.h"

#include "Core/Iterator.h"

#include "Base/Exception.h"
#include "Base/FileInfo.h"
#include "Base/Sequencer.h"
#include "Base/Stream.h"

using namespace Mesh;

AmfExporter::AmfExporter(const char *fileName) :
    outputStreamPtr(nullptr), nextObjectIndex(0)
{
    // ask for write permission
    Base::FileInfo fi(fileName);
    Base::FileInfo di(fi.dirPath().c_str());
    if ((fi.exists() && !fi.isWritable()) || !di.exists() || !di.isWritable())
        throw Base::FileException("No write permission for file", fileName);

    outputStreamPtr = new Base::ofstream(fi, std::ios::out | std::ios::binary);
    if (outputStreamPtr) {
        *outputStreamPtr << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                         << "<amf unit=\"millimeter\">\n";
    }
}

AmfExporter::~AmfExporter()
{
    if (outputStreamPtr) {
        *outputStreamPtr << "\t<constellation id=\"0\">\n";
        for (auto objId(0); objId < nextObjectIndex; ++objId) {
            *outputStreamPtr << "\t\t<instance objectid=\"" << objId << "\">\n"
                             << "\t\t\t<deltax>0</deltax>\n"
                             << "\t\t\t<deltay>0</deltay>\n"
                             << "\t\t\t<rz>0</rz>\n"
                             << "\t\t</instance>\n";
        }
        *outputStreamPtr << "\t</constellation>\n"
                         << "</amf>\n";
        delete outputStreamPtr;
    }
}


int AmfExporter::addObject(const MeshCore::MeshKernel &meshKernel)
{
    if (!outputStreamPtr || outputStreamPtr->bad()) {
        return -1;
    }

    auto numFacets( meshKernel.CountFacets() );

    if (numFacets == 0) {
        return -1;
    }

    MeshCore::MeshFacetIterator clIter(meshKernel), clEnd(meshKernel);

    Base::SequencerLauncher seq("Saving...", 2 * numFacets + 1);

    *outputStreamPtr << "\t<object id=\"" << nextObjectIndex << "\">\n"
                     << "\t\t<mesh>\n"
                     << "\t\t\t<vertices>\n";

    const MeshCore::MeshGeomFacet *facet;

    // Iterate through all facets of the mesh, and construct a:
    //   * Cache (map) of used vertices, outputting each new unique vertex to
    //     the output stream as we find it
    //   * Vector of the vertices, referred to by the indices from 1 
    std::map<Base::Vector3f, unsigned long, AmfExporter::VertLess> vertices;
    auto vertItr(vertices.begin());
    auto vertexCount(0UL);

    // {facet1A, facet1B, facet1C, facet2A, ..., facetNC}
    std::vector<unsigned long> facets;

    // For each facet in mesh
    for(clIter.Begin(), clEnd.End(); clIter < clEnd; ++clIter) {
        facet = &(*clIter);

        // For each vertex in facet
        for (auto i(0); i < 3; ++i) {
            vertItr = vertices.find(facet->_aclPoints[i]);

            if ( vertItr == vertices.end() ) {
                facets.push_back(vertexCount);

                vertices[facet->_aclPoints[i]] = vertexCount++;

                // Output facet
                *outputStreamPtr << "\t\t\t\t<vertex>\n"
                                 << "\t\t\t\t\t<coordinates>\n";
                for ( auto j(0); j < 3; ++j) {
                    char axis('x' + j);
                    *outputStreamPtr << "\t\t\t\t\t\t<" << axis << '>'
                                     << facet->_aclPoints[i][j]
                                     << "</" << axis << ">\n";
                }
                *outputStreamPtr << "\t\t\t\t\t</coordinates>\n"
                                 << "\t\t\t\t</vertex>\n";
            } else {
                facets.push_back(vertItr->second);
            }
        }

        seq.next(true); // allow to cancel
    }

    *outputStreamPtr << "\t\t\t</vertices>\n"
                     << "\t\t\t<volume>\n";
    
    // Now that we've output all the vertices, we can
    // output the facets that refer to them!
    for (auto triItr(facets.begin()); triItr != facets.end(); ) {
        *outputStreamPtr << "\t\t\t\t<triangle>\n";
        for (auto i(1); i < 4; ++i) {
            *outputStreamPtr << "\t\t\t\t\t<v" << i << '>'
                             << *(triItr++) << "</v" << i << ">\n";
        }
        *outputStreamPtr << "\t\t\t\t</triangle>\n";
        seq.next(true); // allow to cancel
    }

    *outputStreamPtr << "\t\t\t</volume>\n"
                     << "\t\t</mesh>\n"
                     << "\t</object>\n";

    return nextObjectIndex++;
}

