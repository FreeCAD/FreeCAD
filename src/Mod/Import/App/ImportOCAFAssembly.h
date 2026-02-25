// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <map>
#include <set>
#include <string>
#include <vector>

#include <TopoDS_Shape.hxx>

#include <App/Material.h>
#include <Mod/Import/ImportGlobal.h>


class TDF_Label;
class TopLoc_Location;

namespace App
{
class Document;
class DocumentObject;
}  // namespace App
namespace Part
{
class Feature;
}

namespace Import
{

class ImportExport ImportOCAFAssembly
{
public:
    ImportOCAFAssembly(
        Handle(TDocStd_Document) h,
        App::Document* d,
        const std::string& name,
        App::DocumentObject* target
    );
    virtual ~ImportOCAFAssembly();
    void loadShapes();
    void loadAssembly();

protected:
    std::string getName(const TDF_Label& label);
    App::DocumentObject* targetObj;


private:
    void loadShapes(
        const TDF_Label& label,
        const TopLoc_Location&,
        const std::string& partname,
        const std::string& assembly,
        bool isRef,
        int dep
    );
    void createShape(const TDF_Label& label, const TopLoc_Location&, const std::string&);
    void createShape(const TopoDS_Shape& label, const TopLoc_Location&, const std::string&);
    virtual void applyColors(Part::Feature*, const std::vector<Base::Color>&)
    {}

private:
    Handle(TDocStd_Document) pDoc;
    App::Document* doc;
    Handle(XCAFDoc_ShapeTool) aShapeTool;
    Handle(XCAFDoc_ColorTool) aColorTool;
    std::string default_name;
    std::set<int> myRefShapes;
};


}  // namespace Import
