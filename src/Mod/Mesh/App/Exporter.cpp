/***************************************************************************
 *   Copyright (c) 2017 Ian Rees <ian.rees@gmail.com>                      *
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
    #include <algorithm>
    #include <vector>
    #include <boost/algorithm/string/replace.hpp>
#endif  //  #ifndef _PreComp_

#include "Exporter.h"
#include "MeshFeature.h"

#include "Core/Iterator.h"

#include "Base/Console.h"
#include "Base/Exception.h"
#include "Base/FileInfo.h"
#include <Base/Interpreter.h>
#include "Base/Sequencer.h"
#include "Base/Stream.h"
#include "Base/Tools.h"

#include "App/Application.h"
#include "App/ComplexGeoData.h"
#include "App/ComplexGeoDataPy.h"
#include "App/DocumentObject.h"

#include <zipios++/zipoutputstream.h>

using namespace Mesh;
using namespace MeshCore;

static std::vector<std::string>
expandSubObjectNames(const App::DocumentObject *obj,
                     std::map<const App::DocumentObject*, std::vector<std::string> > &subObjectNameCache,
                     int depth)
{
    if (!App::GetApplication().checkLinkDepth(depth))
        return {};

    auto subs = obj->getSubObjects();
    if (subs.empty()) {
        subs.emplace_back("");
        return subs;
    }

    std::vector<std::string> res;
    for (auto & sub : subs) {
        int vis = sub.empty() ? 1 : obj->isElementVisible(sub.c_str());
        if (vis == 0)
            continue;
        auto sobj = obj->getSubObject(sub.c_str());
        if (!sobj || (vis < 0 && !sobj->Visibility.getValue()))
            continue;
        auto linked = sobj->getLinkedObject(true);
        auto it = subObjectNameCache.find(linked);
        if (it == subObjectNameCache.end())
            it = subObjectNameCache.emplace(
                    linked, expandSubObjectNames(linked, subObjectNameCache, depth+1)).first;
        for (auto & ssub : it->second)
            res.push_back(sub + ssub);
    }
    return res;
}

Exporter::Exporter()
{ }

//static
std::string Exporter::xmlEscape(const std::string &input)
{
    std::string out(input);
    boost::replace_all(out, "&", "&amp;");
    boost::replace_all(out, "\"", "&quot;");
    boost::replace_all(out, "'", "&apos;");
    boost::replace_all(out, "<", "&lt;");
    boost::replace_all(out, ">", "&gt;");
    return out;
}

int Exporter::addObject(App::DocumentObject *obj, float tol)
{
    int count = 0;
    for (std::string & sub : expandSubObjectNames(obj, subObjectNameCache, 0)) {
        Base::Matrix4D matrix;
        auto sobj = obj->getSubObject(sub.c_str(), nullptr, &matrix);
        auto linked = sobj->getLinkedObject(true, &matrix, false);
        auto it = meshCache.find(linked);
        if (it == meshCache.end()) {
            if (linked->isDerivedFrom(Mesh::Feature::getClassTypeId())) {
                it = meshCache.emplace(linked,
                        static_cast<Mesh::Feature*>(linked)->Mesh.getValue()).first;
                it->second.setTransform(Base::Matrix4D());
            }
            else {
                Base::PyGILStateLocker lock;
                PyObject *pyobj = nullptr;
                linked->getSubObject("", &pyobj, nullptr, false);
                if (!pyobj)
                    continue;
                if (PyObject_TypeCheck(pyobj, &Data::ComplexGeoDataPy::Type)) {
                    std::vector<Base::Vector3d> aPoints;
                    std::vector<Data::ComplexGeoData::Facet> aTopo;
                    auto geoData = static_cast<Data::ComplexGeoDataPy*>(pyobj)->getComplexGeoDataPtr();
                    geoData->getFaces(aPoints, aTopo, tol);
                    it = meshCache.emplace(linked, MeshObject()).first;
                    it->second.setFacets(aTopo, aPoints);
                }
                Py_DECREF(pyobj);
            }
        }

        // Add a new mesh
        if (it != meshCache.end()) {
            MeshObject mesh(it->second);
            mesh.transformGeometry(matrix);
            if (addMesh(sobj->Label.getValue(), mesh))
                ++count;
        }
    }
    return count;
}

MergeExporter::MergeExporter(std::string fileName, MeshIO::Format)
    :fName(fileName)
{
}

MergeExporter::~MergeExporter()
{
    // if we have more than one segment set the 'save' flag
    if (mergingMesh.countSegments() > 1) {
        for (unsigned long i = 0; i < mergingMesh.countSegments(); ++i) {
            mergingMesh.getSegment(i).save(true);
        }
    }

    try {
        mergingMesh.save(fName.c_str());
    }
    catch (const Base::Exception& e) {
        std::cerr << "Saving mesh failed: " << e.what() << std::endl;
    }
}


bool MergeExporter::addMesh(const char *name, const MeshObject & mesh)
{
    const auto & kernel = mesh.getKernel();
    auto countFacets( mergingMesh.countFacets() );
    if (countFacets == 0) {
        mergingMesh.setKernel(kernel);
    } else {
        mergingMesh.addMesh(kernel);
    }

    // if the mesh already has persistent segments then use them instead
    unsigned long numSegm = mesh.countSegments();
    unsigned long canSave = 0;
    for (unsigned long i=0; i<numSegm; i++) {
        if (mesh.getSegment(i).isSaved())
            canSave++;
    }

    if (canSave > 0) {
        for (unsigned long i=0; i<numSegm; i++) {
            const Segment& segm = mesh.getSegment(i);
            if (segm.isSaved()) {
                std::vector<FacetIndex> indices = segm.getIndices();
                std::for_each( indices.begin(), indices.end(),
                               [countFacets] (FacetIndex &v) {
                                   v += countFacets;
                               } );
                Segment new_segm(&mergingMesh, indices, true);
                new_segm.setName(segm.getName());
                mergingMesh.addSegment(new_segm);
            }
        }
    } else {
        // now create a segment for the added mesh
        std::vector<FacetIndex> indices;
        indices.resize(mergingMesh.countFacets() - countFacets);
        std::generate(indices.begin(), indices.end(), Base::iotaGen<FacetIndex>(countFacets));
        Segment segm(&mergingMesh, indices, true);
        segm.setName(name);
        mergingMesh.addSegment(segm);
    }

    return true;
}

AmfExporter::AmfExporter( std::string fileName,
                          const std::map<std::string, std::string> &meta,
                          bool compress ) :
    outputStreamPtr(nullptr), nextObjectIndex(0)
{
    // ask for write permission
    Base::FileInfo fi(fileName.c_str());
    Base::FileInfo di(fi.dirPath().c_str());
    if ((fi.exists() && !fi.isWritable()) || !di.exists() || !di.isWritable()) {
        throw Base::FileException("No write permission for file", fileName);
    }

    if (compress) {
        auto *zipStreamPtr( new zipios::ZipOutputStream(fi.filePath()) );

        // ISO 52915 specifies that compressed AMF files are zip-compressed and
        // must contain the AMF XML in an entry with the same name as the
        // compressed file.  It's OK to have other files in the zip too.
        zipStreamPtr->putNextEntry( zipios::ZipCDirEntry(fi.fileName()) );

        // Default compression seems to work fine.
        outputStreamPtr = zipStreamPtr;

    } else {
        outputStreamPtr = new Base::ofstream(fi, std::ios::out | std::ios::binary);
    }

    if (outputStreamPtr) {
        *outputStreamPtr << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                         << "<amf unit=\"millimeter\">\n";
        for (auto const &metaEntry : meta) {
            *outputStreamPtr << "\t<metadata type=\"" << metaEntry.first
                             << "\">" << metaEntry.second << "</metadata>\n";
        }
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

bool AmfExporter::addMesh(const char *name, const MeshObject & mesh)
{
    const auto & kernel = mesh.getKernel();

    if (!outputStreamPtr || outputStreamPtr->bad()) {
        return false;
    }

    auto numFacets( kernel.CountFacets() );

    if (numFacets == 0) {
        return false;
    }

    MeshCore::MeshFacetIterator clIter(kernel), clEnd(kernel);

    Base::SequencerLauncher seq("Saving...", 2 * numFacets + 1);

    *outputStreamPtr << "\t<object id=\"" << nextObjectIndex << "\">\n";
    *outputStreamPtr << "\t\t<metadata type=\"name\">"
                        << xmlEscape(name) << "</metadata>\n";
    *outputStreamPtr << "\t\t<mesh>\n"
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

    ++nextObjectIndex;
    return true;
}

