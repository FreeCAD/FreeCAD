/*
 * TopoShapeViewProvider.h
 *
 *  Created on: 15.07.2025
 *      Author: jffmichi
 */

#ifndef PATHSIMULATOR_TOPOSHAPEVIEWPROVIDER_H_
#define PATHSIMULATOR_TOPOSHAPEVIEWPROVIDER_H_

#include <Gui/ViewProvider.h>

namespace Part
{
class TopoShape;
}

namespace CAMSimulator
{

class TopoShapeViewProvider: public Gui::ViewProvider
{
public:
    TopoShapeViewProvider();
    TopoShapeViewProvider& operator=(TopoShapeViewProvider&& vp);

    void clear();
    void setShape(const Part::TopoShape& shape);

    void setShapeVisible(bool b);

private:
    SoSwitch* pcSwitch = nullptr;
    SoNode* pcShape = nullptr;
};

}  // namespace CAMSimulator

#endif
