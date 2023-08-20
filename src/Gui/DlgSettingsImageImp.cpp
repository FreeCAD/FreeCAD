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
# include <QRegularExpression>
# include <QRegularExpressionMatch>
#endif

#include "DlgSettingsImageImp.h"
#include "ui_DlgSettingsImage.h"


using namespace Gui::Dialog;
using namespace std;

/* TRANSLATOR Gui::Dialog::DlgSettingsImageImp */

/**
 *  Constructs a DlgSettingsImageImp as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
DlgSettingsImageImp::DlgSettingsImageImp( QWidget* parent )
  : QWidget( parent )
  , ui(new Ui_DlgSettingsImage)
{
    ui->setupUi(this);
    setupConnections();

    SbVec2s res = SoOffscreenRenderer::getMaximumResolution();
    ui->spinWidth->setMaximum((int)res[0]);
    ui->spinHeight->setMaximum((int)res[1]);

    _width = width();
    _height = height();
    _fRatio = (float)_width/(float)_height;

    ui->comboMethod->addItem(tr("Offscreen (New)"), QByteArray("QtOffscreenRenderer"));
    ui->comboMethod->addItem(tr("Offscreen (Old)"), QByteArray("CoinOffscreenRenderer"));
    ui->comboMethod->addItem(tr("Framebuffer (custom)"), QByteArray("FramebufferObject"));
    ui->comboMethod->addItem(tr("Framebuffer (as is)"), QByteArray("GrabFramebuffer"));
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsImageImp::~DlgSettingsImageImp() = default;

void DlgSettingsImageImp::setupConnections()
{
    connect(ui->buttonRatioScreen, &QToolButton::clicked,
            this, &DlgSettingsImageImp::onButtonRatioScreenClicked);
    connect(ui->buttonRatio4x3, &QToolButton::clicked,
            this, &DlgSettingsImageImp::onButtonRatio4x3Clicked);
    connect(ui->buttonRatio16x9, &QToolButton::clicked,
            this, &DlgSettingsImageImp::onButtonRatio16x9Clicked);
    connect(ui->buttonRatio1x1, &QToolButton::clicked,
            this, &DlgSettingsImageImp::onButtonRatio1x1Clicked);
    connect(ui->standardSizeBox, qOverload<int>(&QComboBox::activated),
            this, &DlgSettingsImageImp::onStandardSizeBoxActivated);
    connect(ui->comboMethod, qOverload<int>(&QComboBox::activated),
            this, &DlgSettingsImageImp::onComboMethodActivated);
}

void DlgSettingsImageImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

/**
 * Sets the image size to (\a w, \a h).
 */
void DlgSettingsImageImp::setImageSize(int w, int h)
{
    // set current screen size
    ui->standardSizeBox->setItemData(0, QSize(w,h));

    ui->spinWidth->setValue(w);
    ui->spinHeight->setValue(h);

    // As the image size is in pixel why shouldn't _width and _height be integers?
    _width  = w;
    _height = h;
    _fRatio = (float)_width/(float)_height;
}

/**
 * Sets the image size to \a s.
 */
void DlgSettingsImageImp::setImageSize( const QSize& s )
{
    // set current screen size
    ui->standardSizeBox->setItemData(0, s);

    ui->spinWidth->setValue( s.width() );
    ui->spinHeight->setValue( s.height() );

    // As the image size is in pixel why shouldn't _width and _height be integers?
    _width  = s.width();
    _height = s.height();
    _fRatio = (float)_width/(float)_height;
}

/**
 * Returns the currently set image size.
 */
QSize DlgSettingsImageImp::imageSize() const
{
    return { ui->spinWidth->value(), ui->spinHeight->value() };
}

/**
 * Returns the currently set image width.
 */
int DlgSettingsImageImp::imageWidth() const
{
    return ui->spinWidth->value();
}

/**
 * Returns the currently set image height.
 */
int DlgSettingsImageImp::imageHeight() const
{
    return ui->spinHeight->value();
}

/**
 * Returns the comment of the picture. If for the currently selected image format no comments are supported
 * QString() is returned.
 */
QString DlgSettingsImageImp::comment() const
{
    if ( !ui->textEditComment->isEnabled() )
        return {};
    else
        return ui->textEditComment->toPlainText();
}

int DlgSettingsImageImp::backgroundType() const
{
    return ui->comboBackground->currentIndex();
}

/**
 * Sets the image size to (\a w, \a h).
 */
void DlgSettingsImageImp::setBackgroundType(int t)
{
    if ( t < ui->comboBackground->count() )
        ui->comboBackground->setCurrentIndex(t);
}

bool DlgSettingsImageImp::addWatermark() const
{
    return ui->checkWatermark->isChecked();
}

void DlgSettingsImageImp::onSelectedFilter(const QString& filter)
{
    bool ok = (filter.startsWith(QLatin1String("JPG")) ||
               filter.startsWith(QLatin1String("JPEG")) ||
               filter.startsWith(QLatin1String("PNG")));
    ui->buttonGroupComment->setEnabled( ok );
}

void DlgSettingsImageImp::adjustImageSize(float fRatio)
{
    // if width has changed then adjust height and vice versa, if both has changed then adjust width
    if (_height != ui->spinHeight->value())
    {
        _height = ui->spinHeight->value();
        _width = (int)((float)_height*fRatio);
        ui->spinWidth->setValue( (int)_width );
    }
    else // if( _width != spinWidth->value() )
    {
        _width = ui->spinWidth->value();
        _height = (int)((float)_width/fRatio);
        ui->spinHeight->setValue( (int)_height );
    }
}

void DlgSettingsImageImp::onButtonRatioScreenClicked()
{
    adjustImageSize(_fRatio);
}

void DlgSettingsImageImp::onButtonRatio4x3Clicked()
{
    adjustImageSize(4.0f/3.0f);
}

void DlgSettingsImageImp::onButtonRatio16x9Clicked()
{
    adjustImageSize(16.0f/9.0f);
}

void DlgSettingsImageImp::onButtonRatio1x1Clicked()
{
    adjustImageSize(1.0f);
}

void DlgSettingsImageImp::onStandardSizeBoxActivated(int index)
{
    if (index == 0) {
        // we have set the user data for the 1st item
        QSize s = ui->standardSizeBox->itemData(0).toSize();
        ui->spinWidth->setValue(s.width());
        ui->spinHeight->setValue(s.height());
    }
    else {
        // try to extract from the string
        QString text = ui->standardSizeBox->itemText(index);
        QRegularExpression rx(QLatin1String(R"(\b\d{2,5}\b)"));
        int pos = 0;
        auto match = rx.match(text, pos);
        if (match.hasMatch()) {
            pos = match.capturedStart();
            QString width = text.mid(pos, match.capturedLength());
            ui->spinWidth->setValue(width.toInt());
            pos += match.capturedLength();
        }

        match = rx.match(text, pos);
        if (match.hasMatch()) {
            pos = match.capturedStart();
            QString height = text.mid(pos, match.capturedLength());
            ui->spinHeight->setValue(height.toInt());
        }
    }
}

void DlgSettingsImageImp::setMethod(const QByteArray& m)
{
    int index = ui->comboMethod->findData(m);
    if (index >= 0)
        ui->comboMethod->setCurrentIndex(index);
}

QByteArray DlgSettingsImageImp::method() const
{
    return ui->comboMethod->currentData().toByteArray();
}

void DlgSettingsImageImp::onComboMethodActivated(int index)
{
    QByteArray data = ui->comboMethod->itemData(index).toByteArray();
    if (data == QByteArray("GrabFramebuffer")) {
        ui->comboBackground->setEnabled(false);
    }
    else {
        ui->comboBackground->setEnabled(true);
    }
}

#include "moc_DlgSettingsImageImp.cpp"
