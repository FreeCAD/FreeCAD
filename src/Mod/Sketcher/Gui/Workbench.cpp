/***************************************************************************
 *   Copyright (c) 2008 J�rgen Riegel (juergen.riegel@web.de)              *
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
# include <qobject.h>
#endif

#include "Workbench.h"
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>

using namespace SketcherGui;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "Sketcher");
    qApp->translate("Workbench", "Sketcher geometries");
    qApp->translate("Workbench", "Sketcher constraints");
    qApp->translate("Workbench", "Sketcher tools");
#endif

/// @namespace SketcherGui @class Workbench
TYPESYSTEM_SOURCE(SketcherGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

// == Profile menu ==========================================
    Gui::MenuItem* profile = new Gui::MenuItem;
    root->insertItem(item, profile);
    profile->setCommand("P&rofiles");

    *profile << "Sketcher_ProfilesHexagon1";

// == Sketcher menu ==========================================

    Gui::MenuItem* sketch = new Gui::MenuItem;
    root->insertItem(profile, sketch);
    sketch->setCommand("S&ketch");
    Gui::MenuItem* geom = new Gui::MenuItem();
    geom->setCommand("Sketcher geometries");
    addSketcherWorkbenchGeometries( *geom );

    Gui::MenuItem* cons = new Gui::MenuItem();
    cons->setCommand("Sketcher constraints");
    addSketcherWorkbenchConstraints(*cons);

    Gui::MenuItem* consaccel = new Gui::MenuItem();
    consaccel->setCommand("Sketcher tools");
    addSketcherWorkbenchTools(*consaccel);

    addSketcherWorkbenchSketchActions( *sketch );
    *sketch << geom
	    << cons
	    << consaccel;

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();

    Gui::ToolBarItem* part = new Gui::ToolBarItem(root);
    part->setCommand("Sketcher");
    addSketcherWorkbenchSketchActions( *part );

    Gui::ToolBarItem* geom = new Gui::ToolBarItem(root);
    geom->setCommand("Sketcher geometries");
    addSketcherWorkbenchGeometries(*geom);

    Gui::ToolBarItem* cons = new Gui::ToolBarItem(root);
    cons->setCommand("Sketcher constraints");
    addSketcherWorkbenchConstraints( *cons );

    Gui::ToolBarItem* consaccel = new Gui::ToolBarItem(root);
    consaccel->setCommand("Sketcher tools");
    addSketcherWorkbenchTools( *consaccel );
     return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // Part tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    return root;
}


namespace SketcherGui {

template <typename T>
void SketcherAddWorkbenchConstraints( T& cons );
template <typename T>
void Sketcher_addWorkbenchSketchActions( T& sketch );
template <typename T>
void SketcherAddWorkbenchGeometries( T& geom );



template <typename T>
void SketcherAddWorkspaceArcs(T& geom);
template <>
inline void SketcherAddWorkspaceArcs<Gui::MenuItem>(Gui::MenuItem& geom){
    geom    << "Sketcher_CreateArc"
            << "Sketcher_Create3PointArc"
            << "Sketcher_CreateCircle"
            << "Sketcher_Create3PointCircle";
}
template <>
inline void SketcherAddWorkspaceArcs<Gui::ToolBarItem>(Gui::ToolBarItem& geom){
    geom    << "Sketcher_CompCreateArc"
            << "Sketcher_CompCreateCircle";
}
template <typename T>
void SketcherAddWorkspaceRegularPolygon(T& geom);
template <>
inline void SketcherAddWorkspaceRegularPolygon<Gui::MenuItem>(Gui::MenuItem& geom){
    geom    << "Sketcher_CreateTriangle"
            << "Sketcher_CreateSquare"
            << "Sketcher_CreatePentagon"
            << "Sketcher_CreateHexagon"
            << "Sketcher_CreateHeptagon"
            << "Sketcher_CreateOctagon";
}
template <>
inline void SketcherAddWorkspaceRegularPolygon<Gui::ToolBarItem>(Gui::ToolBarItem& geom){
    geom    << "Sketcher_CompCreateRegularPolygon";
}
template <typename T>
inline void SketcherAddWorkbenchGeometries(T& geom){
    geom    << "Sketcher_CreatePoint"
            << "Sketcher_CreateLine";
    SketcherAddWorkspaceArcs( geom );
    geom    << "Separator"
            << "Sketcher_CreatePolyline"
            << "Sketcher_CreateRectangle";
    SketcherAddWorkspaceRegularPolygon( geom );
    geom    << "Sketcher_CreateSlot"
            << "Separator"
            << "Sketcher_CreateFillet"
            << "Sketcher_Trimming"
            << "Sketcher_External"
            << "Sketcher_ToggleConstruction"
            /*<< "Sketcher_CreateText"*/
            /*<< "Sketcher_CreateDraftLine"*/;
}


template <typename T>
inline void SketcherAddWorkbenchConstraints(T& cons){
    cons    << "Sketcher_ConstrainCoincident"
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
inline void SketcherAddWorkbenchTools(T& consaccel);

template <>
inline void SketcherAddWorkbenchTools<Gui::MenuItem>(Gui::MenuItem& consaccel){
    consaccel 	<< "Sketcher_CloseShape"
		<< "Sketcher_ConnectLines"
		<< "Sketcher_SelectConstraints"
		<< "Sketcher_SelectOrigin"
		<< "Sketcher_SelectVerticalAxis"
		<< "Sketcher_SelectHorizontalAxis"
        << "Sketcher_SelectElementsAssociatedWithConstraints";
}
template <>
inline void SketcherAddWorkbenchTools<Gui::ToolBarItem>(Gui::ToolBarItem& consaccel){
    consaccel << "Sketcher_CloseShape"
        << "Sketcher_ConnectLines"
        << "Sketcher_SelectConstraints"
        << "Sketcher_SelectElementsAssociatedWithConstraints";
}

template <typename T>
inline void SketcherAddWorkspaceSketchExtra(T& sketch){
}

template <>
inline void SketcherAddWorkspaceSketchExtra<Gui::MenuItem>(Gui::MenuItem& sketch){
    sketch  << "Sketcher_ReorientSketch"
            << "Sketcher_ValidateSketch";
}

template <typename T>
inline void Sketcher_addWorkbenchSketchActions(T& sketch){
    sketch  << "Sketcher_NewSketch"
            << "Sketcher_EditSketch"
            << "Sketcher_LeaveSketch"
            << "Sketcher_ViewSketch"
            << "Sketcher_MapSketch";
    SketcherAddWorkspaceSketchExtra( sketch );
}



void addSketcherWorkbenchConstraints( Gui::MenuItem& cons ){
    SketcherAddWorkbenchConstraints( cons );
}
void addSketcherWorkbenchTools( Gui::MenuItem& consaccel ){
    SketcherAddWorkbenchTools( consaccel );
}
void addSketcherWorkbenchSketchActions( Gui::MenuItem& sketch ){
    Sketcher_addWorkbenchSketchActions( sketch );
}
void addSketcherWorkbenchGeometries( Gui::MenuItem& geom ){
    SketcherAddWorkbenchGeometries(geom);
}

void addSketcherWorkbenchConstraints( Gui::ToolBarItem& cons ){
    SketcherAddWorkbenchConstraints( cons );
}
void addSketcherWorkbenchTools( Gui::ToolBarItem& consaccel )
{
    SketcherAddWorkbenchTools( consaccel );
}
void addSketcherWorkbenchSketchActions( Gui::ToolBarItem& sketch ){
    Sketcher_addWorkbenchSketchActions( sketch );
}
void addSketcherWorkbenchGeometries( Gui::ToolBarItem& geom ){
    SketcherAddWorkbenchGeometries(geom);
}

} /* namespace SketcherGui */
