/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef _PreComp_
# include <sstream>
# include <QCoreApplication>
# include <Inventor/fields/SoMFString.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoTransform.h>
#endif

#include "SoFCColorLegend.h"
#include "ViewProvider.h"


using namespace Gui;

SO_NODE_SOURCE(SoFCColorLegend)

/*!
  Constructor.
*/
SoFCColorLegend::SoFCColorLegend() : _bbox(4.0f, -4.0f, 4.5f, 4.0f)
{
    SO_NODE_CONSTRUCTOR(SoFCColorLegend);
    coords = new SoCoordinate3;
    coords->ref();
    labelGroup = new SoSeparator;
    labelGroup->ref();

    valueGroup = new SoSeparator;
    valueGroup->ref();

    setColorLegend(_currentLegend);
    setLegendLabels(_currentLegend, 3);
}

/*!
  Destructor.
*/
SoFCColorLegend::~SoFCColorLegend()
{
    //delete THIS;
    coords->unref();
    labelGroup->unref();
    valueGroup->unref();
}

// doc from parent
void SoFCColorLegend::initClass()
{
    SO_NODE_INIT_CLASS(SoFCColorLegend,SoFCColorBarBase,"Separator");
}

void SoFCColorLegend::finish()
{
    atexit_cleanup();
}

const char* SoFCColorLegend::getColorBarName() const
{
    return QT_TRANSLATE_NOOP("QObject", "Color Legend");
}

namespace {
std::vector<SbVec3f> getLabelPositions(int num, const SbBox2f& bbox)
{
    std::vector<SbVec3f> pos;
    float fMinY = bbox.getMin()[1];
    float fMaxX = bbox.getMax()[0];
    float fMaxY = bbox.getMax()[1] - 0.5f;

    if (num > 1) {
        float fStep = (fMaxY-fMinY) / static_cast<float>(num - 1);
        pos.emplace_back(fMaxX + 0.1f, fMaxY + 0.20f + fStep, 0.0f);
        for (int i=0; i<num; i++) {
            pos.emplace_back(0.0f, -fStep, 0.0f);
        }
    }

    return pos;
}

std::vector<SbVec3f> getValuePositions(int num, const SbBox2f& bbox)
{
    std::vector<SbVec3f> pos;
    float fMinY = bbox.getMin()[1];
    float fMaxX = bbox.getMax()[0];
    float fMaxY = bbox.getMax()[1] - 0.5f;

    if (num > 2) {
        float fStep = (fMaxY-fMinY) / static_cast<float>(num - 2);
        float eps = fStep / 4.0f;

        pos.emplace_back(fMaxX + 0.1f, fMaxY + 0.25f + 1.5f * fStep, 0.0f);
        for (int i=0; i<num; i++) {
            pos.emplace_back(0.0f, -fStep, 0.0f);
        }

        SbVec3f v;
        v = pos[1];
        v[1] -= eps;
        pos[1] = v;

        v = pos[2];
        v[1] += eps;
        pos[2] = v;

        v = pos.back();
        v[1] += eps;
        pos.back() = v;
    }

    return pos;
}
}

void SoFCColorLegend::setMarkerLabel(const SoMFString& label)
{
    coinRemoveAllChildren(labelGroup);

    int num = label.getNum();
    if (num > 1) {
        std::vector<SbVec3f> pos = getLabelPositions(num, _bbox);

        auto trans = new SoTransform;
        trans->translation.setValue(pos[0]);
        labelGroup->addChild(trans);

        for (int i=0; i<num; i++) {
            auto trans = new SoTransform;
            auto color = new SoBaseColor;
            auto text2 = new SoText2;

            trans->translation.setValue(pos[i+1]);
            color->rgb.setValue(0, 0, 0);
            text2->string.setValue(label[i]);
            labelGroup->addChild(trans);
            labelGroup->addChild(color);
            labelGroup->addChild(text2);
        }
    }
}

void SoFCColorLegend::setMarkerValue(const SoMFString& value)
{
    coinRemoveAllChildren(valueGroup);

    int num = value.getNum();
    if (num > 1) {
        std::vector<SbVec3f> pos = getValuePositions(num, _bbox);

        auto trans = new SoTransform;
        trans->translation.setValue(pos[0]);
        valueGroup->addChild(trans);

        for (int i=0; i<num; i++) {
            auto trans = new SoTransform;
            auto color = new SoBaseColor;
            auto text2 = new SoText2;

            trans->translation.setValue(pos[i+1]);
            color->rgb.setValue(0, 0, 0);
            text2->string.setValue(value[i]);
            valueGroup->addChild(trans);
            valueGroup->addChild(color);
            valueGroup->addChild(text2);
        }
    }
}

void SoFCColorLegend::setViewportSize(const SbVec2s& size)
{
    float fMinX, fMinY, fMaxX, fMaxY;
    float boxWidth = getBounds(size, fMinX, fMinY, fMaxX, fMaxY);

    // legend bar is shifted to the left by width of the labels to assure that labels are fully visible
    _bbox.setBounds(fMinX - boxWidth, fMinY, fMaxX - boxWidth, fMaxY);

    arrangeLabels(_bbox);
    arrangeValues(_bbox);
    modifyPoints(_bbox);
}

void SoFCColorLegend::setRange(float fMin, float fMax, int prec)
{
    std::size_t numFields = _currentLegend.hasNumberOfFields();
    for (std::size_t i = 0; i <= numFields; i++) {
        float factor = static_cast<float>(i) / numFields;
        float value = (1 - factor) * fMin + factor * fMax;
        _currentLegend.setValue(i, value);
    }

    setColorLegend(_currentLegend);
    setLegendLabels(_currentLegend, prec);
}

void SoFCColorLegend::setLegendLabels(const App::ColorLegend& legend, int prec)
{
    float fMin = legend.getMinValue();
    float fMax = legend.getMaxValue();

    std::size_t numFields = legend.hasNumberOfFields();

    SoMFString labels, values;

    float eps = std::pow(10.0f, static_cast<float>(-prec));
    float value = std::min<float>(fabs(fMin), fabs(fMax));
    std::ios::fmtflags flags = value < eps ? (std::ios::scientific | std::ios::showpoint | std::ios::showpos)
                                           : (std::ios::fixed | std::ios::showpoint | std::ios::showpos);

    for (std::size_t i=0; i < numFields; i++) {
        std::stringstream s;
        s << legend.getText(numFields - 1 - i);
        labels.set1Value(i, s.str().c_str());
    }

    for (std::size_t i=0; i <= numFields; i++) {
        std::stringstream s;
        s.precision(prec);
        s.setf(flags);
        float fValue = legend.getValue(numFields - i);
        s << fValue;
        values.set1Value(i, s.str().c_str());
    }

    setMarkerLabel(labels);
    setMarkerValue(values);

    setModified();
}

void SoFCColorLegend::modifyPoints(const SbBox2f& box)
{
    float fMinX = box.getMin()[0];
    float fMinY = box.getMin()[1];
    float fMaxX = box.getMax()[0];
    float fMaxY = box.getMax()[1] - 0.5f;

    // set the vertices spanning the faces for the color legend
    int intFields = coords->point.getNum() / 4;
    for (int i = 0; i < intFields; i++) {
        float w = static_cast<float>(i) / (intFields - 1);
        float fPosY1 = w * fMaxY + (1.0f - w) * fMinY;
        float fPosY2 = fPosY1 + 0.5f;
        coords->point.set1Value(4 * i,     fMinX, fPosY1, 0.0f);
        coords->point.set1Value(4 * i + 1, fMaxX, fPosY1, 0.0f);
        coords->point.set1Value(4 * i + 2, fMaxX, fPosY2, 0.0f);
        coords->point.set1Value(4 * i + 3, fMinX, fPosY2, 0.0f);
    }
}

void SoFCColorLegend::arrangeLabels(const SbBox2f& box)
{
    // search for the labels
    int num=0;
    for (int i=0; i<labelGroup->getNumChildren(); i++) {
        if (labelGroup->getChild(i)->getTypeId() == SoTransform::getClassTypeId())
            num++;
    }

    if (num > 2) {
        std::vector<SbVec3f> pos = getLabelPositions(num-1, box);

        int index = 0;
        for (int j=0; j<labelGroup->getNumChildren(); j++) {
            if (labelGroup->getChild(j)->getTypeId() == SoTransform::getClassTypeId()) {
                static_cast<SoTransform*>(labelGroup->getChild(j))->translation.setValue(pos[index++]);
            }
        }
    }
}

void SoFCColorLegend::arrangeValues(const SbBox2f& box)
{
    // search for the labels
    int num=0;
    for (int i=0; i<valueGroup->getNumChildren(); i++) {
        if (valueGroup->getChild(i)->getTypeId() == SoTransform::getClassTypeId())
            num++;
    }

    if (num > 3) {
        std::vector<SbVec3f> pos = getValuePositions(num-1, box);

        int index = 0;
        for (int j=0; j<valueGroup->getNumChildren(); j++) {
            if (valueGroup->getChild(j)->getTypeId() == SoTransform::getClassTypeId()) {
                static_cast<SoTransform*>(valueGroup->getChild(j))->translation.setValue(pos[index++]);
            }
        }
    }
}

void SoFCColorLegend::setColorLegend(const App::ColorLegend& legend)
{
    // create top value field
    std::size_t numFields = legend.hasNumberOfFields();
    int intFields = static_cast<int>(numFields);
    coords->point.setNum(4 * intFields);
    modifyPoints(_bbox);

    // for numFields colors we need numFields quads
    auto faceset = new SoIndexedFaceSet;
    faceset->coordIndex.setNum(5 * intFields);
    for (int j = 0; j < intFields; j++) {
        faceset->coordIndex.set1Value(5*j,   4*j);
        faceset->coordIndex.set1Value(5*j+1, 4*j+1);
        faceset->coordIndex.set1Value(5*j+2, 4*j+2);
        faceset->coordIndex.set1Value(5*j+3, 4*j+3);
        faceset->coordIndex.set1Value(5*j+4, SO_END_FACE_INDEX);
    }

    auto mat = new SoMaterial;
    mat->diffuseColor.setNum(intFields);
    for (std::size_t k = 0; k < numFields; k++) {
        App::Color col = legend.getColor(k);
        mat->diffuseColor.set1Value(k, col.r, col.g, col.b);
    }

    auto matBinding = new SoMaterialBinding;
    matBinding->value = SoMaterialBinding::PER_FACE;

    // first clear the children
    if (getNumChildren() > 0)
        coinRemoveAllChildren(this);
    addChild(labelGroup);
    addChild(valueGroup);
    addChild(coords);
    addChild(mat);
    addChild(matBinding);
    addChild(faceset);
}
