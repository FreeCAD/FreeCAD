// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <QGraphicsItemGroup>
#include <QPen>

#include <Mod/TechDraw/TechDrawGlobal.h>

#include "QGIUserTypes.h"

class QGraphicsPathItem;

namespace App
{
class DocumentObject;
}

namespace TechDraw
{
class DrawView;
}

namespace TechDrawGui
{

class TechDrawGuiExport QGISketch: public QGraphicsItemGroup
{
public:
    explicit QGISketch(App::DocumentObject* sketch, TechDraw::DrawView* owner = nullptr);
    ~QGISketch() override = default;

    enum {Type = UserType::QGISketch};
    int type() const override { return Type; }

    App::DocumentObject* getSketchObject() const;
    TechDraw::DrawView* getOwnerView() const;
    void setOwnerView(TechDraw::DrawView* owner);
    void updateView();
    void setGroupSelection(bool selected);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    void updatePens();

    App::DocumentObject* m_sketch;
    TechDraw::DrawView* m_owner;
    QGraphicsPathItem* m_geometry;
    QPen m_normalPen;
    QPen m_selectedPen;
};

}  // namespace TechDrawGui
