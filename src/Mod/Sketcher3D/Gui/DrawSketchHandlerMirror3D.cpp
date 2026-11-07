// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include "PreCompiled.h"

#include <QComboBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

#include <Base/Console.h>
#include <Gui/Command.h>

#include <Mod/Sketcher3D/App/GeomReferencePlane3D.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DrawSketchHandlerMirror3D.h"
#include "Sketcher3DToolWidget.h"
#include "ViewProviderSketch3D.h"

using namespace Sketcher3DGui;

namespace
{

class MirrorPlaneWidget: public Sketcher3DToolWidget
{
public:
    explicit MirrorPlaneWidget(Sketcher3D::Sketch3DObject* sketch, QWidget* parent = nullptr)
        : Sketcher3DToolWidget(parent)
        , combo(new QComboBox(this))
    {
        if (sketch) {
            const auto& geos = sketch->Geometry.getValues();
            for (int i = 0; i < static_cast<int>(geos.size()); i++) {
                if (geos[i] && geos[i]->is<Sketcher3D::GeomReferencePlane3D>()) {
                    combo->addItem(tr("Plane%1").arg(i + 1), i);
                }
            }
        }

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        auto* form = new QFormLayout();
        form->addRow(tr("Mirror plane:"), combo);
        root->addLayout(form);

        auto* apply = new QPushButton(tr("Apply"), this);
        root->addWidget(apply);
        connect(apply, &QPushButton::clicked, this, &Sketcher3DToolWidget::accept);
    }

    int planeGeoId() const
    {
        return combo->currentData().isValid() ? combo->currentData().toInt()
                                              : Sketcher3D::GeoEnum3D::GeoUndef;
    }

private:
    QComboBox* combo;
};

}  // namespace

DrawSketchHandlerMirror3D::DrawSketchHandlerMirror3D(std::vector<int> ids)
    : geoIds(std::move(ids))
{}

void DrawSketchHandlerMirror3D::onActivated()
{
    auto widget = std::make_unique<MirrorPlaneWidget>(getSketch());
    widget->setAcceptCallback([this]() {
        commitMirror(static_cast<MirrorPlaneWidget*>(toolWidget())->planeGeoId());
    });
    setToolWidget(std::move(widget));
}

bool DrawSketchHandlerMirror3D::pressButton(const Base::Vector3d&)
{
    const auto& pre = getPreselection();
    if (pre.isValid() && pre.Kind == Sketcher3D::GeoKind::Plane) {
        commitMirror(pre.GeoId);
    }
    return true;
}

void DrawSketchHandlerMirror3D::commitMirror(int planeGeoId)
{
    auto* sketch = getSketch();
    auto* vp = getSketchVP();
    if (!sketch || !vp) {
        return;
    }
    if (planeGeoId == Sketcher3D::GeoEnum3D::GeoUndef) {
        Base::Console().warning("Sketcher3D: select a reference plane.\n");
        return;
    }

    int tid = Gui::Command::openActiveDocumentCommand(
        QT_TRANSLATE_NOOP("Command", "Mirror 3D geometry")
    );
    if (!sketch->addMirror(geoIds, planeGeoId)) {
        Gui::Command::abortCommand(tid);
        Base::Console().warning("Sketcher3D: mirror failed.\n");
        return;
    }
    sketch->recomputeFeature();
    Gui::Command::commitCommand(tid);
    vp->purgeHandler();
}
