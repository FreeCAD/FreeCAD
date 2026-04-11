/***************************************************************************
 *   Copyright (c) 2012 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include "ViewProviderDatum.h"

#include "ParamHandler.h"

class SoCoordinate3;
class SoTranslation;

namespace Gui
{
class SoFrameLabel;
}

namespace Gui
{

class GuiExport ViewProviderLine: public ViewProviderDatum
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderLine);

public:
    /// Constructor
    ViewProviderLine();
    ~ViewProviderLine() override;

    void attach(App::DocumentObject*) override;

private:
    void updateLineSize();

protected:
    CoinPtr<SoFrameLabel> pLabel;
    CoinPtr<SoCoordinate3> pCoords;
    CoinPtr<SoTranslation> pLabelTranslation;

    ParamHandlers handlers;
};

}  // namespace Gui
