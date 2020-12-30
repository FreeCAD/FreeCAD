#include "PreCompiled.h"
#include "SketchStorage.h"

using namespace Sketcher;

SketchStorage::SketchStorage() {}
SketchStorage::~SketchStorage() {}


// void SketchStorage::addPoint(Part::GeomPoint &point)
// {
//     size_t geometry = addGeometry(point);
//     Geoms[geometry].pointIds[PointPos::start] =
//     Geoms[geometry].pointIds[PointPos::mid] =
//     Geoms[geometry].pointIds[PointPos::end] =
//     Points.size();

//     Points.push_back(point);
// }

// size_t SketchStorage::addGeometry(const Part::Geometry & geometry)
// {
//     SketchStorage::GeoDef def;
//     def.geo  = geometry;
//     def.type = GeometryType::from(geometry.getTypeId());
//     Geoms.push_back(def);
//     return  Geoms.size()-1;
// }