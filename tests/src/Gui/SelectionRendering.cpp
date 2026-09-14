// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QColor>
#include <QImage>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QPoint>
#include <QSize>
#include <QSurfaceFormat>
#include <QTest>

#include <array>
#include <memory>

#include <FCConfig.h>

#ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
#else
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# include <GL/gl.h>
#endif

#include <Inventor/SoPath.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoSeparator.h>

#include <src/App/InitApplication.h>

#include <Gui/Application.h>
#include <Gui/Inventor/So3DAnnotation.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SoFCUnifiedSelection.h>
#include <Gui/SoFCDB.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/SoBrepPointSet.h>

namespace
{

constexpr int renderWidth = 160;
constexpr int renderHeight = 120;

template<class T>
struct CoinUnref
{
    void operator()(T* object) const
    {
        if (object) {
            object->unref();
        }
    }
};

template<class T>
using CoinRefPtr = std::unique_ptr<T, CoinUnref<T>>;

struct RenderScene
{
    CoinRefPtr<SoSeparator> root;
    CoinRefPtr<SoPath> facePath;
    CoinRefPtr<SoPath> edgePath;
    CoinRefPtr<SoPath> selectionRootPath;
    CoinRefPtr<SoPath> otherFacePath;
    CoinRefPtr<SoPath> otherSelectionRootPath;
};

struct RenderResult
{
    QImage image;
    int delayedPathCount {0};
};

class DelayedRenderGuard
{
public:
    DelayedRenderGuard()
    {
        Gui::So3DAnnotation::render = true;
        Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths = true;
    }

    ~DelayedRenderGuard()
    {
        Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths = false;
        Gui::So3DAnnotation::render = false;
    }
};

void initPartGuiCoinClasses()
{
    static bool initialized = false;
    if (!initialized) {
        PartGui::SoBrepFaceSet::initClass();
        PartGui::SoBrepEdgeSet::initClass();
        PartGui::SoBrepPointSet::initClass();
        initialized = true;
    }
}

SoOrthographicCamera* makeTopCamera()
{
    auto* camera = new SoOrthographicCamera;
    camera->position.setValue(0.0f, 0.0f, 3.0f);
    camera->nearDistance.setValue(0.1f);
    camera->farDistance.setValue(10.0f);
    camera->height.setValue(2.2f);
    return camera;
}

RenderScene makePartialRenderScene(bool nestUnrelatedObject = false, bool addOccluder = false)
{
    auto* root = new SoSeparator;
    root->ref();

    auto* camera = makeTopCamera();
    root->addChild(camera);

    auto* lightModel = new SoLightModel;
    lightModel->model.setValue(SoLightModel::BASE_COLOR);
    root->addChild(lightModel);

    auto* selectionRoot = new Gui::SoFCSelectionRoot;
    root->addChild(selectionRoot);

    auto* coords = new SoCoordinate3;
    const std::array<SbVec3f, 9> points {
        SbVec3f(-0.9f, -0.45f, 0.0f),
        SbVec3f(-0.1f, -0.45f, 0.0f),
        SbVec3f(-0.1f, 0.45f, 0.0f),
        SbVec3f(-0.9f, 0.45f, 0.0f),
        SbVec3f(0.1f, -0.45f, 0.0f),
        SbVec3f(0.9f, -0.45f, 0.0f),
        SbVec3f(0.9f, 0.45f, 0.0f),
        SbVec3f(0.1f, 0.45f, 0.0f),
        SbVec3f(-0.5f, 0.0f, 0.0f),
    };
    coords->point.setValues(0, points.size(), points.data());
    selectionRoot->addChild(coords);

    auto* material = new SoMaterial;
    material->diffuseColor.setValue(SbColor(0.05f, 0.25f, 0.95f));
    selectionRoot->addChild(material);

    auto* binding = new SoMaterialBinding;
    binding->value.setValue(SoMaterialBinding::OVERALL);
    selectionRoot->addChild(binding);

    auto* faces = new PartGui::SoBrepFaceSet;
    // Give the first face more triangles than the second. The highlight
    // overlay reuses one SoIndexedFaceSet, so replacing face 0 with face 1
    // must also shrink its coordIndex field.
    const std::array<int32_t, 24> coordIndex {
        0, 1, 8, -1, 1, 2, 8, -1, 2, 3, 8, -1, 3, 0, 8, -1, 4, 5, 6, -1, 4, 6, 7, -1,
    };
    const std::array<int32_t, 2> partIndex {4, 2};
    faces->coordIndex.setValues(0, coordIndex.size(), coordIndex.data());
    faces->partIndex.setValues(0, partIndex.size(), partIndex.data());
    selectionRoot->addChild(faces);

    auto* edgeMaterial = new SoMaterial;
    edgeMaterial->diffuseColor.setValue(SbColor(0.0f, 0.0f, 0.0f));
    selectionRoot->addChild(edgeMaterial);

    auto* edges = new PartGui::SoBrepEdgeSet;
    const std::array<int32_t, 24> edgeCoordIndex {
        0, 1, -1, 1, 2, -1, 2, 3, -1, 3, 0, -1, 4, 5, -1, 5, 6, -1, 6, 7, -1, 7, 4, -1,
    };
    const std::array<int32_t, 10> faceEdgeIndex {
        0,
        1,
        2,
        3,
        -1,
        4,
        5,
        6,
        7,
        -1,
    };
    edges->coordIndex.setValues(0, edgeCoordIndex.size(), edgeCoordIndex.data());
    edges->faceEdgeIndex.setValues(0, faceEdgeIndex.size(), faceEdgeIndex.data());
    selectionRoot->addChild(edges);

    auto* otherObjectRoot = new Gui::SoFCSelectionRoot;
    (nestUnrelatedObject ? static_cast<SoSeparator*>(selectionRoot) : root)->addChild(otherObjectRoot);
    auto* otherObjectCoords = new SoCoordinate3;
    const std::array<SbVec3f, 4> otherObjectPoints {
        SbVec3f(1.03f, -0.35f, 0.0f),
        SbVec3f(1.38f, -0.35f, 0.0f),
        SbVec3f(1.38f, 0.35f, 0.0f),
        SbVec3f(1.03f, 0.35f, 0.0f),
    };
    otherObjectCoords->point.setValues(0, otherObjectPoints.size(), otherObjectPoints.data());
    otherObjectRoot->addChild(otherObjectCoords);

    auto* otherObjectMaterial = new SoMaterial;
    otherObjectMaterial->diffuseColor.setValue(SbColor(0.05f, 0.25f, 0.95f));
    otherObjectRoot->addChild(otherObjectMaterial);
    auto* otherObjectBinding = new SoMaterialBinding;
    otherObjectBinding->value.setValue(SoMaterialBinding::OVERALL);
    otherObjectRoot->addChild(otherObjectBinding);

    auto* otherObjectFaces = new PartGui::SoBrepFaceSet;
    const std::array<int32_t, 8> otherObjectCoordIndex {0, 1, 2, -1, 0, 2, 3, -1};
    const std::array<int32_t, 1> otherObjectPartIndex {2};
    otherObjectFaces->coordIndex
        .setValues(0, otherObjectCoordIndex.size(), otherObjectCoordIndex.data());
    otherObjectFaces->partIndex.setValues(0, otherObjectPartIndex.size(), otherObjectPartIndex.data());
    otherObjectRoot->addChild(otherObjectFaces);

    auto* otherObjectEdgeMaterial = new SoMaterial;
    otherObjectEdgeMaterial->diffuseColor.setValue(SbColor(0.0f, 0.0f, 0.0f));
    otherObjectRoot->addChild(otherObjectEdgeMaterial);
    auto* otherObjectEdges = new PartGui::SoBrepEdgeSet;
    const std::array<int32_t, 12> otherObjectEdgeCoordIndex {
        0,
        1,
        -1,
        1,
        2,
        -1,
        2,
        3,
        -1,
        3,
        0,
        -1,
    };
    const std::array<int32_t, 5> otherObjectFaceEdgeIndex {0, 1, 2, 3, -1};
    otherObjectEdges->coordIndex
        .setValues(0, otherObjectEdgeCoordIndex.size(), otherObjectEdgeCoordIndex.data());
    otherObjectEdges->faceEdgeIndex
        .setValues(0, otherObjectFaceEdgeIndex.size(), otherObjectFaceEdgeIndex.data());
    otherObjectRoot->addChild(otherObjectEdges);

    if (addOccluder) {
        auto* occluder = new SoSeparator;
        root->addChild(occluder);

        auto* occluderCoords = new SoCoordinate3;
        const std::array<SbVec3f, 4> occluderPoints {
            SbVec3f(-0.7f, -0.25f, 0.2f),
            SbVec3f(-0.3f, -0.25f, 0.2f),
            SbVec3f(-0.3f, 0.25f, 0.2f),
            SbVec3f(-0.7f, 0.25f, 0.2f),
        };
        occluderCoords->point.setValues(0, occluderPoints.size(), occluderPoints.data());
        occluder->addChild(occluderCoords);

        auto* occluderMaterial = new SoMaterial;
        occluderMaterial->diffuseColor.setValue(SbColor(0.95f, 0.75f, 0.05f));
        occluder->addChild(occluderMaterial);

        auto* occluderFace = new SoIndexedFaceSet;
        const std::array<int32_t, 8> occluderIndices {0, 1, 2, -1, 0, 2, 3, -1};
        occluderFace->coordIndex.setValues(0, occluderIndices.size(), occluderIndices.data());
        occluder->addChild(occluderFace);
    }

    SoSearchAction edgeSearch;
    edgeSearch.setNode(edges);
    edgeSearch.apply(root);
    SoPath* edgePath = edgeSearch.getPath();
    if (edgePath) {
        edgePath->ref();
    }

    SoSearchAction search;
    search.setNode(faces);
    search.apply(root);
    SoPath* facePath = search.getPath();
    if (facePath) {
        facePath->ref();
    }

    SoSearchAction selectionRootSearch;
    selectionRootSearch.setNode(selectionRoot);
    selectionRootSearch.apply(root);
    SoPath* selectionRootPath = selectionRootSearch.getPath();
    if (selectionRootPath) {
        selectionRootPath->ref();
    }

    SoSearchAction otherFaceSearch;
    otherFaceSearch.setNode(otherObjectFaces);
    otherFaceSearch.apply(root);
    SoPath* otherFacePath = otherFaceSearch.getPath();
    if (otherFacePath) {
        otherFacePath->ref();
    }

    SoSearchAction otherSelectionRootSearch;
    otherSelectionRootSearch.setNode(otherObjectRoot);
    otherSelectionRootSearch.apply(root);
    SoPath* otherSelectionRootPath = otherSelectionRootSearch.getPath();
    if (otherSelectionRootPath) {
        otherSelectionRootPath->ref();
    }

    return {
        CoinRefPtr<SoSeparator>(root),
        CoinRefPtr<SoPath>(facePath),
        CoinRefPtr<SoPath>(edgePath),
        CoinRefPtr<SoPath>(selectionRootPath),
        CoinRefPtr<SoPath>(otherFacePath),
        CoinRefPtr<SoPath>(otherSelectionRootPath)
    };
}

CoinRefPtr<SoPath> makeProviderRootedPath(SoPath* providerRootPath, int childIndex)
{
    if (!providerRootPath || providerRootPath->getLength() == 0) {
        return {};
    }

    auto* path = new SoPath(providerRootPath->getTail());
    path->ref();
    path->append(childIndex);
    return CoinRefPtr<SoPath>(path);
}

struct PointRenderScene
{
    CoinRefPtr<SoSeparator> root;
    CoinRefPtr<SoPath> providerRootPath;
};

PointRenderScene makePointRenderScene()
{
    auto* root = new SoSeparator;
    root->ref();
    root->addChild(makeTopCamera());

    auto* lightModel = new SoLightModel;
    lightModel->model.setValue(SoLightModel::BASE_COLOR);
    root->addChild(lightModel);

    auto* selectionRoot = new Gui::SoFCSelectionRoot;
    root->addChild(selectionRoot);

    auto* coords = new SoCoordinate3;
    const SbVec3f point(0.0f, 0.0f, 0.0f);
    coords->point.setValues(0, 1, &point);
    selectionRoot->addChild(coords);

    auto* material = new SoMaterial;
    material->diffuseColor.setValue(SbColor(0.05f, 0.25f, 0.95f));
    selectionRoot->addChild(material);

    auto* points = new PartGui::SoBrepPointSet;
    selectionRoot->addChild(points);

    SoSearchAction rootSearch;
    rootSearch.setNode(selectionRoot);
    rootSearch.apply(root);
    SoPath* providerRootPath = rootSearch.getPath();
    if (providerRootPath) {
        providerRootPath->ref();
    }

    return {CoinRefPtr<SoSeparator>(root), CoinRefPtr<SoPath>(providerRootPath)};
}

SoFaceDetail makeFirstFaceDetail()
{
    SoFaceDetail faceDetail;
    faceDetail.setPartIndex(0);
    return faceDetail;
}

SoFaceDetail makeSecondFaceDetail()
{
    SoFaceDetail faceDetail;
    faceDetail.setPartIndex(1);
    return faceDetail;
}

SoLineDetail makeTopEdgeDetail()
{
    SoLineDetail lineDetail;
    lineDetail.setLineIndex(2);
    return lineDetail;
}

void applySelectionState(SoPath* facePath, const SoDetail* detail)
{
    Gui::SoSelectionElementAction selectionAction(Gui::SoSelectionElementAction::Append);
    selectionAction.setColor(SbColor(0.0f, 0.8f, 0.0f));
    selectionAction.setElement(detail);
    selectionAction.apply(facePath);
}

void applyPartialRenderState(SoPath* facePath, const SoDetail* detail)
{
    Gui::SoSelectionElementAction partialRenderAction(Gui::SoSelectionElementAction::Append, true);
    partialRenderAction.setColor(SbColor(0.05f, 0.25f, 0.95f));
    partialRenderAction.setElement(detail);
    partialRenderAction.apply(facePath);
}

void applyClarifyHighlightState(SoPath* path, const SoDetail* detail)
{
    Gui::SoHighlightElementAction highlightAction;
    highlightAction.setHighlighted(true);
    highlightAction.setColor(SbColor(1.0f, 0.0f, 0.0f));
    highlightAction.setElement(detail);
    highlightAction.setHighlightPresentation(
        Gui::HighlightPresentation::DrawOnTop | Gui::HighlightPresentation::FadeOtherElements
    );
    highlightAction.apply(path);
}

void clearHighlightState(SoPath* path)
{
    Gui::SoHighlightElementAction highlightAction;
    highlightAction.setHighlighted(false);
    highlightAction.apply(path);
}

void clearSecondaryRenderState(SoNode* root)
{
    Gui::SoSelectionElementAction clearRenderAction(Gui::SoSelectionElementAction::None, true);
    clearRenderAction.apply(root);
}

RenderResult renderWithDelayedClarifyPass(
    SoNode* root,
    const QColor& background = Qt::white,
    bool renderDelayedPass = true
)
{
    RenderResult result;

    QSurfaceFormat format;
    QOpenGLContext context;
    context.setFormat(format);
    if (!context.create()) {
        return result;
    }

    QOffscreenSurface surface;
    surface.setFormat(format);
    surface.create();
    if (!surface.isValid() || !context.makeCurrent(&surface)) {
        return result;
    }

    QOpenGLFramebufferObjectFormat framebufferFormat;
    framebufferFormat.setAttachment(QOpenGLFramebufferObject::Depth);
    framebufferFormat.setInternalTextureFormat(GL_RGB);
    QOpenGLFramebufferObject framebuffer(renderWidth, renderHeight, framebufferFormat);
    framebuffer.bind();

    const SbViewportRegion viewport(renderWidth, renderHeight);
    SoGLRenderAction renderAction(viewport);
    renderAction.setRenderingIsRemote(false);
    renderAction.setCacheContext(SoGLCacheContextElement::getUniqueCacheContext());

    glViewport(0, 0, renderWidth, renderHeight);
    glEnable(GL_DEPTH_TEST);
    glClearColor(
        static_cast<float>(background.redF()),
        static_cast<float>(background.greenF()),
        static_cast<float>(background.blueF()),
        1.0f
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderAction.apply(root);

    SoPathList delayedPaths = Gui::SoDelayedAnnotationsElement::getDelayedPaths(
        renderAction.getState()
    );
    result.delayedPathCount = delayedPaths.getLength();

    {
        DelayedRenderGuard guard;
        glClear(GL_DEPTH_BUFFER_BIT);
        if (renderDelayedPass && result.delayedPathCount > 0) {
            renderAction.apply(delayedPaths, TRUE);
        }
    }

    result.image = framebuffer.toImage();
    framebuffer.release();
    context.doneCurrent();

    return result;
}

QColor meanColor(const QImage& image, QPoint center, int radius)
{
    int red = 0;
    int green = 0;
    int blue = 0;
    int count = 0;

    for (int y = center.y() - radius; y <= center.y() + radius; ++y) {
        for (int x = center.x() - radius; x <= center.x() + radius; ++x) {
            if (x < 0 || y < 0 || x >= image.width() || y >= image.height()) {
                continue;
            }
            const QColor color = image.pixelColor(x, y);
            red += color.red();
            green += color.green();
            blue += color.blue();
            ++count;
        }
    }

    if (count == 0) {
        return {};
    }

    return QColor(red / count, green / count, blue / count);
}

QByteArray colorMessage(const char* label, const QColor& color)
{
    return QByteArray(label) + " color was rgb(" + QByteArray::number(color.red()) + ", "
        + QByteArray::number(color.green()) + ", " + QByteArray::number(color.blue()) + ")";
}

template<class Predicate>
int countPixelsMatching(const QImage& image, QPoint center, int radius, Predicate predicate)
{
    int count = 0;
    for (int y = center.y() - radius; y <= center.y() + radius; ++y) {
        for (int x = center.x() - radius; x <= center.x() + radius; ++x) {
            if (x < 0 || y < 0 || x >= image.width() || y >= image.height()) {
                continue;
            }
            if (predicate(image.pixelColor(x, y))) {
                ++count;
            }
        }
    }
    return count;
}

}  // namespace

class testSelectionRendering: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
        if (!Gui::Application::Instance) {
            Gui::Application::initApplication();
            _guiApplication = std::make_unique<Gui::Application>(false);
        }
        if (!Gui::SoFCDB::isInitialized()) {
            Gui::Application::initOpenInventor();
        }
        initPartGuiCoinClasses();
    }

    void cleanup()
    {
        Gui::Selection().setClarifySelectionActive(false);
        Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths = false;
        Gui::So3DAnnotation::render = false;
    }

    void clarifyHighlightDelayedPassPreservesPartialRenderMask()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyPartialRenderState(scene.facePath.get(), &faceDetail);
        applyClarifyHighlightState(scene.facePath.get(), &faceDetail);

        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());

        QVERIFY(!result.image.isNull());
        QCOMPARE(result.image.size(), QSize(renderWidth, renderHeight));
        QCOMPARE(result.delayedPathCount, 1);

        const QColor highlightedFace
            = meanColor(result.image, QPoint(renderWidth * 3 / 10, renderHeight / 2), 2);
        const QColor maskedFace
            = meanColor(result.image, QPoint(renderWidth * 7 / 10, renderHeight / 2), 2);

        const QByteArray highlightedMessage = colorMessage("highlighted face", highlightedFace);
        QVERIFY2(
            highlightedFace.red() > 200 && highlightedFace.green() < 80 && highlightedFace.blue() < 80,
            highlightedMessage.constData()
        );

        // Regression guard: the clarify delayed pass must not repaint faces hidden by partial render.
        const QByteArray maskedMessage = colorMessage("masked face", maskedFace);
        QVERIFY2(
            maskedFace.red() > 245 && maskedFace.green() > 245 && maskedFace.blue() > 245,
            maskedMessage.constData()
        );
    }

    void clarifyHighlightFadesUnhighlightedFaces()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyClarifyHighlightState(scene.facePath.get(), &faceDetail);

        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());

        QVERIFY(!result.image.isNull());
        QCOMPARE(result.image.size(), QSize(renderWidth, renderHeight));
        QCOMPARE(result.delayedPathCount, 1);

        const QColor highlightedFace
            = meanColor(result.image, QPoint(renderWidth * 3 / 10, renderHeight / 2), 2);
        const QColor fadedNeighboringFace
            = meanColor(result.image, QPoint(renderWidth * 7 / 10, renderHeight / 2), 2);

        const QByteArray highlightedMessage = colorMessage("highlighted face", highlightedFace);
        QVERIFY2(
            highlightedFace.red() > 200 && highlightedFace.green() < 80 && highlightedFace.blue() < 80,
            highlightedMessage.constData()
        );

        const QByteArray neighboringMessage
            = colorMessage("faded neighboring face", fadedNeighboringFace);
        QVERIFY2(
            fadedNeighboringFace.red() > 80 && fadedNeighboringFace.red() < 245
                && fadedNeighboringFace.green() > 120 && fadedNeighboringFace.blue() > 180,
            neighboringMessage.constData()
        );
    }

    void detailedHighlightReplacementClearsPreviousFaceAndBoundary()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);
        QVERIFY(scene.selectionRootPath);

        auto facePath = makeProviderRootedPath(scene.selectionRootPath.get(), 3);
        QVERIFY(facePath);

        SoFaceDetail firstFace = makeFirstFaceDetail();
        SoFaceDetail secondFace = makeSecondFaceDetail();
        applyClarifyHighlightState(facePath.get(), &firstFace);

        const RenderResult first = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!first.image.isNull());
        QCOMPARE(first.delayedPathCount, 1);

        applyClarifyHighlightState(facePath.get(), &secondFace);
        const RenderResult second = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!second.image.isNull());
        QCOMPARE(second.delayedPathCount, 1);

        const QColor firstFaceAfterSwitch
            = meanColor(second.image, QPoint(renderWidth * 3 / 10, renderHeight / 2), 2);
        const QColor secondFaceAfterSwitch
            = meanColor(second.image, QPoint(renderWidth * 7 / 10, renderHeight / 2), 2);
        QVERIFY(firstFaceAfterSwitch.red() > 80 && firstFaceAfterSwitch.green() > 120);
        QVERIFY(secondFaceAfterSwitch.red() > 200 && secondFaceAfterSwitch.green() < 80);

        const QPoint firstBoundary(renderWidth * 3 / 10, renderHeight * 31 / 100);
        const QPoint secondBoundary(renderWidth * 7 / 10, renderHeight * 31 / 100);
        const auto isAccent = [](const QColor& color) {
            return color.red() > 140 && color.green() < 90 && color.blue() < 90;
        };
        QVERIFY(countPixelsMatching(second.image, firstBoundary, 6, isAccent) == 0);
        QVERIFY(countPixelsMatching(second.image, secondBoundary, 6, isAccent) > 0);
    }

    void detailedHighlightReplacementClearsPreviousOwner()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);
        QVERIFY(scene.otherFacePath);

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyClarifyHighlightState(scene.facePath.get(), &faceDetail);
        const auto firstContext = Gui::SoFCSelectionRoot::getGlobalHighlightContext();
        QVERIFY(firstContext);

        applyClarifyHighlightState(scene.otherFacePath.get(), &faceDetail);
        const auto secondContext = Gui::SoFCSelectionRoot::getGlobalHighlightContext();
        QVERIFY(secondContext);
        QVERIFY(secondContext != firstContext);

        const RenderResult switched = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!switched.image.isNull());
        QCOMPARE(switched.delayedPathCount, 1);

        const QColor previousOwnerFace
            = meanColor(switched.image, QPoint(renderWidth * 3 / 10, renderHeight / 2), 2);
        const QColor activeOwnerFace
            = meanColor(switched.image, QPoint(renderWidth * 89 / 100, renderHeight / 2), 1);
        QVERIFY(
            previousOwnerFace.red() < 80 && previousOwnerFace.green() < 120
            && previousOwnerFace.blue() > 180
        );
        QVERIFY(activeOwnerFace.red() > 200 && activeOwnerFace.green() < 80);

        for (int i = 0; i < 8; ++i) {
            SoPath* activePath = i % 2 == 0 ? scene.facePath.get() : scene.otherFacePath.get();
            applyClarifyHighlightState(activePath, &faceDetail);
            const RenderResult repeated = renderWithDelayedClarifyPass(scene.root.get());
            QVERIFY(!repeated.image.isNull());
            QCOMPARE(repeated.delayedPathCount, 1);
        }

        clearHighlightState(scene.otherFacePath.get());
        QVERIFY(!Gui::SoFCSelectionRoot::getGlobalHighlightContext());
        const RenderResult cleared = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!cleared.image.isNull());
        QCOMPARE(cleared.delayedPathCount, 0);
    }

    void clarifyHighlightClearsPreselectionAndRestoresScene()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);
        QVERIFY(scene.selectionRootPath);

        auto facePath = makeProviderRootedPath(scene.selectionRootPath.get(), 3);
        QVERIFY(facePath);

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyClarifyHighlightState(facePath.get(), &faceDetail);
        const RenderResult highlighted = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!highlighted.image.isNull());
        QCOMPARE(highlighted.delayedPathCount, 1);

        clearHighlightState(facePath.get());
        const RenderResult cleared = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!cleared.image.isNull());
        QCOMPARE(cleared.delayedPathCount, 0);

        const QColor firstFace
            = meanColor(cleared.image, QPoint(renderWidth * 3 / 10, renderHeight / 2), 2);
        const QColor secondFace
            = meanColor(cleared.image, QPoint(renderWidth * 7 / 10, renderHeight / 2), 2);
        QVERIFY(firstFace.red() < 80 && firstFace.green() < 120 && firstFace.blue() > 180);
        QVERIFY(secondFace.red() < 80 && secondFace.green() < 120 && secondFace.blue() > 180);
    }

    void clarifyFaceTintSurvivesEmptySecondaryContext()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyPartialRenderState(scene.facePath.get(), nullptr);
        applyClarifyHighlightState(scene.facePath.get(), &faceDetail);

        const RenderResult clarified = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!clarified.image.isNull());
        QCOMPARE(clarified.delayedPathCount, 1);

        const QColor highlightedFace
            = meanColor(clarified.image, QPoint(renderWidth * 3 / 10, renderHeight / 2), 2);
        QVERIFY2(
            highlightedFace.red() > 200 && highlightedFace.green() < 80 && highlightedFace.blue() < 80,
            colorMessage("highlighted face with empty secondary context", highlightedFace).constData()
        );
    }

    void clarifyFaceTintRespectsOccludingGeometry()
    {
        auto scene = makePartialRenderScene(false, true);
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);

        const QPoint coveredFaceCenter(renderWidth * 3 / 10, renderHeight / 2);
        const RenderResult baseline = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!baseline.image.isNull());

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyClarifyHighlightState(scene.facePath.get(), &faceDetail);
        const RenderResult clarified = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!clarified.image.isNull());
        QCOMPARE(clarified.delayedPathCount, 1);

        const QColor before = meanColor(baseline.image, coveredFaceCenter, 2);
        const QColor after = meanColor(clarified.image, coveredFaceCenter, 2);
        QVERIFY2(
            after.red() > 180 && after.green() > 120 && after.blue() < 80,
            colorMessage("occluding face", after).constData()
        );
        QVERIFY(std::abs(after.red() - before.red()) < 8);
        QVERIFY(std::abs(after.green() - before.green()) < 8);
        QVERIFY(std::abs(after.blue() - before.blue()) < 8);
    }

    void clarifyHighlightFadesAgainstDarkBackground()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);

        const QColor darkBackground(24, 24, 24);
        const RenderResult baseline = renderWithDelayedClarifyPass(scene.root.get(), darkBackground);
        QVERIFY(!baseline.image.isNull());

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyClarifyHighlightState(scene.facePath.get(), &faceDetail);
        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get(), darkBackground);
        QVERIFY(!result.image.isNull());
        const QColor fadedFace
            = meanColor(result.image, QPoint(renderWidth * 7 / 10, renderHeight / 2), 2);
        const QColor originalFace
            = meanColor(baseline.image, QPoint(renderWidth * 7 / 10, renderHeight / 2), 2);
        const auto distance = [](const QColor& first, const QColor& second) {
            const int red = first.red() - second.red();
            const int green = first.green() - second.green();
            const int blue = first.blue() - second.blue();
            return red * red + green * green + blue * blue;
        };
        QVERIFY(distance(fadedFace, darkBackground) > 0);
        QVERIFY(distance(fadedFace, darkBackground) < distance(originalFace, darkBackground));
    }

    void clarifyHighlightKeepsSeparateSceneObjectsUnfaded()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);

        const QPoint otherObjectFace(renderWidth * 89 / 100, renderHeight / 2);
        const QPoint otherObjectEdge(renderWidth * 89 / 100, renderHeight * 34 / 100);
        const RenderResult baseline = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!baseline.image.isNull());

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyClarifyHighlightState(scene.facePath.get(), &faceDetail);
        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!result.image.isNull());

        const QColor originalFace = meanColor(baseline.image, otherObjectFace, 1);
        const QColor fadedFace = meanColor(result.image, otherObjectFace, 1);
        QCOMPARE(fadedFace, originalFace);

        const QColor originalEdge = meanColor(baseline.image, otherObjectEdge, 1);
        const QColor fadedEdge = meanColor(result.image, otherObjectEdge, 1);
        QCOMPARE(fadedEdge, originalEdge);
    }

    void clarifyHighlightDoesNotAccentUnrelatedNestedGeometry()
    {
        auto scene = makePartialRenderScene(true);
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyClarifyHighlightState(scene.facePath.get(), &faceDetail);
        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get(), Qt::white, false);

        QVERIFY(!result.image.isNull());
        // The target face and its boundary each schedule one delayed overlay.
        // Unrelated geometry in a nested selection-root branch must schedule none.
        QCOMPARE(result.delayedPathCount, 1);

        const RenderResult clarified = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!clarified.image.isNull());
        QCOMPARE(clarified.delayedPathCount, 1);

        const QColor unrelatedFace
            = meanColor(clarified.image, QPoint(renderWidth * 89 / 100, renderHeight / 2), 1);
        QVERIFY2(
            !(unrelatedFace.red() > 200 && unrelatedFace.green() < 80 && unrelatedFace.blue() < 80),
            colorMessage("unrelated face", unrelatedFace).constData()
        );

        const QPoint unrelatedBoundary(renderWidth * 89 / 100, renderHeight * 34 / 100);
        const int accentPixels
            = countPixelsMatching(clarified.image, unrelatedBoundary, 6, [](const QColor& color) {
                  return color.red() > 140 && color.green() < 90 && color.blue() < 90;
              });
        QCOMPARE(accentPixels, 0);
    }

    void clarifyHighlightUsesViewProviderRootedFacePath()
    {
        auto scene = makePartialRenderScene(true);
        QVERIFY(scene.root);
        QVERIFY(scene.selectionRootPath);

        auto facePath = makeProviderRootedPath(scene.selectionRootPath.get(), 3);
        QVERIFY(facePath);
        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyClarifyHighlightState(facePath.get(), &faceDetail);

        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!result.image.isNull());
        QCOMPARE(result.delayedPathCount, 1);

        const QColor highlightedFace
            = meanColor(result.image, QPoint(renderWidth * 3 / 10, renderHeight / 2), 2);
        QVERIFY2(
            highlightedFace.red() > 200 && highlightedFace.green() < 80 && highlightedFace.blue() < 80,
            colorMessage("view-provider-rooted highlighted face", highlightedFace).constData()
        );

        const QColor unrelatedFace
            = meanColor(result.image, QPoint(renderWidth * 89 / 100, renderHeight / 2), 1);
        QVERIFY2(
            !(unrelatedFace.red() > 200 && unrelatedFace.green() < 80 && unrelatedFace.blue() < 80),
            colorMessage("unrelated view-provider-rooted face", unrelatedFace).constData()
        );
    }

    void clarifyHighlightUsesViewProviderRootedEdgePath()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.selectionRootPath);

        auto edgePath = makeProviderRootedPath(scene.selectionRootPath.get(), 5);
        QVERIFY(edgePath);
        SoLineDetail edgeDetail = makeTopEdgeDetail();
        applyClarifyHighlightState(edgePath.get(), &edgeDetail);

        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!result.image.isNull());
        QCOMPARE(result.delayedPathCount, 1);

        const QPoint boundaryCenter(renderWidth * 3 / 10, renderHeight * 31 / 100);
        const int accentPixels
            = countPixelsMatching(result.image, boundaryCenter, 6, [](const QColor& color) {
                  return color.red() > 140 && color.green() < 90 && color.blue() < 90;
              });
        QVERIFY(accentPixels > 0);
    }

    void clarifyHighlightUsesViewProviderRootedVertexPath()
    {
        auto scene = makePointRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.providerRootPath);

        auto pointPath = makeProviderRootedPath(scene.providerRootPath.get(), 2);
        QVERIFY(pointPath);
        SoPointDetail pointDetail;
        pointDetail.setCoordinateIndex(0);
        applyClarifyHighlightState(pointPath.get(), &pointDetail);

        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!result.image.isNull());
        QCOMPARE(result.delayedPathCount, 1);

        const QColor highlightedVertex
            = meanColor(result.image, QPoint(renderWidth / 2, renderHeight / 2), 2);
        QVERIFY2(
            highlightedVertex.red() > 200 && highlightedVertex.green() < 130
                && highlightedVertex.blue() < 130,
            colorMessage("view-provider-rooted highlighted vertex", highlightedVertex).constData()
        );
    }

    void edgeClarifyHighlightDrawsItsOwnBoundaryOnTop()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.edgePath);

        SoLineDetail edgeDetail = makeTopEdgeDetail();
        applyClarifyHighlightState(scene.edgePath.get(), &edgeDetail);
        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());

        QVERIFY(!result.image.isNull());
        QCOMPARE(result.delayedPathCount, 1);

        const QPoint boundaryCenter(renderWidth * 3 / 10, renderHeight * 31 / 100);
        const int accentPixels
            = countPixelsMatching(result.image, boundaryCenter, 6, [](const QColor& color) {
                  return color.red() > 140 && color.green() < 90 && color.blue() < 90;
              });
        const int haloPixels
            = countPixelsMatching(result.image, boundaryCenter, 6, [](const QColor& color) {
                  return color.red() < 90 && color.green() < 90 && color.blue() < 90;
              });
        QVERIFY(accentPixels > 0);
        QVERIFY(haloPixels > 0);
    }

    void clarifyHighlightKeepsExistingSelectionVisible()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);

        SoFaceDetail highlightedFaceDetail = makeFirstFaceDetail();
        SoFaceDetail selectedFaceDetail = makeSecondFaceDetail();
        applySelectionState(scene.facePath.get(), &selectedFaceDetail);
        applyClarifyHighlightState(scene.facePath.get(), &highlightedFaceDetail);

        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());

        QVERIFY(!result.image.isNull());
        QCOMPARE(result.image.size(), QSize(renderWidth, renderHeight));
        QCOMPARE(result.delayedPathCount, 1);

        const QColor highlightedFace
            = meanColor(result.image, QPoint(renderWidth * 3 / 10, renderHeight / 2), 2);
        const QColor selectedFace
            = meanColor(result.image, QPoint(renderWidth * 7 / 10, renderHeight / 2), 2);

        const QByteArray highlightedMessage = colorMessage("highlighted face", highlightedFace);
        QVERIFY2(
            highlightedFace.red() > 200 && highlightedFace.green() < 80 && highlightedFace.blue() < 80,
            highlightedMessage.constData()
        );

        const QByteArray selectedMessage = colorMessage("selected face", selectedFace);
        QVERIFY2(
            selectedFace.red() < 80 && selectedFace.green() > 160 && selectedFace.blue() < 80,
            selectedMessage.constData()
        );
    }

    void clarifyHighlightDrawsFaceBoundaryHaloAndAccentOnTop()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyClarifyHighlightState(scene.facePath.get(), &faceDetail);

        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());

        QVERIFY(!result.image.isNull());
        QCOMPARE(result.image.size(), QSize(renderWidth, renderHeight));
        QCOMPARE(result.delayedPathCount, 1);

        const QPoint boundaryCenter(renderWidth * 3 / 10, renderHeight * 31 / 100);
        const int accentPixels
            = countPixelsMatching(result.image, boundaryCenter, 6, [](const QColor& color) {
                  return color.red() > 140 && color.green() < 90 && color.blue() < 90;
              });
        const int haloPixels
            = countPixelsMatching(result.image, boundaryCenter, 6, [](const QColor& color) {
                  return color.red() < 90 && color.green() < 90 && color.blue() < 90;
              });

        const QByteArray accentMessage = "highlighted boundary accent pixels: "
            + QByteArray::number(accentPixels);
        QVERIFY2(accentPixels > 0, accentMessage.constData());

        const QByteArray haloMessage = "highlighted boundary halo pixels: "
            + QByteArray::number(haloPixels);
        QVERIFY2(haloPixels > 0, haloMessage.constData());
    }

    void wholeObjectClarifyHighlightUsesRestrainedAccentOnTop()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.selectionRootPath);

        applyClarifyHighlightState(scene.selectionRootPath.get(), nullptr);

        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());

        QVERIFY(!result.image.isNull());
        QCOMPARE(result.image.size(), QSize(renderWidth, renderHeight));
        QCOMPARE(result.delayedPathCount, 1);

        const QPoint boundaryCenter(renderWidth * 3 / 10, renderHeight * 31 / 100);
        const int accentPixels
            = countPixelsMatching(result.image, boundaryCenter, 6, [](const QColor& color) {
                  return color.red() > 140 && color.green() < 90 && color.blue() < 90;
              });
        const QByteArray accentMessage = "whole-object edge accent pixels: "
            + QByteArray::number(accentPixels);
        QVERIFY2(accentPixels > 0, accentMessage.constData());
    }

    void fullClearRestoresPartialRenderPreview()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyPartialRenderState(scene.facePath.get(), &faceDetail);

        RenderResult partial = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!partial.image.isNull());

        QColor visibleFace
            = meanColor(partial.image, QPoint(renderWidth * 3 / 10, renderHeight / 2), 2);
        QColor maskedFace = meanColor(partial.image, QPoint(renderWidth * 7 / 10, renderHeight / 2), 2);

        QByteArray visibleMessage = colorMessage("visible face", visibleFace);
        QVERIFY2(
            visibleFace.red() < 80 && visibleFace.green() < 120 && visibleFace.blue() > 180,
            visibleMessage.constData()
        );
        QByteArray maskedMessage = colorMessage("masked face", maskedFace);
        QVERIFY2(
            maskedFace.red() > 245 && maskedFace.green() > 245 && maskedFace.blue() > 245,
            maskedMessage.constData()
        );

        clearSecondaryRenderState(scene.root.get());

        RenderResult restored = renderWithDelayedClarifyPass(scene.root.get());
        QVERIFY(!restored.image.isNull());

        QColor restoredLeft
            = meanColor(restored.image, QPoint(renderWidth * 3 / 10, renderHeight / 2), 2);
        QColor restoredRight
            = meanColor(restored.image, QPoint(renderWidth * 7 / 10, renderHeight / 2), 2);

        QByteArray restoredLeftMessage = colorMessage("restored left face", restoredLeft);
        QVERIFY2(
            restoredLeft.red() < 80 && restoredLeft.green() < 120 && restoredLeft.blue() > 180,
            restoredLeftMessage.constData()
        );
        QByteArray restoredRightMessage = colorMessage("restored right face", restoredRight);
        QVERIFY2(
            restoredRight.red() < 80 && restoredRight.green() < 120 && restoredRight.blue() > 180,
            restoredRightMessage.constData()
        );
    }

private:
    std::unique_ptr<Gui::Application> _guiApplication;
};

QTEST_MAIN(testSelectionRendering)

#include "SelectionRendering.moc"
