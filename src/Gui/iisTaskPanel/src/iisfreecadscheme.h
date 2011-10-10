/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef IISFREECADTASKPANELSCHEME_H
#define IISFREECADTASKPANELSCHEME_H

#include "iistaskpanelscheme.h"

#include "iistaskpanel_global.h"


class IISTASKPANEL_EXPORT iisFreeCADTaskPanelScheme : public iisTaskPanelScheme
{
public:
    iisFreeCADTaskPanelScheme(QObject *parent=0);
    ~iisFreeCADTaskPanelScheme();

    static iisTaskPanelScheme* defaultScheme();

protected:
    static iisFreeCADTaskPanelScheme *myDefaultXPScheme;
    QPixmap drawFoldIcon(const QPalette&) const;
};

#endif // IISFREECADTASKPANELSCHEME_H
