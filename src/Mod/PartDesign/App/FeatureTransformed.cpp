// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#include <Bnd_Box.hxx>
#include <BRep_Builder.hxx>
#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>


#include <array>
#include <algorithm>
#include <chrono>
#include <limits>
#include <sstream>
#include <unordered_map>

#include <App/Application.h>
#include <App/AsyncRecomputeDebug.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Mod/Part/App/modelRefine.h>
#include <Mod/Part/App/ProgressIndicator.h>

#include "FeatureTransformed.h"
#include "Body.h"
#include "FeatureAddSub.h"
#include "FeatureMultiTransform.h"
#include "FeatureMirrored.h"
#include "FeatureLinearPattern.h"
#include "FeaturePolarPattern.h"
#include "FeatureSketchBased.h"
#include "Mod/Part/App/TopoShapeOpCode.h"


using namespace PartDesign;

namespace
{

long long elapsedMilliseconds(const std::chrono::steady_clock::time_point& start)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start
    )
        .count();
}

std::string describeDocumentObject(const App::DocumentObject* object)
{
    std::ostringstream stream;
    if (!object) {
        stream << "<null>";
        return stream.str();
    }

    const App::Document* document = object->getDocument();
    stream << "doc=" << (document ? document->getName() : "<null>");
    stream << " object=" << object->getNameInDocument();
    stream << " type=" << object->getTypeId().getName();
    return stream.str();
}

void appendTransformedDebugLog(
    const PartDesign::Transformed& transformed,
    const char* event,
    const std::string& details = {}
)
{
    std::ostringstream stream;
    stream << "[PartDesign::Transformed] " << event << ' ' << describeDocumentObject(&transformed);
    if (!details.empty()) {
        stream << ' ' << details;
    }
    App::appendAsyncRecomputeDebugLog(stream.str());
}

App::DocumentObjectExecReturn* abortIfCanceled(
    const PartDesign::Transformed& transformed,
    const Part::ScopedRecomputeProgress& scope,
    const char* phase
)
{
    if (scope.wasCanceled()) {
        appendTransformedDebugLog(transformed, "canceled", std::string("phase=") + phase);
        return new App::DocumentObjectExecReturn(
            "User aborted",
            nullptr,
            App::RecomputeIssueKind::Canceled
        );
    }
    return nullptr;
}

App::DocumentObjectExecReturn* makeExecReturnFromBaseException(const Base::Exception& exception)
{
    return new App::DocumentObjectExecReturn(
        exception.what(),
        nullptr,
        Base::isUserAbortException(exception) ? App::RecomputeIssueKind::Canceled
                                              : App::RecomputeIssueKind::Failure
    );
}

Part::BooleanRunMode selectInteractiveBooleanRunMode(std::size_t shapeCount)
{
    // Large interactive transformed booleans benefit more from responsive cancel
    // than from OCCT's internal parallel speed-up.
    constexpr std::size_t singleThreadedThreshold = 8;
    return shapeCount >= singleThreadedThreshold ? Part::BooleanRunMode::singleThreaded
                                                 : Part::BooleanRunMode::defaultMode;
}

const char* describeBooleanRunMode(Part::BooleanRunMode runMode)
{
    switch (runMode) {
        case Part::BooleanRunMode::singleThreaded:
            return "single_threaded";
        case Part::BooleanRunMode::defaultMode:
            return "default";
    }

    return "unknown";
}

bool isInteractivePreviewRecompute()
{
    return App::currentRecomputeHasOption(App::RecomputeOption::InteractivePreview);
}

}  // namespace

namespace PartDesign
{
extern bool getPDRefineModelParameter();

PROPERTY_SOURCE(PartDesign::Transformed, PartDesign::FeatureRefine)

std::array<char const*, 3> transformModeEnums = {"Features", "Whole shape", nullptr};

Transformed::Transformed()
{
    ADD_PROPERTY(Originals, (nullptr));
    Originals.setSize(0);
    Placement.setStatus(App::Property::ReadOnly, true);

    ADD_PROPERTY(TransformMode, (static_cast<long>(Mode::Features)));
    TransformMode.setEnums(transformModeEnums.data());
}

void Transformed::positionBySupport()
{
    // TODO May be here better to throw exception (silent=false) (2015-07-27, Fat-Zer)
    Part::Feature* support = getBaseObject(/* silent =*/true);
    if (support) {
        this->Placement.setValue(support->Placement.getValue());
    }
}

Part::Feature* Transformed::getBaseObject(bool silent) const
{
    Part::Feature* rv = Feature::getBaseObject(/* silent = */ true);
    if (rv) {
        return rv;
    }

    const char* err = nullptr;
    const std::vector<App::DocumentObject*>& originals = getOriginals();
    // NOTE: may be here supposed to be last origin but in order to keep the old behaviour keep here
    // first
    App::DocumentObject* firstOriginal = originals.empty() ? nullptr : originals.front();
    if (firstOriginal) {
        rv = freecad_cast<Part::Feature*>(firstOriginal);
        if (!rv) {
            err = QT_TRANSLATE_NOOP(
                "Exception",
                "Transformation feature Linked object is not a Part object"
            );
        }
    }
    else {
        if (freecad_cast<const Mirrored*>(this)) {
            err = QT_TRANSLATE_NOOP("Exception", "No features selected to be mirrored.");
        }
        else if (freecad_cast<const LinearPattern*>(this) || freecad_cast<const PolarPattern*>(this)) {
            err = QT_TRANSLATE_NOOP("Exception", "No features selected to be patterned.");
        }
        else {
            err = QT_TRANSLATE_NOOP("Exception", "No features selected to be transformed.");
        }
    }

    if (!silent && err) {
        throw Base::RuntimeError(err);
    }

    return rv;
}

std::vector<App::DocumentObject*> Transformed::getSortedOriginals() const
{
    std::vector<DocumentObject*> originals = Originals.getValues();

    // Sort originals in chronological order of the body's group history.
    if (auto body = getFeatureBody()) {
        const auto& group = body->Group.getValues();
        std::unordered_map<const DocumentObject*, size_t> indexMap;
        for (size_t i = 0; i < group.size(); ++i) {
            indexMap[group[i]] = i;
        }
        std::ranges::sort(originals, [&indexMap](const DocumentObject* a, const DocumentObject* b) {
            auto itA = indexMap.find(a);
            auto itB = indexMap.find(b);
            size_t idxA = (itA != indexMap.end()) ? itA->second : std::numeric_limits<size_t>::max();
            size_t idxB = (itB != indexMap.end()) ? itB->second : std::numeric_limits<size_t>::max();
            return idxA < idxB;
        });
    }

    return originals;
}

std::vector<App::DocumentObject*> Transformed::getOriginals() const
{
    auto const mode = static_cast<Mode>(TransformMode.getValue());

    if (mode == Mode::WholeShape) {
        return {};
    }

    std::vector<DocumentObject*> originals = getSortedOriginals();

    const auto isSuppressed = [](const DocumentObject* obj) {
        auto feature = freecad_cast<Feature*>(obj);

        return feature != nullptr && feature->Suppressed.getValue();
    };

    // Remove suppressed features from the list so the transformations behave as if they are not
    // there
    auto [first, last] = std::ranges::remove_if(originals, isSuppressed);
    originals.erase(first, last);

    return originals;
}

App::DocumentObject* Transformed::getSketchObject() const
{
    std::vector<DocumentObject*> originals = getOriginals();
    DocumentObject const* firstOriginal = !originals.empty() ? originals.front() : nullptr;

    if (auto feature = freecad_cast<PartDesign::ProfileBased*>(firstOriginal)) {
        return feature->getVerifiedSketch(true);
    }
    if (freecad_cast<PartDesign::FeatureAddSub*>(firstOriginal)) {
        return nullptr;
    }
    if (auto pattern = freecad_cast<LinearPattern*>(this)) {
        return pattern->Direction.getValue();
    }
    if (auto pattern = freecad_cast<PolarPattern*>(this)) {
        return pattern->Axis.getValue();
    }
    if (auto pattern = freecad_cast<Mirrored*>(this)) {
        return pattern->MirrorPlane.getValue();
    }

    return nullptr;
}

void Transformed::Restore(Base::XMLReader& reader)
{
    PartDesign::Feature::Restore(reader);
}

bool Transformed::isMultiTransformChild() const
{
    // Checking for a MultiTransform in the dependency list is not reliable on initialization
    // because the dependencies are only established after creation.
    /*
    for (auto const* obj : getInList()) {
        auto mt = freecad_cast<PartDesign::MultiTransform*>(obj);
        if (!mt) {
            continue;
        }

        auto const transfmt = mt->Transformations.getValues();
        if (std::find(transfmt.begin(), transfmt.end(), this) != transfmt.end()) {
            return true;
        }
    }
    */

    // instead check for default property values because these are invalid for a standalone
    // transform feature. This will mislabel standalone features during the initialization phase.
    if (TransformMode.getValue() == 0 && Originals.getValue().empty()) {
        return true;
    }

    return false;
}

void Transformed::handleChangedPropertyType(
    Base::XMLReader& reader,
    const char* TypeName,
    App::Property* prop
)
{
    // The property 'Angle' of PolarPattern has changed from PropertyFloat
    // to PropertyAngle and the property 'Length' has changed to PropertyLength.
    Base::Type inputType = Base::Type::fromName(TypeName);
    if (auto property = freecad_cast<App::PropertyFloat*>(prop);
        property != nullptr && inputType.isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
        // Do not directly call the property's Restore method in case the implementation
        // has changed. So, create a temporary PropertyFloat object and assign the value.
        App::PropertyFloat floatProp;
        floatProp.Restore(reader);
        property->setValue(floatProp.getValue());
    }
    else {
        PartDesign::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

short Transformed::mustExecute() const
{
    if (Originals.isTouched() || TransformMode.isTouched()) {
        return 1;
    }
    return PartDesign::Feature::mustExecute();
}

App::DocumentObjectExecReturn* Transformed::recomputePreview()
{
    const auto mode = static_cast<Mode>(TransformMode.getValue());

    const Part::Feature* supportFeature = getBaseObject();
    const Part::TopoShape supportShape = supportFeature->Shape.getShape();

    if (supportShape.isNull()) {
        return App::DocumentObject::StdReturn;
    }

    gp_Trsf supportTransform = supportShape.getShape().Location().Transformation();

    const auto makeCompoundOfToolShapes = [this, &supportTransform]() {
        BRep_Builder builder;
        TopoDS_Compound compound;

        builder.MakeCompound(compound);
        for (const auto& original : getOriginals()) {
            if (auto* feature = freecad_cast<FeatureAddSub*>(original)) {
                auto shape = feature->AddSubShape.getShape();

                gp_Trsf trsf = feature->getLocation().Transformation().Multiplied(
                    supportTransform.Inverted()
                );

                if (shape.isNull()) {
                    continue;
                }

                shape = shape.makeElementTransform(trsf);

                builder.Add(compound, shape.getShape());
            }
        }

        return compound;
    };

    switch (mode) {
        case Mode::Features:
            PreviewShape.setValue(makeCompoundOfToolShapes());
            return StdReturn;

        case Mode::WholeShape: {
            auto shape = getBaseTopoShape();
            shape = shape.makeElementTransform(supportTransform.Inverted());

            PreviewShape.setValue(shape.getShape());

            return StdReturn;
        }

        default:
            return FeatureRefine::recomputePreview();
    }
}

void Transformed::onChanged(const App::Property* prop)
{
    if (prop == &TransformMode) {
        auto const mode = static_cast<Mode>(TransformMode.getValue());
        Originals.setStatus(App::Property::Status::Hidden, mode == Mode::WholeShape);
    }

    FeatureRefine::onChanged(prop);
}

App::DocumentObjectExecReturn* Transformed::execute()
{
    const auto executeStart = std::chrono::steady_clock::now();
    if (isMultiTransformChild()) {
        appendTransformedDebugLog(*this, "execute skip_multi_transform_child");
        return App::DocumentObject::StdReturn;
    }

    constexpr std::size_t totalPhases = 4;
    const bool interactivePreview = isInteractivePreviewRecompute();
    Part::ScopedRecomputeProgress progressScope("Transform");
    auto prepareScope = progressScope.makeStepScope(0, totalPhases, "Preparing transform...");

    auto const mode = static_cast<Mode>(TransformMode.getValue());

    std::vector<DocumentObject*> originals = getOriginals();
    {
        std::ostringstream details;
        details << "mode=" << (mode == Mode::Features ? "features" : "whole_shape");
        details << " originals=" << originals.size();
        details << " interactive_preview=" << interactivePreview;
        appendTransformedDebugLog(*this, "execute begin", details.str());
    }

    if (mode == Mode::Features && originals.empty()) {
        appendTransformedDebugLog(*this, "execute no_originals");
        return App::DocumentObject::StdReturn;
    }

    if (!this->BaseFeature.getValue()) {
        if (auto body = getFeatureBody()) {
            body->setBaseProperty(this);
        }
    }

    this->positionBySupport();

    // get transformations from subclass by calling virtual method
    std::vector<gp_Trsf> transformations;
    try {
        const auto getTransformationsStart = std::chrono::steady_clock::now();
        std::list<gp_Trsf> t_list = getTransformations(originals);
        transformations.insert(transformations.end(), t_list.begin(), t_list.end());
        std::ostringstream details;
        details << "count=" << transformations.size();
        details << " elapsed_ms=" << elapsedMilliseconds(getTransformationsStart);
        appendTransformedDebugLog(*this, "getTransformations end", details.str());
    }
    catch (Base::Exception& e) {
        appendTransformedDebugLog(*this, "getTransformations error", e.what());
        return makeExecReturnFromBaseException(e);
    }
    catch (const Standard_Failure& e) {
        appendTransformedDebugLog(*this, "getTransformations std_failure", e.GetMessageString());
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    if (transformations.empty()) {
        appendTransformedDebugLog(*this, "execute no_transformations");
        return App::DocumentObject::StdReturn;  // No transformations defined, exit silently
    }

    // Get the support
    Part::Feature* supportFeature = nullptr;

    try {
        supportFeature = getBaseObject();
    }
    catch (Base::Exception& e) {
        appendTransformedDebugLog(*this, "getBaseObject error", e.what());
        return makeExecReturnFromBaseException(e);
    }

    const Part::TopoShape& supportTopShape = supportFeature->Shape.getShape();
    if (supportTopShape.getShape().IsNull()) {
        appendTransformedDebugLog(*this, "execute invalid_support_shape");
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Cannot transform invalid support shape")
        );
    }

    // create an untransformed copy of the support shape
    Part::TopoShape supportShape(supportTopShape);

    gp_Trsf trsfInv = supportShape.getShape().Location().Transformation().Inverted();

    supportShape.setTransform(Base::Matrix4D());
    if (auto abort = ::abortIfCanceled(*this, prepareScope, "prepare")) {
        return abort;
    }
    prepareScope.complete();
    appendTransformedDebugLog(
        *this,
        "prepare end",
        std::string("elapsed_ms=") + std::to_string(elapsedMilliseconds(executeStart))
    );

    auto getTransformedCompShape = [&](const auto& supportShape,
                                       const auto& origShape,
                                       Part::ScopedRecomputeProgress& scope,
                                       const char* stageLabel) {
        const auto transformShapeStart = std::chrono::steady_clock::now();
        std::vector<TopoShape> shapes = {supportShape};
        TopoShape shape(origShape);
        int idx = 1;
        auto transformIter = transformations.cbegin();
        transformIter++;
        const std::size_t totalTransformations = transformations.size() > 1
            ? transformations.size() - 1
            : 0;
        std::size_t transformIndex = 0;
        for (; transformIter != transformations.end(); transformIter++, transformIndex++) {
            auto transformScope
                = scope.makeStepScope(transformIndex, totalTransformations, "Applying transform...");
            if (auto abort = ::abortIfCanceled(*this, transformScope, stageLabel)) {
                (void)abort;
                appendTransformedDebugLog(
                    *this,
                    "transformInstances canceled",
                    std::string("stage=") + stageLabel
                        + " elapsed_ms=" + std::to_string(elapsedMilliseconds(transformShapeStart))
                );
                return std::vector<TopoShape>();
            }
            auto opName = Data::indexSuffix(idx++);
            shapes.emplace_back(shape.makeElementTransform(*transformIter, opName.c_str()));
            if (auto abort = ::abortIfCanceled(*this, transformScope, stageLabel)) {
                (void)abort;
                appendTransformedDebugLog(
                    *this,
                    "transformInstances canceled",
                    std::string("stage=") + stageLabel
                        + " elapsed_ms=" + std::to_string(elapsedMilliseconds(transformShapeStart))
                );
                return std::vector<TopoShape>();
            }
            transformScope.complete();
        }
        std::ostringstream details;
        details << "stage=" << stageLabel;
        details << " instances=" << shapes.size();
        details << " elapsed_ms=" << elapsedMilliseconds(transformShapeStart);
        appendTransformedDebugLog(*this, "transformInstances end", details.str());
        return shapes;
    };

    auto applyScope = progressScope.makeStepScope(
        1,
        totalPhases,
        mode == Mode::Features ? "Applying transformed features..." : "Applying transformed shape..."
    );

    try {
        switch (mode) {
            case Mode::Features:
                // NOTE: It would be possible to build a compound from all original
                // addShapes/subShapes and then transform the compounds as a whole. But we choose to
                // apply the transformations to each Original separately. This way it is easier to
                // discover what feature causes a fuse/cut to fail. The downside is that performance
                // suffers when there are many originals. But it seems safe to assume that in most
                // cases there are few originals and many transformations
                for (std::size_t originalIndex = 0; originalIndex < originals.size();
                     ++originalIndex) {
                    auto original = originals[originalIndex];
                    const auto originalStart = std::chrono::steady_clock::now();
                    auto originalScope = applyScope.makeStepScope(
                        originalIndex,
                        originals.size(),
                        "Transforming feature..."
                    );
                    {
                        std::ostringstream details;
                        details << "index=" << originalIndex;
                        details << ' ' << describeDocumentObject(original);
                        appendTransformedDebugLog(*this, "original begin", details.str());
                    }
                    if (auto abort = ::abortIfCanceled(*this, originalScope, "original_begin")) {
                        return abort;
                    }
                    // Extract the original shape and determine whether to cut or to fuse
                    Part::TopoShape fuseShape;
                    Part::TopoShape cutShape;

                    auto feature = freecad_cast<PartDesign::FeatureAddSub*>(original);
                    if (!feature) {
                        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                            "Exception",
                            "Only additive and subtractive features can be transformed"
                        ));
                    }

                    feature->getAddSubShape(fuseShape, cutShape);
                    if (fuseShape.isNull() && cutShape.isNull()) {
                        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                            "Exception",
                            "Shape of additive/subtractive feature is empty"
                        ));
                    }
                    gp_Trsf trsf = feature->getLocation().Transformation().Multiplied(trsfInv);
                    if (!fuseShape.isNull()) {
                        fuseShape = fuseShape.makeElementTransform(trsf);
                    }
                    if (!cutShape.isNull()) {
                        cutShape = cutShape.makeElementTransform(trsf);
                    }
                    if (!fuseShape.isNull()) {
                        const auto fuseBuildStart = std::chrono::steady_clock::now();
                        auto shapes = getTransformedCompShape(
                            supportShape,
                            fuseShape,
                            originalScope,
                            "build_fuse_inputs"
                        );
                        {
                            std::ostringstream details;
                            details << "index=" << originalIndex;
                            details << " shapes=" << shapes.size();
                            details << " elapsed_ms=" << elapsedMilliseconds(fuseBuildStart);
                            appendTransformedDebugLog(*this, "fuse inputs end", details.str());
                        }
                        if (auto abort = ::abortIfCanceled(*this, originalScope, "fuse_inputs")) {
                            return abort;
                        }
                        const auto fuseStart = std::chrono::steady_clock::now();
                        const auto runMode = selectInteractiveBooleanRunMode(shapes.size());
                        appendTransformedDebugLog(
                            *this,
                            "fuse begin",
                            std::string("index=") + std::to_string(originalIndex)
                                + " run_mode=" + describeBooleanRunMode(runMode)
                        );
                        supportShape.makeElementFuse(shapes, runMode, nullptr, -1.0);
                        {
                            std::ostringstream details;
                            details << "index=" << originalIndex;
                            details << " elapsed_ms=" << elapsedMilliseconds(fuseStart);
                            appendTransformedDebugLog(*this, "fuse end", details.str());
                        }
                        if (auto abort = ::abortIfCanceled(*this, originalScope, "fuse")) {
                            return abort;
                        }
                    }
                    if (!cutShape.isNull()) {
                        const auto cutBuildStart = std::chrono::steady_clock::now();
                        auto shapes = getTransformedCompShape(
                            supportShape,
                            cutShape,
                            originalScope,
                            "build_cut_inputs"
                        );
                        {
                            std::ostringstream details;
                            details << "index=" << originalIndex;
                            details << " shapes=" << shapes.size();
                            details << " elapsed_ms=" << elapsedMilliseconds(cutBuildStart);
                            appendTransformedDebugLog(*this, "cut inputs end", details.str());
                        }
                        if (auto abort = ::abortIfCanceled(*this, originalScope, "cut_inputs")) {
                            return abort;
                        }
                        const auto cutStart = std::chrono::steady_clock::now();
                        const auto runMode = selectInteractiveBooleanRunMode(shapes.size());
                        appendTransformedDebugLog(
                            *this,
                            "cut begin",
                            std::string("index=") + std::to_string(originalIndex)
                                + " run_mode=" + describeBooleanRunMode(runMode)
                        );
                        supportShape.makeElementCut(shapes, runMode, nullptr, -1.0);
                        {
                            std::ostringstream details;
                            details << "index=" << originalIndex;
                            details << " elapsed_ms=" << elapsedMilliseconds(cutStart);
                            appendTransformedDebugLog(*this, "cut end", details.str());
                        }
                        if (auto abort = ::abortIfCanceled(*this, originalScope, "cut")) {
                            return abort;
                        }
                    }
                    originalScope.complete();
                    appendTransformedDebugLog(
                        *this,
                        "original end",
                        std::string("index=") + std::to_string(originalIndex)
                            + " elapsed_ms=" + std::to_string(elapsedMilliseconds(originalStart))
                    );
                }
                break;
            case Mode::WholeShape: {
                auto wholeScope = applyScope.makeStepScope(0, 1, "Transforming shape...");
                if (auto abort = ::abortIfCanceled(*this, wholeScope, "whole_shape_begin")) {
                    return abort;
                }
                const auto wholeBuildStart = std::chrono::steady_clock::now();
                auto shapes = getTransformedCompShape(
                    supportShape,
                    supportShape,
                    wholeScope,
                    "build_whole_shape_inputs"
                );
                {
                    std::ostringstream details;
                    details << "shapes=" << shapes.size();
                    details << " elapsed_ms=" << elapsedMilliseconds(wholeBuildStart);
                    appendTransformedDebugLog(*this, "whole_shape inputs end", details.str());
                }
                if (auto abort = ::abortIfCanceled(*this, wholeScope, "whole_shape_inputs")) {
                    return abort;
                }
                const auto wholeFuseStart = std::chrono::steady_clock::now();
                const auto runMode = selectInteractiveBooleanRunMode(shapes.size());
                appendTransformedDebugLog(
                    *this,
                    "whole_shape fuse begin",
                    std::string("run_mode=") + describeBooleanRunMode(runMode)
                );
                supportShape.makeElementFuse(shapes, runMode, nullptr, -1.0);
                appendTransformedDebugLog(
                    *this,
                    "whole_shape fuse end",
                    std::string("elapsed_ms=") + std::to_string(elapsedMilliseconds(wholeFuseStart))
                );
                if (auto abort = ::abortIfCanceled(*this, wholeScope, "whole_shape_fuse")) {
                    return abort;
                }
                wholeScope.complete();
                break;
            }
        }
        applyScope.complete();
        appendTransformedDebugLog(
            *this,
            "apply end",
            std::string("elapsed_ms=") + std::to_string(elapsedMilliseconds(executeStart))
        );

        auto refineScope = progressScope.makeStepScope(2, totalPhases, "Refining transform...");
        if (auto abort = ::abortIfCanceled(*this, refineScope, "refine_begin")) {
            return abort;
        }
        if (interactivePreview) {
            appendTransformedDebugLog(*this, "refine skipped", "interactive_preview=1");
        }
        else {
            const auto refineStart = std::chrono::steady_clock::now();
            appendTransformedDebugLog(*this, "refine begin");
            supportShape = refineShapeIfActive((supportShape));
            appendTransformedDebugLog(
                *this,
                "refine end",
                std::string("elapsed_ms=") + std::to_string(elapsedMilliseconds(refineStart))
            );
        }
        if (auto abort = ::abortIfCanceled(*this, refineScope, "refine")) {
            return abort;
        }
        refineScope.complete();

        auto finalizeScope = progressScope.makeStepScope(3, totalPhases, "Finalizing transform...");
        if (auto abort = ::abortIfCanceled(*this, finalizeScope, "finalize_begin")) {
            return abort;
        }
        const auto finalizeStart = std::chrono::steady_clock::now();
        appendTransformedDebugLog(*this, "finalize begin");
        this->Shape.setValue(getSolid(supportShape));
        if (singleSolidRuleMode() == SingleSolidRuleMode::Enforced) {
            rejected = getRemainingSolids(supportShape.getShape());
        }
        else {
            rejected.Nullify();
        }
        appendTransformedDebugLog(
            *this,
            "finalize end",
            std::string("elapsed_ms=") + std::to_string(elapsedMilliseconds(finalizeStart))
        );
        finalizeScope.complete();
        appendTransformedDebugLog(
            *this,
            "execute end",
            std::string("elapsed_ms=") + std::to_string(elapsedMilliseconds(executeStart))
        );

        return App::DocumentObject::StdReturn;
    }
    catch (const Standard_Failure& e) {
        appendTransformedDebugLog(*this, "execute std_failure", e.GetMessageString());
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        appendTransformedDebugLog(*this, "execute base_exception", e.what());
        return makeExecReturnFromBaseException(e);
    }
}

TopoDS_Shape Transformed::getRemainingSolids(const TopoDS_Shape& shape)
{
    BRep_Builder builder;
    TopoDS_Compound compShape;
    builder.MakeCompound(compShape);

    if (shape.IsNull()) {
        Standard_Failure::Raise("Shape is null");
    }
    TopExp_Explorer xp;
    xp.Init(shape, TopAbs_SOLID);
    xp.Next();  // skip the first

    for (; xp.More(); xp.Next()) {
        builder.Add(compShape, xp.Current());
    }

    return {std::move(compShape)};
}

}  // namespace PartDesign
