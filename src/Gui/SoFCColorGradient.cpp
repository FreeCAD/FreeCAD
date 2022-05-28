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
# include <Inventor/fields/SoMFString.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoTransparencyType.h>
#endif

#include "SoFCColorGradient.h"
#include "DlgSettingsColorGradientImp.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "ViewProvider.h"


using namespace Gui;

SO_NODE_SOURCE(SoFCColorGradient)

/*!
  Constructor.
*/
SoFCColorGradient::SoFCColorGradient() : _bbox(4.0f, -4.0f, 4.5f, 4.0f), _precision(3)
{
    SO_NODE_CONSTRUCTOR(SoFCColorGradient);
    coords = new SoCoordinate3;
    coords->ref();
    labels = new SoSeparator;
    labels->ref();

    _cColGrad.setStyle(App::ColorBarStyle::FLOW);
    setColorModel(0);
    setRange(-0.5f, 0.5f, 1);
}

/*!
  Destructor.
*/
SoFCColorGradient::~SoFCColorGradient()
{
    //delete THIS;
    coords->unref();
    labels->unref();
}

// doc from parent
void SoFCColorGradient::initClass()
{
    SO_NODE_INIT_CLASS(SoFCColorGradient, SoFCColorBarBase, "Separator");
}

void SoFCColorGradient::finish()
{
    atexit_cleanup();
}

void SoFCColorGradient::setMarkerLabel(const SoMFString& label)
{
    coinRemoveAllChildren(labels);

    int num = label.getNum();
    if (num > 1) {
        float fStep = 8.0f / ((float)num - 1);
        SoTransform* trans = new SoTransform;
        trans->translation.setValue(_bbox.getMax()[0] + 0.1f, _bbox.getMax()[1] - 0.05f + fStep, 0.0f);
        labels->addChild(trans);

        for (int i = 0; i < num; i++) {
            SoTransform* trans = new SoTransform;
            SoBaseColor* color = new SoBaseColor;
            SoText2    * text2 = new SoText2;

            trans->translation.setValue(0, -fStep, 0);
            color->rgb.setValue(0, 0, 0);
            text2->string.setValue(label[i]);
            labels->addChild(trans);
            labels->addChild(color);
            labels->addChild(text2);
        }
    }
}

void SoFCColorGradient::setViewportSize(const SbVec2s& size)
{
    // don't know why the parameter range isn't between [-1,+1]
    float fRatio = ((float)size[0]) / ((float)size[1]);
    float fMinX =  4.0f, fMaxX = 4.5f;
    float fMinY = -4.0f, fMaxY = 4.0f;

    if (fRatio > 1.0f) {
        fMinX = 4.0f * fRatio;
        fMaxX = fMinX + 0.5f;
    }
    else if (fRatio < 1.0f) {
        fMinY = -4.0f / fRatio;
        fMaxY =  4.0f / fRatio;
    }

    // search for the labels
    int num = 0;
    for (int i = 0; i < labels->getNumChildren(); i++) {
        if (labels->getChild(i)->getTypeId() == SoTransform::getClassTypeId())
            num++;
    }

    // depending ion the number of decimals we need more space for the label
    // the fMinX/fMin settings are optimized for 2 decimals
    // for every decimal we need more space and thus shift the color bar to the left
    float shiftToLeft = 0.0f;
    if (_precision > 2)
        shiftToLeft = (_precision - 2) * 0.4f / fRatio;

    if (num > 2) {
        bool first = true;
        float fStep = (fMaxY - fMinY) / ((float)num - 2);

        for (int j = 0; j < labels->getNumChildren(); j++) {
            if (labels->getChild(j)->getTypeId() == SoTransform::getClassTypeId()) {
                if (first) {
                    first = false;
                    static_cast<SoTransform*>(labels->getChild(j))->translation.setValue(fMaxX + 0.1f - shiftToLeft, fMaxY - 0.05f + fStep, 0.0f);
                }
                else {
                    static_cast<SoTransform*>(labels->getChild(j))->translation.setValue(0, -fStep, 0.0f);
                }
            }
        }
    }

    _bbox.setBounds(fMinX - shiftToLeft, fMinY, fMaxX - shiftToLeft, fMaxY);
    modifyPoints(_bbox);
}

void SoFCColorGradient::setRange(float fMin, float fMax, int prec)
{
    _cColGrad.setRange(fMin, fMax);

    // format the label the following way:
    // if fMin is smaller than 1e-<precision> or fMax greater than 1e+4, output in scientific notation
    // otherwise output "normal" (fixed notation)

    SoMFString label;
    float eps = std::pow(10.0f, static_cast<float>(-prec));
    float value_min = std::min<float>(fabs(fMin), fabs(fMax));
    float value_max = std::max<float>(fabs(fMin), fabs(fMax));

    bool scientific = (value_min < eps && value_min > 0.0f) || value_max > 1e4;
    std::ios::fmtflags flags = scientific ? (std::ios::scientific | std::ios::showpoint | std::ios::showpos)
                                          : (std::ios::fixed | std::ios::showpoint | std::ios::showpos);

    // write the labels
    int i = 0;
    std::vector<float> marks = getMarkerValues(fMin, fMax, _cColGrad.getCountColors());
    for (const auto& it : marks) {
        std::stringstream s;
        s.precision(prec);
        s.setf(flags);
        s << it;
        label.set1Value(i++, s.str().c_str());
    }

    setMarkerLabel(label);
}

std::vector<float> SoFCColorGradient::getMarkerValues(float fMin, float fMax, int count) const
{
    std::vector<float> labels;

    // the middle of the bar is zero
    if (fMin < 0.0f && fMax > 0.0f && _cColGrad.getStyle() == App::ColorBarStyle::ZERO_BASED) {
        if (count % 2 == 0)
            count++;
        int half = count / 2;
        for (int j = 0; j < half + 1; j++) {
            float w = (float)j / ((float)half);
            float fValue = (1.0f - w) * fMax;
            labels.push_back(fValue);
        }
        for (int k = half + 1; k < count; k++) {
            float w = (float)(k - half + 1) / ((float)(count - half));
            float fValue = w * fMin;
            labels.push_back(fValue);
        }
    }
    else { // either not zero based or 0 is not in between [fMin,fMax]
        for (int j = 0; j < count; j++) {
            float w = (float)j / ((float)count - 1.0f);
            float fValue = (1.0f - w) * fMax + w * fMin;
            labels.push_back(fValue);
        }
    }

    return labels;
}

void SoFCColorGradient::modifyPoints(const SbBox2f& box)
{
    float fMinX = box.getMin()[0];
    float fMinY = box.getMin()[1];
    float fMaxX = box.getMax()[0];
    float fMaxY = box.getMax()[1];

    // set the vertices spanning the faces for the color gradient
    int intFields = coords->point.getNum() / 2;
    for (int i = 0; i < intFields; i++) {
        float w = static_cast<float>(i) / (intFields - 1);
        float fPosY = (1.0f - w) * fMaxY + w * fMinY;
        coords->point.set1Value(2 * i,     fMinX, fPosY, 0.0f);
        coords->point.set1Value(2 * i + 1, fMaxX, fPosY, 0.0f);
    }
}

void SoFCColorGradient::setColorModel(std::size_t index)
{
    _cColGrad.setColorModel(index);
    rebuildGradient();
}

void SoFCColorGradient::setColorStyle(App::ColorBarStyle tStyle)
{
    _cColGrad.setStyle(tStyle);
    rebuildGradient();
}

void SoFCColorGradient::rebuildGradient()
{
    App::ColorModel model = _cColGrad.getColorModel();
    int uCtColors = static_cast<int>(model.getCountColors());

    coords->point.setNum(2 * uCtColors);
    modifyPoints(_bbox);

    // trigger recalculation of size since number of decimal might have been changed
    SbVec2s size;
    size.setValue(_bbox.getMax());
    setViewportSize(size);

    // for uCtColors colors we need 2*(uCtColors-1) facets and therefore an array with
    // 8*(uCtColors-1) face indices
    SoIndexedFaceSet* faceset = new SoIndexedFaceSet;
    faceset->coordIndex.setNum(8 * (uCtColors - 1));
    for (int j = 0; j < uCtColors - 1; j++) {
        faceset->coordIndex.set1Value(8 * j, 2 * j);
        faceset->coordIndex.set1Value(8 * j + 1, 2 * j + 3);
        faceset->coordIndex.set1Value(8 * j + 2, 2 * j + 1);
        faceset->coordIndex.set1Value(8 * j + 3, SO_END_FACE_INDEX);
        faceset->coordIndex.set1Value(8 * j + 4, 2 * j);
        faceset->coordIndex.set1Value(8 * j + 5, 2 * j + 2);
        faceset->coordIndex.set1Value(8 * j + 6, 2 * j + 3);
        faceset->coordIndex.set1Value(8 * j + 7, SO_END_FACE_INDEX);
    }

    // set an own transparency type for this color bar only
    SoTransparencyType* ttype = new SoTransparencyType;
    ttype->value = SoGLRenderAction::DELAYED_BLEND;
    SoMaterial* mat = new SoMaterial;
    //mat->transparency = 0.3f;
    mat->diffuseColor.setNum(2 * uCtColors);
    for (int k = 0; k < uCtColors; k++) {
        App::Color col = model.colors[uCtColors - k - 1];
        mat->diffuseColor.set1Value(2 * k, col.r, col.g, col.b);
        mat->diffuseColor.set1Value(2 * k + 1, col.r, col.g, col.b);
    }

    SoMaterialBinding* matBinding = new SoMaterialBinding;
    matBinding->value = SoMaterialBinding::PER_VERTEX_INDEXED;

    // first clear the children
    if (getNumChildren() > 0)
        coinRemoveAllChildren(this);
    addChild(ttype);
    addChild(labels);
    addChild(coords);
    addChild(mat);
    addChild(matBinding);
    addChild(faceset);
}

bool SoFCColorGradient::isVisible(float fVal) const
{
    if (_cColGrad.isOutsideInvisible()) {
        return !_cColGrad.isOutOfRange(fVal);
    }

    return true;
}

void SoFCColorGradient::customize(SoFCColorBarBase* parentNode)
{
    QWidget* parent = Gui::getMainWindow()->activeWindow();
    Gui::Dialog::DlgSettingsColorGradientImp dlg(_cColGrad, parent);
    App::ColorGradientProfile profile = _cColGrad.getProfile();
    dlg.setNumberOfDecimals(_precision, profile.fMin, profile.fMax);

    QPoint pos(QCursor::pos());
    pos += QPoint(int(-1.1 * dlg.width()), int(-0.1 * dlg.height()));
    dlg.move(pos);

    auto applyProfile = [&](const App::ColorGradientProfile& pro, int precision) {
        _cColGrad.setProfile(pro);
        setRange(pro.fMin, pro.fMax, precision);
        rebuildGradient();

        triggerChange(parentNode);
    };
    QObject::connect(&dlg, &Gui::Dialog::DlgSettingsColorGradientImp::colorModelChanged,
                     [&] {
        try {
            applyProfile(dlg.getProfile(), dlg.numberOfDecimals());
        }
        catch (const Base::Exception& e) {
            e.ReportException();
        }
    });

    if (dlg.exec() != QDialog::Accepted) {
        int decimals = dlg.numberOfDecimals();
        if (!profile.isEqual(dlg.getProfile()) || decimals != _precision)
            applyProfile(profile, _precision);
    }
    else {
        _precision = dlg.numberOfDecimals();
    }

    // we need the rebuild the gradient since a changed number of decimals
    // must change the gradient position
    rebuildGradient();
}
