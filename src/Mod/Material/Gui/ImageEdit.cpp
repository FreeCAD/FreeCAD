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
#include <QMessageBox>
#endif

#include <QBuffer>
#include <QMenu>
#include <QPixmap>
#include <QString>

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
    QLabel::setPixmap(pixmap);
}

void ImageLabel::resizeEvent(QResizeEvent* event)
{
    QPixmap px = _pixmap.scaled(event->size(), Qt::KeepAspectRatio);
    QLabel::setPixmap(px);
    QLabel::resizeEvent(event);
}

//===

ImageEdit::ImageEdit(const QString& propertyName,
                     const std::shared_ptr<Materials::Material>& material,
                     QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_ImageEdit)
    , _material(material)
    , _pixmap(QString::fromStdString(":/images/default_image.png"))
{
    ui->setupUi(this);

    if (material->hasPhysicalProperty(propertyName)) {
        _property = material->getPhysicalProperty(propertyName);
    }
    else if (material->hasAppearanceProperty(propertyName)) {
        _property = material->getAppearanceProperty(propertyName);
    }
    else {
        Base::Console().Log("Property '%s' not found\n", propertyName.toStdString().c_str());
        _property = nullptr;
    }
    if (_property) {
        QString value = _property->getString();
        if (!value.isEmpty()) {
            QByteArray by = QByteArray::fromBase64(value.toUtf8());
            QImage img = QImage::fromData(by, "PNG");
            _pixmap = QPixmap::fromImage(img);
        }
    }
    else {
        Base::Console().Log("No value loaded\n");
    }
    showPixmap();

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

void ImageEdit::onFileSelect(bool checked)
{
    Q_UNUSED(checked)

    QFileDialog::Options dlgOpt;
    if (Gui::DialogOptions::dontUseNativeFileDialog()) {
        dlgOpt = QFileDialog::DontUseNativeDialog;
    }

    QString directory = Gui::FileDialog::getWorkingDirectory();
    QString fn = Gui::FileDialog::getOpenFileName(
        this,
        tr("Select an image"),
        directory,
        tr("Image files (*.jpg *.jpeg *.png *.bmp);;All files (*)"),
        nullptr,
        dlgOpt);

    if (!fn.isEmpty()) {
        fn = QDir::fromNativeSeparators(fn);
        Gui::FileDialog::setWorkingDirectory(fn);

        _pixmap = QPixmap(fn);
        showPixmap();
    }
}


void ImageEdit::accept()
{
    if (_property) {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        _pixmap.save(&buffer, "PNG");
        QByteArray base64 = buffer.data().toBase64();
        QString encoded = QString::fromUtf8(base64);
        _property->setValue(encoded);
    }
    QDialog::accept();
}

void ImageEdit::reject()
{
    QDialog::reject();
}

#include "moc_ImageEdit.cpp"
