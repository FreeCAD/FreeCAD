// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <FCConfig.h>

#include <deque>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include <Mod/Part/App/FCBRepAlgoAPI_BooleanOperation.h>
#include <BRepCheck_Analyzer.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Shape.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/SemanticDocumentState.h>
#include <App/SemanticReference.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/ProgramVersion.h>

#include "FeaturePartBoolean.h"
#include "SemanticHistoryAdapter.h"
#include "SemanticSourceCollector.h"
#include "TopoShapeOpCode.h"
#include "modelRefine.h"


using namespace Part;

namespace
{

void publishBooleanSemanticHistory(App::DocumentObject* self,
                                   const char* opcode,
                                   BRepAlgoAPI_BooleanOperation* mkBool,
                                   const TopoShape& result,
                                   App::DocumentObject* baseObj,
                                   const TopoShape& baseShape,
                                   App::DocumentObject* toolObj,
                                   const TopoShape& toolShape)
{
    if (!self || !mkBool || !mkBool->IsDone() || result.isNull()) {
        return;
    }
    App::SemanticGraph* graph = App::SemanticDocumentState::graphFor(self);
    if (!graph && self->getDocument()) {
        graph = &self->getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }

    std::deque<TopoDS_Shape> held;
    std::vector<std::pair<App::SemanticId, const void*>> inputs;
    std::unordered_set<App::SemanticHandle> seenSeeds;
    Part::collectUniqueSourceSeeds(graph, baseObj, baseShape, held, inputs, seenSeeds);
    Part::collectUniqueSourceSeeds(graph, toolObj, toolShape, held, inputs, seenSeeds);
    if (inputs.empty()) {
        return;
    }

    auto indexOf = [&result](const void* occ) -> App::ElementIndex {
        App::ElementIndex idx;
        if (!occ) {
            return idx;
        }
        return Part::indexOnPublishedPartnerCoplanar(result, *static_cast<const TopoDS_Shape*>(occ));
    };

    const HistoryTable raw =
        SemanticHistoryAdapter::fromMaker(mkBool, inputs, indexOf);
    const HistoryTable unique =
        SemanticHistoryAdapter::uniqueOneImageGenerated(raw);

    const App::ObjectId selfId = static_cast<App::ObjectId>(self->getID());
    HistoryTable toApply;
    std::vector<App::SemanticId> seeds;
    toApply.reserve(unique.size());
    seeds.reserve(unique.size());
    for (const HistoryRecord& rec : unique) {
        // I13 leave-unnamed / C1: already bound, conflicted slot, or unique owner.
        // Match Extrusion/Revolution/Mirroring: refuse unnamed slots before apply.
        if (!isNamedIndex(rec.toIndex)) {
            continue;
        }
        if (App::shouldRefuseGeneratedMint(graph, rec.fromSeed, selfId, rec.toIndex)) {
            continue;
        }
        toApply.push_back(rec);
        seeds.push_back(rec.fromSeed);
    }
    if (toApply.empty()) {
        return;
    }

    App::EvalSerial eval = 0;
    if (App::Document* doc = self->getDocument()) {
        eval = doc->semanticState().currentEval();
    }
    SemanticHistoryAdapter::applyHistory(
        graph, selfId, eval, opcode ? opcode : "", seeds, toApply);
}

}  // namespace

namespace Part
{
void throwIfInvalidIfCheckModel(const TopoDS_Shape& shape)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Part/Boolean");

    if (hGrp->GetBool("CheckModel", true)) {
        BRepCheck_Analyzer aChecker(shape);
        if (!aChecker.IsValid()) {
            throw Base::RuntimeError("Resulting shape is invalid");
        }
    }
}

bool getRefineModelParameter()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Part/Boolean");
    return hGrp->GetBool("RefineModel", true);
}

}  // namespace Part

PROPERTY_SOURCE_ABSTRACT(Part::Boolean, Part::Feature)


Boolean::Boolean()
{
    ADD_PROPERTY(Base, (nullptr));
    ADD_PROPERTY(Tool, (nullptr));
    ADD_PROPERTY_TYPE(
        History,
        (ShapeHistory()),
        "Boolean",
        (App::PropertyType)(App::Prop_Output | App::Prop_Transient | App::Prop_Hidden),
        "Shape history"
    );
    History.setSize(0);

    ADD_PROPERTY_TYPE(
        Refine,
        (0),
        "Boolean",
        (App::PropertyType)(App::Prop_None),
        "Refine shape (clean up redundant edges) after this boolean operation"
    );

    this->Refine.setValue(getRefineModelParameter());
}

short Boolean::mustExecute() const
{
    if (Base.getValue() && Tool.getValue()) {
        if (Base.isTouched()) {
            return 1;
        }
        if (Tool.isTouched()) {
            return 1;
        }
    }
    return 0;
}

const char* Boolean::opCode() const
{
    return Part::OpCodes::Boolean;
}

App::DocumentObjectExecReturn* Boolean::execute()
{
    try {
#if defined(__GNUC__) && defined(FC_OS_LINUX)
        Base::SignalException se;
#endif
        auto base = Base.getValue();
        auto tool = Tool.getValue();

        if (!base || !tool) {
            return new App::DocumentObjectExecReturn("Linked object is not a Part object");
        }
        std::vector<TopoShape> shapes;
        shapes.reserve(2);
        // Now, let's get the TopoDS_Shape
        shapes.push_back(
            Feature::getTopoShape(Base.getValue(), ShapeOption::ResolveLink | ShapeOption::Transform)
        );
        auto BaseShape = shapes[0].getShape();
        if (BaseShape.IsNull()) {
            throw NullShapeException("Base shape is null");
        }
        shapes.push_back(
            Feature::getTopoShape(Tool.getValue(), ShapeOption::ResolveLink | ShapeOption::Transform)
        );
        auto ToolShape = shapes[1].getShape();
        if (ToolShape.IsNull()) {
            throw NullShapeException("Tool shape is null");
        }

        std::unique_ptr<BRepAlgoAPI_BooleanOperation> mkBool(makeOperation(BaseShape, ToolShape));
        if (!mkBool->IsDone()) {
            std::stringstream error;
            error << "Boolean operation failed";
            if (BaseShape.ShapeType() != TopAbs_SOLID) {
                error << std::endl << base->Label.getValue() << " is not a solid";
            }
            if (ToolShape.ShapeType() != TopAbs_SOLID) {
                error << std::endl << tool->Label.getValue() << " is not a solid";
            }
            return new App::DocumentObjectExecReturn(error.str());
        }
        TopoDS_Shape resShape = mkBool->Shape();
        if (resShape.IsNull()) {
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        }

        throwIfInvalidIfCheckModel(resShape);

        TopoShape res(0);
        res.makeElementShape(*mkBool, shapes, opCode());
        if (this->Refine.getValue()) {
            res = res.makeElementRefine();
        }
        this->Shape.setValue(res);
        // Unique 1-image Base/Tool Face/Edge Binding publish (I13) for Part WB
        // Fillet/Chamfer consume — restored after on-main Boolean rewrite.
        publishBooleanSemanticHistory(
            this, opCode(), mkBool.get(), res, base, shapes[0], tool, shapes[1]);
        copyMaterial(base);
        return Part::Feature::execute();
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn(
            "A fatal error occurred when running boolean operation"
        );
    }
}

void Boolean::Restore(Base::XMLReader& reader)
{
    ExtensionContainer::Restore(reader);

    // The Refine property was added in FreeCAD 0.17, so any file before that will not have it set.
    // For these files, the appropriate default value is false.
    if (Base::getVersion(reader.ProgramVersion) < Base::Version::v0_17) {
        Refine.setValue(false);
    }
}
