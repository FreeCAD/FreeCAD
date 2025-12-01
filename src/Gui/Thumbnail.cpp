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


#include <QApplication>
#include <QBuffer>
#include <QByteArray>
#include <QDateTime>
#include <QImage>
#include <QThread>


#include <App/Application.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#ifdef _MSC_VER
# include <zipios++/zipios-config.h>
#endif
#include <zipios++/zipfile.h>

#include "Thumbnail.h"
#include "BitmapFactory.h"
#include "View3DInventorViewer.h"


using namespace Gui;

Thumbnail::Thumbnail(int s)
    : size(s)
{}

Thumbnail::~Thumbnail() = default;

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

unsigned int Thumbnail::getMemSize() const
{
    return 0;
}

void Thumbnail::Save(Base::Writer& writer) const
{
    // It's only possible to add extra information if force of XML is disabled
    if (!writer.isForceXML()) {
        writer.addFile("thumbnails/Thumbnail.png", this);
    }
}

void Thumbnail::Restore(Base::XMLReader& reader)
{
    Q_UNUSED(reader);
    // reader.addFile("Thumbnail.png",this);
}

void Thumbnail::SaveDocFile(Base::Writer& writer) const
{
    QImage img;
    bool created = false;

    // 1. Try to create the thumbnail from the viewer
    if (this->viewer) {
        if (this->viewer->thread() != QThread::currentThread()) {
            qWarning("Cannot create a thumbnail from non-GUI thread");
        }
        else {
            QColor invalid;
            this->viewer->imageFromFramebuffer(this->size, this->size, 4, invalid, img);
            created = !img.isNull();
        }
    }

    // 2. If creation failed (e.g. no viewer or background thread), try to restore from the existing file
    if (!created) {
        QString filename = this->uri.toLocalFile();
        Base::FileInfo fi(filename.toUtf8().constData());
        if (fi.exists()) {
            try {
                zipios::ZipFile zf(fi.filePath());
                // getEntry uses default MatchPath=MATCH.
                zipios::ConstEntryPointer entry = zf.getEntry("thumbnails/Thumbnail.png");
                if (entry && entry->isValid()) {
                    // getInputStream returns a pointer that must be deleted
                    std::istream* is = zf.getInputStream(entry);
                    if (is) {
                        if (is->good()) {
                            writer.Stream() << is->rdbuf();
                            delete is;
                            return;
                        }
                        delete is;
                    }
                }
            }
            catch (const std::exception&) {
                // If the file isn't a zip or is locked, we ignore it and proceed to fallback
            }
            catch (...) {
                // Ignore unknown exceptions
            }
        }
    }

    // If we still have no image and no viewer to generate one, we can do nothing more
    if (!this->viewer) {
        return;
    }

    // Get app icon and resize to half size to insert in topbottom position over the current view
    // snapshot
    QPixmap appIcon = Gui::BitmapFactory().pixmap(App::Application::Config()["AppIcon"].c_str());
    QPixmap px = appIcon;
    if (!img.isNull()) {
        // Create a small "Fc" Application icon in the bottom right of the thumbnail
        if (App::GetApplication()
                .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                ->GetBool("AddThumbnailLogo", false)) {
            // only scale app icon if an offscreen image could be created
            appIcon = appIcon.scaled(
                this->size / 4,
                this->size / 4,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );
            px = BitmapFactory().merge(QPixmap::fromImage(img), appIcon, BitmapFactoryInst::BottomRight);
        }
        else {
            px = QPixmap::fromImage(img);
        }
    }

    if (!px.isNull()) {
        // according to specification add some meta-information to the image
        qint64 mt = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
        QString mtime = QStringLiteral("%1").arg(mt);
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
}

void Thumbnail::RestoreDocFile(Base::Reader& reader)
{
    Q_UNUSED(reader);
}
