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


#ifndef GUI_DIALOG_DLGSETTINGSIMAGE_IMP_H
#define GUI_DIALOG_DLGSETTINGSIMAGE_IMP_H

#include <Inventor/SoOffscreenRenderer.h> 
#include <Inventor/SbMatrix.h> 

#include "ui_DlgSettingsImage.h"

namespace Gui {
namespace Dialog {
/**
 * The DlgSettings3DViewImp class implements a preference page to change settings
 * for the Inventor viewer.
 * @author Werner Mayer
 */
class DlgSettingsImageImp : public QWidget, public Ui::DlgSettingsImage
{
    Q_OBJECT

public:
    DlgSettingsImageImp( QWidget* parent = 0 );
    ~DlgSettingsImageImp();

    /** @name Image dimensions */
    //@{
    void setImageSize( int, int );
    void setImageSize( const QSize& );
    QSize imageSize() const;
    int imageWidth() const;
    int imageHeight() const;
    //@}

    /** @name Image meta information */
    //@{
    QString comment() const;
    int backgroundType() const;
    void setBackgroundType( int );
    bool addWatermark() const;
    //@}

public Q_SLOTS:
    void onSelectedFilter( const QString& );

protected Q_SLOTS:
    void on_buttonRatioScreen_clicked();
    void on_buttonRatio4x3_clicked();
    void on_buttonRatio16x9_clicked();
    void on_buttonRatio1x1_clicked();
    void on_standardSizeBox_activated(int);

protected:
    // helper to force an aspect ratio
    void adjustImageSize(float fRatio);
    void changeEvent(QEvent *e);


private:
    float _fRatio;
    int _width, _height;
    SbMatrix _Matrix;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGSETTINGSIMAGE_IMP_H
