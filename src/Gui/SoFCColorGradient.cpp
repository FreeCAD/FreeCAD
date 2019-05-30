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

SO_NODE_SOURCE(SoFCColorGradient);

/*!
  Constructor.
*/
SoFCColorGradient::SoFCColorGradient() : _fMaxX(4.5f), _fMinX(4.0f), _fMaxY(4.0f), _fMinY(-4.0f), _bOutInvisible(false), _precision(3)
{
    SO_NODE_CONSTRUCTOR(SoFCColorGradient);
    coords = new SoCoordinate3;
    coords->ref();
    labels = new SoSeparator;
    labels->ref();

    _cColGrad.setStyle(App::ColorGradient::FLOW);
    setColorModel( App::ColorGradient::TRIA );
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
void SoFCColorGradient::initClass(void)
{
    SO_NODE_INIT_CLASS(SoFCColorGradient,SoFCColorBarBase,"Separator");
}

void SoFCColorGradient::finish()
{
    atexit_cleanup();
}

void SoFCColorGradient::setMarkerLabel( const SoMFString& label )
{
    coinRemoveAllChildren(labels);

    float fH=8.0f;
    int num = label.getNum();
    if ( num > 1 )
    {
        float fStep = fH / ((float)num-1);
        SoTransform* trans = new SoTransform;
        trans->translation.setValue(_fMaxX+0.1f,_fMaxY-0.05f+fStep,0.0f);
        labels->addChild(trans);

        for ( int i=0; i<num; i++ )
        {
            SoTransform* trans = new SoTransform;
            SoBaseColor* color = new SoBaseColor;
            SoText2    * text2 = new SoText2;

            trans->translation.setValue(0,-fStep,0);
            color->rgb.setValue(0,0,0);
            text2->string.setValue( label[i] );
            labels->addChild(trans);
            labels->addChild(color);
            labels->addChild(text2);
        }
    }
}

void SoFCColorGradient::setViewportSize( const SbVec2s& size )
{
    // don't know why the parameter range isn't between [-1,+1]
    float fRatio = ((float)size[0])/((float)size[1]);
    float fMinX=  4.0f, fMaxX=4.5f;
    float fMinY= -4.0f, fMaxY=4.0f;

    if ( fRatio > 1.0f )
    {
        fMinX = 4.0f * fRatio;
        fMaxX = fMinX+0.5f;
    }
    else if ( fRatio < 1.0f )
    {
        fMinY =  -4.0f / fRatio;
        fMaxY =   4.0f / fRatio;
    }

    _fMaxX = fMaxX;
    _fMinX = fMinX;
    _fMaxY = fMaxY;
    _fMinY = fMinY;

    // search for the labels
    int num=0;
    for ( int i=0; i<labels->getNumChildren(); i++ )
    {
        if ( labels->getChild(i)->getTypeId() == SoTransform::getClassTypeId() )
            num++;
    }

    if ( num > 2 )
    {
        bool first=true;
        float fStep = (fMaxY-fMinY) / ((float)num-2);

        for ( int j=0; j<labels->getNumChildren(); j++ )
        {
            if ( labels->getChild(j)->getTypeId() == SoTransform::getClassTypeId() )
            {
                if ( first )
                {
                    first = false;
                    static_cast<SoTransform*>(labels->getChild(j))->translation.setValue(fMaxX+0.1f,fMaxY-0.05f+fStep,0.0f);
                }
                else
                {
                    static_cast<SoTransform*>(labels->getChild(j))->translation.setValue(0,-fStep,0.0f);
                }
            }
        }
    }

    // set the vertices spanning the faces for the color gradient
    int ct = coords->point.getNum()/2;
    for ( int j=0; j<ct; j++ )
    {
        float w = (float)j/(float)(ct-1);
        float fPosY = (1.0f-w)*_fMaxY + w*_fMinY;
        coords->point.set1Value(2*j, _fMinX, fPosY, 0.0f);
        coords->point.set1Value(2*j+1, _fMaxX, fPosY, 0.0f);
    }
}

void SoFCColorGradient::setRange( float fMin, float fMax, int prec )
{
    _cColGrad.setRange(fMin, fMax);

    SoMFString label;

    float fFac = (float)pow(10.0, (double)prec);

    int i=0;
    std::vector<float> marks = getMarkerValues(fMin, fMax, _cColGrad.getCountColors());
    for ( std::vector<float>::iterator it = marks.begin(); it != marks.end(); ++it )
    {
        std::stringstream s;
        s.precision(prec);
        s.setf(std::ios::fixed | std::ios::showpoint | std::ios::showpos);
        float fValue = *it;
        if ( fabs(fValue*fFac) < 1.0 )
            fValue = 0.0f;
        s << fValue;
        label.set1Value(i++, s.str().c_str());
    }

    setMarkerLabel( label );
}

std::vector<float> SoFCColorGradient::getMarkerValues(float fMin, float fMax, int count) const
{
    std::vector<float> labels;

    // the middle of the bar is zero
    if ( fMin < 0.0f && fMax > 0.0f && _cColGrad.getStyle() == App::ColorGradient::ZERO_BASED )
    {
        if ( count % 2 == 0) count++;
        int half = count / 2;
        for (int j=0; j<half+1; j++)
        {
            float w = (float)j/((float)half);
            float fValue = (1.0f-w)*fMax;
            labels.push_back( fValue );
        }
        for (int k=half+1; k<count; k++)
        {
            float w = (float)(k-half+1)/((float)(count-half));
            float fValue = w*fMin;
            labels.push_back( fValue );
        }
    }
    else // either not zero based or 0 is not in between [fMin,fMax]
    {
        for (int j=0; j<count; j++)
        {
            float w = (float)j/((float)count-1.0f);
            float fValue = (1.0f-w)*fMax+w*fMin;
            labels.push_back( fValue );
        }
    }

    return labels;
}

void SoFCColorGradient::setColorModel( App::ColorGradient::TColorModel tModel )
{
    _cColGrad.setColorModel( tModel );
    rebuildGradient();
}

void SoFCColorGradient::setColorStyle (App::ColorGradient::TStyle tStyle)
{
    _cColGrad.setStyle( tStyle );
    rebuildGradient();
}

void SoFCColorGradient::rebuildGradient()
{
    App::ColorModel model = _cColGrad.getColorModel();
    int uCtColors = (int)model._usColors;

    coords->point.setNum(2*uCtColors);
    for ( int i=0; i<uCtColors; i++ )
    {
        float w = (float)i/(float)(uCtColors-1);
        float fPosY = (1.0f-w)*_fMaxY + w*_fMinY;
        coords->point.set1Value(2*i, _fMinX, fPosY, 0.0f);
        coords->point.set1Value(2*i+1, _fMaxX, fPosY, 0.0f);
    }

    // for uCtColors colors we need 2*(uCtColors-1) facets and therefore an array with
    // 8*(uCtColors-1) face indices
    SoIndexedFaceSet * faceset = new SoIndexedFaceSet;
    faceset->coordIndex.setNum(8*(uCtColors-1));
    for ( int j=0; j<uCtColors-1; j++ )
    {
        faceset->coordIndex.set1Value(8*j,   2*j);
        faceset->coordIndex.set1Value(8*j+1, 2*j+3);
        faceset->coordIndex.set1Value(8*j+2, 2*j+1);
        faceset->coordIndex.set1Value(8*j+3, SO_END_FACE_INDEX);
        faceset->coordIndex.set1Value(8*j+4, 2*j);
        faceset->coordIndex.set1Value(8*j+5, 2*j+2);
        faceset->coordIndex.set1Value(8*j+6, 2*j+3);
        faceset->coordIndex.set1Value(8*j+7, SO_END_FACE_INDEX);
    }

    // set an own transparency type for this color bar only
    SoTransparencyType* ttype = new SoTransparencyType;
    ttype->value = SoGLRenderAction::DELAYED_BLEND;
    SoMaterial* mat = new SoMaterial;
    //mat->transparency = 0.3f;
    mat->diffuseColor.setNum(2*uCtColors);
    for ( int k=0; k<uCtColors; k++ )
    {
        App::Color col = model._pclColors[uCtColors-k-1];
        mat->diffuseColor.set1Value(2*k, col.r, col.g, col.b);
        mat->diffuseColor.set1Value(2*k+1, col.r, col.g, col.b);
    }

    SoMaterialBinding* matBinding = new SoMaterialBinding;
    matBinding->value = SoMaterialBinding::PER_VERTEX_INDEXED;

    // first clear the children
    if ( getNumChildren() > 0 )
        coinRemoveAllChildren(this);
    addChild(ttype);
    addChild(labels);
    addChild(coords);
    addChild(mat);
    addChild(matBinding);
    addChild(faceset);
}

bool SoFCColorGradient::isVisible (float fVal) const
{
    if (_bOutInvisible == true)
    {
        float fMin, fMax;
        _cColGrad.getRange(fMin, fMax);
        if ((fVal > fMax) || (fVal < fMin))
            return false;
        else
            return true;
    }

    return true;
}

bool SoFCColorGradient::customize()
{
    QWidget* parent = Gui::getMainWindow()->activeWindow();
    Gui::Dialog::DlgSettingsColorGradientImp dlg(parent);

    dlg.setColorModel( _cColGrad.getColorModelType() );
    dlg.setColorStyle( _cColGrad.getStyle() );
    dlg.setOutGrayed( _cColGrad.isOutsideGrayed() );
    dlg.setOutInvisible( _bOutInvisible );
    dlg.setNumberOfLabels( _cColGrad.getCountColors() );
    dlg.setNumberOfDecimals( _precision );
    float fMin, fMax;
    _cColGrad.getRange(fMin, fMax);
    dlg.setRange(fMin, fMax);

    QPoint pos(QCursor::pos());
    pos += QPoint((int)(-1.1*dlg.width()),(int)(-0.1*dlg.height()));
    dlg.move( pos );

    if ( dlg.exec() == QDialog::Accepted )
    {
        _cColGrad.setColorModel( dlg.colorModel() );
        _cColGrad.setStyle( dlg.colorStyle() );
        _cColGrad.setOutsideGrayed( dlg.isOutGrayed() );
        _bOutInvisible = dlg.isOutInvisible();
        _cColGrad.setCountColors( dlg.numberOfLabels() );
        _precision = dlg.numberOfDecimals();
        dlg.getRange( fMin, fMax );
        int dec = dlg.numberOfDecimals();
        setRange( fMin, fMax, dec );
        rebuildGradient();

        return true;
    }

    return false;
}
