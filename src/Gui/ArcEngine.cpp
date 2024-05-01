/***************************************************************************
 *   Copyright (c) 2013 Thomas Anderson <blobfish[at]gmx.com>              *
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

#include "ArcEngine.h"
#include <limits>
#include <vector>

# include <Inventor/engines/SoCalculator.h>
# include <Inventor/engines/SoComposeVec3f.h>
# include <Inventor/engines/SoConcatenate.h>
# include <Inventor/engines/SoComposeRotation.h>
# include <Inventor/engines/SoComposeRotationFromTo.h>

# include <Inventor/nodekits/SoShapeKit.h>
# include <Inventor/nodes/SoCone.h>
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMatrixTransform.h>
# include <Inventor/nodes/SoResetTransform.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoVertexProperty.h>

using namespace Gui;



SO_ENGINE_SOURCE(ArcEngine)

ArcEngine::ArcEngine()
{
  SO_ENGINE_CONSTRUCTOR(ArcEngine);

  SO_ENGINE_ADD_INPUT(radius, (10.0));
  SO_ENGINE_ADD_INPUT(angle, (1.0));
  SO_ENGINE_ADD_INPUT(deviation, (0.25));

  SO_ENGINE_ADD_OUTPUT(points, SoMFVec3f);
  SO_ENGINE_ADD_OUTPUT(pointCount, SoSFInt32);
  SO_ENGINE_ADD_OUTPUT(midpoint, SoSFVec3f);
}

void ArcEngine::initClass()
{
  SO_ENGINE_INIT_CLASS(ArcEngine, SoEngine, "Engine");
}

void ArcEngine::evaluate()
{
  float angle = abs(this->angle.getValue());

  if (radius.getValue() < std::numeric_limits<float>::epsilon() ||
    deviation.getValue() < std::numeric_limits<float>::epsilon())
  {
    defaultValues();
    return;
  }

  float deviationAngle(acos((radius.getValue() - deviation.getValue()) / radius.getValue()));
  std::vector<SbVec3f> tempPoints;
  int segmentCount;
  if (deviationAngle >= angle) {
    segmentCount = 1;
  }
  else {
    segmentCount = static_cast<int>(angle / deviationAngle) + 1;
    if (segmentCount < 2) {
      defaultValues();
      return;
    }
  }
  float angleIncrement = (this->angle.getValue() > 0 ? angle : -angle) / static_cast<float>(segmentCount);
  for (int index = 0; index < segmentCount + 1; ++index)
  {
    SbVec3f currentNormal(1.0, 0.0, 0.0);
    float currentAngle = index * angleIncrement;
    SbRotation rotation(SbVec3f(0.0, 0.0, 1.0), currentAngle);
    rotation.multVec(currentNormal, currentNormal);
    tempPoints.push_back(currentNormal * radius.getValue());
  }
  int tempCount = tempPoints.size(); //for macro.
  SO_ENGINE_OUTPUT(points, SoMFVec3f, setNum(tempCount));
  SO_ENGINE_OUTPUT(pointCount, SoSFInt32, setValue(tempCount));
  std::vector<SbVec3f>::const_iterator it;
  for (it = tempPoints.begin(); it != tempPoints.end(); ++it)
  {
    int currentIndex = it-tempPoints.begin(); //for macro.
    SbVec3f temp(*it); //for macro
    SO_ENGINE_OUTPUT(points, SoMFVec3f, set1Value(currentIndex, temp));
  }

  // Get Midpoint
  float a = angle / 2;
  SbRotation rot(SbVec3f(0.0, 0.0, 1.0), a);
  SbVec3f midPnt(1.0, 0.0, 0.0);
  rot.multVec(midPnt, midPnt);
  midPnt = midPnt * radius.getValue();

  SO_ENGINE_OUTPUT(midpoint, SoSFVec3f, setValue(midPnt));

}

void ArcEngine::defaultValues()
{
  //just some non-failing info.
  SO_ENGINE_OUTPUT(points, SoMFVec3f, setNum(2));
  SbVec3f point1(10.0, 0.0, 0.0);
  SO_ENGINE_OUTPUT(points, SoMFVec3f, set1Value(0, point1));
  SbVec3f point2(7.07f, 7.07f, 0.0);
  SO_ENGINE_OUTPUT(points, SoMFVec3f, set1Value(1, point2));
  SO_ENGINE_OUTPUT(pointCount, SoSFInt32, setValue(2));
  SbVec3f point3(7.07f, 7.07f, 0.0);
  SO_ENGINE_OUTPUT(midpoint, SoSFVec3f, setValue(point3));
}
