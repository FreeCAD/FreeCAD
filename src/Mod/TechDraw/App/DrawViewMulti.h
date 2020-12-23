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

#ifndef _DrawViewMulti_h_
#define _DrawViewMulti_h_

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyFile.h>
#include <App/FeaturePython.h>
#include <App/Material.h>

#include <TopoDS_Compound.hxx>

#include "DrawViewPart.h"

class gp_Pln;
class TopoDS_Face;

namespace TechDraw
{
//class Face;
}

namespace TechDraw
{


/** Base class of all View Features in the drawing module
 */
class TechDrawExport DrawViewMulti : public DrawViewPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::DrawViewMulti);

public:
    /// Constructor
    DrawViewMulti(void);
    virtual ~DrawViewMulti();
  
    App::PropertyLinkList    Sources;

    virtual short mustExecute() const override;
    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void) override;
    virtual void onChanged(const App::Property* prop) override;
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderViewPart";
    }

protected:
    TopoDS_Compound m_compound;

//    void getParameters(void);
};

typedef App::FeaturePythonT<DrawViewMulti> DrawViewMultiPython;

} //namespace TechDraw

#endif
