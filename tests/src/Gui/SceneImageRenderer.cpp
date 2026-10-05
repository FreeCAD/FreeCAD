// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QTest>

#include <Inventor/SoDB.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoSeparator.h>

#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>

class SceneImageRendererTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        SoDB::init();
    }

    void whiteGeometryKeepsItsAlpha_data()
    {
        QTest::addColumn<int>("samples");
        QTest::newRow("single sample") << 0;
        QTest::newRow("multisample") << 4;
    }

    void whiteGeometryKeepsItsAlpha()
    {
        QFETCH(int, samples);
        const Gui::CoinPtr<SoSeparator> scene(new SoSeparator);
        auto* lightModel = new SoLightModel;
        lightModel->model = SoLightModel::BASE_COLOR;
        scene->addChild(lightModel);
        auto* material = new SoMaterial;
        material->diffuseColor.setValue(1.0F, 1.0F, 1.0F);
        scene->addChild(material);
        scene->addChild(new SoCube);
        const Gui::CoinPtr<SoOrthographicCamera> camera(new SoOrthographicCamera);
        camera->position.setValue(0.0F, 0.0F, 5.0F);
        camera->height = 4.0F;
        camera->nearDistance = 1.0F;
        camera->farDistance = 10.0F;
        const int sceneReferences = scene->getRefCount();
        const int cameraReferences = camera->getRefCount();

        Gui::View3DInventorViewer::RenderImageOptions options;
        options.width = 64;
        options.height = 64;
        options.samples = samples;
        options.background = Qt::transparent;
        options.alphaMode = Gui::View3DInventorViewer::AlphaMode::PerPixel;
        options.camera = camera;
        options.scene = scene;
        const QImage image = Gui::View3DInventorViewer::renderSceneToImage(options);
        QVERIFY(!image.isNull());
        QCOMPARE(image.size(), QSize(64, 64));
        QCOMPARE(image.pixelColor(0, 0).alpha(), 0);
        QCOMPARE(image.pixelColor(32, 32), QColor(Qt::white));
        QCOMPARE(scene->getRefCount(), sceneReferences);
        QCOMPARE(camera->getRefCount(), cameraReferences);

        // A second capture must honor the new material and must not reuse stale
        // GL resources from the first offscreen context.
        material->diffuseColor.setValue(1.0F, 0.0F, 0.0F);
        const QImage red = Gui::View3DInventorViewer::renderSceneToImage(options);
        QVERIFY(!red.isNull());
        QCOMPARE(red.pixelColor(32, 32), QColor(Qt::red));
        QCOMPARE(red.pixelColor(0, 0).alpha(), 0);
    }

    void restoresTheCallingOpenGLContext()
    {
        QOpenGLContext context;
        QVERIFY(context.create());
        QOffscreenSurface surface;
        surface.setFormat(context.format());
        surface.create();
        QVERIFY(context.makeCurrent(&surface));
        const Gui::CoinPtr<SoSeparator> scene(new SoSeparator);
        const Gui::CoinPtr<SoOrthographicCamera> camera(new SoOrthographicCamera);
        Gui::View3DInventorViewer::RenderImageOptions options;
        options.width = 16;
        options.height = 16;
        options.samples = 0;
        options.scene = scene;
        options.camera = camera;
        QVERIFY(!Gui::View3DInventorViewer::renderSceneToImage(options).isNull());
        QCOMPARE(QOpenGLContext::currentContext(), &context);
        QCOMPARE(context.surface(), &surface);
        context.doneCurrent();
    }
};

QTEST_MAIN(SceneImageRendererTest)

#include "SceneImageRenderer.moc"
