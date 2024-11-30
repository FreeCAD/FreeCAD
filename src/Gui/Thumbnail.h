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


#ifndef GUI_THUMBNAIL_H
#define GUI_THUMBNAIL_H

#include <Base/Persistence.h>
#include <QUrl>

class QImage;

namespace Gui {
class View3DInventorViewer;

class Thumbnail : public Base::Persistence
{
public:
    Thumbnail(int s = 128);
    ~Thumbnail() override;

    void setViewer(View3DInventorViewer*);
    void setSize(int);
    void setFileName(const char*);

    /** @name I/O of the document */
    //@{
    unsigned int getMemSize () const override;
    /// This method is used to save properties or very small amounts of data to an XML document.
    void Save (Base::Writer &writer) const override;
    /// This method is used to restore properties from an XML document.
    void Restore(Base::XMLReader &reader) override;
    /// This method is used to save large amounts of data to a binary file.
    void SaveDocFile (Base::Writer &writer) const override;
    /// This method is used to restore large amounts of data from a binary file.
    void RestoreDocFile(Base::Reader &reader) override;
    //@}

private:
    QUrl uri;
    View3DInventorViewer* viewer{nullptr};
    int size;
};

}

#endif // GUI_THUMBNAIL_H
