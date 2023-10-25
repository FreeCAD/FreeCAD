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
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoTransparencyType.h>
#endif

#include <Base/Parameter.h>
#include "SoFCColorGradient.h"
#include "SoTextLabel.h"
#include "DlgSettingsColorGradientImp.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "ViewProvider.h"


using namespace Gui;

SO_NODE_SOURCE(SoFCColorGradient)

/*!
  Constructor.
*/
SoFCColorGradient::SoFCColorGradient() : _bbox(5.0f, -4.0f, 5.5f, 4.0f)
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

const char* SoFCColorGradient::getColorBarName() const
{
    return QT_TRANSLATE_NOOP("QObject", "Color Gradient");
}

void SoFCColorGradient::setMarkerLabel(const SoMFString& label)
{
    coinRemoveAllChildren(labels);

    int num = label.getNum();
    if (num > 1) {
        SbVec2f maxPt = _bbox.getMax();
        SbVec2f minPt = _bbox.getMin();
        float fStep = (maxPt[1] - minPt[1]) / ((float)num - 1);
        auto trans = new SoTransform;

        ParameterGrp::handle hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("View");
        auto LabelTextSize = hGrp->GetInt("CbLabelTextSize", 13);
        auto LabelTextColor =
            App::Color((uint32_t)hGrp->GetUnsigned("CbLabelColor", 0xffffffff));
        auto textFont = new SoFont;
        auto color = new SoBaseColor;
        textFont->name.setValue("Helvetica,Arial,Times New Roman");
        textFont->size.setValue(LabelTextSize);
        trans->translation.setValue(maxPt[0] + 0.1f, maxPt[1] - 0.05f + fStep, 0.0f);
        color->rgb.setValue(LabelTextColor.r,LabelTextColor.g,LabelTextColor.b);
        labels->addChild(trans);
        labels->addChild(color);
        labels->addChild(textFont);

        for (int i = 0; i < num; i++) {
            auto trans = new SoTransform;
            auto text2 = new SoColorBarLabel;
            trans->translation.setValue(0, -fStep, 0);
            text2->string.setValue(label[i]);
            labels->addChild(trans);
            labels->addChild(text2);
        }
    }

    setModified();
}

void SoFCColorGradient::setViewportSize(const SbVec2s& size)
{
    float fMinX, fMinY, fMaxX, fMaxY;
    float boxWidth = getBounds(size, fMinX, fMinY, fMaxX, fMaxY);

    // search for the labels
    int num = 0;
    for (int i = 0; i < labels->getNumChildren(); i++) {
        if (labels->getChild(i)->getTypeId() == SoTransform::getClassTypeId())
            num++;
    }

    if (num > 2) {
        bool first = true;
        float fStep = (fMaxY - fMinY) / ((float)num - 2);

        for (int j = 0; j < labels->getNumChildren(); j++) {
            if (labels->getChild(j)->getTypeId() == SoTransform::getClassTypeId()) {
                if (first) {
                    first = false;
                    // set the labels with a small space of 0.1f besides the bar
                    static_cast<SoTransform*>(labels->getChild(j))->translation.setValue(fMaxX + 0.1f - boxWidth, fMaxY - 0.05f + fStep, 0.0f);
                }
                else {
                    static_cast<SoTransform*>(labels->getChild(j))->translation.setValue(0, -fStep, 0.0f);
                }
            }
        }
    }

    // gradient bar is shifted to the left by width of the labels to assure that labels are fully visible
    _bbox.setBounds(fMinX - boxWidth, fMinY, fMaxX - boxWidth, fMaxY);
    modifyPoints(_bbox);
}

void SoFCColorGradient::setRange(float fMin, float fMax, int prec)
{
    _cColGrad.setRange(fMin, fMax);

    SoMFString label;
    float eps = std::pow(10.0f, static_cast<float>(-prec));
    float value_min = std::min<float>(fabs(fMin), fabs(fMax));
    float value_max = std::max<float>(fabs(fMin), fabs(fMax));

    // format the label the following way:
    // if Min is smaller than 1e-<precision>,
    //  or Max greater than 1e+4,
    //  or (Max - Min) < 1e-<precision> * number of labels - 1 (assures every label shows different number)
    // -> output in scientific notation
    // otherwise output "normal" (fixed notation)
    bool scientific = (value_min < eps && value_min > 0.0f)
        || (value_max - value_min) < eps * (_cColGrad.getCountColors() - 1)
        || value_max > 1e4;
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

    // for uCtColors colors we need 2*(uCtColors-1) facets and therefore an array with
    // 8*(uCtColors-1) face indices
    auto faceset = new SoIndexedFaceSet;
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
    auto ttype = new SoTransparencyType;
    ttype->value = SoGLRenderAction::DELAYED_BLEND;
    auto mat = new SoMaterial;
    //mat->transparency = 0.3f;
    mat->diffuseColor.setNum(2 * uCtColors);
    for (int k = 0; k < uCtColors; k++) {
        App::Color col = model.colors[uCtColors - k - 1];
        mat->diffuseColor.set1Value(2 * k, col.r, col.g, col.b);
        mat->diffuseColor.set1Value(2 * k + 1, col.r, col.g, col.b);
    }

    auto matBinding = new SoMaterialBinding;
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
}
