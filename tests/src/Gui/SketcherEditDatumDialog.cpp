// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QDialog>
#include <QTest>

#include <vector>

#include <App/Application.h>
#include <App/Document.h>
#include <App/TransactionDefs.h>
#include <Gui/Application.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/Gui/CommandSketcherTools.h>
#include <Mod/Sketcher/Gui/EditDatumDialog.h>
#include <Mod/Sketcher/Gui/Utils.h>
#include <Mod/Sketcher/Gui/ViewProviderSketch.h>

#include <src/App/InitApplication.h>

// The conflict path does not call these helpers. Stub them so
// EditDatumDialog.cpp can be linked without SketcherGui.
Sketcher::SketchObject* SketcherGui::ViewProviderSketch::getSketchObject() const
{
    return nullptr;
}

bool SketcherGui::tryAutoRecompute(Sketcher::SketchObject*)
{
    return false;
}

void SketcherGui::centerScale(double)
{}

class SketcherEditDatumDialogTest: public QObject
{
    Q_OBJECT

public:
    SketcherEditDatumDialogTest()
    {
        tests::initApplication();
        if (!Gui::Application::Instance) {
            new Gui::Application(false);
        }
    }

private Q_SLOTS:
    void init()
    {
        docName = App::GetApplication().getUniqueDocumentName("sketcher_edit_datum");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        sketch = static_cast<Sketcher::SketchObject*>(
            doc->addObject("Sketcher::SketchObject", "Sketch")
        );
        std::vector<Part::Geometry*> geometry;
        auto addLine = [&geometry](const Base::Vector3d& start, const Base::Vector3d& end) {
            auto* line = new Part::GeomLineSegment();
            line->setPoints(start, end);
            geometry.push_back(line);
        };
        addLine(Base::Vector3d(-5, 5, 0), Base::Vector3d(-5, -5, 0));
        addLine(Base::Vector3d(-5, -5, 0), Base::Vector3d(5, -5, 0));
        addLine(Base::Vector3d(5, -5, 0), Base::Vector3d(5, 5, 0));
        addLine(Base::Vector3d(5, 5, 0), Base::Vector3d(-5, 5, 0));

        std::vector<Sketcher::Constraint*> constraints;
        auto addConstraint = [&constraints](
                                 Sketcher::ConstraintType type,
                                 int first,
                                 Sketcher::PointPos firstPos = Sketcher::PointPos::none,
                                 int second = Sketcher::GeoEnum::GeoUndef,
                                 Sketcher::PointPos secondPos = Sketcher::PointPos::none,
                                 int third = Sketcher::GeoEnum::GeoUndef
                             ) {
            auto* constraint = new Sketcher::Constraint();
            constraint->Type = type;
            constraint->First = first;
            constraint->FirstPos = firstPos;
            constraint->Second = second;
            constraint->SecondPos = secondPos;
            constraint->Third = third;
            constraints.push_back(constraint);
        };

        addConstraint(Sketcher::Coincident, 0, Sketcher::PointPos::end, 1, Sketcher::PointPos::start);
        addConstraint(Sketcher::Coincident, 1, Sketcher::PointPos::end, 2, Sketcher::PointPos::start);
        addConstraint(Sketcher::Coincident, 2, Sketcher::PointPos::end, 3, Sketcher::PointPos::start);
        addConstraint(Sketcher::Coincident, 3, Sketcher::PointPos::end, 0, Sketcher::PointPos::start);
        addConstraint(Sketcher::Vertical, 0);
        addConstraint(Sketcher::Horizontal, 1);
        addConstraint(Sketcher::Equal, 3, Sketcher::PointPos::none, 2);
        addConstraint(
            Sketcher::Symmetric,
            2,
            Sketcher::PointPos::end,
            1,
            Sketcher::PointPos::end,
            Sketcher::GeoEnum::HAxis
        );
        addConstraint(
            Sketcher::Symmetric,
            2,
            Sketcher::PointPos::end,
            0,
            Sketcher::PointPos::start,
            Sketcher::GeoEnum::VAxis
        );

        auto* distance = new Sketcher::Constraint();
        distance->Type = Sketcher::DistanceX;
        distance->First = 3;
        distance->FirstPos = Sketcher::PointPos::end;
        distance->Second = 3;
        distance->SecondPos = Sketcher::PointPos::start;
        distance->setValue(10.0);
        constraints.push_back(distance);
        editedConstraint = 9;

        sketch->Geometry.setValues(std::move(geometry));
        sketch->Constraints.setValues(std::move(constraints));
        doc->recompute();
    }

    void cleanup()
    {
        if (doc) {
            if (doc->getBookedTransactionID() != App::NullTransaction) {
                doc->abortTransaction();
            }
            App::GetApplication().closeDocument(docName.c_str());
        }
        doc = nullptr;
        sketch = nullptr;
    }

    void conflictRejectsAndClosesItsTransaction()
    {
        QCOMPARE(sketch->hasConflicts(), 0);

        const int existingTransactionID = doc->openTransaction("Edit Sketch. Constraints");
        QVERIFY(existingTransactionID != App::NullTransaction);
        QCOMPARE(sketch->setDatum(editedConstraint, 0.0), 0);
        doc->recompute();
        QVERIFY(sketch->solve() != 0);
        doc->commitTransaction();
        QVERIFY(sketch->hasConflicts() != 0);
        const int undoCountBefore = doc->getAvailableUndos();
        QVERIFY(undoCountBefore > 0);

        const int transactionID = doc->openTransaction("Modify sketch constraints");
        QVERIFY(transactionID != App::NullTransaction);

        {
            SketcherGui::EditDatumDialog dialog(transactionID, sketch, editedConstraint);
            QCOMPARE(dialog.exec(false), QDialog::Rejected);
            QVERIFY(!dialog.isSuccess());
        }

        QCOMPARE(doc->getBookedTransactionID(), App::NullTransaction);
        QCOMPARE(doc->getAvailableUndos(), undoCountBefore);

        QVERIFY(doc->undo());
        QCOMPARE(sketch->getDatum(editedConstraint), 10.0);
        QCOMPARE(sketch->solve(), 0);
        QCOMPARE(sketch->hasConflicts(), 0);
    }

private:
    std::string docName;
    App::Document* doc {nullptr};
    Sketcher::SketchObject* sketch {nullptr};
    int editedConstraint {-1};
};

QTEST_MAIN(SketcherEditDatumDialogTest)
#include "SketcherEditDatumDialog.moc"
