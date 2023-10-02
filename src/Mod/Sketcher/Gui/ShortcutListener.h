/***************************************************************************
 *   Copyright (c) 2018 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHERGUI_SHORTCUTLISTENER_H
#define SKETCHERGUI_SHORTCUTLISTENER_H

#include <QObject>


namespace SketcherGui
{

class ViewProviderSketch;

class ViewProviderSketchShortcutListenerAttorney
{
private:
    static inline void deleteSelected(ViewProviderSketch& vp);


    friend class ShortcutListener;
};

class ShortcutListener: public QObject
{
    // Q_OBJECT

public:
    explicit ShortcutListener(ViewProviderSketch* vp);
    ~ShortcutListener() override;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

    ViewProviderSketch* pViewProvider;
};

}  // namespace SketcherGui


#endif  // SKETCHERGUI_SHORTCUTLISTENER_H
