/*
 * Dummy3DViewer.cpp
 *
 *  Created on: 15.07.2025
 *      Author: jffmichi
 */

#include "PreCompiled.h"

#include "Dummy3DViewer.h"

using namespace Gui;

namespace CAMSimulator
{

Dummy3DViewer::Dummy3DViewer(QWidget* parent)
    : View3DInventorViewer(parent)
{
    addViewProvider(&stockViewProvider);
    addViewProvider(&baseViewProvider);
}

void Dummy3DViewer::cloneFrom(Dummy3DViewer& viewer)
{
    // move view providers from viewer to us

    stockViewProvider = std::move(viewer.stockViewProvider);
    baseViewProvider = std::move(viewer.baseViewProvider);
}

void Dummy3DViewer::setStockShape(const Part::TopoShape& shape)
{
    stockViewProvider.setShape(shape);
}

void Dummy3DViewer::setStockVisible(bool b)
{
    stockViewProvider.setShapeVisible(b);
}

void Dummy3DViewer::setBaseShape(const Part::TopoShape& shape)
{
    baseViewProvider.setShape(shape);
}

void Dummy3DViewer::setBaseVisible(bool b)
{
    baseViewProvider.setShapeVisible(b);
}

void Dummy3DViewer::paintEvent(QPaintEvent* event)
{
    if (discardPaintEvent_) {
        return;
    }

    View3DInventorViewer::paintEvent(event);
}

}  // namespace CAMSimulator
