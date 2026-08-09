// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

#include <QTest>

#include <Base/Vector3D.h>
#include <Gui/Dialogs/Placement.h>


class TestPlacement final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void centerIsRelativeToTranslatedObject()
    {
        const Base::Vector3d globalCenter(5.0, 35.0, 5.0);
        const Base::Vector3d objectPosition(0.0, 30.0, 0.0);

        const auto center = Gui::Dialog::PlacementHandler::relativeCenter(globalCenter, objectPosition);

        QVERIFY(center == Base::Vector3d(5.0, 5.0, 5.0));
    }

    void untranslatedObjectKeepsCenter()
    {
        const Base::Vector3d globalCenter(5.0, 5.0, 5.0);

        const auto center
            = Gui::Dialog::PlacementHandler::relativeCenter(globalCenter, Base::Vector3d());

        QVERIFY(center == globalCenter);
    }
};


QTEST_MAIN(TestPlacement)

#include "Placement.moc"
