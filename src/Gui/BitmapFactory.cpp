/***************************************************************************
 *   Copyright (c) 2004 J�rgen Riegel <juergen.riegel@web.de>              *
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
# include <QBitmap>
# include <QDir>
# include <QFile>
# include <QFileInfo>
# include <QMap>
# include <QImageReader>
# include <QPainter>
# include <QPalette>
# include <QSvgRenderer>
# include <QStyleOption>
# include <sstream>
#endif

#ifdef FC_OS_WIN32
#define QTWEBKIT
#endif

#ifdef QTWEBKIT
#include <QWebView>
#include <QWebFrame>
#endif

#include <string>
#include <Inventor/fields/SoSFImage.h>

#include <Base/Console.h>
#include <App/Application.h>

#include "BitmapFactory.h"
#include "Icons/images.cpp"
#include "Icons/Feature.xpm"
#include "Icons/Document.xpm"
#include "Icons/BmpFactoryIcons.cpp"

using namespace Gui;

namespace Gui {
class BitmapFactoryInstP
{
public:
    QMap<std::string, const char**> xpmMap;
    QMap<std::string, QPixmap> xpmCache;
    QStringList paths;
};
}

BitmapFactoryInst* BitmapFactoryInst::_pcSingleton = NULL;

BitmapFactoryInst& BitmapFactoryInst::instance(void)
{
    if (_pcSingleton == NULL)
    {
        _pcSingleton = new BitmapFactoryInst;
        std::map<std::string,std::string>::const_iterator it;
        it = App::GetApplication().Config().find("ProgramIcons");
        if (it != App::GetApplication().Config().end()) {
            QString home = QString::fromUtf8(App::GetApplication().GetHomePath());
            QString path = QString::fromUtf8(it->second.c_str());
            if (QDir(path).isRelative()) {
                path = QFileInfo(QDir(home), path).absoluteFilePath();
            }
            _pcSingleton->addPath(path);
        }
        _pcSingleton->addPath(QString::fromAscii("%1/icons").arg(QString::fromUtf8(App::GetApplication().GetHomePath())));
        _pcSingleton->addPath(QString::fromAscii("%1/icons").arg(QString::fromUtf8(App::GetApplication().Config()["UserAppData"].c_str())));
        _pcSingleton->addPath(QLatin1String(":/icons/"));
        _pcSingleton->addPath(QLatin1String(":/Icons/"));

        RegisterIcons();
    }

    return *_pcSingleton;
}

void BitmapFactoryInst::destruct (void)
{
    if (_pcSingleton != 0)
    delete _pcSingleton;
    _pcSingleton = 0;
}

BitmapFactoryInst::BitmapFactoryInst()
{
    d = new BitmapFactoryInstP;
    restoreCustomPaths();
}

BitmapFactoryInst::~BitmapFactoryInst()
{
    delete d;
}

void BitmapFactoryInst::addCustomPath(const QString& path)
{
    Base::Reference<ParameterGrp> group = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Bitmaps");
    std::vector<std::string> paths = group->GetASCIIs("CustomPath");
    std::stringstream str;
    str << "CustomPath" << paths.size();
    group->SetASCII(str.str().c_str(), (const char*)path.toUtf8());
}

void BitmapFactoryInst::restoreCustomPaths()
{
    Base::Reference<ParameterGrp> group = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Bitmaps");
    std::vector<std::string> paths = group->GetASCIIs("CustomPath");
    for (std::vector<std::string>::iterator it = paths.begin(); it != paths.end(); ++it) {
        addPath(QString::fromUtf8(it->c_str()));
    }
}

void BitmapFactoryInst::addPath(const QString& path)
{
    d->paths.push_back(path);
}

void BitmapFactoryInst::removePath(const QString& path)
{
    int pos = d->paths.indexOf(path);
    if (pos != -1) d->paths.removeAt(pos);
}

QStringList BitmapFactoryInst::findIconFiles() const
{
    QStringList files, filters;
    QList<QByteArray> formats = QImageReader::supportedImageFormats();
    for (QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); ++it)
        filters << QString::fromAscii("*.%1").arg(QString::fromAscii(*it).toLower());

    QStringList paths = d->paths;
#if QT_VERSION >= 0x040500
    paths.removeDuplicates();
#endif
    for (QStringList::ConstIterator pt = paths.begin(); pt != paths.end(); ++pt) {
        QDir d(*pt);
        d.setNameFilters(filters);
        QFileInfoList fi = d.entryInfoList();
        for (QFileInfoList::iterator it = fi.begin(); it != fi.end(); ++it)
            files << it->absoluteFilePath();
    }

    return files;
}

void BitmapFactoryInst::addXPM(const char* name, const char** pXPM)
{
    d->xpmMap[name] = pXPM;
}

void BitmapFactoryInst::addPixmapToCache(const char* name, const QPixmap& icon)
{
    d->xpmCache[name] = icon;
}

bool BitmapFactoryInst::findPixmapInCache(const char* name, QPixmap& px) const
{
    QMap<std::string, QPixmap>::ConstIterator it = d->xpmCache.find(name);
    if (it != d->xpmCache.end()) {
        px = it.value();
        return true;
    }
    return false;
}

bool BitmapFactoryInst::loadPixmap(const QString& filename, QPixmap& icon) const
{
    QFileInfo fi(filename);
    if (fi.exists()) {
        // first check if it's an SVG because Qt's qsvg4 module shouldn't be used therefore
        if (fi.suffix().toLower() == QLatin1String("svg")) {
            QFile svgFile(filename);
            if (svgFile.open(QFile::ReadOnly | QFile::Text)) {
                QByteArray content = svgFile.readAll();
                icon = pixmapFromSvg(content, QSize(64,64));
            }
        }
        else {
            // try with Qt plugins
            icon.load(filename);
        }
    }

    return !icon.isNull();
}

QPixmap BitmapFactoryInst::pixmap(const char* name) const
{
    if (!name || *name == '\0')
        return QPixmap(px);

    // as very first test check whether the pixmap is in the cache
    QMap<std::string, QPixmap>::ConstIterator it = d->xpmCache.find(name);
    if (it != d->xpmCache.end())
        return it.value();

    // now try to find it in the built-in XPM
    QPixmap icon;
    QMap<std::string,const char**>::ConstIterator It = d->xpmMap.find(name);
    if (It != d->xpmMap.end())
        icon = QPixmap(It.value());

    // Try whether an absolute path is given
    QString fn = QString::fromUtf8(name);
    if (icon.isNull())
        loadPixmap(fn, icon);

    // try to find it in the given directories
    if (icon.isNull()) {
        bool found = false;
        QList<QByteArray> formats = QImageReader::supportedImageFormats();
        formats.prepend("SVG"); // check first for SVG to use special import mechanism
        for (QStringList::ConstIterator pt = d->paths.begin(); pt != d->paths.end() && !found; ++pt) {
            QDir d(*pt);
            QString fileName = d.filePath(fn);
            if (loadPixmap(fileName, icon)) {
                found = true;
                break;
            }
            else {
                // Go through supported file formats
                for (QList<QByteArray>::iterator fm = formats.begin(); fm != formats.end(); ++fm) {
                    QString path = QString::fromAscii("%1.%2").arg(fileName).
                        arg(QString::fromAscii((*fm).toLower().constData()));
                    if (loadPixmap(path, icon)) {
                        found = true;
                        break;
                    }
                }
            }
        }
    }

    if (!icon.isNull()) {
        d->xpmCache[name] = icon;
        return icon;
    }

    Base::Console().Warning("Cannot find icon: %s\n", name);
    return QPixmap(px);
}

QPixmap BitmapFactoryInst::pixmapFromSvg(const char* name, const QSize& size) const
{
    // If an absolute path is given
    QPixmap icon;
    QString iconPath;
    QString fn = QString::fromUtf8(name);
    if (QFile(fn).exists())
        iconPath = fn;

    // try to find it in the given directories
    if (iconPath.isEmpty()) {
        for (QStringList::ConstIterator pt = d->paths.begin(); pt != d->paths.end(); ++pt) {
            QDir d(*pt);
            QString fileName = d.filePath(fn);
            if (QFile(fileName).exists()) {
                iconPath = fileName;
                break;
            } else {
                fileName += QLatin1String(".svg");
                if (QFile(fileName).exists()) {
                    iconPath = fileName;
                    break;
                }
            }
        }
    }

    if (!iconPath.isEmpty()) {
        QFile file(iconPath);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QByteArray content = file.readAll();
            icon = pixmapFromSvg(content, size);
        }
    }

    return icon;
}

QPixmap BitmapFactoryInst::pixmapFromSvg(const QByteArray& contents, const QSize& size) const
{
#ifdef QTWEBKIT
    QWebView webView;
    QPalette pal = webView.palette();
    pal.setColor(QPalette::Background, Qt::transparent);
    webView.setPalette(pal);
    webView.setContent(contents, QString::fromAscii("image/svg+xml"));
    QString node = QString::fromAscii("document.rootElement.nodeName");
    QString root = webView.page()->mainFrame()->evaluateJavaScript(node).toString();
    if (root.isEmpty() || root.compare(QLatin1String("svg"), Qt::CaseInsensitive)) {
        return QPixmap();
    }

    QString w = QString::fromAscii("document.rootElement.width.baseVal.value");
    QString h = QString::fromAscii("document.rootElement.height.baseVal.value");
    double ww = webView.page()->mainFrame()->evaluateJavaScript(w).toDouble();
    double hh = webView.page()->mainFrame()->evaluateJavaScript(h).toDouble();
    if (ww == 0.0 || hh == 0.0)
        return QPixmap();
#endif

    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(0x00000000);

    QPainter p(&image);
#ifdef QTWEBKIT
    qreal xs = size.isValid() ? size.width() / ww : 1.0;
    qreal ys = size.isValid() ? size.height() / hh : 1.0;
    p.scale(xs, ys);

    // the best quality
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setOpacity(0); // important to keep transparent background
    webView.page()->mainFrame()->render(&p);
#else
    // tmp. disable the report window to suppress some bothering warnings
    Base::Console().SetEnabledMsgType("ReportOutput", ConsoleMsgType::MsgType_Wrn, false);
    QSvgRenderer svg(contents);
    Base::Console().SetEnabledMsgType("ReportOutput", ConsoleMsgType::MsgType_Wrn, true);
    svg.render(&p);
#endif
    p.end();

    return QPixmap::fromImage(image);
}

QStringList BitmapFactoryInst::pixmapNames() const
{
    QStringList names;
    for (QMap<std::string,const char**>::ConstIterator It = d->xpmMap.begin(); It != d->xpmMap.end(); ++It)
        names << QString::fromUtf8(It.key().c_str());
    for (QMap<std::string, QPixmap>::ConstIterator It = d->xpmCache.begin(); It != d->xpmCache.end(); ++It) {
        QString item = QString::fromUtf8(It.key().c_str());
        if (!names.contains(item))
            names << item;
    }
    return names;
}

QPixmap BitmapFactoryInst::resize(int w, int h, const QPixmap& p, Qt::BGMode bgmode) const
{
    if (bgmode == Qt::TransparentMode) {
        if (p.width() == 0 || p.height() == 0)
            w = 1;

        QPixmap pix = p;
        int x = pix.width () > w ? 0 : (w - pix.width ())/2;
        int y = pix.height() > h ? 0 : (h - pix.height())/2;

        if (x == 0 && y == 0)
            return pix;

        QPixmap pm (w,h);
        QBitmap mask (w,h);
        mask.fill(Qt::color0);

        QBitmap bm = pix.mask();
        if (!bm.isNull())
        {
            QPainter painter(&mask);
            painter.drawPixmap(QPoint(x, y), bm, QRect(0, 0, pix.width(), pix.height()));
            pm.setMask(mask);
        }
        else
        {
            pm.setMask(mask);
            pm = fillRect(x, y, pix.width(), pix.height(), pm, Qt::OpaqueMode);
        }

        QPainter pt;
        pt.begin( &pm );
        pt.drawPixmap(x, y, pix);
        pt.end();
        return pm;
    } else { // Qt::OpaqueMode
        QPixmap pix = p;

        if (pix.width() == 0 || pix.height() == 0)
            return pix; // do not resize a null pixmap

        QPalette pal = qApp->palette();
        QColor dl = pal.color(QPalette::Disabled, QPalette::Light);
        QColor dt = pal.color(QPalette::Disabled, QPalette::Text);

        QPixmap pm = pix;
        pm = QPixmap(w,h);
        pm.fill(dl);

        QPainter pt;
        pt.begin( &pm );
        pt.setPen( dl );
        pt.drawPixmap(1, 1, pix);
        pt.setPen( dt );
        pt.drawPixmap(0, 0, pix);
        pt.end();
        return pm;
    }
}

QPixmap BitmapFactoryInst::fillRect(int x, int y, int w, int h, const QPixmap& p, Qt::BGMode bgmode) const
{
    QBitmap b = p.mask();
    if (b.isNull())
        return p; // sorry, but cannot do anything

    QPixmap pix = p;

    // modify the mask
    QPainter pt;
    pt.begin(&b);
    if (bgmode == Qt::OpaqueMode)
        pt.fillRect(x, y, w, h, Qt::color1); // make opaque
    else // Qt::TransparentMode
        pt.fillRect(x, y, w, h, Qt::color0); // make transparent
    pt.end();

    pix.setMask(b);

    return pix;
}

QPixmap BitmapFactoryInst::merge(const QPixmap& p1, const QPixmap& p2, bool vertical) const
{
    int width = 0;
    int height = 0;

    int x = 0;
    int y = 0;

    // get the size for the new pixmap
    if (vertical) {
        y = p1.height();
        width  = qMax( p1.width(), p2.width() );
        height = p1.height() + p2.height();
    } else {
        x = p1.width();
        width  = p1.width() + p2.width();
        height = qMax( p1.height(), p2.height() );
    }

    QPixmap res( width, height );
    QBitmap mask( width, height );
    QBitmap mask1 = p1.mask();
    QBitmap mask2 = p2.mask();
    mask.fill( Qt::color0 );

    QPainter* pt1 = new QPainter(&res);
    pt1->drawPixmap(0, 0, p1);
    pt1->drawPixmap(x, y, p2);
    delete pt1;

    QPainter* pt2 = new QPainter(&mask);
    pt2->drawPixmap(0, 0, mask1);
    pt2->drawPixmap(x, y, mask2);
    delete pt2;

    res.setMask(mask);
    return res;
}

QPixmap BitmapFactoryInst::merge(const QPixmap& p1, const QPixmap& p2, Position pos) const
{
    // does the similar as the method above except that this method does not resize the resulting pixmap
    int x = 0, y = 0;

    switch (pos)
    {
    case Qt::TopLeftCorner:
        break;
    case Qt::TopRightCorner:
        x = p1.width () - p2.width ();
        break;
    case Qt::BottomLeftCorner:
        y = p1.height() - p2.height();
        break;
    case Qt::BottomRightCorner:
        x = p1.width () - p2.width ();
        y = p1.height() - p2.height();
        break;
    }

    QPixmap p = p1;
    p = fillRect(x, y, p2.width(), p2.height(), p, Qt::OpaqueMode);

    QPainter pt;
    pt.begin( &p );
    pt.setPen(Qt::NoPen);
    pt.drawRect(x, y, p2.width(), p2.height());
    pt.drawPixmap(x, y, p2);
    pt.end();

    return p;
}

QPixmap BitmapFactoryInst::disabled(const QPixmap& p) const
{
    QStyleOption opt;
    opt.palette = QApplication::palette();
    return QApplication::style()->generatedIconPixmap(QIcon::Disabled, p, &opt);
}

void BitmapFactoryInst::convert(const QImage& p, SoSFImage& img) const
{
    SbVec2s size;
    size[0] = p.width();
    size[1] = p.height();

    int buffersize = p.numBytes();
    int numcomponents = buffersize / ( size[0] * size[1] );

    // allocate image data
    img.setValue(size, numcomponents, NULL);

    unsigned char * bytes = img.startEditing(size, numcomponents);

    int width  = (int)size[0];
    int height = (int)size[1];

    for (int y = 0; y < height; y++) 
    {
        unsigned char * line = &bytes[width*numcomponents*(height-(y+1))];
        for (int x = 0; x < width; x++) 
        {
            QRgb rgb = p.pixel(x,y);
            switch (numcomponents) 
            {
            default:
                break;
            case 1:
                line[0] = qGray( rgb );
                break;
            case 2:
                line[0] = qGray( rgb );
                line[1] = qAlpha( rgb );
                break;
            case 3:
                line[0] = qRed( rgb );
                line[1] = qGreen( rgb );
                line[2] = qBlue( rgb );
                break;
            case 4:
                line[0] = qRed( rgb );
                line[1] = qGreen( rgb );
                line[2] = qBlue( rgb );
                line[3] = qAlpha( rgb );
                break;
            }

            line += numcomponents;
        }
    }

    img.finishEditing();
}

void BitmapFactoryInst::convert(const SoSFImage& p, QImage& img) const
{
    SbVec2s size;
    int numcomponents;

    const unsigned char * bytes = p.getValue(size, numcomponents);

    int width  = (int)size[0];
    int height = (int)size[1];

    img = QImage(width, height, QImage::Format_RGB32);
    QRgb * bits = (QRgb*) img.bits();
    
    for (int y = 0; y < height; y++) 
    {
        const unsigned char * line = &bytes[width*numcomponents*(height-(y+1))];
        for (int x = 0; x < width; x++) 
        {
            switch (numcomponents) 
            {
            default:
            case 1:
                *bits++ = qRgb(line[0], line[0], line[0]);
                break;
            case 2:
                *bits++ = qRgba(line[0], line[0], line[0], line[1]);
                break;
            case 3:
                *bits++ = qRgb(line[0], line[1], line[2]);
                break;
            case 4:
                *bits++ = qRgba(line[0], line[1], line[2], line[3]);
                break;
            }

            line += numcomponents;
        }
    }
}

