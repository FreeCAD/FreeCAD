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
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/nodes/SoCoordinate3.h>
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
    CoinRefPtr<SoPath> selectionRootPath;
    PartGui::SoBrepEdgeSet* edges {nullptr};
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

RenderScene makePartialRenderScene()
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
    const std::array<SbVec3f, 8> points {
        SbVec3f(-0.9f, -0.45f, 0.0f),
        SbVec3f(-0.1f, -0.45f, 0.0f),
        SbVec3f(-0.1f, 0.45f, 0.0f),
        SbVec3f(-0.9f, 0.45f, 0.0f),
        SbVec3f(0.1f, -0.45f, 0.0f),
        SbVec3f(0.9f, -0.45f, 0.0f),
        SbVec3f(0.9f, 0.45f, 0.0f),
        SbVec3f(0.1f, 0.45f, 0.0f),
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
    const std::array<int32_t, 16> coordIndex {
        0,
        1,
        2,
        -1,
        0,
        2,
        3,
        -1,
        4,
        5,
        6,
        -1,
        4,
        6,
        7,
        -1,
    };
    const std::array<int32_t, 2> partIndex {2, 2};
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

    return {
        CoinRefPtr<SoSeparator>(root),
        CoinRefPtr<SoPath>(facePath),
        CoinRefPtr<SoPath>(selectionRootPath),
        edges
    };
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
    highlightAction.setHighlightPresentation(Gui::HighlightPresentation::DrawOnTop);
    highlightAction.apply(path);
}

void applyClarifyFaceBoundaryHighlightState(PartGui::SoBrepEdgeSet* edges, int faceIndex)
{
    if (!edges) {
        return;
    }
    edges->setFaceHighlight(
        faceIndex,
        SbColor(1.0f, 0.0f, 0.0f),
        SbColor(0.0f, 0.0f, 0.0f),
        Gui::HighlightPresentation::DrawOnTop,
        4.0F,
        6.0F
    );
}

void clearSecondaryRenderState(SoNode* root)
{
    Gui::SoSelectionElementAction clearRenderAction(Gui::SoSelectionElementAction::None, true);
    clearRenderAction.apply(root);
}

RenderResult renderWithDelayedClarifyPass(SoNode* root)
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
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderAction.apply(root);

    SoPathList delayedPaths = Gui::SoDelayedAnnotationsElement::getDelayedPaths(
        renderAction.getState()
    );
    result.delayedPathCount = delayedPaths.getLength();

    {
        DelayedRenderGuard guard;
        glClear(GL_DEPTH_BUFFER_BIT);
        if (result.delayedPathCount > 0) {
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
        QVERIFY(scene.edges);

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyPartialRenderState(scene.facePath.get(), &faceDetail);
        applyClarifyHighlightState(scene.facePath.get(), &faceDetail);
        applyClarifyFaceBoundaryHighlightState(scene.edges, 0);

        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());

        QVERIFY(!result.image.isNull());
        QCOMPARE(result.image.size(), QSize(renderWidth, renderHeight));
        QCOMPARE(result.delayedPathCount, 2);

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

    void clarifyHighlightKeepsUnhighlightedFacesVisible()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);
        QVERIFY(scene.edges);

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyClarifyHighlightState(scene.facePath.get(), &faceDetail);
        applyClarifyFaceBoundaryHighlightState(scene.edges, 0);

        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());

        QVERIFY(!result.image.isNull());
        QCOMPARE(result.image.size(), QSize(renderWidth, renderHeight));
        QCOMPARE(result.delayedPathCount, 2);

        const QColor highlightedFace
            = meanColor(result.image, QPoint(renderWidth * 3 / 10, renderHeight / 2), 2);
        const QColor neighboringFace
            = meanColor(result.image, QPoint(renderWidth * 7 / 10, renderHeight / 2), 2);

        const QByteArray highlightedMessage = colorMessage("highlighted face", highlightedFace);
        QVERIFY2(
            highlightedFace.red() > 200 && highlightedFace.green() < 80 && highlightedFace.blue() < 80,
            highlightedMessage.constData()
        );

        const QByteArray neighboringMessage = colorMessage("neighboring face", neighboringFace);
        QVERIFY2(
            neighboringFace.red() < 80 && neighboringFace.green() < 120
                && neighboringFace.blue() > 180,
            neighboringMessage.constData()
        );
    }

    void clarifyHighlightKeepsExistingSelectionVisible()
    {
        auto scene = makePartialRenderScene();
        QVERIFY(scene.root);
        QVERIFY(scene.facePath);
        QVERIFY(scene.edges);

        SoFaceDetail highlightedFaceDetail = makeFirstFaceDetail();
        SoFaceDetail selectedFaceDetail = makeSecondFaceDetail();
        applySelectionState(scene.facePath.get(), &selectedFaceDetail);
        applyClarifyHighlightState(scene.facePath.get(), &highlightedFaceDetail);
        applyClarifyFaceBoundaryHighlightState(scene.edges, 0);

        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());

        QVERIFY(!result.image.isNull());
        QCOMPARE(result.image.size(), QSize(renderWidth, renderHeight));
        QCOMPARE(result.delayedPathCount, 2);

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
        QVERIFY(scene.edges);

        SoFaceDetail faceDetail = makeFirstFaceDetail();
        applyClarifyHighlightState(scene.facePath.get(), &faceDetail);
        applyClarifyFaceBoundaryHighlightState(scene.edges, 0);

        const RenderResult result = renderWithDelayedClarifyPass(scene.root.get());

        QVERIFY(!result.image.isNull());
        QCOMPARE(result.image.size(), QSize(renderWidth, renderHeight));
        QCOMPARE(result.delayedPathCount, 2);

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

    void wholeObjectClarifyHighlightDrawsEdgeHaloAndAccentOnTop()
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
        const int haloPixels
            = countPixelsMatching(result.image, boundaryCenter, 6, [](const QColor& color) {
                  return color.red() < 90 && color.green() < 90 && color.blue() < 90;
              });

        const QByteArray accentMessage = "whole-object edge accent pixels: "
            + QByteArray::number(accentPixels);
        QVERIFY2(accentPixels > 0, accentMessage.constData());

        const QByteArray haloMessage = "whole-object edge halo pixels: "
            + QByteArray::number(haloPixels);
        QVERIFY2(haloPixels > 0, haloMessage.constData());
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
