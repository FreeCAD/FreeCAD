/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef FREECADTASKPANELSCHEME_H
#define FREECADTASKPANELSCHEME_H

#include "actionpanelscheme.h"


namespace QSint
{


class QSINT_EXPORT FreeCADPanelScheme : public ActionPanelScheme
{
public:
    explicit FreeCADPanelScheme();

    static ActionPanelScheme* defaultScheme()
    {
        static FreeCADPanelScheme scheme;
        return &scheme;
    }

    void clearActionStyle();
    void restoreActionStyle();

private:
    QString builtinScheme;
    QString minimumStyle;
    QPixmap builtinFold;
    QPixmap builtinFoldOver;
    QPixmap builtinUnfold;
    QPixmap builtinUnfoldOver;
};

class QSINT_EXPORT SystemPanelScheme : public ActionPanelScheme
{
public:
    explicit SystemPanelScheme();

    static ActionPanelScheme* defaultScheme()
    {
        static SystemPanelScheme scheme;
        return &scheme;
    }

private:
    QPixmap drawFoldIcon(const QPalette& p, bool fold) const;
    QString systemStyle(const QPalette& p) const;
};

}

#endif // IISFREECADTASKPANELSCHEME_H
