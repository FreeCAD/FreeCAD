/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_VIEWPROVIDERBUILDER_H
#define GUI_VIEWPROVIDERBUILDER_H

#include <vector>
#include <map>
#include <memory>
#include <Base/Type.h>
#include <Base/Factory.h>

class SoNode;

namespace App {
    class Property;
}

namespace Gui
{

class ViewProvider;
class GuiExport ViewProviderBuilder
{
public:
    /// Constructor
    ViewProviderBuilder(void);
    virtual ~ViewProviderBuilder();
    virtual void buildNodes(const App::Property*, std::vector<SoNode*>&) const = 0;

    static void add(const Base::Type&, const Base::Type&);
    static ViewProvider* create(const Base::Type&);

private:
    static std::map<Base::Type, Base::Type> _prop_to_view;
};

class GuiExport ViewProviderColorBuilder : public ViewProviderBuilder
{
public:
    /// Constructor
    ViewProviderColorBuilder(void);
    virtual ~ViewProviderColorBuilder();
    virtual void buildNodes(const App::Property*, std::vector<SoNode*>&) const;
};

} //namespace Gui


#endif // GUI_VIEWPROVIDERBUILDER_H
