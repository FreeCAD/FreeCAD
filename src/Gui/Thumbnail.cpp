/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <QBuffer>
# include <QByteArray>
# include <QDateTime>
# include <QImage>
# include <QGLFramebufferObject>
#endif

#include "Thumbnail.h"
#include "BitmapFactory.h"
#include "View3DInventorViewer.h"
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <App/Application.h>

using namespace Gui;

Thumbnail::Thumbnail(int s) : viewer(0), size(s)
{
}

Thumbnail::~Thumbnail()
{
}

void Thumbnail::setViewer(View3DInventorViewer* v)
{
    this->viewer = v;
}

void Thumbnail::setSize(int s)
{
    this->size = s;
}

void Thumbnail::setFileName(const char* fn)
{
    this->uri = QUrl::fromLocalFile(QString::fromUtf8(fn));
}

unsigned int Thumbnail::getMemSize (void) const
{
    return 0;
}

void Thumbnail::Save (Base::Writer &writer) const
{
    // It's only possible to add extra information if force of XML is disabled
    if (writer.isForceXML() == false)
        writer.addFile("thumbnails/Thumbnail.png", this);
}

void Thumbnail::Restore(Base::XMLReader &reader)
{
    //reader.addFile("Thumbnail.png",this);
}

void Thumbnail::SaveDocFile (Base::Writer &writer) const
{
    if (!this->viewer)
        return;
    QImage img;
    if (App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Document")->GetBool("DisablePBuffers",false)) {
        this->createThumbnailFromFramebuffer(img);
    }
    else {
        try {
            this->viewer->savePicture(this->size, this->size, View3DInventorViewer::Current, img);
        }
        catch (...) {
            this->createThumbnailFromFramebuffer(img);
        }
    }

    QPixmap px = Gui::BitmapFactory().pixmap(App::Application::Config()["AppIcon"].c_str());
    px = BitmapFactory().merge(QPixmap::fromImage(img),px,BitmapFactoryInst::BottomRight);

    // according to specification add some meta-information to the image
    uint mt = QDateTime::currentDateTime().toTime_t();
    QString mtime = QString::fromAscii("%1").arg(mt);
    img.setText(QLatin1String("Software"), qApp->applicationName());
    img.setText(QLatin1String("Thumb::Mimetype"), QLatin1String("application/x-extension-fcstd"));
    img.setText(QLatin1String("Thumb::MTime"), mtime);
    img.setText(QLatin1String("Thumb::URI"), this->uri.toString());

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    px.save(&buffer, "PNG");
    writer.Stream().write(ba.constData(), ba.length());
}

void Thumbnail::RestoreDocFile(Base::Reader &reader)
{
}

void Thumbnail::createThumbnailFromFramebuffer(QImage& img) const
{
    // Alternative way of off-screen rendering
    QGLFramebufferObject fbo(this->size, this->size,QGLFramebufferObject::Depth);
    this->viewer->renderToFramebuffer(&fbo);
    img = fbo.toImage();
}
