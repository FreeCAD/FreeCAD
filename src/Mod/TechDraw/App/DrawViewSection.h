/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
 *   Copyright (c) Luke Parry             (l.parry@warwick.ac.uk) 2013     *
 *   Copyright (c) WandererFan            (wandererfan@gmail.com) 2016     *
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

#ifndef _DrawViewSection_h_
#define _DrawViewSection_h_

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyFile.h>
#include <App/FeaturePython.h>
#include <App/Material.h>

#include <TopoDS_Compound.hxx>

#include "DrawViewPart.h"

class gp_Pln;
class TopoDS_Face;

namespace TechDrawGeometry
{
class Face;
}

namespace TechDraw
{


/** Base class of all View Features in the drawing module
 */
class TechDrawExport DrawViewSection : public DrawViewPart
{
    PROPERTY_HEADER(Part::DrawViewSection);

public:
    /// Constructor
    DrawViewSection(void);
    virtual ~DrawViewSection();

    App::PropertyLink   BaseView;
    App::PropertyVector SectionNormal;
    App::PropertyVector SectionOrigin;
    App::PropertyEnumeration SectionDirection;
    App::PropertyBool   ShowCutSurface;
    App::PropertyColor  CutSurfaceColor;
    App::PropertyBool   HatchCutSurface;
    App::PropertyFile   HatchPattern;
    App::PropertyColor  HatchColor;
    App::PropertyString SectionSymbol;

    virtual short mustExecute() const;
    bool isReallyInBox (const Base::Vector3d v, const Base::BoundBox3d bb) const;
    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void);
    virtual void onChanged(const App::Property* prop);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderViewSection";
    }

public:
    std::vector<TechDrawGeometry::Face*> getFaceGeometry();
    static Base::Vector3d getSectionVector (const Base::Vector3d baseViewDir, const std::string sectionName);

    static const char* SectionDirEnums[];

protected:
    TopoDS_Compound sectionFaces;

    gp_Pln getSectionPlane() const;
    TopoDS_Compound findSectionPlaneIntersections(const TopoDS_Shape& shape);
    TopoDS_Face projectFace(const TopoDS_Shape &face,
                                     gp_Pnt faceCenter,
                                     const Base::Vector3d &direction);
    void getParameters(void);
};

typedef App::FeaturePythonT<DrawViewSection> DrawViewSectionPython;

} //namespace TechDraw

#endif
