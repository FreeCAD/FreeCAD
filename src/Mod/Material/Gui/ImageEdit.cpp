/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QBuffer>
#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QSvgRenderer>
#include <QTextStream>
#endif

#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/Materials.h>

#include "ArrayDelegate.h"
#include "ArrayModel.h"
#include "ImageEdit.h"
#include "ListModel.h"
#include "ui_ImageEdit.h"


using namespace MatGui;

/* TRANSLATOR MatGui::ImageEdit */

ImageLabel::ImageLabel(QWidget* parent)
    : QLabel(parent)
{}

void ImageLabel::setPixmap(const QPixmap& pixmap)
{
    _pixmap = pixmap;
    _svg.clear();
    QLabel::setPixmap(pixmap);
}

void ImageLabel::setSVG(const QString& svg)
{
    _svg = svg;
    _pixmap = QPixmap();
    update();
    // renderSVG();
}

void ImageLabel::renderSVG()
{
    QPainter painter(this);
    QSvgRenderer renderer(_svg.toUtf8(), this);

    painter.begin(this);

    renderer.render(&painter);

    painter.end();
}

void ImageLabel::resizeEvent(QResizeEvent* event)
{
    if (_svg.isEmpty()) {
        QPixmap px = _pixmap.scaled(event->size(), Qt::KeepAspectRatio);
        QLabel::setPixmap(px);
        QLabel::resizeEvent(event);
    }
    // else {
    //     renderSVG();
    // }
}

void ImageLabel::paintEvent(QPaintEvent* event)
{
    if (!_svg.isEmpty()) {
        QSvgRenderer renderer(_svg.toUtf8());
        QPainter painter(this);
        renderer.render(&painter, QRectF(event->rect()));
    }
    else {
        QLabel::paintEvent(event);
    }
}
//===

ImageEdit::ImageEdit(const QString& propertyName,
                     const std::shared_ptr<Materials::Material>& material,
                     QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_ImageEdit)
    , _material(material)
    , _pixmap(QStringLiteral(":/images/default_image.png"))
{
    ui->setupUi(this);

    if (material->hasPhysicalProperty(propertyName)) {
        _property = material->getPhysicalProperty(propertyName);
    }
    else if (material->hasAppearanceProperty(propertyName)) {
        _property = material->getAppearanceProperty(propertyName);
    }
    else {
        Base::Console().log("Property '%s' not found\n", propertyName.toStdString().c_str());
        _property = nullptr;
    }
    if (_property) {
        if (_property->getType() == Materials::MaterialValue::SVG) {
            _svg = _property->getString();
            showSVG();
        }
        else {
            QString value = _property->getString();
            if (!value.isEmpty()) {
                QByteArray by = QByteArray::fromBase64(value.toUtf8());
                QImage img = QImage::fromData(by);
                _pixmap = QPixmap::fromImage(img);
            }
            showPixmap();
        }
    }
    else {
        Base::Console().log("No value loaded\n");
        showPixmap();
    }

    connect(ui->buttonFileSelect, &QPushButton::clicked, this, &ImageEdit::onFileSelect);

    connect(ui->standardButtons, &QDialogButtonBox::accepted, this, &ImageEdit::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected, this, &ImageEdit::reject);
}

void ImageEdit::showPixmap()
{
    ui->labelThumb->setPixmap(_pixmap);
    ui->labelThumb->setFixedSize(64, 64);
    ui->labelImage->setPixmap(_pixmap);
    QString text;
    ui->editWidth->setText(text.setNum(_pixmap.width()));
    ui->editHeight->setText(text.setNum(_pixmap.height()));
}

void ImageEdit::showSVG()
{
    ui->labelThumb->setSVG(_svg);
    ui->labelThumb->setFixedSize(64, 64);
    ui->labelImage->setSVG(_svg);
    // QString text;
    // ui->editWidth->setText(text.setNum(_pixmap.width()));
    // ui->editHeight->setText(text.setNum(_pixmap.height()));
}

void ImageEdit::onFileSelect(bool checked)
{
    Q_UNUSED(checked)

    if (_property && _property->getType() == Materials::MaterialValue::SVG) {
        onFileSelectSVG();
    }
    else {
        onFileSelectImage();
    }
}

QString ImageEdit::selectFile(const QString& filePatterns)
{
    QFileDialog::Options dlgOpt;
    if (Gui::DialogOptions::dontUseNativeFileDialog()) {
        dlgOpt = QFileDialog::DontUseNativeDialog;
    }

    QString directory = Gui::FileDialog::getWorkingDirectory();
    QString fn = Gui::FileDialog::getOpenFileName(this,
                                                  tr("Select an image"),
                                                  directory,
                                                  filePatterns,
                                                  nullptr,
                                                  dlgOpt);

    return fn;
}

void ImageEdit::onFileSelectImage()
{
    QString fn = selectFile(tr("Image files (*.jpg *.jpeg *.png *.bmp);;All files (*)"));
    if (!fn.isEmpty()) {
        fn = QDir::fromNativeSeparators(fn);

        _pixmap = QPixmap(fn);
        _svg.clear();
        showPixmap();
    }
}

void ImageEdit::onFileSelectSVG()
{
    QString fn = selectFile(tr("Image files (*.svg);;All files (*)"));
    if (!fn.isEmpty()) {
        fn = QDir::fromNativeSeparators(fn);

        _pixmap = QPixmap();
        QFile file(fn);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            _svg.clear();
        }
        else {
            QTextStream stream(&file);
            _svg = stream.readAll();
        }
        showSVG();
    }
}


void ImageEdit::accept()
{
    if (_property) {
        if (_property->getType() == Materials::MaterialValue::SVG) {
            _property->setValue(_svg);
        }
        else {
            QBuffer buffer;
            buffer.open(QIODevice::WriteOnly);
            _pixmap.save(&buffer, "PNG");
            QByteArray base64 = buffer.data().toBase64();
            QString encoded = QString::fromUtf8(base64);
            _property->setValue(encoded);
        }
    }
    QDialog::accept();
}

void ImageEdit::reject()
{
    QDialog::reject();
}

#include "moc_ImageEdit.cpp"
