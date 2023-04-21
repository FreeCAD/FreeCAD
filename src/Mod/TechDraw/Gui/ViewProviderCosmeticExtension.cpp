/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
# ifdef _MSC_VER
#  define _USE_MATH_DEFINES
#  include <cmath>
# endif //_MSC_VER
#endif

#include <Mod/TechDraw/App/CosmeticExtension.h>
#include "ViewProviderCosmeticExtension.h"

using namespace TechDrawGui;

EXTENSION_PROPERTY_SOURCE(TechDrawGui::ViewProviderCosmeticExtension, Gui::ViewProviderExtension)


ViewProviderCosmeticExtension::ViewProviderCosmeticExtension()
{
    initExtensionType(ViewProviderCosmeticExtension::getExtensionClassTypeId());
}

QIcon ViewProviderCosmeticExtension::extensionMergeGreyableOverlayIcons(const QIcon & orig) const
{
    QIcon mergedicon = orig;

    return mergedicon;
}

namespace Gui {
    EXTENSION_PROPERTY_SOURCE_TEMPLATE(TechDrawGui::ViewProviderCosmeticExtensionPython, TechDrawGui::ViewProviderCosmeticExtension)

// explicit template instantiation
    template class TechDrawGuiExport ViewProviderExtensionPythonT<TechDrawGui::ViewProviderCosmeticExtension>;
}
