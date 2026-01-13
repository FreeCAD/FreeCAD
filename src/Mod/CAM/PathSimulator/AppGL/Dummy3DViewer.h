/*
 * Dummy3DViewer.h
 *
 *  Created on: 15.07.2025
 *      Author: jffmichi
 */

#ifndef PATHSIMULATOR_DUMMY3DVIEWER_H_
#define PATHSIMULATOR_DUMMY3DVIEWER_H_

#include <Gui/View3DInventorViewer.h>

#include "TopoShapeViewProvider.h"

namespace CAMSimulator
{

class Dummy3DViewer: public Gui::View3DInventorViewer
{
public:
    Dummy3DViewer(QWidget* parent = nullptr);

    void cloneFrom(Dummy3DViewer& viewer);

    void setStockShape(const Part::TopoShape& shape);
    void setStockVisible(bool b);
    void setBaseShape(const Part::TopoShape& shape);
    void setBaseVisible(bool b);

protected:
    void paintEvent(QPaintEvent* event) override;

public:
    bool discardPaintEvent_ = true;

private:
    TopoShapeViewProvider stockViewProvider;
    TopoShapeViewProvider baseViewProvider;
};

} /* namespace CAMSimulator */

#endif /* SRC_MOD_CAM_PATHSIMULATOR_APPGL_DUMMY3DVIEWER_H_ */
