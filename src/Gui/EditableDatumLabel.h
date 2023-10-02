 /// SPDX-License-Identifier: LGPL-2.1-or-later
 /****************************************************************************
  *                                                                          *
  *   Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
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

#ifndef GUI_EDITABLEDATUMLABEL_H
#define GUI_EDITABLEDATUMLABEL_H

#include <QObject>
#include <Gui/QuantitySpinBox.h>

#include "SoDatumLabel.h"

#include <FCGlobal.h>

class SoNodeSensor;
class SoTransform;

namespace Gui {

class View3DInventorViewer;


class GuiExport EditableDatumLabel : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(EditableDatumLabel)

public:
    EditableDatumLabel(View3DInventorViewer* view, const Base::Placement& plc, SbColor color, bool autoDistance = false);
    ~EditableDatumLabel() override;

    void activate();
    void deactivate();

    void startEdit(double val, QObject* eventFilteringObj = nullptr);
    void stopEdit();
    double getValue();
    void setSpinboxValue(double val);
    void setPlacement(const Base::Placement& plc);
    void setColor(SbColor color);
    void setFocus();
    void setPoints(SbVec3f p1, SbVec3f p2);
    void setPoints(Base::Vector3d p1, Base::Vector3d p2);
    void setFocusToSpinbox();
    void setLabelType(SoDatumLabel::Type type);
    void setLabelDistance(double distance);
    void setLabelRecommendedDistance();
    void setLabelAutoDistanceReverse(bool val);

    // NOLINTBEGIN
    SoDatumLabel* label;
    bool isSet;
    bool autoDistance;
    bool autoDistanceReverse;
    // NOLINTEND

Q_SIGNALS:
    void valueChanged(double val);

private:
    void positionSpinbox();
    SbVec3f getTextCenterPoint() const;

private:
    SoSeparator* root;
    SoTransform* transform;
    View3DInventorViewer* viewer;
    QuantitySpinBox* spinBox;
    SoNodeSensor* cameraSensor;
    SbVec3f midpos;
};

}


#endif // GUI_EDITABLEDATUMLABEL_H
