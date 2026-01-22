/***************************************************************************
 *   Copyright (c) 2014 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#if BUILD_VR

# include "CoinRiftWidget.h"

namespace Gui
{

class View3DInventorRiftViewer: public CoinRiftWidget
{
public:
    View3DInventorRiftViewer();
    ~View3DInventorRiftViewer();

    virtual void setSceneGraph(SoNode* sceneGraph);

protected:
    SoGroup* workplace;
    SoTranslation* translation;
    SoRotationXYZ* rotation1;
    SoRotationXYZ* rotation2;
    SoScale* scale;

protected:
    void keyPressEvent(QKeyEvent*);
};


}  // namespace Gui

#endif  // BUILD_VR
