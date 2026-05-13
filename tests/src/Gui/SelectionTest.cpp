// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include <Inventor/SoDB.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SoType.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/nodekits/SoNodeKit.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTranslation.h>

#include <src/App/InitApplication.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Type.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/SoFCDB.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SoFCUnifiedSelection.h>
#include <Mod/Part/App/AttachExtension.h>
#include <Mod/Part/App/FeaturePartBox.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PrimitiveFeature.h>
#include <Mod/Part/App/PropertyTopoShape.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/SoBrepPointSet.h>
#include <Mod/Part/Gui/ViewProviderExt.h>
#include <QApplication>

namespace
{

class ObjectNameGate: public Gui::SelectionGate
{
public:
    explicit ObjectNameGate(const char* allowedName)
        : _allowedName(allowedName)
    {}

    bool allow(App::Document*, App::DocumentObject* obj, const char*) override
    {
        if (!obj || _allowedName != obj->getNameInDocument()) {
            notAllowedReason = "rejected by test gate";
            return false;
        }
        return true;
    }

private:
    std::string _allowedName;
};

class FaceOnlyGate: public Gui::SelectionGate
{
public:
    bool allow(App::Document*, App::DocumentObject*, const char* subname) override
    {
        return subname && std::string_view(subname).starts_with("Face");
    }

    std::unordered_set<std::string> getGatedTypes(
        const std::vector<const char*>& allTypesForGeometry
    ) const override
    {
        std::unordered_set<std::string> gatedTypes;
        for (const auto* type : allTypesForGeometry) {
            if (type && std::string_view(type) == "Face") {
                gatedTypes.emplace(type);
            }
        }
        return gatedTypes;
    }
};

struct DocumentGuard
{
    explicit DocumentGuard(std::string name)
        : name(std::move(name))
    {}

    ~DocumentGuard()
    {
        if (App::GetApplication().getDocument(name.c_str())) {
            App::GetApplication().closeDocument(name.c_str());
        }
    }

    std::string name;
};

struct SelectionGuard
{
    explicit SelectionGuard(std::string documentName)
        : documentName(std::move(documentName))
    {}

    ~SelectionGuard()
    {
        Gui::SelectionLogDisabler disableSelectionLog(true);
        Gui::Selection().rmvSelectionGate(documentName.c_str());
        Gui::Selection().clearSelection(documentName.c_str());
    }

    std::string documentName;
};

struct EdgePromotionProbe
{
    SbVec2s position;
    std::string rawEdge;
    std::string visibleFace;
};

struct FacePickProbe
{
    SbVec2s position;
    std::shared_ptr<const SoPickedPoint> pickedPoint;
};

std::set<std::string> collectVisibleFaceElements(
    const PartGui::ViewProviderPartExt& viewProvider,
    SoNode* sceneRoot,
    const SbViewportRegion& viewportRegion
);

std::optional<EdgePromotionProbe> findSilhouetteEdgePromotionProbe(
    const PartGui::ViewProviderPartExt& viewProvider,
    const Part::TopoShape& shape,
    SoNode* sceneRoot,
    const SbViewportRegion& viewportRegion,
    const std::set<std::string>& visibleFaces
);

std::optional<FacePickProbe> findRawFacePickProbe(
    const PartGui::ViewProviderPartExt& viewProvider,
    SoNode* sceneRoot,
    const SbViewportRegion& viewportRegion,
    const std::string& faceElement
);

std::string summarizeRawEdgeCandidates(
    const PartGui::ViewProviderPartExt& viewProvider,
    const Part::TopoShape& shape,
    SoNode* sceneRoot,
    const SbViewportRegion& viewportRegion,
    const std::set<std::string>& visibleFaces
);

struct PartSelectionScene
{
    explicit PartSelectionScene(const char* documentPrefix)
        : docName(App::GetApplication().getUniqueDocumentName(documentPrefix))
        , documentGuard(docName)
        , selectionGuard(docName)
    {}

    ~PartSelectionScene()
    {
        if (root) {
            root->unref();
        }
    }

    void initialize()
    {
        App::DocumentInitFlags createFlags;
        createFlags.createView = false;
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", createFlags);
        if (!doc) {
            return;
        }

        auto* boxObject = doc->addObject("Part::Box", "Box");
        box = freecad_cast<Part::Box*>(boxObject);
        if (!box) {
            return;
        }

        box->Length.setValue(10.0);
        box->Width.setValue(10.0);
        box->Height.setValue(10.0);
        doc->recompute();

        viewProvider = Gui::Application::Instance->getViewProvider<PartGui::ViewProviderPartExt>(box);
        if (!viewProvider) {
            return;
        }

        viewProvider->setDisplayMode("Flat Lines");

        root = new SoSeparator;
        root->ref();

        auto* camera = new SoOrthographicCamera;
        const SbVec3f cameraPosition(-18.0F, -16.0F, 18.0F);
        const SbVec3f cameraTarget(5.0F, 5.0F, 5.0F);
        camera->position.setValue(cameraPosition);
        camera->pointAt(cameraTarget, SbVec3f(0.0F, 0.0F, 1.0F));
        camera->focalDistance = (cameraTarget - cameraPosition).length();
        camera->nearDistance = 1.0F;
        camera->farDistance = 100.0F;
        camera->height = 40.0F;
        root->addChild(camera);
        root->addChild(viewProvider->getRoot());

        renderedShape = viewProvider->getRenderedShape();
        visibleFaces = collectVisibleFaceElements(*viewProvider, root, viewportRegion);
    }

    std::optional<EdgePromotionProbe> findSilhouetteEdgeProbe() const
    {
        return findSilhouetteEdgePromotionProbe(
            *viewProvider,
            renderedShape,
            root,
            viewportRegion,
            visibleFaces
        );
    }

    std::optional<FacePickProbe> findRawFacePick(const std::string& faceElement) const
    {
        return findRawFacePickProbe(*viewProvider, root, viewportRegion, faceElement);
    }

    std::string summarizeRawEdges() const
    {
        return summarizeRawEdgeCandidates(*viewProvider, renderedShape, root, viewportRegion, visibleFaces);
    }

    std::string docName;
    DocumentGuard documentGuard;
    SelectionGuard selectionGuard;
    App::Document* doc {};
    Part::Box* box {};
    PartGui::ViewProviderPartExt* viewProvider {};
    SoSeparator* root {};
    SbViewportRegion viewportRegion {SbVec2s {240, 240}};
    Part::TopoShape renderedShape;
    std::set<std::string> visibleFaces;
};

class SelectionTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        SoDB::init();
    }

    void SetUp() override
    {
        // Keep this fixture headless even if another Gui gtest already created
        // Gui::Application in the shared binary.
        App::DocumentInitFlags createFlags;
        createFlags.createView = false;
        _docName = App::GetApplication().getUniqueDocumentName("selection_test");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser", createFlags);
        _allowedObject = _doc->addObject("App::FeatureTest", "Allowed");
        _rejectedObject = _doc->addObject("App::FeatureTest", "Rejected");
    }

    void TearDown() override
    {
        Gui::SelectionLogDisabler disableSelectionLog(true);
        Gui::Selection().rmvSelectionGate(_docName.c_str());
        Gui::Selection().clearSelection(_docName.c_str());
        if (App::GetApplication().getDocument(_docName.c_str())) {
            App::GetApplication().closeDocument(_docName.c_str());
        }
    }

    std::string _docName;
    App::Document* _doc {};
    App::DocumentObject* _allowedObject {};
    App::DocumentObject* _rejectedObject {};
};

class PartSelectionPromotionTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        if (!qApp) {
            qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
            static int argc = 1;
            static char appName[] = "Gui_tests_run";
            static char* argv[] = {appName, nullptr};
            qtApp = new QApplication(argc, argv);
        }
        if (Base::Type::fromName("Gui::BaseView").isBad()) {
            Gui::Application::initApplication();
        }
        if (!Gui::Application::Instance) {
            guiApp = new Gui::Application(true);
        }
        if (!SoDB::isInitialized()) {
            SoDB::init();
            SoNodeKit::init();
            SoInteraction::init();
        }
        if (!Gui::SoFCDB::isInitialized()) {
            Gui::SoFCDB::init();
        }
        static const bool partGuiLoaded = Base::Interpreter().loadModule("PartGui");
        ASSERT_TRUE(partGuiLoaded);
    }

    static Gui::Application* guiApp;
    static QApplication* qtApp;
};

Gui::Application* PartSelectionPromotionTest::guiApp = nullptr;
QApplication* PartSelectionPromotionTest::qtApp = nullptr;

Gui::SelectionPickPolicy::Candidate pickCandidate(
    const void* owner,
    int priority,
    bool closeToFirst,
    bool hasGate,
    bool passesGate,
    float cursorDistanceSquared = std::numeric_limits<float>::infinity()
)
{
    Gui::SelectionPickPolicy::Candidate candidate;
    candidate.owner = owner;
    candidate.priority = priority;
    candidate.closeToFirst = closeToFirst;
    candidate.hasGate = hasGate;
    candidate.passesGate = passesGate;
    candidate.cursorDistanceSquared = cursorDistanceSquared;
    return candidate;
}

SbVec2f normalizedPosition(const SbVec2s& cursorPosition, const SbViewportRegion& viewportRegion)
{
    SoLocation2Event event;
    event.setPosition(cursorPosition);
    return event.getNormalizedPosition(viewportRegion);
}

std::shared_ptr<const SoPickedPoint> pickCubePoint(
    float xTranslation,
    const SbVec2s& pickPosition,
    const SbViewportRegion& viewportRegion
)
{
    auto root = new SoSeparator;
    root->ref();

    auto camera = new SoOrthographicCamera;
    camera->position.setValue(0.0F, 0.0F, 5.0F);
    camera->nearDistance = 1.0F;
    camera->farDistance = 10.0F;
    camera->height = 2.0F;
    root->addChild(camera);

    auto transform = new SoTranslation;
    transform->translation.setValue(xTranslation, 0.0F, 0.0F);
    root->addChild(transform);

    auto cube = new SoCube;
    cube->width = 0.2F;
    cube->height = 0.2F;
    cube->depth = 0.2F;
    root->addChild(cube);

    SoRayPickAction pickAction(viewportRegion);
    pickAction.setPoint(pickPosition);
    pickAction.setRadius(1.0F);
    pickAction.apply(root);

    std::shared_ptr<const SoPickedPoint> pickedPoint;
    if (auto* point = pickAction.getPickedPoint()) {
        pickedPoint = std::shared_ptr<const SoPickedPoint>(new SoPickedPoint(*point));
    }

    root->unref();
    return pickedPoint;
}

std::shared_ptr<const SoPickedPoint> pickScenePoint(
    SoNode* root,
    const SbViewportRegion& viewportRegion,
    const SbVec2s& pickPosition,
    float pickRadius
)
{
    SoRayPickAction pickAction(viewportRegion);
    pickAction.setPoint(pickPosition);
    pickAction.setRadius(pickRadius);
    pickAction.apply(root);

    std::shared_ptr<const SoPickedPoint> pickedPoint;
    if (auto* point = pickAction.getPickedPoint()) {
        pickedPoint = std::shared_ptr<const SoPickedPoint>(new SoPickedPoint(*point));
    }

    return pickedPoint;
}

bool hasElementType(const std::string& element, const char* typeName)
{
    return element.rfind(typeName, 0) == 0;
}

std::vector<std::string> getAdjacentFaceElements(const Part::TopoShape& shape, const std::string& element)
{
    const auto edgeShape = shape.findShape(element.c_str());
    if (edgeShape.IsNull()) {
        return {};
    }

    std::vector<std::string> adjacentFaces;
    for (const auto faceIndex : shape.findAncestors(edgeShape, TopAbs_FACE)) {
        adjacentFaces.emplace_back("Face" + std::to_string(faceIndex));
    }
    return adjacentFaces;
}

template<typename ContainerT>
std::string joinElements(const ContainerT& container)
{
    std::string result = "[";
    bool first = true;
    for (const auto& element : container) {
        if (!first) {
            result += ", ";
        }
        result += element;
        first = false;
    }
    result += "]";
    return result;
}

std::set<std::string> collectVisibleFaceElements(
    const PartGui::ViewProviderPartExt& viewProvider,
    SoNode* sceneRoot,
    const SbViewportRegion& viewportRegion
)
{
    std::set<std::string> visibleFaces;
    const auto viewportSize = viewportRegion.getViewportSizePixels();
    for (int y = 20; y < viewportSize[1] - 20; y += 2) {
        for (int x = 20; x < viewportSize[0] - 20; x += 2) {
            const SbVec2s pickPosition(static_cast<short>(x), static_cast<short>(y));
            auto pickedPoint = pickScenePoint(sceneRoot, viewportRegion, pickPosition, 1.0F);
            if (!pickedPoint) {
                continue;
            }

            std::string rawElement;
            if (viewProvider.getElementPicked(pickedPoint.get(), rawElement)
                && hasElementType(rawElement, "Face")) {
                visibleFaces.insert(rawElement);
            }
        }
    }

    return visibleFaces;
}

std::optional<EdgePromotionProbe> findSilhouetteEdgePromotionProbe(
    const PartGui::ViewProviderPartExt& viewProvider,
    const Part::TopoShape& shape,
    SoNode* sceneRoot,
    const SbViewportRegion& viewportRegion,
    const std::set<std::string>& visibleFaces
)
{
    const auto viewportSize = viewportRegion.getViewportSizePixels();
    for (int step : {2, 1}) {
        for (int y = 20; y < viewportSize[1] - 20; y += step) {
            for (int x = 20; x < viewportSize[0] - 20; x += step) {
                const SbVec2s pickPosition(static_cast<short>(x), static_cast<short>(y));
                auto pickedPoint = pickScenePoint(sceneRoot, viewportRegion, pickPosition, 1.0F);
                if (!pickedPoint) {
                    continue;
                }

                std::string rawElement;
                if (!viewProvider.getElementPicked(pickedPoint.get(), rawElement)
                    || !hasElementType(rawElement, "Edge")) {
                    continue;
                }

                const auto adjacentFaces = getAdjacentFaceElements(shape, rawElement);
                if (adjacentFaces.size() != 2) {
                    continue;
                }

                std::optional<std::string> visibleFace;
                for (const auto& face : adjacentFaces) {
                    if (visibleFaces.contains(face)) {
                        if (visibleFace) {
                            visibleFace.reset();
                            break;
                        }
                        visibleFace = face;
                    }
                }

                if (visibleFace) {
                    return EdgePromotionProbe {pickPosition, rawElement, *visibleFace};
                }
            }
        }
    }

    return {};
}

std::optional<FacePickProbe> findRawFacePickProbe(
    const PartGui::ViewProviderPartExt& viewProvider,
    SoNode* sceneRoot,
    const SbViewportRegion& viewportRegion,
    const std::string& faceElement
)
{
    const auto viewportSize = viewportRegion.getViewportSizePixels();
    for (int step : {2, 1}) {
        for (int y = 20; y < viewportSize[1] - 20; y += step) {
            for (int x = 20; x < viewportSize[0] - 20; x += step) {
                const SbVec2s pickPosition(static_cast<short>(x), static_cast<short>(y));
                auto pickedPoint = pickScenePoint(sceneRoot, viewportRegion, pickPosition, 1.0F);
                if (!pickedPoint) {
                    continue;
                }

                std::string rawElement;
                if (viewProvider.getElementPicked(pickedPoint.get(), rawElement)
                    && rawElement == faceElement) {
                    return FacePickProbe {pickPosition, std::move(pickedPoint)};
                }
            }
        }
    }

    return {};
}

std::string summarizeRawEdgeCandidates(
    const PartGui::ViewProviderPartExt& viewProvider,
    const Part::TopoShape& shape,
    SoNode* sceneRoot,
    const SbViewportRegion& viewportRegion,
    const std::set<std::string>& visibleFaces
)
{
    std::ostringstream stream;
    const auto viewportSize = viewportRegion.getViewportSizePixels();
    int reportedEdges = 0;
    for (int y = 20; y < viewportSize[1] - 20 && reportedEdges < 8; y += 4) {
        for (int x = 20; x < viewportSize[0] - 20 && reportedEdges < 8; x += 4) {
            const SbVec2s pickPosition(static_cast<short>(x), static_cast<short>(y));
            auto pickedPoint = pickScenePoint(sceneRoot, viewportRegion, pickPosition, 1.0F);
            if (!pickedPoint) {
                continue;
            }

            std::string rawElement;
            if (!viewProvider.getElementPicked(pickedPoint.get(), rawElement)
                || !hasElementType(rawElement, "Edge")) {
                continue;
            }

            const auto adjacentFaces = getAdjacentFaceElements(shape, rawElement);
            stream << rawElement << '@' << x << ',' << y
                   << " adjacent=" << joinElements(adjacentFaces) << " visible=";

            std::vector<std::string> visibleAdjacentFaces;
            for (const auto& face : adjacentFaces) {
                if (visibleFaces.contains(face)) {
                    visibleAdjacentFaces.push_back(face);
                }
            }
            stream << joinElements(visibleAdjacentFaces) << "; ";
            ++reportedEdges;
        }
    }
    return stream.str();
}

}  // namespace

namespace Gui
{

class SoFCUnifiedSelectionTestAccess
{
public:
    static SelectionPickPolicy::Candidate getPickCandidate(
        const std::shared_ptr<const SoPickedPoint>& pickedPoint,
        const SbVec2f& cursorPosition
    )
    {
        SoFCUnifiedSelection::PickedInfo info;
        info.ownedPoint = pickedPoint;
        info.pp = info.ownedPoint.get();
        return SoFCUnifiedSelection::getPickCandidate(info, nullptr, nullptr, &cursorPosition);
    }

    static bool setSelection(
        SoFCUnifiedSelection& selectionNode,
        const std::shared_ptr<const SoPickedPoint>& pickedPoint,
        ViewProviderDocumentObject* vpd,
        const std::string& element,
        bool ctrlDown = false
    )
    {
        std::vector<SoFCUnifiedSelection::PickedInfo> infos(1);
        auto& info = infos.front();
        info.ownedPoint = pickedPoint;
        info.pp = info.ownedPoint.get();
        info.vpd = vpd;
        info.element = element;
        return selectionNode.setSelection(infos, ctrlDown);
    }
};

}  // namespace Gui

TEST_F(SelectionTest, testSelectionAllowsObjectsWhenNoGateIsInstalled)
{
    EXPECT_TRUE(Gui::Selection().testSelection(_doc, _allowedObject));
}

TEST_F(SelectionTest, testSelectionUsesActiveGateWithoutKeepingRejectionReason)
{
    auto* gate = new ObjectNameGate(_allowedObject->getNameInDocument());
    gate->notAllowedReason = "existing reason";
    Gui::Selection().addSelectionGate(gate, Gui::ResolveMode::NoResolve, _docName.c_str());

    EXPECT_TRUE(Gui::Selection().testSelection(_doc, _allowedObject));
    EXPECT_EQ(gate->notAllowedReason, "existing reason");

    EXPECT_FALSE(Gui::Selection().testSelection(_doc, _rejectedObject));
    EXPECT_EQ(gate->notAllowedReason, "existing reason");
    EXPECT_FALSE(Gui::Selection().hasPreselection());
    EXPECT_FALSE(Gui::Selection().hasSelection(_docName.c_str(), Gui::ResolveMode::NoResolve));
}

TEST(SelectionPickPolicyTest, canFinalizeSinglePickWhenNoGateIsInstalled)
{
    int owner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&owner, 0, true, false, true),
    };

    EXPECT_TRUE(Gui::SelectionPickPolicy::canFinalizeSinglePick(candidates));
}

TEST(SelectionPickPolicyTest, doesNotExpandPickRadiusWhenNoGateIsInstalled)
{
    int owner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&owner, 0, true, false, true),
    };

    EXPECT_FALSE(Gui::SelectionPickPolicy::shouldExpandPickRadius(candidates));
}

TEST(SelectionPickPolicyTest, continuesSinglePickWhenGateRejectedAllCurrentCandidates)
{
    int owner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&owner, 0, true, true, false),
    };

    EXPECT_FALSE(Gui::SelectionPickPolicy::canFinalizeSinglePick(candidates));
}

TEST(SelectionPickPolicyTest, expandsPickRadiusWhenGateRejectedAllCurrentCandidates)
{
    int owner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&owner, 0, true, true, false),
    };

    EXPECT_TRUE(Gui::SelectionPickPolicy::shouldExpandPickRadius(candidates));
}

TEST(SelectionPickPolicyTest, doesNotExpandPickRadiusWhenCurrentPickAlreadyPassesGate)
{
    int firstOwner {};
    int secondOwner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&firstOwner, 1, true, true, false),
        pickCandidate(&secondOwner, 0, true, true, true),
    };

    EXPECT_FALSE(Gui::SelectionPickPolicy::shouldExpandPickRadius(candidates));
}

TEST(SelectionPickPolicyTest, doesNotExpandPickRadiusWhenAnotherSelectableCandidateIsPresent)
{
    int firstOwner {};
    int secondOwner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&firstOwner, 1, true, true, false),
        pickCandidate(&secondOwner, 0, true, false, true),
    };

    EXPECT_FALSE(Gui::SelectionPickPolicy::shouldExpandPickRadius(candidates));
}

TEST(SelectionPickPolicyTest, choosesAllowedCandidateWhenPreferredPickIsRejected)
{
    int firstOwner {};
    int secondOwner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&firstOwner, 1, true, true, false),
        pickCandidate(&secondOwner, 0, true, true, true),
    };

    EXPECT_EQ(Gui::SelectionPickPolicy::choosePreferredPick(candidates), 1U);
}

TEST(SelectionPickPolicyTest, choosesNearestAllowedCandidateWhenPreferredPickIsRejected)
{
    int blockedOwner {};
    int fartherOwner {};
    int nearerOwner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&blockedOwner, 1, true, true, false, 0.0F),
        pickCandidate(&fartherOwner, 0, true, true, true, 0.09F),
        pickCandidate(&nearerOwner, 0, true, true, true, 0.01F),
    };

    EXPECT_EQ(Gui::SelectionPickPolicy::choosePreferredPick(candidates), 2U);
}

TEST(SelectionPickPolicyTest, choosesHigherPriorityFallbackWhenDistancesAreEquivalent)
{
    int lowerPriorityOwner {};
    int higherPriorityOwner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&lowerPriorityOwner, 1, true, true, true, 0.0100000F),
        pickCandidate(&higherPriorityOwner, 2, true, true, true, 0.0100005F),
    };

    auto fallback = Gui::SelectionPickPolicy::chooseAllowedFallbackPick(candidates);
    ASSERT_TRUE(fallback.has_value());
    EXPECT_EQ(*fallback, 1U);
}

TEST(SelectionPickPolicyTest, choosesAnnotationFallbackWhenPriorityAndDistanceAreEquivalent)
{
    int regularOwner {};
    int annotationOwner {};
    auto regular = pickCandidate(&regularOwner, 1, true, true, true, 0.0200000F);
    auto annotation = pickCandidate(&annotationOwner, 1, true, true, true, 0.0200005F);
    annotation.isAnnotation = true;

    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {regular, annotation};

    auto fallback = Gui::SelectionPickPolicy::chooseAllowedFallbackPick(candidates);
    ASSERT_TRUE(fallback.has_value());
    EXPECT_EQ(*fallback, 1U);
}

TEST(SelectionPickPolicyTest, choosesAllowedFallbackCandidateUsingRealPickedPointDistances)
{
    const SbViewportRegion viewportRegion {SbVec2s {100, 100}};
    const SbVec2s cursorPosition {50, 50};

    auto nearerPoint = pickCubePoint(0.0F, cursorPosition, viewportRegion);
    auto fartherPoint = pickCubePoint(0.4F, SbVec2s {70, 50}, viewportRegion);

    ASSERT_NE(nearerPoint, nullptr);
    ASSERT_NE(fartherPoint, nullptr);

    const auto normalizedCursorPosition = normalizedPosition(cursorPosition, viewportRegion);
    auto nearerCandidate = Gui::SoFCUnifiedSelectionTestAccess::getPickCandidate(
        nearerPoint,
        normalizedCursorPosition
    );
    auto fartherCandidate = Gui::SoFCUnifiedSelectionTestAccess::getPickCandidate(
        fartherPoint,
        normalizedCursorPosition
    );

    nearerCandidate.hasGate = true;
    nearerCandidate.passesGate = true;
    fartherCandidate.hasGate = true;
    fartherCandidate.passesGate = true;

    int blockedOwner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&blockedOwner, 1, true, true, false, 0.0F),
        fartherCandidate,
        nearerCandidate,
    };

    auto fallback = Gui::SelectionPickPolicy::chooseAllowedFallbackPick(candidates);
    ASSERT_TRUE(fallback.has_value());
    EXPECT_LT(nearerCandidate.cursorDistanceSquared, fartherCandidate.cursorDistanceSquared);
    EXPECT_EQ(*fallback, 2U);
}

TEST(SelectionPickPolicyTest, preservesPriorityChoiceWithinFirstOwner)
{
    int firstOwner {};
    int secondOwner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&firstOwner, 1, true, false, true),
        pickCandidate(&firstOwner, 2, true, false, true),
        pickCandidate(&secondOwner, 3, true, false, true),
    };

    EXPECT_EQ(Gui::SelectionPickPolicy::choosePreferredPick(candidates), 1U);
}

TEST(SelectionPickPolicyTest, keepsPreferredPickWhenNoCandidatePassesGate)
{
    int firstOwner {};
    int secondOwner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&firstOwner, 1, true, true, false),
        pickCandidate(&secondOwner, 0, true, true, false),
    };

    EXPECT_EQ(Gui::SelectionPickPolicy::choosePreferredPick(candidates), 0U);
}

TEST_F(PartSelectionPromotionTest, promotesSilhouetteEdgeToVisibleFaceForFaceOnlyGate)
{
    PartSelectionScene scene("part_selection_test");
    scene.initialize();
    ASSERT_NE(scene.doc, nullptr);
    ASSERT_NE(scene.box, nullptr);
    ASSERT_NE(scene.viewProvider, nullptr);
    ASSERT_FALSE(scene.renderedShape.isNull());

    // Derive the visible face from the actual rendered view so the test checks
    // face-filter promotion behavior instead of relying on face numbering.
    const auto probe = scene.findSilhouetteEdgeProbe();
    ASSERT_TRUE(probe.has_value()) << "visibleFaces=" << joinElements(scene.visibleFaces)
                                   << " rawEdges=" << scene.summarizeRawEdges();

    const auto pickedPoint = pickScenePoint(scene.root, scene.viewportRegion, probe->position, 1.0F);
    ASSERT_NE(pickedPoint, nullptr);

    FaceOnlyGate gate;
    const auto normalizedCursorPosition = normalizedPosition(probe->position, scene.viewportRegion);
    Gui::SelectionPickContext pickContext;
    pickContext.gate = &gate;
    pickContext.normalizedCursorPosition = &normalizedCursorPosition;
    pickContext.repick.sceneRoot = scene.root;
    pickContext.repick.viewportRegion = &scene.viewportRegion;
    pickContext.repick.eventPosition = &probe->position;
    pickContext.repick.pickRadius = 1.0F;

    std::string promotedElement = probe->rawEdge;
    ASSERT_TRUE(scene.viewProvider->getElementPicked(pickedPoint.get(), promotedElement, &pickContext));
    EXPECT_EQ(promotedElement, probe->visibleFace);
}

TEST_F(PartSelectionPromotionTest, keepsFaceSelectionWhenHierarchyAscentIsBlockedByFaceOnlyGate)
{
    PartSelectionScene scene("part_selection_test");
    scene.initialize();
    ASSERT_NE(scene.doc, nullptr);
    ASSERT_NE(scene.box, nullptr);
    ASSERT_NE(scene.viewProvider, nullptr);
    ASSERT_FALSE(scene.renderedShape.isNull());

    const auto probe = scene.findSilhouetteEdgeProbe();
    ASSERT_TRUE(probe.has_value()) << "visibleFaces=" << joinElements(scene.visibleFaces)
                                   << " rawEdges=" << scene.summarizeRawEdges();

    const auto facePick = scene.findRawFacePick(probe->visibleFace);
    ASSERT_TRUE(facePick.has_value()) << "visibleFaces=" << joinElements(scene.visibleFaces)
                                      << " targetFace=" << probe->visibleFace;

    Gui::Selection()
        .addSelectionGate(new FaceOnlyGate, Gui::ResolveMode::NoResolve, scene.docName.c_str());
    EXPECT_FALSE(Gui::Selection().testSelection(scene.doc, scene.box, nullptr));

    auto* selectionNode = new Gui::SoFCUnifiedSelection;
    selectionNode->ref();
    Gui::SelectionLogDisabler disableSelectionLog(true);

    ASSERT_TRUE(
        Gui::SoFCUnifiedSelectionTestAccess::setSelection(
            *selectionNode,
            facePick->pickedPoint,
            scene.viewProvider,
            probe->visibleFace
        )
    );
    EXPECT_TRUE(
        Gui::Selection().isSelected(scene.box, probe->visibleFace.c_str(), Gui::ResolveMode::NoResolve)
    );

    // A repeated click on the same face should not escalate to an object selection
    // when the active face-only gate would reject that parent target.
    ASSERT_TRUE(
        Gui::SoFCUnifiedSelectionTestAccess::setSelection(
            *selectionNode,
            facePick->pickedPoint,
            scene.viewProvider,
            probe->visibleFace
        )
    );
    EXPECT_TRUE(
        Gui::Selection().isSelected(scene.box, probe->visibleFace.c_str(), Gui::ResolveMode::NoResolve)
    );
    EXPECT_FALSE(Gui::Selection().isSelected(scene.box, nullptr, Gui::ResolveMode::NoResolve));
    EXPECT_EQ(
        Gui::Selection().getSelectedElement(scene.box, probe->visibleFace.c_str()),
        probe->visibleFace
    );

    selectionNode->unref();
}
