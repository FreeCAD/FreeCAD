/***************************************************************************
 *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
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

#ifndef DRAFTDXF_H
#define DRAFTDXF_H

#include "dxf.h"
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Draft/DraftGlobal.h>
#include <App/Document.h>
#include <gp_Pnt.hxx>

namespace DraftUtils
{
    class DraftUtilsExport DraftDxfRead : public CDxfRead
    {
    public:
        DraftDxfRead(std::string filepath, App::Document *pcDoc);
    
        // CDxfRead's virtual functions
        void OnReadLine(const double* s, const double* e, bool hidden);
        void OnReadPoint(const double* s);
        void OnReadText(const double* point, const double height, const char* text);
        void OnReadArc(const double* s, const double* e, const double* c, bool dir, bool hidden);
        void OnReadCircle(const double* s, const double* c, bool dir, bool hidden);
        void OnReadEllipse(const double* c, double major_radius, double minor_radius, double rotation, double start_angle, double end_angle, bool dir);
        void OnReadSpline(struct SplineData& sd);
        void OnReadInsert(const double* point, const double* scale, const char* name, double rotation);
        void OnReadDimension(const double* s, const double* e, const double* point, double rotation);
        void AddGraphics() const;
    
        // FreeCAD-specific functions
        void AddObject(Part::TopoShape *shape); //Called by OnRead functions to add Part objects
        std::string Deformat(const char* text); // Removes DXF formatting from texts
        
    private:
        gp_Pnt makePoint(const double* p);
        
    protected:
        App::Document *document;
        bool optionGroupLayers;
        bool optionImportAnnotations;
        double optionScaling;
        std::map <std::string, std::vector <Part::TopoShape*> > layers;
    };
}

#endif // DRAFTDXF_H
