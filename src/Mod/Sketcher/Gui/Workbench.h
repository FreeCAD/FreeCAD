/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef IMAGE_WORKBENCH_H
#define IMAGE_WORKBENCH_H

#include <Gui/Workbench.h>
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>

namespace SketcherGui {

/**
 * @author Werner Mayer
 */
class SketcherGuiExport Workbench : public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER();

public:
    Workbench();
    virtual ~Workbench();

protected:
    Gui::MenuItem* setupMenuBar() const;
    Gui::ToolBarItem* setupToolBars() const;
    Gui::ToolBarItem* setupCommandBars() const;
};



template <typename T>
void SketcherAddWorkspaceArcs(T& geom);
template <>
inline void SketcherAddWorkspaceArcs<Gui::MenuItem>(Gui::MenuItem& geom){
	geom 	<< "Sketcher_CreateArc"
			<< "Sketcher_Create3PointArc"
			<< "Sketcher_CreateCircle"
			<< "Sketcher_Create3PointCircle";
}
template <>
inline void SketcherAddWorkspaceArcs<Gui::ToolBarItem>(Gui::ToolBarItem& geom){
	geom 	<< "Sketcher_CompCreateArc"
			<< "Sketcher_CompCreateCircle";
}

template <typename T>
inline void SketcherAddWorkspaceGeometries(T& geom){
	geom 	<< "Sketcher_CreatePoint"
			<< "Sketcher_CreateLine";
	SketcherAddWorkspaceArcs<T>( geom );
	geom	<< "Separator"
			<< "Sketcher_CreatePolyline"
			<< "Sketcher_CreateRectangle"
			<< "Sketcher_CreateHexagon"
			<< "Sketcher_CreateSlot"
			<< "Separator"
			<< "Sketcher_CreateFillet"
			<< "Sketcher_Trimming"
			<< "Sketcher_External"
			<< "Sketcher_ToggleConstruction"
			/*<< "Sketcher_CreateText"*/
			/*<< "Sketcher_CreateDraftLine"*/;
}

template <typename T>
inline void SketcherAddWorkspaceConstraints(T& cons){
	cons 	<< "Sketcher_ConstrainCoincident"
			<< "Sketcher_ConstrainPointOnObject"
			<< "Sketcher_ConstrainVertical"
			<< "Sketcher_ConstrainHorizontal"
			<< "Sketcher_ConstrainParallel"
			<< "Sketcher_ConstrainPerpendicular"
			<< "Sketcher_ConstrainTangent"
			<< "Sketcher_ConstrainEqual"
			<< "Sketcher_ConstrainSymmetric"
			<< "Separator"
			<< "Sketcher_ConstrainLock"
			<< "Sketcher_ConstrainDistanceX"
			<< "Sketcher_ConstrainDistanceY"
			<< "Sketcher_ConstrainDistance"
			<< "Sketcher_ConstrainRadius"
			<< "Sketcher_ConstrainAngle";
}

template <typename T>
inline void SketcherAddWorkspaceSketchExtra(T& sketch){
}

template <>
inline void SketcherAddWorkspaceSketchExtra<Gui::MenuItem>(Gui::MenuItem& sketch){
	sketch.operator <<( "Sketcher_ReorientSketch" )
			<< "Sketcher_ValidateSketch";
}

template <typename T>
inline void SketcherAddWorkspaceSketchitems(T& sketch){
	sketch	<< "Sketcher_NewSketch"
			<< "Sketcher_LeaveSketch"
			<< "Sketcher_ViewSketch"
			<< "Sketcher_MapSketch";
	SketcherAddWorkspaceSketchExtra( sketch );
}


} // namespace SketcherGui


#endif // IMAGE_WORKBENCH_H 
