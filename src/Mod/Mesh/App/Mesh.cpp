/***************************************************************************
 *   Copyright (c) Jürgen Riegel <juergen.riegel@web.de>                   *
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
#include <sstream>
#endif

#include <Base/Builder3D.h>
#include <Base/Console.h>
#include <Base/Converter.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Reader.h>
#include <Base/Sequencer.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Base/ViewProj.h>
#include <Base/Writer.h>

#include "Core/Builder.h"
#include "Core/Decimation.h"
#include "Core/Degeneration.h"
#include "Core/Grid.h"
#include "Core/Info.h"
#include "Core/Iterator.h"
#include "Core/MeshKernel.h"
#include "Core/Segmentation.h"
#include "Core/SetOperations.h"
#include "Core/TopoAlgorithm.h"
#include "Core/Trim.h"
#include "Core/TrimByPlane.h"

#include "Mesh.h"


using namespace Mesh;

const float MeshObject::Epsilon = 1.0e-5F;

TYPESYSTEM_SOURCE(Mesh::MeshObject, Data::ComplexGeoData)
TYPESYSTEM_SOURCE(Mesh::MeshSegment, Data::Segment)

MeshObject::MeshObject() = default;

MeshObject::MeshObject(const MeshCore::MeshKernel& Kernel)  // NOLINT
    : _kernel(Kernel)
{
    // copy the mesh structure
}

MeshObject::MeshObject(const MeshCore::MeshKernel& Kernel, const Base::Matrix4D& Mtrx)  // NOLINT
    : _Mtrx(Mtrx)
    , _kernel(Kernel)
{
    // copy the mesh structure
}

MeshObject::MeshObject(const MeshObject& mesh)
    : _Mtrx(mesh._Mtrx)
    , _kernel(mesh._kernel)
{
    // copy the mesh structure
    copySegments(mesh);
}

MeshObject::MeshObject(MeshObject&& mesh)
    : _Mtrx(mesh._Mtrx)
    , _kernel(mesh._kernel)
{
    // copy the mesh structure
    copySegments(mesh);
}

MeshObject::~MeshObject() = default;

std::vector<const char*> MeshObject::getElementTypes() const
{
    std::vector<const char*> temp;
    temp.push_back("Mesh");
    temp.push_back("Segment");

    return temp;
}

unsigned long MeshObject::countSubElements(const char* Type) const
{
    std::string element(Type);
    if (element == "Mesh") {
        return 1;
    }
    else if (element == "Segment") {
        return countSegments();
    }
    return 0;
}

Data::Segment* MeshObject::getSubElement(const char* Type, unsigned long n) const
{
    std::string element(Type);
    if (element == "Mesh" && n == 0) {
        MeshSegment* segm = new MeshSegment();
        segm->mesh = new MeshObject(*this);
        return segm;
    }
    else if (element == "Segment" && n < countSegments()) {
        MeshSegment* segm = new MeshSegment();
        segm->mesh = new MeshObject(*this);
        const Segment& faces = getSegment(n);
        segm->segment = std::make_unique<Segment>(static_cast<MeshObject*>(segm->mesh),
                                                  faces.getIndices(),
                                                  false);
        return segm;
    }

    return nullptr;
}

void MeshObject::getFacesFromSubElement(const Data::Segment* element,
                                        std::vector<Base::Vector3d>& points,
                                        std::vector<Base::Vector3d>& /*pointNormals*/,
                                        std::vector<Facet>& faces) const
{
    if (element && element->getTypeId() == MeshSegment::getClassTypeId()) {
        const MeshSegment* segm = static_cast<const MeshSegment*>(element);
        if (segm->segment) {
            Base::Reference<MeshObject> submesh(
                segm->mesh->meshFromSegment(segm->segment->getIndices()));
            submesh->getFaces(points, faces, 0.0);
        }
        else {
            segm->mesh->getFaces(points, faces, 0.0);
        }
    }
}

void MeshObject::transformGeometry(const Base::Matrix4D& rclMat)
{
    MeshCore::MeshKernel kernel;
    swap(kernel);
    kernel.Transform(rclMat);
    swap(kernel);
}

void MeshObject::setTransform(const Base::Matrix4D& rclTrf)
{
    _Mtrx = rclTrf;
}

Base::Matrix4D MeshObject::getTransform() const
{
    return _Mtrx;
}

Base::BoundBox3d MeshObject::getBoundBox() const
{
    _kernel.RecalcBoundBox();
    Base::BoundBox3f Bnd = _kernel.GetBoundBox();

    Base::BoundBox3d Bnd2;
    if (Bnd.IsValid()) {
        for (int i = 0; i <= 7; i++) {
            Bnd2.Add(transformPointToOutside(Bnd.CalcPoint(Base::BoundBox3f::CORNER(i))));
        }
    }

    return Bnd2;
}

bool MeshObject::getCenterOfGravity(Base::Vector3d& center) const
{
    MeshCore::MeshAlgorithm alg(_kernel);
    Base::Vector3f pnt = alg.GetGravityPoint();
    center = transformPointToOutside(pnt);
    return true;
}

void MeshObject::copySegments(const MeshObject& mesh)
{
    // After copying the segments the mesh pointers must be adjusted
    this->_segments = mesh._segments;
    std::for_each(this->_segments.begin(), this->_segments.end(), [this](Segment& s) {
        s._mesh = this;
    });
}

void MeshObject::swapSegments(MeshObject& mesh)
{
    this->_segments.swap(mesh._segments);
    std::for_each(this->_segments.begin(), this->_segments.end(), [this](Segment& s) {
        s._mesh = this;
    });
    std::for_each(mesh._segments.begin(), mesh._segments.end(), [&mesh](Segment& s) {
        s._mesh = &mesh;
    });
}

MeshObject& MeshObject::operator=(const MeshObject& mesh)
{
    if (this != &mesh) {
        // copy the mesh structure
        setTransform(mesh._Mtrx);
        this->_kernel = mesh._kernel;
        copySegments(mesh);
    }

    return *this;
}

MeshObject& MeshObject::operator=(MeshObject&& mesh)
{
    if (this != &mesh) {
        // copy the mesh structure
        setTransform(mesh._Mtrx);
        this->_kernel = mesh._kernel;
        copySegments(mesh);
    }

    return *this;
}

void MeshObject::setKernel(const MeshCore::MeshKernel& m)
{
    this->_kernel = m;
    this->_segments.clear();
}

void MeshObject::swap(MeshCore::MeshKernel& Kernel)
{
    this->_kernel.Swap(Kernel);
    // clear the segments because we don't know how the new
    // topology looks like
    this->_segments.clear();
}

void MeshObject::swap(MeshObject& mesh)
{
    this->_kernel.Swap(mesh._kernel);
    swapSegments(mesh);
    Base::Matrix4D tmp = this->_Mtrx;
    this->_Mtrx = mesh._Mtrx;
    mesh._Mtrx = tmp;
}

std::string MeshObject::representation() const
{
    std::stringstream str;
    MeshCore::MeshInfo info(_kernel);
    info.GeneralInformation(str);
    return str.str();
}

std::string MeshObject::topologyInfo() const
{
    std::stringstream str;
    MeshCore::MeshInfo info(_kernel);
    info.TopologyInformation(str);
    return str.str();
}

unsigned long MeshObject::countPoints() const
{
    return _kernel.CountPoints();
}

unsigned long MeshObject::countFacets() const
{
    return _kernel.CountFacets();
}

unsigned long MeshObject::countEdges() const
{
    return _kernel.CountEdges();
}

unsigned long MeshObject::countSegments() const
{
    return this->_segments.size();
}

bool MeshObject::isSolid() const
{
    MeshCore::MeshEvalSolid cMeshEval(_kernel);
    return cMeshEval.Evaluate();
}

double MeshObject::getSurface() const
{
    return _kernel.GetSurface();
}

double MeshObject::getVolume() const
{
    return _kernel.GetVolume();
}

Base::Vector3d MeshObject::getPoint(PointIndex index) const
{
    MeshCore::MeshPoint vertf = _kernel.GetPoint(index);
    Base::Vector3d vertd(vertf.x, vertf.y, vertf.z);
    vertd = _Mtrx * vertd;
    return vertd;
}

MeshPoint MeshObject::getMeshPoint(PointIndex index) const
{
    MeshPoint point(getPoint(index), this, index);
    return point;
}

void MeshObject::getPoints(std::vector<Base::Vector3d>& Points,
                           std::vector<Base::Vector3d>& Normals,
                           double /*Accuracy*/,
                           uint16_t /*flags*/) const
{
    Points = transformPointsToOutside(_kernel.GetPoints());
    MeshCore::MeshRefNormalToPoints ptNormals(_kernel);
    Normals = transformVectorsToOutside(ptNormals.GetValues());
}

Mesh::Facet MeshObject::getMeshFacet(FacetIndex index) const
{
    Mesh::Facet face(_kernel.GetFacets()[index], this, index);
    return face;
}

void MeshObject::getFaces(std::vector<Base::Vector3d>& Points,
                          std::vector<Facet>& Topo,
                          double /*Accuracy*/,
                          uint16_t /*flags*/) const
{
    unsigned long ctpoints = _kernel.CountPoints();
    Points.reserve(ctpoints);
    for (unsigned long i = 0; i < ctpoints; i++) {
        Points.push_back(getPoint(i));
    }

    unsigned long ctfacets = _kernel.CountFacets();
    const MeshCore::MeshFacetArray& ary = _kernel.GetFacets();
    Topo.reserve(ctfacets);
    for (unsigned long i = 0; i < ctfacets; i++) {
        Facet face {};
        face.I1 = (unsigned int)ary[i]._aulPoints[0];
        face.I2 = (unsigned int)ary[i]._aulPoints[1];
        face.I3 = (unsigned int)ary[i]._aulPoints[2];
        Topo.push_back(face);
    }
}

unsigned int MeshObject::getMemSize() const
{
    return _kernel.GetMemSize();
}

void MeshObject::Save(Base::Writer& /*writer*/) const
{
    // this is handled by the property class
}

void MeshObject::SaveDocFile(Base::Writer& writer) const
{
    _kernel.Write(writer.Stream());
}

void MeshObject::Restore(Base::XMLReader& /*reader*/)
{
    // this is handled by the property class
}

void MeshObject::RestoreDocFile(Base::Reader& reader)
{
    load(reader);
}

void MeshObject::save(const char* file,
                      MeshCore::MeshIO::Format f,
                      const MeshCore::Material* mat,
                      const char* objectname) const
{
    MeshCore::MeshOutput aWriter(this->_kernel, mat);
    if (objectname) {
        aWriter.SetObjectName(objectname);
    }

    // go through the segment list and put them to the exporter when
    // the "save" flag is set
    std::vector<MeshCore::Group> groups;
    for (const auto& segment : this->_segments) {
        if (segment.isSaved()) {
            MeshCore::Group g;
            g.indices = segment.getIndices();
            g.name = segment.getName();
            groups.push_back(g);
        }
    }
    aWriter.SetGroups(groups);
    if (mat && mat->library.empty()) {
        Base::FileInfo fi(file);
        mat->library = fi.fileNamePure() + ".mtl";
    }

    aWriter.Transform(this->_Mtrx);
    aWriter.SaveAny(file, f);
}

void MeshObject::save(std::ostream& str,
                      MeshCore::MeshIO::Format f,
                      const MeshCore::Material* mat,
                      const char* objectname) const
{
    MeshCore::MeshOutput aWriter(this->_kernel, mat);
    if (objectname) {
        aWriter.SetObjectName(objectname);
    }

    // go through the segment list and put them to the exporter when
    // the "save" flag is set
    std::vector<MeshCore::Group> groups;
    for (const auto& segment : this->_segments) {
        if (segment.isSaved()) {
            MeshCore::Group g;
            g.indices = segment.getIndices();
            g.name = segment.getName();
            groups.push_back(g);
        }
    }
    aWriter.SetGroups(groups);

    aWriter.Transform(this->_Mtrx);
    aWriter.SaveFormat(str, f);
}

bool MeshObject::load(const char* file, MeshCore::Material* mat)
{
    MeshCore::MeshKernel kernel;
    MeshCore::MeshInput aReader(kernel, mat);
    if (!aReader.LoadAny(file)) {
        return false;
    }

    swapKernel(kernel, aReader.GetGroupNames());
    return true;
}

bool MeshObject::load(std::istream& str, MeshCore::MeshIO::Format f, MeshCore::Material* mat)
{
    MeshCore::MeshKernel kernel;
    MeshCore::MeshInput aReader(kernel, mat);
    if (!aReader.LoadFormat(str, f)) {
        return false;
    }

    swapKernel(kernel, aReader.GetGroupNames());
    return true;
}

void MeshObject::swapKernel(MeshCore::MeshKernel& kernel, const std::vector<std::string>& g)
{
    _kernel.Swap(kernel);
    // Some file formats define several objects per file (e.g. OBJ).
    // Now we mark each object as an own segment so that we can break
    // the object into its original objects again.
    this->_segments.clear();
    const MeshCore::MeshFacetArray& faces = _kernel.GetFacets();
    MeshCore::MeshFacetArray::_TConstIterator it;
    std::vector<FacetIndex> segment;
    segment.reserve(faces.size());
    unsigned long prop = 0;
    unsigned long index = 0;
    for (it = faces.begin(); it != faces.end(); ++it) {
        if (prop < it->_ulProp) {
            prop = it->_ulProp;
            if (!segment.empty()) {
                this->_segments.emplace_back(this, segment, true);
                segment.clear();
            }
        }

        segment.push_back(index++);
    }

    // if the whole mesh is a single object then don't mark as segment
    if (!segment.empty() && (segment.size() < faces.size())) {
        this->_segments.emplace_back(this, segment, true);
    }

    // apply the group names to the segments
    if (this->_segments.size() == g.size()) {
        for (std::size_t index = 0; index < this->_segments.size(); index++) {
            this->_segments[index].setName(g[index]);
        }
    }
}

void MeshObject::save(std::ostream& out) const
{
    _kernel.Write(out);
}

void MeshObject::load(std::istream& in)
{
    _kernel.Read(in);
    this->_segments.clear();

#ifndef FC_DEBUG
    try {
        MeshCore::MeshEvalNeighbourhood nb(_kernel);
        if (!nb.Evaluate()) {
            Base::Console().Warning("Errors in neighbourhood of mesh found...");
            _kernel.RebuildNeighbours();
            Base::Console().Warning("fixed\n");
        }

        MeshCore::MeshEvalTopology eval(_kernel);
        if (!eval.Evaluate()) {
            Base::Console().Warning("The mesh data structure has some defects\n");
        }
    }
    catch (const Base::MemoryException&) {
        // ignore memory exceptions and continue
        Base::Console().Log("Check for defects in mesh data structure failed\n");
    }
#endif
}

void MeshObject::writeInventor(std::ostream& str, float creaseangle) const
{
    const MeshCore::MeshPointArray& point = getKernel().GetPoints();
    const MeshCore::MeshFacetArray& faces = getKernel().GetFacets();

    std::vector<Base::Vector3f> coords;
    coords.reserve(point.size());
    std::copy(point.begin(), point.end(), std::back_inserter(coords));

    std::vector<int> indices;
    indices.reserve(4 * faces.size());
    for (const auto& it : faces) {
        indices.push_back(it._aulPoints[0]);
        indices.push_back(it._aulPoints[1]);
        indices.push_back(it._aulPoints[2]);
        indices.push_back(-1);
    }

    Base::InventorBuilder builder(str);
    builder.beginSeparator();
    builder.addNode(Base::TransformItem {getTransform()});
    Base::ShapeHintsItem shapeHints {creaseangle};
    builder.addNode(shapeHints);
    builder.addNode(Base::Coordinate3Item {coords});
    builder.addNode(Base::IndexedFaceSetItem {indices});
    builder.endSeparator();
}

void MeshObject::addFacet(const MeshCore::MeshGeomFacet& facet)
{
    _kernel.AddFacet(facet);
}

void MeshObject::addFacets(const std::vector<MeshCore::MeshGeomFacet>& facets)
{
    _kernel.AddFacets(facets);
}

void MeshObject::addFacets(const std::vector<MeshCore::MeshFacet>& facets, bool checkManifolds)
{
    _kernel.AddFacets(facets, checkManifolds);
}

void MeshObject::addFacets(const std::vector<MeshCore::MeshFacet>& facets,
                           const std::vector<Base::Vector3f>& points,
                           bool checkManifolds)
{
    _kernel.AddFacets(facets, points, checkManifolds);
}

void MeshObject::addFacets(const std::vector<Data::ComplexGeoData::Facet>& facets,
                           const std::vector<Base::Vector3d>& points,
                           bool checkManifolds)
{
    std::vector<MeshCore::MeshFacet> facet_v;
    facet_v.reserve(facets.size());
    for (auto facet : facets) {
        MeshCore::MeshFacet f;
        f._aulPoints[0] = facet.I1;
        f._aulPoints[1] = facet.I2;
        f._aulPoints[2] = facet.I3;
        facet_v.push_back(f);
    }

    std::vector<Base::Vector3f> point_v;
    point_v.reserve(points.size());
    for (const auto& point : points) {
        Base::Vector3f p((float)point.x, (float)point.y, (float)point.z);
        point_v.push_back(p);
    }

    _kernel.AddFacets(facet_v, point_v, checkManifolds);
}

void MeshObject::setFacets(const std::vector<MeshCore::MeshGeomFacet>& facets)
{
    _kernel = facets;
}

void MeshObject::setFacets(const std::vector<Data::ComplexGeoData::Facet>& facets,
                           const std::vector<Base::Vector3d>& points)
{
    MeshCore::MeshFacetArray facet_v;
    facet_v.reserve(facets.size());
    for (auto facet : facets) {
        MeshCore::MeshFacet f;
        f._aulPoints[0] = facet.I1;
        f._aulPoints[1] = facet.I2;
        f._aulPoints[2] = facet.I3;
        facet_v.push_back(f);
    }

    MeshCore::MeshPointArray point_v;
    point_v.reserve(points.size());
    for (const auto& point : points) {
        Base::Vector3f p((float)point.x, (float)point.y, (float)point.z);
        point_v.push_back(p);
    }

    _kernel.Adopt(point_v, facet_v, true);
}

void MeshObject::addMesh(const MeshObject& mesh)
{
    _kernel.Merge(mesh._kernel);
}

void MeshObject::addMesh(const MeshCore::MeshKernel& kernel)
{
    _kernel.Merge(kernel);
}

void MeshObject::deleteFacets(const std::vector<FacetIndex>& removeIndices)
{
    if (removeIndices.empty()) {
        return;
    }
    _kernel.DeleteFacets(removeIndices);
    deletedFacets(removeIndices);
}

void MeshObject::deletePoints(const std::vector<PointIndex>& removeIndices)
{
    if (removeIndices.empty()) {
        return;
    }
    _kernel.DeletePoints(removeIndices);
    this->_segments.clear();
}

void MeshObject::deletedFacets(const std::vector<FacetIndex>& remFacets)
{
    if (remFacets.empty()) {
        return;  // nothing has changed
    }
    if (this->_segments.empty()) {
        return;  // nothing to do
    }
    // set an array with the original indices and mark the removed as MeshCore::FACET_INDEX_MAX
    std::vector<FacetIndex> f_indices(_kernel.CountFacets() + remFacets.size());
    for (FacetIndex remFacet : remFacets) {
        f_indices[remFacet] = MeshCore::FACET_INDEX_MAX;
    }

    FacetIndex index = 0;
    for (FacetIndex& it : f_indices) {
        if (it == 0) {
            it = index++;
        }
    }

    // the array serves now as LUT to set the new indices in the segments
    for (auto& segment : this->_segments) {
        std::vector<FacetIndex> segm = segment._indices;
        for (FacetIndex& jt : segm) {
            jt = f_indices[jt];
        }

        // remove the invalid indices
        std::sort(segm.begin(), segm.end());
        std::vector<FacetIndex>::iterator ft =
            std::find_if(segm.begin(), segm.end(), [](FacetIndex v) {
                return v == MeshCore::FACET_INDEX_MAX;
            });
        if (ft != segm.end()) {
            segm.erase(ft, segm.end());
        }
        segment._indices = segm;
    }
}

void MeshObject::deleteSelectedFacets()
{
    std::vector<FacetIndex> facets;
    MeshCore::MeshAlgorithm(this->_kernel).GetFacetsFlag(facets, MeshCore::MeshFacet::SELECTED);
    deleteFacets(facets);
}

void MeshObject::deleteSelectedPoints()
{
    std::vector<PointIndex> points;
    MeshCore::MeshAlgorithm(this->_kernel).GetPointsFlag(points, MeshCore::MeshPoint::SELECTED);
    deletePoints(points);
}

void MeshObject::clearFacetSelection() const
{
    MeshCore::MeshAlgorithm(this->_kernel).ResetFacetFlag(MeshCore::MeshFacet::SELECTED);
}

void MeshObject::clearPointSelection() const
{
    MeshCore::MeshAlgorithm(this->_kernel).ResetPointFlag(MeshCore::MeshPoint::SELECTED);
}

void MeshObject::addFacetsToSelection(const std::vector<FacetIndex>& inds) const
{
    MeshCore::MeshAlgorithm(this->_kernel).SetFacetsFlag(inds, MeshCore::MeshFacet::SELECTED);
}

void MeshObject::addPointsToSelection(const std::vector<PointIndex>& inds) const
{
    MeshCore::MeshAlgorithm(this->_kernel).SetPointsFlag(inds, MeshCore::MeshPoint::SELECTED);
}

void MeshObject::removeFacetsFromSelection(const std::vector<FacetIndex>& inds) const
{
    MeshCore::MeshAlgorithm(this->_kernel).ResetFacetsFlag(inds, MeshCore::MeshFacet::SELECTED);
}

void MeshObject::removePointsFromSelection(const std::vector<PointIndex>& inds) const
{
    MeshCore::MeshAlgorithm(this->_kernel).ResetPointsFlag(inds, MeshCore::MeshPoint::SELECTED);
}

void MeshObject::getFacetsFromSelection(std::vector<FacetIndex>& inds) const
{
    MeshCore::MeshAlgorithm(this->_kernel).GetFacetsFlag(inds, MeshCore::MeshFacet::SELECTED);
}

void MeshObject::getPointsFromSelection(std::vector<PointIndex>& inds) const
{
    MeshCore::MeshAlgorithm(this->_kernel).GetPointsFlag(inds, MeshCore::MeshPoint::SELECTED);
}

unsigned long MeshObject::countSelectedFacets() const
{
    return MeshCore::MeshAlgorithm(this->_kernel).CountFacetFlag(MeshCore::MeshFacet::SELECTED);
}

bool MeshObject::hasSelectedFacets() const
{
    return (countSelectedFacets() > 0);
}

unsigned long MeshObject::countSelectedPoints() const
{
    return MeshCore::MeshAlgorithm(this->_kernel).CountPointFlag(MeshCore::MeshPoint::SELECTED);
}

bool MeshObject::hasSelectedPoints() const
{
    return (countSelectedPoints() > 0);
}

std::vector<PointIndex> MeshObject::getPointsFromFacets(const std::vector<FacetIndex>& facets) const
{
    return _kernel.GetFacetPoints(facets);
}

bool MeshObject::nearestFacetOnRay(const MeshObject::TRay& ray,
                                   double maxAngle,
                                   MeshObject::TFaceSection& output) const
{
    Base::Vector3f pnt = Base::toVector<float>(ray.first);
    Base::Vector3f dir = Base::toVector<float>(ray.second);

    Base::Placement plm = getPlacement();
    Base::Placement inv = plm.inverse();

    // transform the ray relative to the mesh kernel
    inv.multVec(pnt, pnt);
    inv.getRotation().multVec(dir, dir);

    FacetIndex index = 0;
    Base::Vector3f res;
    MeshCore::MeshAlgorithm alg(getKernel());

    if (alg.NearestFacetOnRay(pnt, dir, static_cast<float>(maxAngle), res, index)) {
        plm.multVec(res, res);
        output.first = index;
        output.second = Base::toVector<double>(res);
        return true;
    }

    return false;
}

std::vector<MeshObject::TFaceSection> MeshObject::foraminate(const TRay& ray, double maxAngle) const
{
    Base::Vector3f pnt = Base::toVector<float>(ray.first);
    Base::Vector3f dir = Base::toVector<float>(ray.second);

    Base::Placement plm = getPlacement();
    Base::Placement inv = plm.inverse();

    // transform the ray relative to the mesh kernel
    inv.multVec(pnt, pnt);
    inv.getRotation().multVec(dir, dir);

    Base::Vector3f res;
    MeshCore::MeshFacetIterator f_it(getKernel());
    int index = 0;

    std::vector<MeshObject::TFaceSection> output;
    for (f_it.Begin(); f_it.More(); f_it.Next(), index++) {
        if (f_it->Foraminate(pnt, dir, res, static_cast<float>(maxAngle))) {
            plm.multVec(res, res);

            MeshObject::TFaceSection section;
            section.first = index;
            section.second = Base::toVector<double>(res);
            output.push_back(section);
        }
    }

    return output;
}

void MeshObject::updateMesh(const std::vector<FacetIndex>& facets) const
{
    std::vector<PointIndex> points;
    points = _kernel.GetFacetPoints(facets);

    MeshCore::MeshAlgorithm alg(_kernel);
    alg.SetFacetsFlag(facets, MeshCore::MeshFacet::SEGMENT);
    alg.SetPointsFlag(points, MeshCore::MeshPoint::SEGMENT);
}

void MeshObject::updateMesh() const
{
    MeshCore::MeshAlgorithm alg(_kernel);
    alg.ResetFacetFlag(MeshCore::MeshFacet::SEGMENT);
    alg.ResetPointFlag(MeshCore::MeshPoint::SEGMENT);
    for (const auto& segment : this->_segments) {
        std::vector<PointIndex> points;
        points = _kernel.GetFacetPoints(segment.getIndices());
        alg.SetFacetsFlag(segment.getIndices(), MeshCore::MeshFacet::SEGMENT);
        alg.SetPointsFlag(points, MeshCore::MeshPoint::SEGMENT);
    }
}

std::vector<std::vector<FacetIndex>> MeshObject::getComponents() const
{
    std::vector<std::vector<FacetIndex>> segments;
    MeshCore::MeshComponents comp(_kernel);
    comp.SearchForComponents(MeshCore::MeshComponents::OverEdge, segments);
    return segments;
}

unsigned long MeshObject::countComponents() const
{
    std::vector<std::vector<FacetIndex>> segments;
    MeshCore::MeshComponents comp(_kernel);
    comp.SearchForComponents(MeshCore::MeshComponents::OverEdge, segments);
    return segments.size();
}

void MeshObject::removeComponents(unsigned long count)
{
    std::vector<FacetIndex> removeIndices;
    MeshCore::MeshTopoAlgorithm(_kernel).FindComponents(count, removeIndices);
    _kernel.DeleteFacets(removeIndices);
    deletedFacets(removeIndices);
}

unsigned long MeshObject::getPointDegree(const std::vector<FacetIndex>& indices,
                                         std::vector<PointIndex>& point_degree) const
{
    const MeshCore::MeshFacetArray& faces = _kernel.GetFacets();
    std::vector<PointIndex> pointDeg(_kernel.CountPoints());

    for (const auto& face : faces) {
        pointDeg[face._aulPoints[0]]++;
        pointDeg[face._aulPoints[1]]++;
        pointDeg[face._aulPoints[2]]++;
    }

    for (FacetIndex it : indices) {
        const MeshCore::MeshFacet& face = faces[it];
        pointDeg[face._aulPoints[0]]--;
        pointDeg[face._aulPoints[1]]--;
        pointDeg[face._aulPoints[2]]--;
    }

    unsigned long countInvalids = std::count_if(pointDeg.begin(), pointDeg.end(), [](PointIndex v) {
        return v == 0;
    });

    point_degree.swap(pointDeg);
    return countInvalids;
}

void MeshObject::fillupHoles(unsigned long length,
                             int level,
                             MeshCore::AbstractPolygonTriangulator& cTria)
{
    std::list<std::vector<PointIndex>> aFailed;
    MeshCore::MeshTopoAlgorithm topalg(_kernel);
    topalg.FillupHoles(length, level, cTria, aFailed);
}

void MeshObject::offset(float fSize)
{
    std::vector<Base::Vector3f> normals = _kernel.CalcVertexNormals();

    unsigned int i = 0;
    // go through all the vertex normals
    for (std::vector<Base::Vector3f>::iterator It = normals.begin(); It != normals.end();
         ++It, i++) {
        // and move each mesh point in the normal direction
        _kernel.MovePoint(i, It->Normalize() * fSize);
    }
    _kernel.RecalcBoundBox();
}

void MeshObject::offsetSpecial2(float fSize)
{
    Base::Builder3D builder;
    std::vector<Base::Vector3f> PointNormals = _kernel.CalcVertexNormals();
    std::vector<Base::Vector3f> FaceNormals;
    std::set<FacetIndex> fliped;

    MeshCore::MeshFacetIterator it(_kernel);
    for (it.Init(); it.More(); it.Next()) {
        FaceNormals.push_back(it->GetNormal().Normalize());
    }

    unsigned int i = 0;

    // go through all the vertex normals
    for (std::vector<Base::Vector3f>::iterator It = PointNormals.begin(); It != PointNormals.end();
         ++It, i++) {
        Base::Line3f line {_kernel.GetPoint(i), _kernel.GetPoint(i) + It->Normalize() * fSize};
        Base::DrawStyle drawStyle;
        builder.addNode(Base::LineItem {line, drawStyle});
        // and move each mesh point in the normal direction
        _kernel.MovePoint(i, It->Normalize() * fSize);
    }
    _kernel.RecalcBoundBox();

    MeshCore::MeshTopoAlgorithm alg(_kernel);

    for (int l = 0; l < 1; l++) {
        for (it.Init(), i = 0; it.More(); it.Next(), i++) {
            if (it->IsFlag(MeshCore::MeshFacet::INVALID)) {
                continue;
            }
            // calculate the angle between them
            float angle = acos((FaceNormals[i] * it->GetNormal())
                               / (it->GetNormal().Length() * FaceNormals[i].Length()));
            if (angle > 1.6) {
                Base::DrawStyle drawStyle;
                drawStyle.pointSize = 4.0F;
                Base::PointItem item {it->GetGravityPoint(),
                                      drawStyle,
                                      Base::ColorRGB {1.0F, 0.0F, 0.0F}};
                builder.addNode(item);
                fliped.insert(it.Position());
            }
        }

        // if there are no flipped triangles -> stop
        // int f =fliped.size();
        if (fliped.empty()) {
            break;
        }

        for (FacetIndex It : fliped) {
            alg.CollapseFacet(It);
        }
        fliped.clear();
    }

    alg.Cleanup();

    // search for intersected facets
    MeshCore::MeshEvalSelfIntersection eval(_kernel);
    std::vector<std::pair<FacetIndex, FacetIndex>> faces;
    eval.GetIntersections(faces);
    builder.saveToLog();
}

void MeshObject::offsetSpecial(float fSize, float zmax, float zmin)
{
    std::vector<Base::Vector3f> normals = _kernel.CalcVertexNormals();

    unsigned int i = 0;
    // go through all the vertex normals
    for (std::vector<Base::Vector3f>::iterator It = normals.begin(); It != normals.end();
         ++It, i++) {
        auto Pnt = _kernel.GetPoint(i);
        if (Pnt.z < zmax && Pnt.z > zmin) {
            Pnt.z = 0;
            _kernel.MovePoint(i, Pnt.Normalize() * fSize);
        }
        else {
            // and move each mesh point in the normal direction
            _kernel.MovePoint(i, It->Normalize() * fSize);
        }
    }
}

void MeshObject::clear()
{
    _kernel.Clear();
    this->_segments.clear();
    setTransform(Base::Matrix4D());
}

void MeshObject::transformToEigenSystem()
{
    MeshCore::MeshEigensystem cMeshEval(_kernel);
    cMeshEval.Evaluate();
    this->setTransform(cMeshEval.Transform());
}

Base::Matrix4D MeshObject::getEigenSystem(Base::Vector3d& v) const
{
    MeshCore::MeshEigensystem cMeshEval(_kernel);
    cMeshEval.Evaluate();
    Base::Vector3f uvw = cMeshEval.GetBoundings();
    v.Set(uvw.x, uvw.y, uvw.z);
    return cMeshEval.Transform();
}

void MeshObject::movePoint(PointIndex index, const Base::Vector3d& v)
{
    // v is a vector, hence we must not apply the translation part
    // of the transformation to the vector
    Base::Vector3d vec(v);
    vec.x += _Mtrx[0][3];
    vec.y += _Mtrx[1][3];
    vec.z += _Mtrx[2][3];
    _kernel.MovePoint(index, transformPointToInside(vec));
}

void MeshObject::setPoint(PointIndex index, const Base::Vector3d& p)
{
    _kernel.SetPoint(index, transformPointToInside(p));
}

void MeshObject::smooth(int iterations, float d_max)
{
    _kernel.Smooth(iterations, d_max);
}

void MeshObject::decimate(float fTolerance, float fReduction)
{
    MeshCore::MeshSimplify dm(this->_kernel);
    dm.simplify(fTolerance, fReduction);
}

void MeshObject::decimate(int targetSize)
{
    MeshCore::MeshSimplify dm(this->_kernel);
    dm.simplify(targetSize);
}

Base::Vector3d MeshObject::getPointNormal(PointIndex index) const
{
    std::vector<Base::Vector3f> temp = _kernel.CalcVertexNormals();
    Base::Vector3d normal = transformVectorToOutside(temp[index]);
    normal.Normalize();
    return normal;
}

std::vector<Base::Vector3d> MeshObject::getPointNormals() const
{
    std::vector<Base::Vector3f> temp = _kernel.CalcVertexNormals();

    std::vector<Base::Vector3d> normals = transformVectorsToOutside(temp);
    for (auto& n : normals) {
        n.Normalize();
    }
    return normals;
}

void MeshObject::crossSections(const std::vector<MeshObject::TPlane>& planes,
                               std::vector<MeshObject::TPolylines>& sections,
                               float fMinEps,
                               bool bConnectPolygons) const
{
    MeshCore::MeshKernel kernel(this->_kernel);
    kernel.Transform(this->_Mtrx);

    MeshCore::MeshFacetGrid grid(kernel);
    MeshCore::MeshAlgorithm algo(kernel);
    for (const auto& plane : planes) {
        MeshObject::TPolylines polylines;
        algo.CutWithPlane(plane.first, plane.second, grid, polylines, fMinEps, bConnectPolygons);
        sections.push_back(polylines);
    }
}

void MeshObject::cut(const Base::Polygon2d& polygon2d,
                     const Base::ViewProjMethod& proj,
                     MeshObject::CutType type)
{
    MeshCore::MeshKernel kernel(this->_kernel);
    kernel.Transform(getTransform());

    MeshCore::MeshAlgorithm meshAlg(kernel);
    std::vector<FacetIndex> check;

    bool inner {};
    switch (type) {
        case INNER:
            inner = true;
            break;
        case OUTER:
            inner = false;
            break;
        default:
            inner = true;
            break;
    }

    MeshCore::MeshFacetGrid meshGrid(kernel);
    meshAlg.CheckFacets(meshGrid, &proj, polygon2d, inner, check);
    if (!check.empty()) {
        this->deleteFacets(check);
    }
}

void MeshObject::trim(const Base::Polygon2d& polygon2d,
                      const Base::ViewProjMethod& proj,
                      MeshObject::CutType type)
{
    MeshCore::MeshKernel kernel(this->_kernel);
    kernel.Transform(getTransform());

    MeshCore::MeshTrimming trim(kernel, &proj, polygon2d);
    std::vector<FacetIndex> check;
    std::vector<MeshCore::MeshGeomFacet> triangle;

    switch (type) {
        case INNER:
            trim.SetInnerOrOuter(MeshCore::MeshTrimming::INNER);
            break;
        case OUTER:
            trim.SetInnerOrOuter(MeshCore::MeshTrimming::OUTER);
            break;
    }

    MeshCore::MeshFacetGrid meshGrid(kernel);
    trim.CheckFacets(meshGrid, check);
    trim.TrimFacets(check, triangle);
    if (!check.empty()) {
        this->deleteFacets(check);
    }

    // Re-add some triangles
    if (!triangle.empty()) {
        Base::Matrix4D mat(getTransform());
        mat.inverse();
        for (auto& it : triangle) {
            it.Transform(mat);
        }
        this->_kernel.AddFacets(triangle);
    }
}

void MeshObject::trimByPlane(const Base::Vector3f& base, const Base::Vector3f& normal)
{
    MeshCore::MeshTrimByPlane trim(this->_kernel);
    std::vector<FacetIndex> trimFacets, removeFacets;
    std::vector<MeshCore::MeshGeomFacet> triangle;

    // Apply the inverted mesh placement to the plane because the trimming is done
    // on the untransformed mesh data
    Base::Vector3f basePlane, normalPlane;
    Base::Placement meshPlacement = getPlacement();
    meshPlacement.invert();
    meshPlacement.multVec(base, basePlane);
    meshPlacement.getRotation().multVec(normal, normalPlane);

    MeshCore::MeshFacetGrid meshGrid(this->_kernel);
    trim.CheckFacets(meshGrid, basePlane, normalPlane, trimFacets, removeFacets);
    trim.TrimFacets(trimFacets, basePlane, normalPlane, triangle);
    if (!removeFacets.empty()) {
        this->deleteFacets(removeFacets);
    }
    if (!triangle.empty()) {
        this->_kernel.AddFacets(triangle);
    }
}

MeshObject* MeshObject::unite(const MeshObject& mesh) const
{
    MeshCore::MeshKernel result;
    MeshCore::MeshKernel kernel1(this->_kernel);
    kernel1.Transform(this->_Mtrx);
    MeshCore::MeshKernel kernel2(mesh._kernel);
    kernel2.Transform(mesh._Mtrx);
    MeshCore::SetOperations setOp(kernel1,
                                  kernel2,
                                  result,
                                  MeshCore::SetOperations::Union,
                                  Epsilon);
    setOp.Do();
    return new MeshObject(result);
}

MeshObject* MeshObject::intersect(const MeshObject& mesh) const
{
    MeshCore::MeshKernel result;
    MeshCore::MeshKernel kernel1(this->_kernel);
    kernel1.Transform(this->_Mtrx);
    MeshCore::MeshKernel kernel2(mesh._kernel);
    kernel2.Transform(mesh._Mtrx);
    MeshCore::SetOperations setOp(kernel1,
                                  kernel2,
                                  result,
                                  MeshCore::SetOperations::Intersect,
                                  Epsilon);
    setOp.Do();
    return new MeshObject(result);
}

MeshObject* MeshObject::subtract(const MeshObject& mesh) const
{
    MeshCore::MeshKernel result;
    MeshCore::MeshKernel kernel1(this->_kernel);
    kernel1.Transform(this->_Mtrx);
    MeshCore::MeshKernel kernel2(mesh._kernel);
    kernel2.Transform(mesh._Mtrx);
    MeshCore::SetOperations setOp(kernel1,
                                  kernel2,
                                  result,
                                  MeshCore::SetOperations::Difference,
                                  Epsilon);
    setOp.Do();
    return new MeshObject(result);
}

MeshObject* MeshObject::inner(const MeshObject& mesh) const
{
    MeshCore::MeshKernel result;
    MeshCore::MeshKernel kernel1(this->_kernel);
    kernel1.Transform(this->_Mtrx);
    MeshCore::MeshKernel kernel2(mesh._kernel);
    kernel2.Transform(mesh._Mtrx);
    MeshCore::SetOperations setOp(kernel1,
                                  kernel2,
                                  result,
                                  MeshCore::SetOperations::Inner,
                                  Epsilon);
    setOp.Do();
    return new MeshObject(result);
}

MeshObject* MeshObject::outer(const MeshObject& mesh) const
{
    MeshCore::MeshKernel result;
    MeshCore::MeshKernel kernel1(this->_kernel);
    kernel1.Transform(this->_Mtrx);
    MeshCore::MeshKernel kernel2(mesh._kernel);
    kernel2.Transform(mesh._Mtrx);
    MeshCore::SetOperations setOp(kernel1,
                                  kernel2,
                                  result,
                                  MeshCore::SetOperations::Outer,
                                  Epsilon);
    setOp.Do();
    return new MeshObject(result);
}

std::vector<std::vector<Base::Vector3f>>
MeshObject::section(const MeshObject& mesh, bool connectLines, float fMinDist) const
{
    MeshCore::MeshKernel kernel1(this->_kernel);
    kernel1.Transform(this->_Mtrx);
    MeshCore::MeshKernel kernel2(mesh._kernel);
    kernel2.Transform(mesh._Mtrx);
    std::vector<std::vector<Base::Vector3f>> lines;

    MeshCore::MeshIntersection sec(kernel1, kernel2, fMinDist);
    std::list<MeshCore::MeshIntersection::Tuple> tuple;
    sec.getIntersection(tuple);

    if (!connectLines) {
        for (const auto& it : tuple) {
            std::vector<Base::Vector3f> curve;
            curve.push_back(it.p1);
            curve.push_back(it.p2);
            lines.push_back(curve);
        }
    }
    else {
        std::list<std::list<MeshCore::MeshIntersection::Triple>> triple;
        sec.connectLines(false, tuple, triple);

        for (const auto& it : triple) {
            std::vector<Base::Vector3f> curve;
            curve.reserve(it.size());

            for (const auto& jt : it) {
                curve.push_back(jt.p);
            }
            lines.push_back(curve);
        }
    }

    return lines;
}

void MeshObject::refine()
{
    unsigned long cnt = _kernel.CountFacets();
    MeshCore::MeshFacetIterator cF(_kernel);
    MeshCore::MeshTopoAlgorithm topalg(_kernel);

    // x < 30 deg => cos(x) > sqrt(3)/2 or x > 120 deg => cos(x) < -0.5
    for (unsigned long i = 0; i < cnt; i++) {
        cF.Set(i);
        if (!cF->IsDeformed(0.86f, -0.5f)) {
            topalg.InsertVertexAndSwapEdge(i, cF->GetGravityPoint(), 0.1f);
        }
    }

    // clear the segments because we don't know how the new
    // topology looks like
    this->_segments.clear();
}

void MeshObject::removeNeedles(float length)
{
    unsigned long count = _kernel.CountFacets();
    MeshCore::MeshRemoveNeedles eval(_kernel, length);
    eval.Fixup();
    if (_kernel.CountFacets() < count) {
        this->_segments.clear();
    }
}

void MeshObject::validateCaps(float fMaxAngle, float fSplitFactor)
{
    MeshCore::MeshFixCaps eval(_kernel, fMaxAngle, fSplitFactor);
    eval.Fixup();
}

void MeshObject::optimizeTopology(float fMaxAngle)
{
    MeshCore::MeshTopoAlgorithm topalg(_kernel);
    if (fMaxAngle > 0.0f) {
        topalg.OptimizeTopology(fMaxAngle);
    }
    else {
        topalg.OptimizeTopology();
    }

    // clear the segments because we don't know how the new
    // topology looks like
    this->_segments.clear();
}

void MeshObject::optimizeEdges()
{
    MeshCore::MeshTopoAlgorithm topalg(_kernel);
    topalg.AdjustEdgesToCurvatureDirection();
}

void MeshObject::splitEdges()
{
    std::vector<std::pair<FacetIndex, FacetIndex>> adjacentFacet;
    MeshCore::MeshAlgorithm alg(_kernel);
    alg.ResetFacetFlag(MeshCore::MeshFacet::VISIT);
    const MeshCore::MeshFacetArray& rFacets = _kernel.GetFacets();
    for (MeshCore::MeshFacetArray::_TConstIterator pF = rFacets.begin(); pF != rFacets.end();
         ++pF) {
        int id = 2;
        if (pF->_aulNeighbours[id] != MeshCore::FACET_INDEX_MAX) {
            const MeshCore::MeshFacet& rFace = rFacets[pF->_aulNeighbours[id]];
            if (!pF->IsFlag(MeshCore::MeshFacet::VISIT)
                && !rFace.IsFlag(MeshCore::MeshFacet::VISIT)) {
                pF->SetFlag(MeshCore::MeshFacet::VISIT);
                rFace.SetFlag(MeshCore::MeshFacet::VISIT);
                adjacentFacet.emplace_back(pF - rFacets.begin(), pF->_aulNeighbours[id]);
            }
        }
    }

    MeshCore::MeshFacetIterator cIter(_kernel);
    MeshCore::MeshTopoAlgorithm topalg(_kernel);
    for (const auto& it : adjacentFacet) {
        cIter.Set(it.first);
        Base::Vector3f mid = 0.5f * (cIter->_aclPoints[0] + cIter->_aclPoints[2]);
        topalg.SplitEdge(it.first, it.second, mid);
    }

    // clear the segments because we don't know how the new
    // topology looks like
    this->_segments.clear();
}

void MeshObject::splitEdge(FacetIndex facet, FacetIndex neighbour, const Base::Vector3f& v)
{
    MeshCore::MeshTopoAlgorithm topalg(_kernel);
    topalg.SplitEdge(facet, neighbour, v);
}

void MeshObject::splitFacet(FacetIndex facet, const Base::Vector3f& v1, const Base::Vector3f& v2)
{
    MeshCore::MeshTopoAlgorithm topalg(_kernel);
    topalg.SplitFacet(facet, v1, v2);
}

void MeshObject::swapEdge(FacetIndex facet, FacetIndex neighbour)
{
    MeshCore::MeshTopoAlgorithm topalg(_kernel);
    topalg.SwapEdge(facet, neighbour);
}

void MeshObject::collapseEdge(FacetIndex facet, FacetIndex neighbour)
{
    MeshCore::MeshTopoAlgorithm topalg(_kernel);
    topalg.CollapseEdge(facet, neighbour);

    std::vector<FacetIndex> remFacets;
    remFacets.push_back(facet);
    remFacets.push_back(neighbour);
    deletedFacets(remFacets);
}

void MeshObject::collapseFacet(FacetIndex facet)
{
    MeshCore::MeshTopoAlgorithm topalg(_kernel);
    topalg.CollapseFacet(facet);

    std::vector<FacetIndex> remFacets;
    remFacets.push_back(facet);
    deletedFacets(remFacets);
}

void MeshObject::collapseFacets(const std::vector<FacetIndex>& facets)
{
    MeshCore::MeshTopoAlgorithm alg(_kernel);
    for (FacetIndex it : facets) {
        alg.CollapseFacet(it);
    }

    deletedFacets(facets);
}

void MeshObject::insertVertex(FacetIndex facet, const Base::Vector3f& v)
{
    MeshCore::MeshTopoAlgorithm topalg(_kernel);
    topalg.InsertVertex(facet, v);
}

void MeshObject::snapVertex(FacetIndex facet, const Base::Vector3f& v)
{
    MeshCore::MeshTopoAlgorithm topalg(_kernel);
    topalg.SnapVertex(facet, v);
}

unsigned long MeshObject::countNonUniformOrientedFacets() const
{
    MeshCore::MeshEvalOrientation cMeshEval(_kernel);
    std::vector<FacetIndex> inds = cMeshEval.GetIndices();
    return inds.size();
}

void MeshObject::flipNormals()
{
    MeshCore::MeshTopoAlgorithm alg(_kernel);
    alg.FlipNormals();
}

void MeshObject::harmonizeNormals()
{
    MeshCore::MeshTopoAlgorithm alg(_kernel);
    alg.HarmonizeNormals();
}

bool MeshObject::hasNonManifolds() const
{
    MeshCore::MeshEvalTopology cMeshEval(_kernel);
    return !cMeshEval.Evaluate();
}

void MeshObject::removeNonManifolds()
{
    MeshCore::MeshEvalTopology f_eval(_kernel);
    if (!f_eval.Evaluate()) {
        MeshCore::MeshFixTopology f_fix(_kernel, f_eval.GetFacets());
        f_fix.Fixup();
        deletedFacets(f_fix.GetDeletedFaces());
    }
}

void MeshObject::removeNonManifoldPoints()
{
    MeshCore::MeshEvalPointManifolds p_eval(_kernel);
    if (!p_eval.Evaluate()) {
        std::vector<FacetIndex> faces;
        p_eval.GetFacetIndices(faces);
        deleteFacets(faces);
    }
}

bool MeshObject::hasSelfIntersections() const
{
    MeshCore::MeshEvalSelfIntersection cMeshEval(_kernel);
    return !cMeshEval.Evaluate();
}

MeshObject::TFacePairs MeshObject::getSelfIntersections() const
{
    MeshCore::MeshEvalSelfIntersection eval(getKernel());
    MeshObject::TFacePairs pairs;
    eval.GetIntersections(pairs);
    return pairs;
}

std::vector<Base::Line3d>
MeshObject::getSelfIntersections(const MeshObject::TFacePairs& facets) const
{
    MeshCore::MeshEvalSelfIntersection eval(getKernel());
    using Section = std::pair<Base::Vector3f, Base::Vector3f>;
    std::vector<Section> selfPoints;
    eval.GetIntersections(facets, selfPoints);

    std::vector<Base::Line3d> lines;
    lines.reserve(selfPoints.size());

    Base::Matrix4D mat(getTransform());
    std::transform(selfPoints.begin(),
                   selfPoints.end(),
                   std::back_inserter(lines),
                   [&mat](const Section& l) {
                       return Base::Line3d(mat * Base::convertTo<Base::Vector3d>(l.first),
                                           mat * Base::convertTo<Base::Vector3d>(l.second));
                   });
    return lines;
}

void MeshObject::removeSelfIntersections()
{
    std::vector<std::pair<FacetIndex, FacetIndex>> selfIntersections;
    MeshCore::MeshEvalSelfIntersection cMeshEval(_kernel);
    cMeshEval.GetIntersections(selfIntersections);

    if (!selfIntersections.empty()) {
        MeshCore::MeshFixSelfIntersection cMeshFix(_kernel, selfIntersections);
        deleteFacets(cMeshFix.GetFacets());
    }
}

void MeshObject::removeSelfIntersections(const std::vector<FacetIndex>& indices)
{
    // make sure that the number of indices is even and are in range
    if (indices.size() % 2 != 0) {
        return;
    }
    unsigned long cntfacets = _kernel.CountFacets();
    if (std::find_if(indices.begin(),
                     indices.end(),
                     [cntfacets](FacetIndex v) {
                         return v >= cntfacets;
                     })
        < indices.end()) {
        return;
    }
    std::vector<std::pair<FacetIndex, FacetIndex>> selfIntersections;
    std::vector<FacetIndex>::const_iterator it;
    for (it = indices.begin(); it != indices.end();) {
        FacetIndex id1 = *it;
        ++it;
        FacetIndex id2 = *it;
        ++it;
        selfIntersections.emplace_back(id1, id2);
    }

    if (!selfIntersections.empty()) {
        MeshCore::MeshFixSelfIntersection cMeshFix(_kernel, selfIntersections);
        cMeshFix.Fixup();
        this->_segments.clear();
    }
}

void MeshObject::removeFoldsOnSurface()
{
    std::vector<FacetIndex> indices;
    MeshCore::MeshEvalFoldsOnSurface s_eval(_kernel);
    MeshCore::MeshEvalFoldOversOnSurface f_eval(_kernel);

    f_eval.Evaluate();
    std::vector<FacetIndex> inds = f_eval.GetIndices();

    s_eval.Evaluate();
    std::vector<FacetIndex> inds1 = s_eval.GetIndices();

    // remove duplicates
    inds.insert(inds.end(), inds1.begin(), inds1.end());
    std::sort(inds.begin(), inds.end());
    inds.erase(std::unique(inds.begin(), inds.end()), inds.end());

    if (!inds.empty()) {
        deleteFacets(inds);
    }

    // do this as additional check after removing folds on closed area
    for (int i = 0; i < 5; i++) {
        MeshCore::MeshEvalFoldsOnBoundary b_eval(_kernel);
        if (b_eval.Evaluate()) {
            break;
        }
        inds = b_eval.GetIndices();
        if (!inds.empty()) {
            deleteFacets(inds);
        }
    }
}

void MeshObject::removeFullBoundaryFacets()
{
    std::vector<FacetIndex> facets;
    if (!MeshCore::MeshEvalBorderFacet(_kernel, facets).Evaluate()) {
        deleteFacets(facets);
    }
}

bool MeshObject::hasInvalidPoints() const
{
    MeshCore::MeshEvalNaNPoints nan(_kernel);
    return !nan.GetIndices().empty();
}

void MeshObject::removeInvalidPoints()
{
    MeshCore::MeshEvalNaNPoints nan(_kernel);
    deletePoints(nan.GetIndices());
}

bool MeshObject::hasPointsOnEdge() const
{
    MeshCore::MeshEvalPointOnEdge nan(_kernel);
    return !nan.Evaluate();
}

void MeshObject::removePointsOnEdge(bool fillBoundary)
{
    MeshCore::MeshFixPointOnEdge nan(_kernel, fillBoundary);
    nan.Fixup();
}

void MeshObject::mergeFacets()
{
    unsigned long count = _kernel.CountFacets();
    MeshCore::MeshFixMergeFacets merge(_kernel);
    merge.Fixup();
    if (_kernel.CountFacets() < count) {
        this->_segments.clear();
    }
}

void MeshObject::validateIndices()
{
    unsigned long count = _kernel.CountFacets();

    // for invalid neighbour indices we don't need to check first
    // but start directly with the validation
    MeshCore::MeshFixNeighbourhood fix(_kernel);
    fix.Fixup();

    MeshCore::MeshEvalRangeFacet rf(_kernel);
    if (!rf.Evaluate()) {
        MeshCore::MeshFixRangeFacet fix(_kernel);
        fix.Fixup();
    }

    MeshCore::MeshEvalRangePoint rp(_kernel);
    if (!rp.Evaluate()) {
        MeshCore::MeshFixRangePoint fix(_kernel);
        fix.Fixup();
    }

    MeshCore::MeshEvalCorruptedFacets cf(_kernel);
    if (!cf.Evaluate()) {
        MeshCore::MeshFixCorruptedFacets fix(_kernel);
        fix.Fixup();
    }

    if (_kernel.CountFacets() < count) {
        this->_segments.clear();
    }
}

bool MeshObject::hasInvalidNeighbourhood() const
{
    MeshCore::MeshEvalNeighbourhood eval(_kernel);
    return !eval.Evaluate();
}

bool MeshObject::hasPointsOutOfRange() const
{
    MeshCore::MeshEvalRangePoint eval(_kernel);
    return !eval.Evaluate();
}

bool MeshObject::hasFacetsOutOfRange() const
{
    MeshCore::MeshEvalRangeFacet eval(_kernel);
    return !eval.Evaluate();
}

bool MeshObject::hasCorruptedFacets() const
{
    MeshCore::MeshEvalCorruptedFacets eval(_kernel);
    return !eval.Evaluate();
}

void MeshObject::validateDeformations(float fMaxAngle, float fEps)
{
    unsigned long count = _kernel.CountFacets();
    MeshCore::MeshFixDeformedFacets eval(_kernel,
                                         Base::toRadians(15.0f),
                                         Base::toRadians(150.0f),
                                         fMaxAngle,
                                         fEps);
    eval.Fixup();
    if (_kernel.CountFacets() < count) {
        this->_segments.clear();
    }
}

void MeshObject::validateDegenerations(float fEps)
{
    unsigned long count = _kernel.CountFacets();
    MeshCore::MeshFixDegeneratedFacets eval(_kernel, fEps);
    eval.Fixup();
    if (_kernel.CountFacets() < count) {
        this->_segments.clear();
    }
}

void MeshObject::removeDuplicatedPoints()
{
    unsigned long count = _kernel.CountFacets();
    MeshCore::MeshFixDuplicatePoints eval(_kernel);
    eval.Fixup();
    if (_kernel.CountFacets() < count) {
        this->_segments.clear();
    }
}

void MeshObject::removeDuplicatedFacets()
{
    unsigned long count = _kernel.CountFacets();
    MeshCore::MeshFixDuplicateFacets eval(_kernel);
    eval.Fixup();
    if (_kernel.CountFacets() < count) {
        this->_segments.clear();
    }
}

MeshObject* MeshObject::createMeshFromList(Py::List& list)
{
    std::vector<MeshCore::MeshGeomFacet> facets;
    MeshCore::MeshGeomFacet facet;
    int i = 0;
    for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
        Py::List item(*it);
        for (int j = 0; j < 3; j++) {
            Py::Float value(item[j]);
            facet._aclPoints[i][j] = (float)value;
        }
        if (++i == 3) {
            i = 0;
            facet.CalcNormal();
            facets.push_back(facet);
        }
    }

    Base::EmptySequencer seq;
    std::unique_ptr<MeshObject> mesh(new MeshObject);
    // mesh->addFacets(facets);
    mesh->getKernel() = facets;
    return mesh.release();
}

MeshObject* MeshObject::createSphere(float radius, int sampling)
{
    // load the 'BuildRegularGeoms' module
    Base::PyGILStateLocker lock;
    try {
        Py::Module module(PyImport_ImportModule("BuildRegularGeoms"), true);
        if (module.isNull()) {
            return nullptr;
        }
        Py::Dict dict = module.getDict();
        Py::Callable call(dict.getItem("Sphere"));
        Py::Tuple args(2);
        args.setItem(0, Py::Float(radius));
        args.setItem(1, Py::Long(sampling));
        Py::List list(call.apply(args));
        return createMeshFromList(list);
    }
    catch (Py::Exception& e) {
        e.clear();
    }

    return nullptr;
}

MeshObject* MeshObject::createEllipsoid(float radius1, float radius2, int sampling)
{
    // load the 'BuildRegularGeoms' module
    Base::PyGILStateLocker lock;
    try {
        Py::Module module(PyImport_ImportModule("BuildRegularGeoms"), true);
        if (module.isNull()) {
            return nullptr;
        }
        Py::Dict dict = module.getDict();
        Py::Callable call(dict.getItem("Ellipsoid"));
        Py::Tuple args(3);
        args.setItem(0, Py::Float(radius1));
        args.setItem(1, Py::Float(radius2));
        args.setItem(2, Py::Long(sampling));
        Py::List list(call.apply(args));
        return createMeshFromList(list);
    }
    catch (Py::Exception& e) {
        e.clear();
    }

    return nullptr;
}

MeshObject*
MeshObject::createCylinder(float radius, float length, int closed, float edgelen, int sampling)
{
    // load the 'BuildRegularGeoms' module
    Base::PyGILStateLocker lock;
    try {
        Py::Module module(PyImport_ImportModule("BuildRegularGeoms"), true);
        if (module.isNull()) {
            return nullptr;
        }
        Py::Dict dict = module.getDict();
        Py::Callable call(dict.getItem("Cylinder"));
        Py::Tuple args(5);
        args.setItem(0, Py::Float(radius));
        args.setItem(1, Py::Float(length));
        args.setItem(2, Py::Long(closed));
        args.setItem(3, Py::Float(edgelen));
        args.setItem(4, Py::Long(sampling));
        Py::List list(call.apply(args));
        return createMeshFromList(list);
    }
    catch (Py::Exception& e) {
        e.clear();
    }

    return nullptr;
}

MeshObject* MeshObject::createCone(float radius1,
                                   float radius2,
                                   float len,
                                   int closed,
                                   float edgelen,
                                   int sampling)
{
    // load the 'BuildRegularGeoms' module
    Base::PyGILStateLocker lock;
    try {
        Py::Module module(PyImport_ImportModule("BuildRegularGeoms"), true);
        if (module.isNull()) {
            return nullptr;
        }
        Py::Dict dict = module.getDict();
        Py::Callable call(dict.getItem("Cone"));
        Py::Tuple args(6);
        args.setItem(0, Py::Float(radius1));
        args.setItem(1, Py::Float(radius2));
        args.setItem(2, Py::Float(len));
        args.setItem(3, Py::Long(closed));
        args.setItem(4, Py::Float(edgelen));
        args.setItem(5, Py::Long(sampling));
        Py::List list(call.apply(args));
        return createMeshFromList(list);
    }
    catch (Py::Exception& e) {
        e.clear();
    }

    return nullptr;
}

MeshObject* MeshObject::createTorus(float radius1, float radius2, int sampling)
{
    // load the 'BuildRegularGeoms' module
    Base::PyGILStateLocker lock;
    try {
        Py::Module module(PyImport_ImportModule("BuildRegularGeoms"), true);
        if (module.isNull()) {
            return nullptr;
        }
        Py::Dict dict = module.getDict();
        Py::Callable call(dict.getItem("Toroid"));
        Py::Tuple args(3);
        args.setItem(0, Py::Float(radius1));
        args.setItem(1, Py::Float(radius2));
        args.setItem(2, Py::Long(sampling));
        Py::List list(call.apply(args));
        return createMeshFromList(list);
    }
    catch (Py::Exception& e) {
        e.clear();
    }

    return nullptr;
}

MeshObject* MeshObject::createCube(float length, float width, float height)
{
    // load the 'BuildRegularGeoms' module
    Base::PyGILStateLocker lock;
    try {
        Py::Module module(PyImport_ImportModule("BuildRegularGeoms"), true);
        if (module.isNull()) {
            return nullptr;
        }
        Py::Dict dict = module.getDict();
        Py::Callable call(dict.getItem("Cube"));
        Py::Tuple args(3);
        args.setItem(0, Py::Float(length));
        args.setItem(1, Py::Float(width));
        args.setItem(2, Py::Float(height));
        Py::List list(call.apply(args));
        return createMeshFromList(list);
    }
    catch (Py::Exception& e) {
        e.clear();
    }

    return nullptr;
}

MeshObject* MeshObject::createCube(float length, float width, float height, float edgelen)
{
    // load the 'BuildRegularGeoms' module
    Base::PyGILStateLocker lock;
    try {
        Py::Module module(PyImport_ImportModule("BuildRegularGeoms"), true);
        if (module.isNull()) {
            return nullptr;
        }
        Py::Dict dict = module.getDict();
        Py::Callable call(dict.getItem("FineCube"));
        Py::Tuple args(4);
        args.setItem(0, Py::Float(length));
        args.setItem(1, Py::Float(width));
        args.setItem(2, Py::Float(height));
        args.setItem(3, Py::Float(edgelen));
        Py::List list(call.apply(args));
        return createMeshFromList(list);
    }
    catch (Py::Exception& e) {
        e.clear();
    }

    return nullptr;
}

MeshObject* MeshObject::createCube(const Base::BoundBox3d& bbox)
{
    using Corner = Base::BoundBox3d::CORNER;
    std::vector<MeshCore::MeshGeomFacet> facets;
    auto createFacet = [&bbox](Corner p1, Corner p2, Corner p3) {
        MeshCore::MeshGeomFacet facet;
        facet._aclPoints[0] = Base::convertTo<Base::Vector3f>(bbox.CalcPoint(p1));
        facet._aclPoints[1] = Base::convertTo<Base::Vector3f>(bbox.CalcPoint(p2));
        facet._aclPoints[2] = Base::convertTo<Base::Vector3f>(bbox.CalcPoint(p3));
        facet.CalcNormal();
        return facet;
    };

    facets.push_back(createFacet(Corner::TLB, Corner::TLF, Corner::TRF));
    facets.push_back(createFacet(Corner::TLB, Corner::TRF, Corner::TRB));
    facets.push_back(createFacet(Corner::TLB, Corner::BLF, Corner::TLF));
    facets.push_back(createFacet(Corner::TLB, Corner::BLB, Corner::BLF));
    facets.push_back(createFacet(Corner::TLB, Corner::TRB, Corner::BRB));
    facets.push_back(createFacet(Corner::TLB, Corner::BRB, Corner::BLB));
    facets.push_back(createFacet(Corner::BLB, Corner::BRF, Corner::BLF));
    facets.push_back(createFacet(Corner::BLB, Corner::BRB, Corner::BRF));
    facets.push_back(createFacet(Corner::TLF, Corner::BRF, Corner::TRF));
    facets.push_back(createFacet(Corner::TLF, Corner::BLF, Corner::BRF));
    facets.push_back(createFacet(Corner::TRF, Corner::BRB, Corner::TRB));
    facets.push_back(createFacet(Corner::TRF, Corner::BRF, Corner::BRB));

    Base::EmptySequencer seq;
    std::unique_ptr<MeshObject> mesh(new MeshObject);
    mesh->getKernel() = facets;
    return mesh.release();
}

void MeshObject::addSegment(const Segment& s)
{
    addSegment(s.getIndices());
    this->_segments.back().setName(s.getName());
    this->_segments.back().setColor(s.getColor());
    this->_segments.back().save(s.isSaved());
    this->_segments.back()._modifykernel = s._modifykernel;
}

void MeshObject::addSegment(const std::vector<FacetIndex>& inds)
{
    unsigned long maxIndex = _kernel.CountFacets();
    for (FacetIndex it : inds) {
        if (it >= maxIndex) {
            throw Base::IndexError("Index out of range");
        }
    }

    this->_segments.emplace_back(this, inds, true);
}

const Segment& MeshObject::getSegment(unsigned long index) const
{
    return this->_segments[index];
}

Segment& MeshObject::getSegment(unsigned long index)
{
    return this->_segments[index];
}

MeshObject* MeshObject::meshFromSegment(const std::vector<FacetIndex>& indices) const
{
    MeshCore::MeshFacetArray facets;
    facets.reserve(indices.size());
    const MeshCore::MeshPointArray& kernel_p = _kernel.GetPoints();
    const MeshCore::MeshFacetArray& kernel_f = _kernel.GetFacets();
    for (FacetIndex it : indices) {
        facets.push_back(kernel_f[it]);
    }

    MeshCore::MeshKernel kernel;
    kernel.Merge(kernel_p, facets);

    return new MeshObject(kernel, _Mtrx);
}

std::vector<Segment> MeshObject::getSegmentsOfType(MeshObject::GeometryType type,
                                                   float dev,
                                                   unsigned long minFacets) const
{
    std::vector<Segment> segm;
    if (this->_kernel.CountFacets() == 0) {
        return segm;
    }

    MeshCore::MeshSegmentAlgorithm finder(this->_kernel);
    std::shared_ptr<MeshCore::MeshDistanceSurfaceSegment> surf;
    switch (type) {
        case PLANE:
            surf.reset(
                new MeshCore::MeshDistanceGenericSurfaceFitSegment(new MeshCore::PlaneSurfaceFit,
                                                                   this->_kernel,
                                                                   minFacets,
                                                                   dev));
            break;
        case CYLINDER:
            surf.reset(
                new MeshCore::MeshDistanceGenericSurfaceFitSegment(new MeshCore::CylinderSurfaceFit,
                                                                   this->_kernel,
                                                                   minFacets,
                                                                   dev));
            break;
        case SPHERE:
            surf.reset(
                new MeshCore::MeshDistanceGenericSurfaceFitSegment(new MeshCore::SphereSurfaceFit,
                                                                   this->_kernel,
                                                                   minFacets,
                                                                   dev));
            break;
        default:
            break;
    }

    if (surf.get()) {
        std::vector<MeshCore::MeshSurfaceSegmentPtr> surfaces;
        surfaces.push_back(surf);
        finder.FindSegments(surfaces);

        const std::vector<MeshCore::MeshSegment>& data = surf->GetSegments();
        for (const auto& it : data) {
            segm.emplace_back(this, it, false);
        }
    }

    return segm;
}

// ----------------------------------------------------------------------------

MeshObject::const_point_iterator::const_point_iterator(const MeshObject* mesh, PointIndex index)
    : _mesh(mesh)
    , _p_it(mesh->getKernel())
{
    this->_p_it.Set(index);
    this->_p_it.Transform(_mesh->_Mtrx);
    this->_point.Mesh = _mesh;
}

MeshObject::const_point_iterator::const_point_iterator(const MeshObject::const_point_iterator& fi) =
    default;

MeshObject::const_point_iterator::const_point_iterator(MeshObject::const_point_iterator&& fi) =
    default;

MeshObject::const_point_iterator::~const_point_iterator() = default;

MeshObject::const_point_iterator&
MeshObject::const_point_iterator::operator=(const MeshObject::const_point_iterator& pi) = default;

MeshObject::const_point_iterator&
MeshObject::const_point_iterator::operator=(MeshObject::const_point_iterator&& pi) = default;

void MeshObject::const_point_iterator::dereference()
{
    this->_point.x = _p_it->x;
    this->_point.y = _p_it->y;
    this->_point.z = _p_it->z;
    this->_point.Index = _p_it.Position();
}

const MeshPoint& MeshObject::const_point_iterator::operator*()
{
    dereference();
    return this->_point;
}

const MeshPoint* MeshObject::const_point_iterator::operator->()
{
    dereference();
    return &(this->_point);
}

bool MeshObject::const_point_iterator::operator==(const MeshObject::const_point_iterator& pi) const
{
    return (this->_mesh == pi._mesh) && (this->_p_it == pi._p_it);
}

bool MeshObject::const_point_iterator::operator!=(const MeshObject::const_point_iterator& pi) const
{
    return !operator==(pi);
}

MeshObject::const_point_iterator& MeshObject::const_point_iterator::operator++()
{
    ++(this->_p_it);
    return *this;
}

MeshObject::const_point_iterator& MeshObject::const_point_iterator::operator--()
{
    --(this->_p_it);
    return *this;
}

// ----------------------------------------------------------------------------

MeshObject::const_facet_iterator::const_facet_iterator(const MeshObject* mesh, FacetIndex index)
    : _mesh(mesh)
    , _f_it(mesh->getKernel())
{
    this->_f_it.Set(index);
    this->_f_it.Transform(_mesh->_Mtrx);
    this->_facet.Mesh = _mesh;
}

MeshObject::const_facet_iterator::const_facet_iterator(const MeshObject::const_facet_iterator& fi) =
    default;

MeshObject::const_facet_iterator::const_facet_iterator(MeshObject::const_facet_iterator&& fi) =
    default;

MeshObject::const_facet_iterator::~const_facet_iterator() = default;

MeshObject::const_facet_iterator&
MeshObject::const_facet_iterator::operator=(const MeshObject::const_facet_iterator& fi) = default;

MeshObject::const_facet_iterator&
MeshObject::const_facet_iterator::operator=(MeshObject::const_facet_iterator&& fi) = default;

void MeshObject::const_facet_iterator::dereference()
{
    this->_facet.MeshCore::MeshGeomFacet::operator=(*_f_it);
    this->_facet.Index = _f_it.Position();
    const MeshCore::MeshFacet& face = _f_it.GetReference();
    for (int i = 0; i < 3; i++) {
        this->_facet.PIndex[i] = face._aulPoints[i];
        this->_facet.NIndex[i] = face._aulNeighbours[i];
    }
}

Facet& MeshObject::const_facet_iterator::operator*()
{
    dereference();
    return this->_facet;
}

Facet* MeshObject::const_facet_iterator::operator->()
{
    dereference();
    return &(this->_facet);
}

bool MeshObject::const_facet_iterator::operator==(const MeshObject::const_facet_iterator& fi) const
{
    return (this->_mesh == fi._mesh) && (this->_f_it == fi._f_it);
}

bool MeshObject::const_facet_iterator::operator!=(const MeshObject::const_facet_iterator& fi) const
{
    return !operator==(fi);
}

MeshObject::const_facet_iterator& MeshObject::const_facet_iterator::operator++()
{
    ++(this->_f_it);
    return *this;
}

MeshObject::const_facet_iterator& MeshObject::const_facet_iterator::operator--()
{
    --(this->_f_it);
    return *this;
}
