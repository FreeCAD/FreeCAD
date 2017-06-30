/***************************************************************************
 *   Copyright (c) Emmanuel Delage - CNRS <delage[at]clermont.in2p3.fr>    *
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

//#pragma once
#include "./persistency/G4GDMLReadStructure.hh"
#include "./persistency/G4GDMLParser.hh"

#include "./geometry/management/G4LogicalVolume.hh"
#include "./geometry/management/G4VPhysicalVolume.hh"
#include "./persistency/G4GDMLReadStructure.hh"
#include "./global/G4Types.hh" //G4String
#include "./geometry/management/G4LogicalVolumeStore.hh"
#include "./geometry/management/G4PhysicalVolumeStore.hh"

#include "./geometry/solid/G4Box.hh"
#include "./geometry/solid/G4Tubs.hh"
#include "./geometry/solid/G4Cons.hh"
#include "./geometry/solid/Boolean/G4UnionSolid.hh"


#include "./geometry/management/G4AffineTransform.hh"


#include "geometry/volume/G4PVPlacement.hh"


#include "../PreCompiled.h"
#include <App/Document.h>


class G4FreeCAD
{
public:
//	G4FreeCAD(G4GDMLParser*);
	G4FreeCAD();
	~G4FreeCAD(void);

//	void GDML2FreeCAD(G4GDMLParser*, char *);
	void GDML2FreeCAD(G4GDMLParser*, std::string);
	void FreeCAD2GDML(G4GDMLParser*, G4String  );
//	void FreeCAD2GDML(G4GDMLParser*, G4String ,std::string );

private:
//   int browsePhysVol(G4LogicalVolume *,G4AffineTransform , App::Document *);
   int browsePhysVol(G4LogicalVolume *,const G4AffineTransform *at, App::Document *) const;
   
   //int 
	   G4PVPlacement * browseDoc( App::Document *) const;

   G4GDMLParser *Parser;
};
