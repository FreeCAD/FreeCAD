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

#ifndef DrawProjGroupItem_h_
#define DrawProjGroupItem_h_

#include <gp_Ax2.hxx>

#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

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
    ~DrawProjGroupItem() override = default;

    App::PropertyEnumeration Type;
    App::PropertyVector      RotationVector;    //this is superseded by dvp xdirection

    short mustExecute() const override;
    void onDocumentRestored() override;
    void unsetupObject() override;

    void postHlrTasks(void) override;

    DrawProjGroup* getPGroup() const;
    double getRotateAngle();
    Base::Vector3d getXDirection() const override;
    Base::Vector3d getLegacyX(const Base::Vector3d& pt,
                              const Base::Vector3d& axis,
                              const bool flip = true)  const override;

    App::DocumentObjectExecReturn *execute() override;

    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderProjGroupItem";
    }
    //return PyObject as DrawProjGroupItemPy
    PyObject *getPyObject() override;

    gp_Ax2 getViewAxis(const Base::Vector3d& pt,
                       const Base::Vector3d& direction,
                       const bool flip=true) const override;

    double getScale() const override;
    void autoPosition();
    bool isAnchor() const;

    //DPGI always fits on page since DPG handles scaling
    bool checkFit() const override { return true; }
    bool checkFit(DrawPage* page) const override { (void) page;         //avoid unused variable warning
                                                   return true; }

    int countParentPages() const override;
    DrawPage* findParentPage() const override;
    std::vector<DrawPage*> findAllParentPages() const override;

protected:
    void onChanged(const App::Property* prop) override;
    bool isLocked() const override;
    bool showLock() const override;

private:
    static const char* TypeEnums[];
};

} //namespace TechDraw

#endif
