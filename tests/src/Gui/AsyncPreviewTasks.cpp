// SPDX-License-Identifier: LGPL-2.1-or-later

#include <chrono>
#include <functional>
#include <string>
#include <thread>

#include <QApplication>
#include <QCheckBox>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Interpreter.h>
#include <Gui/AsyncTaskRecompute.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Macro.h>
#include <Gui/QuantitySpinBox.h>
#include <Mod/Part/App/FeaturePartBox.h>
#include <Mod/Part/App/PartFeatures.h>
#include <Mod/Part/Gui/TaskThickness.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePolarPattern.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <src/App/InitApplication.h>

using namespace std::chrono_literals;

namespace
{

class AsyncPreviewTasksTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
        if (!Gui::Application::Instance) {
            Gui::Application::initApplication();
            Gui::Application::initOpenInventor();
            // ThicknessWidget records the Python commands it runs through the
            // GUI macro manager, so use the real GUI-enabled application even
            // though the test is running headlessly under Xvfb.
            new Gui::Application(true);
        }
        if (!Gui::getMainWindow()) {
            new Gui::MainWindow;
        }
        Base::Interpreter().runString("import Part");
        Base::Interpreter().loadModule("_PartDesign");
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
            ->SetBool("EnableAsyncRecompute", true);
    }

    void thicknessPreviewIsReusedByAccept()
    {
        _docName = App::GetApplication().getUniqueDocumentName("async_preview_tasks");
        // No 3D view is needed for this widget-level test.
        _doc = App::GetApplication().newDocument(
            _docName.c_str(),
            "testUser",
            App::DocumentInitFlags {.createView = false}
        );

        _box = _doc->addObject<Part::Box>("Box");
        _box->Length.setValue(1.0);
        _box->Width.setValue(2.0);
        _box->Height.setValue(3.0);

        _thickness = _doc->addObject<Part::Thickness>("Thickness");
        _thickness->Faces.setValue(_box, {"Face1"});
        _thickness->Value.setValue(0.25);
        _thickness->Join.setValue("Intersection");
        QVERIFY2(_doc->recompute(), "The fixture model must recompute before opening the task");
        int recomputeCount = 0;
        fastsignals::scoped_connection recomputed = _doc->signalRecomputedObject.connect(
            [&](const App::DocumentObject& object) {
                if (&object == _thickness) {
                    ++recomputeCount;
                }
            }
        );

        Gui::MacroManager::MacroRedirector macroRedirector([](Gui::MacroManager::LineType,
                                                              const char*) {});
        PartGui::ThicknessWidget widget(_thickness);
        widget.show();

        auto* updateView = widget.findChild<QCheckBox*>(QStringLiteral("updateView"));
        auto* offset = widget.findChild<Gui::QuantitySpinBox*>(QStringLiteral("spinOffset"));
        QVERIFY(updateView);
        QVERIFY(offset);

        updateView->setChecked(true);
        offset->setValue(0.30);

        QVERIFY2(
            waitFor([&] {
                return recomputeCount == 1 && _thickness->isValid() && !_thickness->mustRecompute();
            }),
            "The Thickness task did not settle one successful async preview"
        );
        QCOMPARE(recomputeCount, 1);
        QVERIFY(offset->isEnabled());
        QVERIFY(_thickness->isValid());
        QVERIFY(!_thickness->mustRecompute());

        QCOMPARE(recomputeCount, 1);
        QVERIFY(widget.accept());

        // Accepting a successful preview settles dependents, but must not execute Thickness again.
        QTest::qWait(100);
        QCOMPARE(recomputeCount, 1);
    }

    void cleanup()
    {
        if (_doc && App::GetApplication().getDocument(_docName.c_str())) {
            App::GetApplication().closeDocument(_docName.c_str());
        }
        _doc = nullptr;
        _box = nullptr;
        _thickness = nullptr;

        if (_patternDoc && App::GetApplication().getDocument(_patternDocName.c_str())) {
            App::GetApplication().closeDocument(_patternDocName.c_str());
        }
        _patternDoc = nullptr;
        _polarPattern = nullptr;
    }

    void polarPatternPreviewRecomputesAsynchronously()
    {
        _patternDocName = App::GetApplication().getUniqueDocumentName("async_polar_pattern");
        _patternDoc = App::GetApplication().newDocument(
            _patternDocName.c_str(),
            "testUser",
            App::DocumentInitFlags {.createView = false}
        );

        auto* body = _patternDoc->addObject<PartDesign::Body>("Body");
        auto* box = _patternDoc->addObject<PartDesign::Box>("Box");
        body->addObject(box);
        box->Length.setValue(2.0);
        box->Width.setValue(2.0);
        box->Height.setValue(2.0);
        QVERIFY2(_patternDoc->recompute(), "The Pattern fixture support must recompute");

        _polarPattern = _patternDoc->addObject<PartDesign::PolarPattern>("PolarPattern");
        body->addObject(_polarPattern);
        _polarPattern->Originals.setValues({box});
        _polarPattern->Axis.setValue(_patternDoc->getObject("Z_Axis"), {});
        _polarPattern->Angle.setValue(360.0);
        _polarPattern->Occurrences.setValue(24);
        QVERIFY2(_patternDoc->recompute(), "The Pattern fixture must recompute");

        int recomputeCount = 0;
        fastsignals::scoped_connection recomputed = _patternDoc->signalRecomputedObject.connect(
            [&](const App::DocumentObject& object) {
                if (&object == _polarPattern) {
                    ++recomputeCount;
                }
            }
        );

        _polarPattern->Angle.setValue(180.0);
        bool callbackCalled = false;
        App::RecomputeFailure failure = App::RecomputeFailure::Exception;
        bool success = false;
        Gui::AsyncTaskRecompute asyncRecompute;
        auto request = App::RecomputeRequest::fromDocumentObject(*_polarPattern);
        asyncRecompute.schedule(std::move(request), 0, [&](App::RecomputeResult& result) {
            callbackCalled = true;
            success = result.success;
            failure = result.failure;
            if (success && failure == App::RecomputeFailure::None) {
                // This is the same main-thread settle performed by the
                // Pattern task after a successful worker preview.
                _polarPattern->purgeTouched();
            }
        });
        QVERIFY2(
            waitFor([&] { return callbackCalled; }),
            "The Polar Pattern async preview did not deliver its completion"
        );
        QVERIFY(success);
        QCOMPARE(failure, App::RecomputeFailure::None);
        QCOMPARE(recomputeCount, 1);
        QVERIFY(_polarPattern->isValid());
        QVERIFY(!_polarPattern->mustRecompute());
    }

private:
    static bool waitFor(const std::function<bool()>& predicate)
    {
        const auto deadline = std::chrono::steady_clock::now() + 5s;
        while (!predicate() && std::chrono::steady_clock::now() < deadline) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
            std::this_thread::sleep_for(1ms);
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        return predicate();
    }

    std::string _docName;
    App::Document* _doc = nullptr;
    Part::Box* _box = nullptr;
    Part::Thickness* _thickness = nullptr;
    std::string _patternDocName;
    App::Document* _patternDoc = nullptr;
    PartDesign::PolarPattern* _polarPattern = nullptr;
};

}  // namespace

QTEST_MAIN(AsyncPreviewTasksTest)
#include "AsyncPreviewTasks.moc"
