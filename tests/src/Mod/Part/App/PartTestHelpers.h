// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include <boost/format.hpp>
#include <App/Application.h>
#include <App/Document.h>
#include "Base/Interpreter.h"
#include <Base/Precision.h>
#include "Mod/Part/App/FeaturePartBox.h"
#include "Mod/Part/App/FeaturePartFuse.h"
#include "Mod/Part/App/FeatureFillet.h"
#include <BRepGProp.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <GC_MakeCircle.hxx>
#include <TopoDS.hxx>

namespace PartTestHelpers
{

using namespace Data;
using namespace Part;

double getVolume(const TopoDS_Shape& shape);

double getArea(const TopoDS_Shape& shape);

double getLength(const TopoDS_Shape& shape);

std::vector<Part::FilletElement>
_getFilletEdges(const std::vector<int>& edges, double startRadius, double endRadius);

class PartTestHelperClass
{
public:
    App::Document* _doc;
    std::string _docName;
    std::array<Part::Box*, 6> _boxes;  // NOLINT magic number
    void createTestDoc();
};

const double minimalDistance = Base::Precision::Confusion() * 1000;

void executePython(const std::vector<std::string>& python);

void rectangle(double height, double width, char* name);

std::tuple<TopoDS_Face, TopoDS_Wire, TopoDS_Edge, TopoDS_Edge, TopoDS_Edge, TopoDS_Edge>
CreateRectFace(float len = 2.0, float wid = 3.0);

std::tuple<TopoDS_Face, TopoDS_Wire, TopoDS_Wire>
CreateFaceWithRoundHole(float len = 2.0, float wid = 3.0, float radius = 1.0);

testing::AssertionResult
boxesMatch(const Base::BoundBox3d& b1, const Base::BoundBox3d& b2, double prec = 1e-05);  // NOLINT

std::map<IndexedName, MappedName> elementMap(const TopoShape& shape);

std::pair<TopoDS_Shape, TopoDS_Shape> CreateTwoCubes();

std::pair<TopoShape, TopoShape> CreateTwoTopoShapeCubes();
}  // namespace PartTestHelpers
