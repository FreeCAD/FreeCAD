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


#pragma once

#include <Base/Factory.h>
#include <QPixmap>
#include <QIcon>

// forward declaration
class SoSFImage;
class QImage;

namespace Gui
{

using ColorMap = std::map<unsigned long, unsigned long>;

/** The Bitmap Factory
 * the main purpose is to collect all build in Bitmaps and
 * hold all paths for the extern bitmaps (files) to serve
 * as a single point of accessing bitmaps in FreeCAD
 * \author Werner Mayer, Jürgen Riegel
 */
class BitmapFactoryInstP;
class GuiExport BitmapFactoryInst: public Base::Factory
{
public:
    enum Position
    {
        TopLeft,    /**< Place to the top left corner */
        TopRight,   /**< Place to the top right corner */
        BottomLeft, /**< Place to the bottom left corner */
        BottomRight /**< Place to the bottom right corner */
    };

    static BitmapFactoryInst& instance();
    static void destruct();

    /// Adds a path where pixmaps can be found
    void addPath(const QString& path);
    /// Removes a path from the list of pixmap paths
    void removePath(const QString& path);
    /// Returns the list of search paths
    QStringList getPaths() const;
    /// Returns the absolute file names of icons found in the given search paths
    QStringList findIconFiles() const;
    /// Adds a build in XPM pixmap under a given name
    void addPixmapToCache(const char* name, const QPixmap& icon);
    /// Checks whether the pixmap is already registered.
    bool findPixmapInCache(const char* name, QPixmap& icon) const;
    /** Returns the QIcon corresponding to name in the current icon theme.
     * If no such icon is found in the current theme fallback is returned instead.
     */
    QIcon iconFromTheme(const char* name, const QIcon& fallback = QIcon());
    /** Returns the QIcon corresponding to name in the default (FreeCAD's) icon theme.
     * If no such icon is found in the current theme fallback is returned instead.
     */
    QIcon iconFromDefaultTheme(const char* name, const QIcon& fallback = QIcon());
    /// Retrieves a pixmap by name
    QPixmap pixmap(const char* name) const;
    /** Retrieves a pixmap by name and size created by an
     * scalable vector graphics (SVG).
     *
     * @param colorMapping - a dictionary of substitute colors.
     * Can be used to customize icon color scheme, e.g. crosshair color
     */
    QPixmap pixmapFromSvg(
        const char* name,
        const QSizeF& size,
        const ColorMap& colorMapping = ColorMap()
    ) const;
    /** This method is provided for convenience and does the same
     * as the method above except that it creates the pixmap from
     * a byte array.
     * @param colorMapping - see above.
     */
    QPixmap pixmapFromSvg(
        const QByteArray& contents,
        const QSizeF& size,
        const ColorMap& colorMapping = ColorMap()
    ) const;
    /** Returns the names of all registered pixmaps.
     * To get the appropriate pixmaps call pixmap() for each name.
     */
    QStringList pixmapNames() const;
    /** Resizes the area of a pixmap
     * If the new size is greater than the old one the pixmap
     * will be placed in the center. The border area will be made
     * depending on \a bgmode transparent or opaque.
     */
    QPixmap resize(int w, int h, const QPixmap& p, Qt::BGMode bgmode) const;
    /** Creates an opaque or transparent area in a pixmap
     * If the background mode is opaque then this method can
     * be used for drawing a smaller pixmap into pixmap \a p.
     * Note: To draw a smaller pixmap into another one the
     * area in the resulting pixmap for the small pixmapmust
     * be opaque in every pixel, otherwise the drawing may fail.
     *
     * If the background mode is transparent then this method can
     * be used for resizing the pixmap \a p and make the new space
     * transparent.
     */
    QPixmap fillRect(int x, int y, int w, int h, const QPixmap& p, Qt::BGMode) const;
    /** Merges the two pixmaps  \a p1 and \a p2 to one pixmap in
     * vertical order if \a vertical is true, in horizontal order
     * otherwise. The method resizes the resulting pixmap.
     */
    QPixmap merge(const QPixmap& p1, const QPixmap& p2, bool vertical) const;
    /** Merges the two pixmaps  \a p1 and \a p2 to one pixmap.
     * The position of the smaller pixmap \a p2 is drawn into the given
     * position \a pos of the bigger pixmap \a p1. This method does not
     * resize the resulting pixmap.
     */
    QPixmap merge(const QPixmap& p1, const QPixmap& p2, Position pos = BitmapFactoryInst::BottomLeft) const;
    /** Creates a disabled pixmap of the given pixmap \a p by changing the brightness
     * of all opaque pixels to a higher value.
     */
    QPixmap disabled(const QPixmap& p) const;

    /** Creates an empty pixmap, takes care of DPI and clearing out the image.
     */
    QPixmap empty(QSize size) const;

    /** Converts a QImage into a SoSFImage to use it inside a SoImage node.
     */
    void convert(const QImage& img, SoSFImage& out) const;
    /** Converts a SoSFImage into a QImage.
     */
    void convert(const SoSFImage& img, QImage& out) const;

    /// Helper method to merge a pixmap into one corner of a QIcon
    static QIcon mergePixmap(
        const QIcon& base,
        const QPixmap& px,
        Gui::BitmapFactoryInst::Position position
    );

    static qreal getMaximumDPR();

private:
    bool loadPixmap(const QString& path, QPixmap&) const;
    void restoreCustomPaths();
    void configureUseIconTheme();

    static BitmapFactoryInst* _pcSingleton;
    BitmapFactoryInst();
    ~BitmapFactoryInst() override;

    BitmapFactoryInstP* d;
};

/// Get the global instance
inline BitmapFactoryInst& BitmapFactory()
{
    return BitmapFactoryInst::instance();
}

}  // namespace Gui
