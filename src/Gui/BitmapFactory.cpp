/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <QBitmap>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QImageReader>
#include <QPainter>
#include <QPalette>
#include <QScreen>
#include <QString>
#include <QSvgRenderer>
#include <QStyleOption>

#include <string>
#include <Inventor/fields/SoSFImage.h>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/ConsoleObserver.h>

#include "BitmapFactory.h"

using namespace Gui;

namespace Gui
{
class BitmapFactoryInstP
{
public:
    QMap<std::string, QPixmap> xpmCache;

    bool useIconTheme;
    QString externalThemePath;
    bool externalThemeEnabled {false};
    bool preferExternal {true};
};
}  // namespace Gui

BitmapFactoryInst* BitmapFactoryInst::_pcSingleton = nullptr;

BitmapFactoryInst& BitmapFactoryInst::instance()
{
    if (!_pcSingleton) {
        _pcSingleton = new BitmapFactoryInst;
        std::map<std::string, std::string>::const_iterator it;
        it = App::GetApplication().Config().find("ProgramIcons");
        if (it != App::GetApplication().Config().end()) {
            QString home = QString::fromStdString(App::Application::getHomePath());
            QString path = QString::fromUtf8(it->second.c_str());
            if (QDir(path).isRelative()) {
                path = QFileInfo(QDir(home), path).absoluteFilePath();
            }
            _pcSingleton->addPath(path);
        }
        _pcSingleton->addPath(
            QStringLiteral("%1/icons").arg(QString::fromStdString(App::Application::getHomePath()))
        );
        _pcSingleton->addPath(
            QStringLiteral("%1/icons").arg(QString::fromStdString(App::Application::getUserAppDataDir()))
        );
        _pcSingleton->addPath(QLatin1String(":/icons/"));
        _pcSingleton->addPath(QLatin1String(":/Icons/"));
    }

    return *_pcSingleton;
}

void BitmapFactoryInst::destruct()
{
    if (_pcSingleton) {
        delete _pcSingleton;
    }
    _pcSingleton = nullptr;
}

BitmapFactoryInst::BitmapFactoryInst()
{
    d = new BitmapFactoryInstP;

    restoreCustomPaths();
    configureUseIconTheme();
    configureExternalTheme();
}

BitmapFactoryInst::~BitmapFactoryInst()
{
    delete d;
}

void BitmapFactoryInst::restoreCustomPaths()
{
    Base::Reference<ParameterGrp> group = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Bitmaps"
    );
    std::vector<std::string> paths = group->GetASCIIs("CustomPath");
    for (auto& path : paths) {
        addPath(QString::fromUtf8(path.c_str()));
    }
}

void Gui::BitmapFactoryInst::configureUseIconTheme()
{
    Base::Reference<ParameterGrp> group = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Bitmaps/Theme"
    );

    d->useIconTheme = group->GetBool("UseIconTheme", group->GetBool("ThemeSearchPaths", false));
}

void Gui::BitmapFactoryInst::configureExternalTheme()
{
    Base::Reference<ParameterGrp> group = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Bitmaps/ExternalTheme"
    );

    d->externalThemeEnabled = group->GetBool("Enabled", false);
    d->preferExternal = group->GetBool("PreferExternal", true);
    QString path = QString::fromUtf8(group->GetASCII("Path", "").c_str());

    const auto isEnabledValue = [](const QString& value) {
        const QString normalized = value.trimmed().toLower();
        return normalized != QLatin1String("0") && normalized != QLatin1String("false")
            && normalized != QLatin1String("no") && normalized != QLatin1String("off");
    };

    const bool enabledOverrideSet = qEnvironmentVariableIsSet("FREECAD_EXTERNAL_ICON_THEME_ENABLED");
    if (enabledOverrideSet) {
        d->externalThemeEnabled = isEnabledValue(
            qEnvironmentVariable("FREECAD_EXTERNAL_ICON_THEME_ENABLED")
        );
    }

    if (qEnvironmentVariableIsSet("FREECAD_EXTERNAL_ICON_THEME_PREFER_EXTERNAL")) {
        d->preferExternal = isEnabledValue(
            qEnvironmentVariable("FREECAD_EXTERNAL_ICON_THEME_PREFER_EXTERNAL")
        );
    }

    if (qEnvironmentVariableIsSet("FREECAD_EXTERNAL_ICON_THEME")) {
        path = qEnvironmentVariable("FREECAD_EXTERNAL_ICON_THEME");
        if (!enabledOverrideSet && !path.isEmpty()) {
            d->externalThemeEnabled = true;
        }
    }

    if (path.isEmpty()) {
        d->externalThemePath.clear();
        return;
    }

    const QFileInfo pathInfo(path);
    if (!pathInfo.exists() || !pathInfo.isDir()) {
        d->externalThemePath.clear();
        d->externalThemeEnabled = false;
        return;
    }

    d->externalThemePath = pathInfo.absoluteFilePath();
}

void BitmapFactoryInst::addPath(const QString& path)
{
    QDir::addSearchPath(QStringLiteral("icons"), path);
}

void BitmapFactoryInst::removePath(const QString& path)
{
    QStringList iconPaths = QDir::searchPaths(QStringLiteral("icons"));
    int pos = iconPaths.indexOf(path);
    if (pos != -1) {
        iconPaths.removeAt(pos);
        QDir::setSearchPaths(QStringLiteral("icons"), iconPaths);
    }
}

QStringList BitmapFactoryInst::getPaths() const
{
    return QDir::searchPaths(QStringLiteral("icons"));
}

QStringList BitmapFactoryInst::findIconFiles() const
{
    QStringList files, filters;
    QList<QByteArray> formats = QImageReader::supportedImageFormats();
    for (QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); ++it) {
        filters << QStringLiteral("*.%1").arg(QString::fromLatin1(*it).toLower());
    }

    QStringList paths = QDir::searchPaths(QStringLiteral("icons"));
    paths.removeDuplicates();
    for (QStringList::Iterator pt = paths.begin(); pt != paths.end(); ++pt) {
        QDir d(*pt);
        d.setNameFilters(filters);
        QFileInfoList fi = d.entryInfoList();
        for (QFileInfoList::iterator it = fi.begin(); it != fi.end(); ++it) {
            files << it->absoluteFilePath();
        }
    }

    files.removeDuplicates();
    return files;
}

void BitmapFactoryInst::addPixmapToCache(const char* name, const QPixmap& icon)
{
    d->xpmCache[name] = icon;
}

bool BitmapFactoryInst::findPixmapInCache(const char* name, QPixmap& px) const
{
    QMap<std::string, QPixmap>::Iterator it = d->xpmCache.find(name);
    if (it != d->xpmCache.end()) {
        px = it.value();
        return true;
    }
    return false;
}

void BitmapFactoryInst::clearCache()
{
    d->xpmCache.clear();
}

void BitmapFactoryInst::reloadExternalThemeSettings()
{
    configureExternalTheme();
    clearCache();
}

QIcon BitmapFactoryInst::iconFromTheme(const char* name, const QIcon& fallback)
{
    const QString iconName = QString::fromUtf8(name);

    if (d->externalThemeEnabled && d->preferExternal) {
        const QString externalPath = findInExternalTheme(iconName);
        if (!externalPath.isEmpty()) {
            QPixmap externalPixmap;
            if (loadPixmap(externalPath, externalPixmap)) {
                return QIcon(externalPixmap);
            }
        }
    }

    if (!d->useIconTheme) {
        return iconFromDefaultTheme(name, fallback);
    }

    QIcon icon = QIcon::fromTheme(iconName, fallback);
    if (icon.isNull()) {
        const QPixmap px = pixmap(name);
        if (!px.isNull()) {
            icon.addPixmap(px);
        }
    }

    return icon;
}

bool BitmapFactoryInst::loadPixmap(const QString& filename, QPixmap& icon) const
{
    QFileInfo fi(filename);
    if (fi.exists()) {
        // first check if it's an SVG because Qt's qsvg4 module shouldn't be used therefore
        if (fi.suffix().toLower() == QLatin1String("svg")) {
            QFile svgFile(fi.filePath());
            if (svgFile.open(QFile::ReadOnly | QFile::Text)) {
                QByteArray content = svgFile.readAll();
                icon = pixmapFromSvg(content, QSize(64, 64));
            }
        }
        else {
            // try with Qt plugins
            icon.load(fi.filePath());
        }
    }

    return !icon.isNull();
}

QString BitmapFactoryInst::findInExternalTheme(const QString& name) const
{
    if (!d->externalThemeEnabled || d->externalThemePath.isEmpty()) {
        return {};
    }

    const QFileInfo themeInfo(d->externalThemePath);
    if (!themeInfo.exists() || !themeInfo.isDir()) {
        return {};
    }

    const QFileInfo inputInfo(name);
    const QString completeName = inputInfo.fileName();
    const QString baseName = inputInfo.completeBaseName().isEmpty() ? completeName
                                                                    : inputInfo.completeBaseName();

    QStringList candidates;
    if (!completeName.isEmpty()) {
        candidates << completeName;
    }
    if (!baseName.isEmpty()) {
        candidates << baseName << (baseName + QLatin1String(".svg"))
                   << (baseName + QLatin1String(".png")) << (baseName + QLatin1String(".xpm"));
    }
    candidates.removeDuplicates();

    const QStringList subdirectories = {
        QString(),
        QStringLiteral("icons"),
        QStringLiteral("Icons"),
        QStringLiteral("images"),
        QStringLiteral("Images"),
    };

    for (const QString& subdirectory : subdirectories) {
        QDir directory(d->externalThemePath);
        if (!subdirectory.isEmpty() && !directory.cd(subdirectory)) {
            continue;
        }

        for (const QString& candidate : candidates) {
            const QFileInfo fileInfo(directory.absoluteFilePath(candidate));
            if (fileInfo.exists() && fileInfo.isFile()) {
                return fileInfo.absoluteFilePath();
            }
        }
    }

    QDirIterator iterator(
        d->externalThemePath,
        QStringList {QStringLiteral("*.svg"), QStringLiteral("*.png"), QStringLiteral("*.xpm")},
        QDir::Files,
        QDirIterator::Subdirectories
    );
    while (iterator.hasNext()) {
        const QFileInfo fileInfo(iterator.next());
        if ((!completeName.isEmpty()
             && fileInfo.fileName().compare(completeName, Qt::CaseInsensitive) == 0)
            || (!baseName.isEmpty()
                && fileInfo.completeBaseName().compare(baseName, Qt::CaseInsensitive) == 0)) {
            return fileInfo.absoluteFilePath();
        }
    }

    return {};
}

QPixmap BitmapFactoryInst::findPixmapNoFallback(const char* name) const
{
    if (!name || *name == '\0') {
        return {};
    }

    QMap<std::string, QPixmap>::Iterator it = d->xpmCache.find(name);
    if (it != d->xpmCache.end()) {
        return it.value();
    }

    QPixmap icon;
    QString fn = QString::fromUtf8(name);

    loadPixmap(fn, icon);

    if (icon.isNull() && d->externalThemeEnabled && d->preferExternal) {
        const QString externalPath = findInExternalTheme(fn);
        if (!externalPath.isEmpty()) {
            loadPixmap(externalPath, icon);
        }
    }

    if (icon.isNull()) {
        QList<QByteArray> formats = QImageReader::supportedImageFormats();
        formats.prepend("SVG");

        QString fileName = QStringLiteral("icons:") + fn;
        if (!loadPixmap(fileName, icon)) {
            for (QList<QByteArray>::iterator fm = formats.begin(); fm != formats.end(); ++fm) {
                QString path = QStringLiteral("%1.%2").arg(
                    fileName,
                    QString::fromLatin1((*fm).toLower().constData())
                );
                if (loadPixmap(path, icon)) {
                    break;
                }
            }
        }
    }

    if (icon.isNull() && d->externalThemeEnabled && !d->preferExternal) {
        const QString externalPath = findInExternalTheme(fn);
        if (!externalPath.isEmpty()) {
            loadPixmap(externalPath, icon);
        }
    }

    if (!icon.isNull()) {
        d->xpmCache[name] = icon;
    }

    return icon;
}

QIcon Gui::BitmapFactoryInst::iconFromDefaultTheme(const char* name, const QIcon& fallback)
{
    QIcon icon;
    const QPixmap px = findPixmapNoFallback(name);

    if (!px.isNull()) {
        icon.addPixmap(px);
        return icon;
    }

    return fallback;
}

QPixmap BitmapFactoryInst::pixmap(const char* name) const
{
    if (!name || *name == '\0') {
        return {};
    }

    // as very first test check whether the pixmap is in the cache
    QMap<std::string, QPixmap>::Iterator it = d->xpmCache.find(name);
    if (it != d->xpmCache.end()) {
        return it.value();
    }

    QPixmap icon;

    // Try whether an absolute path is given
    QString fn = QString::fromUtf8(name);
    loadPixmap(fn, icon);

    if (icon.isNull() && d->externalThemeEnabled && d->preferExternal) {
        const QString externalPath = findInExternalTheme(fn);
        if (!externalPath.isEmpty()) {
            loadPixmap(externalPath, icon);
        }
    }

    // try to find it in the 'icons' search paths
    if (icon.isNull()) {
        QList<QByteArray> formats = QImageReader::supportedImageFormats();
        formats.prepend("SVG");  // check first for SVG to use special import mechanism

        QString fileName = QStringLiteral("icons:") + fn;
        if (!loadPixmap(fileName, icon)) {
            // Go through supported file formats
            for (QList<QByteArray>::iterator fm = formats.begin(); fm != formats.end(); ++fm) {
                QString path = QStringLiteral("%1.%2").arg(
                    fileName,
                    QString::fromLatin1((*fm).toLower().constData())
                );
                if (loadPixmap(path, icon)) {
                    break;
                }
            }
        }
    }

    if (icon.isNull() && d->externalThemeEnabled && !d->preferExternal) {
        const QString externalPath = findInExternalTheme(fn);
        if (!externalPath.isEmpty()) {
            loadPixmap(externalPath, icon);
        }
    }

    if (!icon.isNull()) {
        d->xpmCache[name] = icon;
        return icon;
    }

    Base::Console().warning("Cannot find icon: %s\n", name);
    return QPixmap(Gui::BitmapFactory().pixmapFromSvg("help-browser", QSize(16, 16)));
}

QPixmap BitmapFactoryInst::pixmapFromSvg(
    const char* name,
    const QSizeF& size,
    const ColorMap& colorMapping
) const
{
    static qreal dpr = getMaximumDPR();

    // If an absolute path is given
    QPixmap icon;
    QString iconPath;
    QString fn = QString::fromUtf8(name);
    if (QFile(fn).exists()) {
        iconPath = fn;
    }

    if (iconPath.isEmpty() && d->externalThemeEnabled && d->preferExternal) {
        iconPath = findInExternalTheme(fn);
    }

    // try to find it in the 'icons' search paths
    if (iconPath.isEmpty()) {
        QString fileName = QStringLiteral("icons:") + fn;
        QFileInfo fi(fileName);
        if (fi.exists()) {
            iconPath = fi.filePath();
        }
        else {
            fileName += QLatin1String(".svg");
            fi.setFile(fileName);
            if (fi.exists()) {
                iconPath = fi.filePath();
            }
        }
    }

    if (iconPath.isEmpty() && d->externalThemeEnabled && !d->preferExternal) {
        iconPath = findInExternalTheme(fn);
    }

    if (!iconPath.isEmpty()) {
        QFile file(iconPath);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QByteArray content = file.readAll();
            icon = pixmapFromSvg(content, size * dpr, colorMapping);
        }
    }

    if (!icon.isNull()) {
        icon.setDevicePixelRatio(dpr);
    }

    return icon;
}

QPixmap BitmapFactoryInst::pixmapFromSvg(
    const QByteArray& originalContents,
    const QSizeF& size,
    const ColorMap& colorMapping
) const
{
    QString stringContents = QString::fromUtf8(originalContents);
    for (const auto& colorToColor : colorMapping) {
        ulong fromColor = colorToColor.first;
        ulong toColor = colorToColor.second;
        QString fromColorString = QStringLiteral("#%1").arg(fromColor, 6, 16, QChar::fromLatin1('0'));
        QString toColorString = QStringLiteral("#%1").arg(toColor, 6, 16, QChar::fromLatin1('0'));
        stringContents = stringContents.replace(fromColorString, toColorString);
    }
    QByteArray contents = stringContents.toUtf8();

    QImage image(size.toSize(), QImage::Format_ARGB32_Premultiplied);
    image.fill(0x00000000);

    QPainter p(&image);
    QSvgRenderer svg;
    {
        // tmp. disable the report window to suppress some bothering warnings
        const Base::ILoggerBlocker blocker("ReportOutput", Base::ConsoleSingleton::MsgType_Wrn);
        svg.load(contents);
    }
    svg.render(&p);
    p.end();

    return QPixmap::fromImage(image);
}

QStringList BitmapFactoryInst::pixmapNames() const
{
    QStringList names;
    for (QMap<std::string, QPixmap>::Iterator It = d->xpmCache.begin(); It != d->xpmCache.end();
         ++It) {
        QString item = QString::fromUtf8(It.key().c_str());
        if (!names.contains(item)) {
            names << item;
        }
    }
    return names;
}

QPixmap BitmapFactoryInst::resize(int w, int h, const QPixmap& p, Qt::BGMode bgmode) const
{
    if (bgmode == Qt::TransparentMode) {
        if (p.width() == 0 || p.height() == 0) {
            w = 1;
        }

        QPixmap pix = p;
        int x = pix.width() > w ? 0 : (w - pix.width()) / 2;
        int y = pix.height() > h ? 0 : (h - pix.height()) / 2;

        if (x == 0 && y == 0) {
            return pix;
        }

        QPixmap pm(w, h);
        QBitmap mask(w, h);
        mask.fill(Qt::color0);

        QBitmap bm = pix.mask();
        if (!bm.isNull()) {
            QPainter painter(&mask);
            painter.drawPixmap(QPoint(x, y), bm, QRect(0, 0, pix.width(), pix.height()));
            pm.setMask(mask);
        }
        else {
            pm.setMask(mask);
            pm = fillRect(x, y, pix.width(), pix.height(), pm, Qt::OpaqueMode);
        }

        QPainter pt;
        pt.begin(&pm);
        pt.drawPixmap(x, y, pix);
        pt.end();
        return pm;
    }
    else {  // Qt::OpaqueMode
        QPixmap pix = p;

        if (pix.width() == 0 || pix.height() == 0) {
            return pix;  // do not resize a null pixmap
        }

        QPalette pal = qApp->palette();
        QColor dl = pal.color(QPalette::Disabled, QPalette::Light);
        QColor dt = pal.color(QPalette::Disabled, QPalette::Text);

        QPixmap pm(w, h);
        pm.fill(dl);

        QPainter pt;
        pt.begin(&pm);
        pt.setPen(dl);
        pt.drawPixmap(1, 1, pix);
        pt.setPen(dt);
        pt.drawPixmap(0, 0, pix);
        pt.end();
        return pm;
    }
}

QPixmap BitmapFactoryInst::fillRect(int x, int y, int w, int h, const QPixmap& p, Qt::BGMode bgmode) const
{
    QBitmap b = p.mask();
    if (b.isNull()) {
        return p;  // sorry, but cannot do anything
    }

    QPixmap pix = p;

    // modify the mask
    QPainter pt;
    pt.begin(&b);
    if (bgmode == Qt::OpaqueMode) {
        pt.fillRect(x, y, w, h, Qt::color1);  // make opaque
    }
    else {                                    // Qt::TransparentMode
        pt.fillRect(x, y, w, h, Qt::color0);  // make transparent
    }
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
        width = qMax(p1.width(), p2.width());
        height = p1.height() + p2.height();
    }
    else {
        x = p1.width();
        width = p1.width() + p2.width();
        height = qMax(p1.height(), p2.height());
    }

    QPixmap res(width, height);
    QBitmap mask(width, height);
    QBitmap mask1 = p1.mask();
    QBitmap mask2 = p2.mask();
    mask.fill(Qt::color0);

    auto* pt1 = new QPainter(&res);
    pt1->drawPixmap(0, 0, p1);
    pt1->drawPixmap(x, y, p2);
    delete pt1;

    auto* pt2 = new QPainter(&mask);
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
    qreal dpr1 = p1.devicePixelRatio();
    qreal dpr2 = p2.devicePixelRatio();

    switch (pos) {
        case TopLeft:
            break;
        case TopRight:
            x = p1.width() / dpr1 - p2.width() / dpr2;
            break;
        case BottomLeft:
            y = p1.height() / dpr1 - p2.height() / dpr2;
            break;
        case BottomRight:
            x = p1.width() / dpr1 - p2.width() / dpr2;
            y = p1.height() / dpr1 - p2.height() / dpr2;
            break;
    }

    QPixmap p = p1;
    p = fillRect(x, y, p2.width(), p2.height(), p, Qt::OpaqueMode);

    QPainter pt;
    pt.begin(&p);
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

QPixmap BitmapFactoryInst::empty(QSize size) const
{
    qreal dpr = getMaximumDPR();

    QPixmap res(size * dpr);
    res.fill(Qt::transparent);
    res.setDevicePixelRatio(dpr);

    return res;
}

void BitmapFactoryInst::convert(const QImage& p, SoSFImage& img) const
{
    SbVec2s size;
    size[0] = p.width();
    size[1] = p.height();

    int buffersize = static_cast<int>(p.sizeInBytes());

    int numcomponents = 0;
    QVector<QRgb> table = p.colorTable();
    if (!table.isEmpty()) {
        if (p.hasAlphaChannel()) {
            if (p.allGray()) {
                numcomponents = 2;
            }
            else {
                numcomponents = 4;
            }
        }
        else {
            if (p.allGray()) {
                numcomponents = 1;
            }
            else {
                numcomponents = 3;
            }
        }
    }
    else {
        numcomponents = buffersize / (size[0] * size[1]);
    }

    int depth = numcomponents;

    // Coin3D only supports up to 32-bit images
    if (numcomponents == 8) {
        numcomponents = 4;
    }

    // allocate image data
    img.setValue(size, numcomponents, nullptr);

    unsigned char* bytes = img.startEditing(size, numcomponents);

    int width = (int)size[0];
    int height = (int)size[1];

    for (int y = 0; y < height; y++) {
        unsigned char* line = &bytes[width * numcomponents * (height - (y + 1))];
        for (int x = 0; x < width; x++) {
            QColor col = p.pixelColor(x, y);
            switch (depth) {
                default:
                    break;
                case 1: {
                    QRgb rgb = col.rgb();
                    line[0] = qGray(rgb);
                } break;
                case 2: {
                    QRgb rgb = col.rgba();
                    line[0] = qGray(rgb);
                    line[1] = qAlpha(rgb);
                } break;
                case 3: {
                    QRgb rgb = col.rgb();
                    line[0] = qRed(rgb);
                    line[1] = qGreen(rgb);
                    line[2] = qBlue(rgb);
                } break;
                case 4: {
                    QRgb rgb = col.rgba();
                    line[0] = qRed(rgb);
                    line[1] = qGreen(rgb);
                    line[2] = qBlue(rgb);
                    line[3] = qAlpha(rgb);
                } break;
                case 8: {
                    QRgba64 rgb = col.rgba64();
                    line[0] = qRed(rgb);
                    line[1] = qGreen(rgb);
                    line[2] = qBlue(rgb);
                    line[3] = qAlpha(rgb);
                } break;
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

    const unsigned char* bytes = p.getValue(size, numcomponents);
    if (!bytes) {
        return;
    }

    int width = (int)size[0];
    int height = (int)size[1];

    img = QImage(width, height, QImage::Format_RGB32);
    QRgb* bits = (QRgb*)img.bits();

    for (int y = 0; y < height; y++) {
        const unsigned char* line = &bytes[width * numcomponents * (height - (y + 1))];
        for (int x = 0; x < width; x++) {
            switch (numcomponents) {
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

QIcon BitmapFactoryInst::mergePixmap(
    const QIcon& base,
    const QPixmap& px,
    Gui::BitmapFactoryInst::Position position
)
{
    QIcon overlayedIcon;

    int w = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
    auto sizes = base.availableSizes();
    if (sizes.size() == 1) {
        w = sizes.front().width();
    }

    overlayedIcon.addPixmap(
        Gui::BitmapFactory().merge(base.pixmap(w, w, QIcon::Normal, QIcon::Off), px, position),
        QIcon::Normal,
        QIcon::Off
    );

    overlayedIcon.addPixmap(
        Gui::BitmapFactory().merge(base.pixmap(w, w, QIcon::Normal, QIcon::On), px, position),
        QIcon::Normal,
        QIcon::Off
    );

    return overlayedIcon;
}

qreal BitmapFactoryInst::getMaximumDPR()
{
    qreal dpr = 1.0F;

    for (QScreen* screen : QGuiApplication::screens()) {
        dpr = std::max(screen->devicePixelRatio(), dpr);
    }

    return dpr;
}
