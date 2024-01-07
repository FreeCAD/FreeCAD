// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Base/Precision.h>
#include "Mod/Part/App/FeaturePartBox.h"
#include "Mod/Part/App/FeaturePartFuse.h"
#include "Mod/Part/App/FeatureFillet.h"
#include <BRepGProp.hxx>
#include "Base/Interpreter.h"
#include <boost/format.hpp>

namespace PartTestHelpers
{

double getVolume(const TopoDS_Shape& shape);

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

testing::AssertionResult
boxesMatch(const Base::BoundBox3d& b1, const Base::BoundBox3d& b2, double prec = 1e-05);  // NOLINT
}  // namespace PartTestHelpers
