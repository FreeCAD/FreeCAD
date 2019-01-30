/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef _DrawProjGroupItem_h_
#define _DrawProjGroupItem_h_

#include <gp_Ax2.hxx>

#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <App/FeaturePython.h>
#include "DrawViewPart.h"

namespace TechDraw
{

enum ProjItemType{ Front,
          Left,
          Right,
          Rear,
          Top,
          Bottom,
          FrontTopLeft,
          FrontTopRight,
          FrontBottomLeft,
          FrontBottomRight };
          
class DrawProjGroup;

class TechDrawExport DrawProjGroupItem : public TechDraw::DrawViewPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawProjGroupItem);

public:
    /// Constructor
    DrawProjGroupItem();
    ~DrawProjGroupItem();

    App::PropertyEnumeration Type;
    App::PropertyVector      RotationVector;

    short mustExecute() const override;
    virtual void onDocumentRestored() override;
    virtual void unsetupObject() override;

    DrawProjGroup* getPGroup(void) const;
    double getRotateAngle();

    virtual App::DocumentObjectExecReturn *execute(void) override;
    virtual const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderProjGroupItem";
    }
    //return PyObject as DrawProjGroupItemPy
    virtual PyObject *getPyObject(void) override;

    virtual gp_Ax2 getViewAxis(const Base::Vector3d& pt,
                               const Base::Vector3d& direction, 
                               const bool flip=true) const override;

    virtual double getScale(void) const override;
    void autoPosition(void);
    bool isAnchor(void);


protected:
    void onChanged(const App::Property* prop) override;
    virtual bool isLocked(void) const override;

private:
    static const char* TypeEnums[];
};

} //namespace TechDraw

#endif
