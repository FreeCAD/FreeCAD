/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <boost/core/ignore_unused.hpp>
#include <numeric>

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp_Face.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt.hxx>

#include <QEventLoop>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentMap>
#endif

#include <Base/Console.h>
#include <Base/FutureWatcherProgress.h>
#include <Base/Sequencer.h>
#include <Base/Stream.h>

#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Grid.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Points/App/PointsFeature.h>
#include <Mod/Points/App/PointsGrid.h>

#include "InspectionFeature.h"


using namespace Inspection;
namespace sp = std::placeholders;

InspectActualMesh::InspectActualMesh(const Mesh::MeshObject& rMesh)
    : _mesh(rMesh.getKernel())
{
    Base::Matrix4D tmp;
    _clTrf = rMesh.getTransform();
    _bApply = _clTrf != tmp;
}

InspectActualMesh::~InspectActualMesh() = default;

unsigned long InspectActualMesh::countPoints() const
{
    return _mesh.CountPoints();
}

Base::Vector3f InspectActualMesh::getPoint(unsigned long index) const
{
    Base::Vector3f point = _mesh.GetPoint(index);
    if (_bApply) {
        _clTrf.multVec(point, point);
    }
    return point;
}

// ----------------------------------------------------------------

InspectActualPoints::InspectActualPoints(const Points::PointKernel& rPoints)
    : _rKernel(rPoints)
{}

unsigned long InspectActualPoints::countPoints() const
{
    return _rKernel.size();
}

Base::Vector3f InspectActualPoints::getPoint(unsigned long index) const
{
    Base::Vector3d pnt = _rKernel.getPoint(index);
    return Base::Vector3f(float(pnt.x), float(pnt.y), float(pnt.z));
}

// ----------------------------------------------------------------

InspectActualShape::InspectActualShape(const Part::TopoShape& shape)
    : _rShape(shape)
{
    Standard_Real deflection = _rShape.getAccuracy();
    fetchPoints(deflection);
}

void InspectActualShape::fetchPoints(double deflection)
{
    // get points from faces or sub-sampled edges
    TopTools_IndexedMapOfShape mapOfShapes;
    TopExp::MapShapes(_rShape.getShape(), TopAbs_FACE, mapOfShapes);
    if (!mapOfShapes.IsEmpty()) {
        std::vector<Data::ComplexGeoData::Facet> faces;
        _rShape.getFaces(points, faces, deflection);
    }
    else {
        TopExp::MapShapes(_rShape.getShape(), TopAbs_EDGE, mapOfShapes);
        if (!mapOfShapes.IsEmpty()) {
            std::vector<Data::ComplexGeoData::Line> lines;
            _rShape.getLines(points, lines, deflection);
        }
        else {
            std::vector<Base::Vector3d> normals;
            _rShape.getPoints(points, normals, deflection);
        }
    }
}

unsigned long InspectActualShape::countPoints() const
{
    return points.size();
}

Base::Vector3f InspectActualShape::getPoint(unsigned long index) const
{
    return Base::toVector<float>(points[index]);
}

// ----------------------------------------------------------------

namespace Inspection
{
class MeshInspectGrid: public MeshCore::MeshGrid
{
public:
    MeshInspectGrid(const MeshCore::MeshKernel& mesh, float fGridLen, const Base::Matrix4D& mat)
        : MeshCore::MeshGrid(mesh)
        , _transform(mat)
    {
        Base::BoundBox3f clBBMesh = _pclMesh->GetBoundBox().Transformed(mat);
        Rebuild(std::max<unsigned long>((unsigned long)(clBBMesh.LengthX() / fGridLen), 1),
                std::max<unsigned long>((unsigned long)(clBBMesh.LengthY() / fGridLen), 1),
                std::max<unsigned long>((unsigned long)(clBBMesh.LengthZ() / fGridLen), 1));
    }

    void Validate(const MeshCore::MeshKernel& kernel) override
    {
        // do nothing
        boost::ignore_unused(kernel);
    }

    void Validate()
    {
        // do nothing
    }

    bool Verify() const override
    {
        // do nothing
        return true;
    }

protected:
    void CalculateGridLength(unsigned long /*ulCtGrid*/, unsigned long /*ulMaxGrids*/) override
    {
        // do nothing
    }

    void CalculateGridLength(int /*iCtGridPerAxis*/) override
    {
        // do nothing
    }

    unsigned long HasElements() const override
    {
        return _pclMesh->CountFacets();
    }

    void Pos(const Base::Vector3f& rclPoint,
             unsigned long& rulX,
             unsigned long& rulY,
             unsigned long& rulZ) const
    {
        rulX = (unsigned long)((rclPoint.x - _fMinX) / _fGridLenX);
        rulY = (unsigned long)((rclPoint.y - _fMinY) / _fGridLenY);
        rulZ = (unsigned long)((rclPoint.z - _fMinZ) / _fGridLenZ);

        assert((rulX < _ulCtGridsX) && (rulY < _ulCtGridsY) && (rulZ < _ulCtGridsZ));
    }

    void AddFacet(const MeshCore::MeshGeomFacet& rclFacet, unsigned long ulFacetIndex)
    {
        unsigned long ulX1;
        unsigned long ulY1;
        unsigned long ulZ1;
        unsigned long ulX2;
        unsigned long ulY2;
        unsigned long ulZ2;

        Base::BoundBox3f clBB;
        clBB.Add(rclFacet._aclPoints[0]);
        clBB.Add(rclFacet._aclPoints[1]);
        clBB.Add(rclFacet._aclPoints[2]);

        Pos(Base::Vector3f(clBB.MinX, clBB.MinY, clBB.MinZ), ulX1, ulY1, ulZ1);
        Pos(Base::Vector3f(clBB.MaxX, clBB.MaxY, clBB.MaxZ), ulX2, ulY2, ulZ2);


        if ((ulX1 < ulX2) || (ulY1 < ulY2) || (ulZ1 < ulZ2)) {
            for (unsigned long ulX = ulX1; ulX <= ulX2; ulX++) {
                for (unsigned long ulY = ulY1; ulY <= ulY2; ulY++) {
                    for (unsigned long ulZ = ulZ1; ulZ <= ulZ2; ulZ++) {
                        if (rclFacet.IntersectBoundingBox(GetBoundBox(ulX, ulY, ulZ))) {
                            _aulGrid[ulX][ulY][ulZ].insert(ulFacetIndex);
                        }
                    }
                }
            }
        }
        else {
            _aulGrid[ulX1][ulY1][ulZ1].insert(ulFacetIndex);
        }
    }

    void InitGrid() override
    {
        unsigned long i, j;

        Base::BoundBox3f clBBMesh = _pclMesh->GetBoundBox().Transformed(_transform);

        float fLengthX = clBBMesh.LengthX();
        float fLengthY = clBBMesh.LengthY();
        float fLengthZ = clBBMesh.LengthZ();

        _fGridLenX = (1.0f + fLengthX) / float(_ulCtGridsX);
        _fMinX = clBBMesh.MinX - 0.5f;

        _fGridLenY = (1.0f + fLengthY) / float(_ulCtGridsY);
        _fMinY = clBBMesh.MinY - 0.5f;

        _fGridLenZ = (1.0f + fLengthZ) / float(_ulCtGridsZ);
        _fMinZ = clBBMesh.MinZ - 0.5f;

        _aulGrid.clear();
        _aulGrid.resize(_ulCtGridsX);
        for (i = 0; i < _ulCtGridsX; i++) {
            _aulGrid[i].resize(_ulCtGridsY);
            for (j = 0; j < _ulCtGridsY; j++) {
                _aulGrid[i][j].resize(_ulCtGridsZ);
            }
        }
    }

    void RebuildGrid() override
    {
        _ulCtElements = _pclMesh->CountFacets();
        InitGrid();

        unsigned long i = 0;
        MeshCore::MeshFacetIterator clFIter(*_pclMesh);
        clFIter.Transform(_transform);
        for (clFIter.Init(); clFIter.More(); clFIter.Next()) {
            AddFacet(*clFIter, i++);
        }
    }

private:
    Base::Matrix4D _transform;
};
}  // namespace Inspection

InspectNominalMesh::InspectNominalMesh(const Mesh::MeshObject& rMesh, float offset)
    : _mesh(rMesh.getKernel())
{
    Base::Matrix4D tmp;
    _clTrf = rMesh.getTransform();
    _bApply = _clTrf != tmp;

    // Max. limit of grid elements
    float fMaxGridElements = 8000000.0f;
    Base::BoundBox3f box = _mesh.GetBoundBox().Transformed(rMesh.getTransform());

    // estimate the minimum allowed grid length
    float fMinGridLen =
        (float)pow((box.LengthX() * box.LengthY() * box.LengthZ() / fMaxGridElements), 0.3333f);
    float fGridLen = 5.0f * MeshCore::MeshAlgorithm(_mesh).GetAverageEdgeLength();

    // We want to avoid to get too small grid elements otherwise building up the grid structure
    // would take too much time and memory. Having quite a dense grid speeds up more the following
    // algorithms extremely. Due to the issue above it's always a compromise between speed and
    // memory usage.
    fGridLen = std::max<float>(fMinGridLen, fGridLen);

    // build up grid structure to speed up algorithms
    _pGrid = new MeshInspectGrid(_mesh, fGridLen, rMesh.getTransform());
    _box = box;
    _box.Enlarge(offset);
}

InspectNominalMesh::~InspectNominalMesh()
{
    delete this->_pGrid;
}

float InspectNominalMesh::getDistance(const Base::Vector3f& point) const
{
    if (!_box.IsInBox(point)) {
        return FLT_MAX;  // must be inside bbox
    }

    std::vector<unsigned long> indices;
    //_pGrid->GetElements(point, indices);
    if (indices.empty()) {
        std::set<unsigned long> inds;
        _pGrid->MeshGrid::SearchNearestFromPoint(point, inds);
        indices.insert(indices.begin(), inds.begin(), inds.end());
    }

    float fMinDist = FLT_MAX;
    bool positive = true;
    for (unsigned long it : indices) {
        MeshCore::MeshGeomFacet geomFace = _mesh.GetFacet(it);
        if (_bApply) {
            geomFace.Transform(_clTrf);
        }

        float fDist = geomFace.DistanceToPoint(point);
        if (fabs(fDist) < fabs(fMinDist)) {
            fMinDist = fDist;
            positive = point.DistanceToPlane(geomFace._aclPoints[0], geomFace.GetNormal()) > 0;
        }
    }

    if (!positive) {
        fMinDist = -fMinDist;
    }
    return fMinDist;
}

// ----------------------------------------------------------------

InspectNominalFastMesh::InspectNominalFastMesh(const Mesh::MeshObject& rMesh, float offset)
    : _mesh(rMesh.getKernel())
{
    const MeshCore::MeshKernel& kernel = rMesh.getKernel();

    Base::Matrix4D tmp;
    _clTrf = rMesh.getTransform();
    _bApply = _clTrf != tmp;

    // Max. limit of grid elements
    float fMaxGridElements = 8000000.0f;
    Base::BoundBox3f box = kernel.GetBoundBox().Transformed(rMesh.getTransform());

    // estimate the minimum allowed grid length
    float fMinGridLen =
        (float)pow((box.LengthX() * box.LengthY() * box.LengthZ() / fMaxGridElements), 0.3333f);
    float fGridLen = 5.0f * MeshCore::MeshAlgorithm(kernel).GetAverageEdgeLength();

    // We want to avoid to get too small grid elements otherwise building up the grid structure
    // would take too much time and memory. Having quite a dense grid speeds up more the following
    // algorithms extremely. Due to the issue above it's always a compromise between speed and
    // memory usage.
    fGridLen = std::max<float>(fMinGridLen, fGridLen);

    // build up grid structure to speed up algorithms
    _pGrid = new MeshInspectGrid(kernel, fGridLen, rMesh.getTransform());
    _box = box;
    _box.Enlarge(offset);
    max_level = (unsigned long)(offset / fGridLen);
}

InspectNominalFastMesh::~InspectNominalFastMesh()
{
    delete this->_pGrid;
}

/**
 * This algorithm is not that exact as that from InspectNominalMesh but is by
 * factors faster and sufficient for many cases.
 */
float InspectNominalFastMesh::getDistance(const Base::Vector3f& point) const
{
    if (!_box.IsInBox(point)) {
        return FLT_MAX;  // must be inside bbox
    }

    std::set<unsigned long> indices;
#if 0  // a point in a neighbour grid can be nearer
    std::vector<unsigned long> elements;
    _pGrid->GetElements(point, elements);
    indices.insert(elements.begin(), elements.end());
#else
    unsigned long ulX, ulY, ulZ;
    _pGrid->Position(point, ulX, ulY, ulZ);
    unsigned long ulLevel = 0;
    while (indices.empty() && ulLevel <= max_level) {
        _pGrid->GetHull(ulX, ulY, ulZ, ulLevel++, indices);
    }
    if (indices.empty() || ulLevel == 1) {
        _pGrid->GetHull(ulX, ulY, ulZ, ulLevel, indices);
    }
#endif

    float fMinDist = FLT_MAX;
    bool positive = true;
    for (unsigned long it : indices) {
        MeshCore::MeshGeomFacet geomFace = _mesh.GetFacet(it);
        if (_bApply) {
            geomFace.Transform(_clTrf);
        }

        float fDist = geomFace.DistanceToPoint(point);
        if (fabs(fDist) < fabs(fMinDist)) {
            fMinDist = fDist;
            positive = point.DistanceToPlane(geomFace._aclPoints[0], geomFace.GetNormal()) > 0;
        }
    }

    if (!positive) {
        fMinDist = -fMinDist;
    }
    return fMinDist;
}

// ----------------------------------------------------------------

InspectNominalPoints::InspectNominalPoints(const Points::PointKernel& Kernel, float /*offset*/)
    : _rKernel(Kernel)
{
    int uGridPerAxis = 50;  // totally 125.000 grid elements
    this->_pGrid = new Points::PointsGrid(Kernel, uGridPerAxis);
}

InspectNominalPoints::~InspectNominalPoints()
{
    delete this->_pGrid;
}

float InspectNominalPoints::getDistance(const Base::Vector3f& point) const
{
    // TODO: Make faster
    std::set<unsigned long> indices;
    unsigned long x, y, z;
    Base::Vector3d pointd(point.x, point.y, point.z);
    _pGrid->Position(pointd, x, y, z);
    _pGrid->GetElements(x, y, z, indices);

    double fMinDist = DBL_MAX;
    for (unsigned long it : indices) {
        Base::Vector3d pt = _rKernel.getPoint(it);
        double fDist = Base::Distance(pointd, pt);
        if (fDist < fMinDist) {
            fMinDist = fDist;
        }
    }

    return (float)fMinDist;
}

// ----------------------------------------------------------------

InspectNominalShape::InspectNominalShape(const TopoDS_Shape& shape, float /*radius*/)
    : _rShape(shape)
{
    distss = new BRepExtrema_DistShapeShape();
    distss->LoadS1(_rShape);

    // When having a solid then use its shell because otherwise the distance
    // for inner points will always be zero
    if (!_rShape.IsNull() && _rShape.ShapeType() == TopAbs_SOLID) {
        TopExp_Explorer xp;
        xp.Init(_rShape, TopAbs_SHELL);
        if (xp.More()) {
            distss->LoadS1(xp.Current());
            isSolid = true;
        }
    }
    // distss->SetDeflection(radius);
}

InspectNominalShape::~InspectNominalShape()
{
    delete distss;
}

float InspectNominalShape::getDistance(const Base::Vector3f& point) const
{
    gp_Pnt pnt3d(point.x, point.y, point.z);
    BRepBuilderAPI_MakeVertex mkVert(pnt3d);
    distss->LoadS2(mkVert.Vertex());

    float fMinDist = FLT_MAX;
    if (distss->Perform() && distss->NbSolution() > 0) {
        fMinDist = (float)distss->Value();
        // the shape is a solid, check if the vertex is inside
        if (isSolid) {
            if (isInsideSolid(pnt3d)) {
                fMinDist = -fMinDist;
            }
        }
        else if (fMinDist > 0) {
            // check if the distance was computed from a face
            if (isBelowFace(pnt3d)) {
                fMinDist = -fMinDist;
            }
        }
    }
    return fMinDist;
}

bool InspectNominalShape::isInsideSolid(const gp_Pnt& pnt3d) const
{
    const Standard_Real tol = 0.001;
    BRepClass3d_SolidClassifier classifier(_rShape);
    classifier.Perform(pnt3d, tol);
    return (classifier.State() == TopAbs_IN);
}

bool InspectNominalShape::isBelowFace(const gp_Pnt& pnt3d) const
{
    // check if the distance was computed from a face
    for (Standard_Integer index = 1; index <= distss->NbSolution(); index++) {
        if (distss->SupportTypeShape1(index) == BRepExtrema_IsInFace) {
            TopoDS_Shape face = distss->SupportOnShape1(index);
            Standard_Real u, v;
            distss->ParOnFaceS1(index, u, v);
            // gp_Pnt pnt = distss->PointOnShape1(index);
            BRepGProp_Face props(TopoDS::Face(face));
            gp_Vec normal;
            gp_Pnt center;
            props.Normal(u, v, center, normal);
            gp_Vec dir(center, pnt3d);
            Standard_Real scalar = normal.Dot(dir);
            if (scalar < 0) {
                return true;
            }
            break;
        }
    }

    return false;
}

// ----------------------------------------------------------------

TYPESYSTEM_SOURCE(Inspection::PropertyDistanceList, App::PropertyLists)

PropertyDistanceList::PropertyDistanceList() = default;

PropertyDistanceList::~PropertyDistanceList() = default;

void PropertyDistanceList::setSize(int newSize)
{
    _lValueList.resize(newSize);
}

int PropertyDistanceList::getSize() const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyDistanceList::setValue(float lValue)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0] = lValue;
    hasSetValue();
}

void PropertyDistanceList::setValues(const std::vector<float>& values)
{
    aboutToSetValue();
    _lValueList = values;
    hasSetValue();
}

PyObject* PropertyDistanceList::getPyObject()
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++) {
        PyList_SetItem(list, i, PyFloat_FromDouble(_lValueList[i]));
    }
    return list;
}

void PropertyDistanceList::setPyObject(PyObject* value)
{
    if (PyList_Check(value)) {
        Py_ssize_t nSize = PyList_Size(value);
        std::vector<float> values;
        values.resize(nSize);

        for (Py_ssize_t i = 0; i < nSize; ++i) {
            PyObject* item = PyList_GetItem(value, i);
            if (!PyFloat_Check(item)) {
                std::string error = std::string("type in list must be float, not ");
                error += item->ob_type->tp_name;
                throw Py::TypeError(error);
            }

            values[i] = (float)PyFloat_AsDouble(item);
        }

        setValues(values);
    }
    else if (PyFloat_Check(value)) {
        setValue((float)PyFloat_AsDouble(value));
    }
    else {
        std::string error = std::string("type must be float or list of float, not ");
        error += value->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

void PropertyDistanceList::Save(Base::Writer& writer) const
{
    if (writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<FloatList count=\"" << getSize() << "\">" << std::endl;
        writer.incInd();
        for (int i = 0; i < getSize(); i++) {
            writer.Stream() << writer.ind() << "<F v=\"" << _lValueList[i] << "\"/>" << std::endl;
        }
        writer.decInd();
        writer.Stream() << writer.ind() << "</FloatList>" << std::endl;
    }
    else {
        writer.Stream() << writer.ind() << "<FloatList file=\"" << writer.addFile(getName(), this)
                        << "\"/>" << std::endl;
    }
}

void PropertyDistanceList::Restore(Base::XMLReader& reader)
{
    reader.readElement("FloatList");
    std::string file(reader.getAttribute("file"));

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), this);
    }
}

void PropertyDistanceList::SaveDocFile(Base::Writer& writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (float it : _lValueList) {
        str << it;
    }
}

void PropertyDistanceList::RestoreDocFile(Base::Reader& reader)
{
    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<float> values(uCt);
    for (float& it : values) {
        str >> it;
    }
    setValues(values);
}

App::Property* PropertyDistanceList::Copy() const
{
    PropertyDistanceList* p = new PropertyDistanceList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyDistanceList::Paste(const App::Property& from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyDistanceList&>(from)._lValueList;
    hasSetValue();
}

unsigned int PropertyDistanceList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(float));
}

// ----------------------------------------------------------------

namespace Inspection
{
// helper class to use Qt's concurrent framework
struct DistanceInspection
{

    DistanceInspection(float radius,
                       InspectActualGeometry* a,
                       std::vector<InspectNominalGeometry*> n)
        : radius(radius)
        , actual(a)
        , nominal(n)
    {}
    float mapped(unsigned long index) const
    {
        Base::Vector3f pnt = actual->getPoint(index);

        float fMinDist = FLT_MAX;
        for (auto it : nominal) {
            float fDist = it->getDistance(pnt);
            if (fabs(fDist) < fabs(fMinDist)) {
                fMinDist = fDist;
            }
        }

        if (fMinDist > this->radius) {
            fMinDist = FLT_MAX;
        }
        else if (-fMinDist > this->radius) {
            fMinDist = -FLT_MAX;
        }

        return fMinDist;
    }

    float radius;
    InspectActualGeometry* actual;
    std::vector<InspectNominalGeometry*> nominal;
};

// Helper internal class for QtConcurrent map operation. Holds sums-of-squares and counts for RMS
// calculation
class DistanceInspectionRMS
{
public:
    DistanceInspectionRMS() = default;
    DistanceInspectionRMS& operator+=(const DistanceInspectionRMS& rhs)
    {
        this->m_numv += rhs.m_numv;
        this->m_sumsq += rhs.m_sumsq;
        return *this;
    }
    double getRMS()
    {
        if (this->m_numv == 0) {
            return 0.0;
        }
        return sqrt(this->m_sumsq / (double)this->m_numv);
    }
    int m_numv {0};
    double m_sumsq {0.0};
};
}  // namespace Inspection

PROPERTY_SOURCE(Inspection::Feature, App::DocumentObject)

Feature::Feature()
{
    ADD_PROPERTY(SearchRadius, (0.05));
    ADD_PROPERTY(Thickness, (0.0));
    ADD_PROPERTY(Actual, (nullptr));
    ADD_PROPERTY(Nominals, (nullptr));
    ADD_PROPERTY(Distances, (0.0));
}

Feature::~Feature() = default;

short Feature::mustExecute() const
{
    if (SearchRadius.isTouched()) {
        return 1;
    }
    if (Thickness.isTouched()) {
        return 1;
    }
    if (Actual.isTouched()) {
        return 1;
    }
    if (Nominals.isTouched()) {
        return 1;
    }
    return 0;
}

App::DocumentObjectExecReturn* Feature::execute()
{
    bool useMultithreading = true;

    App::DocumentObject* pcActual = Actual.getValue();
    if (!pcActual) {
        throw Base::ValueError("No actual geometry to inspect specified");
    }

    InspectActualGeometry* actual = nullptr;
    if (pcActual->getTypeId().isDerivedFrom(Mesh::Feature::getClassTypeId())) {
        Mesh::Feature* mesh = static_cast<Mesh::Feature*>(pcActual);
        actual = new InspectActualMesh(mesh->Mesh.getValue());
    }
    else if (pcActual->getTypeId().isDerivedFrom(Points::Feature::getClassTypeId())) {
        Points::Feature* pts = static_cast<Points::Feature*>(pcActual);
        actual = new InspectActualPoints(pts->Points.getValue());
    }
    else if (pcActual->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        useMultithreading = false;
        Part::Feature* part = static_cast<Part::Feature*>(pcActual);
        actual = new InspectActualShape(part->Shape.getShape());
    }
    else {
        throw Base::TypeError("Unknown geometric type");
    }

    // clang-format off
    // get a list of nominals
    std::vector<InspectNominalGeometry*> inspectNominal;
    const std::vector<App::DocumentObject*>& nominals = Nominals.getValues();
    for (auto it : nominals) {
        InspectNominalGeometry* nominal = nullptr;
        if (it->getTypeId().isDerivedFrom(Mesh::Feature::getClassTypeId())) {
            Mesh::Feature* mesh = static_cast<Mesh::Feature*>(it);
            nominal = new InspectNominalMesh(mesh->Mesh.getValue(), this->SearchRadius.getValue());
        }
        else if (it->getTypeId().isDerivedFrom(Points::Feature::getClassTypeId())) {
            Points::Feature* pts = static_cast<Points::Feature*>(it);
            nominal = new InspectNominalPoints(pts->Points.getValue(), this->SearchRadius.getValue());
        }
        else if (it->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            useMultithreading = false;
            Part::Feature* part = static_cast<Part::Feature*>(it);
            nominal = new InspectNominalShape(part->Shape.getValue(), this->SearchRadius.getValue());
        }

        if (nominal) {
            inspectNominal.push_back(nominal);
        }
    }
    // clang-format on

#if 0
#if 1  // test with some huge data sets
    std::vector<unsigned long> index(actual->countPoints());
    std::generate(index.begin(), index.end(), Base::iotaGen<unsigned long>(0));
    DistanceInspection check(this->SearchRadius.getValue(), actual, inspectNominal);
    QFuture<float> future = QtConcurrent::mapped
        (index, std::bind(&DistanceInspection::mapped, &check, sp::_1));
    //future.waitForFinished(); // blocks the GUI
    Base::FutureWatcherProgress progress("Inspecting...", actual->countPoints());
    QFutureWatcher<float> watcher;
    QObject::connect(&watcher, &QFutureWatcher<float>::progressValueChanged,
                     &progress, &Base::FutureWatcherProgress::progressValueChanged);

    // keep it responsive during computation
    QEventLoop loop;
    QObject::connect(&watcher, &QFutureWatcher::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(future);
    loop.exec();

    std::vector<float> vals;
    vals.insert(vals.end(), future.begin(), future.end());
#else
    DistanceInspection insp(this->SearchRadius.getValue(), actual, inspectNominal);
    unsigned long count = actual->countPoints();
    std::stringstream str;
    str << "Inspecting " << this->Label.getValue() << "...";
    Base::SequencerLauncher seq(str.str().c_str(), count);

    std::vector<float> vals(count);
    for (unsigned long index = 0; index < count; index++) {
        float fMinDist = insp.mapped(index);
        vals[index] = fMinDist;
        seq.next();
    }
#endif

    Distances.setValues(vals);

    float fRMS = 0;
    int countRMS = 0;
    for (std::vector<float>::iterator it = vals.begin(); it != vals.end(); ++it) {
        if (fabs(*it) < FLT_MAX) {
            fRMS += (*it) * (*it);
            countRMS++;
        }
    }

    if (countRMS > 0) {
        fRMS = fRMS / countRMS;
        fRMS = sqrt(fRMS);
    }

    Base::Console().Message("RMS value for '%s' with search radius [%.4f,%.4f] is: %.4f\n",
        this->Label.getValue(), -this->SearchRadius.getValue(), this->SearchRadius.getValue(), fRMS);
#else
    unsigned long count = actual->countPoints();
    std::vector<float> vals(count);
    std::function<DistanceInspectionRMS(int)> fMap = [&](unsigned int index) {
        DistanceInspectionRMS res;
        Base::Vector3f pnt = actual->getPoint(index);

        float fMinDist = FLT_MAX;
        for (auto it : inspectNominal) {
            float fDist = it->getDistance(pnt);
            if (fabs(fDist) < fabs(fMinDist)) {
                fMinDist = fDist;
            }
        }

        if (fMinDist > this->SearchRadius.getValue()) {
            fMinDist = FLT_MAX;
        }
        else if (-fMinDist > this->SearchRadius.getValue()) {
            fMinDist = -FLT_MAX;
        }
        else {
            res.m_sumsq += fMinDist * fMinDist;
            res.m_numv++;
        }

        vals[index] = fMinDist;
        return res;
    };

    DistanceInspectionRMS res;

    if (useMultithreading) {
        // Build vector of increasing indices
        std::vector<unsigned long> index(count);
        std::iota(index.begin(), index.end(), 0);
        // Perform map-reduce operation : compute distances and update sum of squares for RMS
        // computation
        QFuture<DistanceInspectionRMS> future =
            QtConcurrent::mappedReduced(index, fMap, &DistanceInspectionRMS::operator+=);
        // Setup progress bar
        Base::FutureWatcherProgress progress("Inspecting...", actual->countPoints());
        QFutureWatcher<DistanceInspectionRMS> watcher;
        QObject::connect(&watcher,
                         &QFutureWatcher<DistanceInspectionRMS>::progressValueChanged,
                         &progress,
                         &Base::FutureWatcherProgress::progressValueChanged);
        // Keep UI responsive during computation
        QEventLoop loop;
        QObject::connect(&watcher,
                         &QFutureWatcher<DistanceInspectionRMS>::finished,
                         &loop,
                         &QEventLoop::quit);
        watcher.setFuture(future);
        loop.exec();
        res = future.result();
    }
    else {
        // Single-threaded operation
        std::stringstream str;
        str << "Inspecting " << this->Label.getValue() << "...";
        Base::SequencerLauncher seq(str.str().c_str(), count);

        for (unsigned int i = 0; i < count; i++) {
            res += fMap(i);
        }
    }

    Base::Console().Message("RMS value for '%s' with search radius [%.4f,%.4f] is: %.4f\n",
                            this->Label.getValue(),
                            -this->SearchRadius.getValue(),
                            this->SearchRadius.getValue(),
                            res.getRMS());
    Distances.setValues(vals);
#endif

    delete actual;
    for (auto it : inspectNominal) {
        delete it;
    }

    return nullptr;
}

// ----------------------------------------------------------------

PROPERTY_SOURCE(Inspection::Group, App::DocumentObjectGroup)


Group::Group() = default;

Group::~Group() = default;
