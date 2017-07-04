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

#include "G4FreeCAD.h"

#include "../PreCompiled.h"
#include <Base/Console.h>

//#include "../../App/PartFeature.h"
//#include "../../App/TopoShape.h"
//#include "DlgPartCylinderImp.h"
#include <Base/Exception.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Command.h>


#include <BRepLib.hxx>
#include <gp_GTrsf.hxx>
///#include <BRepPrimAPI_MakeBox.hxx>
///# include <BRepPrimAPI_MakeCylinder.hxx>
/*# include <BRepPrimAPI_MakeCone.hxx>
# include <BRepPrimAPI_MakeSphere.hxx>
# include <BRepPrimAPI_MakeTorus.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_GTransform.hxx>
# include <GCE2d_MakeSegment.hxx>
# include <Geom_Plane.hxx>
# include <Geom_ConicalSurface.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom2d_Line.hxx>
# include <Geom2d_TrimmedCurve.hxx>
# include <Handle_Geom_Plane.hxx>
# include <Handle_Geom_CylindricalSurface.hxx>
# include <Handle_Geom2d_Line.hxx>
# include <Handle_Geom2d_TrimmedCurve.hxx>
# include <Precision.hxx>
# include <Standard_Real.hxx>*/
# include <Standard_Version.hxx>
///#include <TopoDS_Solid.hxx>

#include <Base/Placement.h>
#include <Base/Rotation.h>

/*BEGIN******wmayer**********/
//#include "../../../Part/App/TopoShape.h"
#include "../../../Part/App/FeaturePartBox.h"
#include "../../../Part/App/FeaturePartBoolean.h"
/*END********wmayer**********/
#include "../../../Part/App/PartFeature.h"
#include "materials/G4Material.hh"
#include "geometry/volume/G4PVPlacement.hh"


using namespace Part;

//G4FreeCAD::G4FreeCAD(G4GDMLParser* parser)
G4FreeCAD::G4FreeCAD()
{
}

G4FreeCAD::~G4FreeCAD(void)
{
}

//void G4FreeCAD::GDML2FreeCAD(G4GDMLParser* parser, char *fileNameWoExt)
void G4FreeCAD::GDML2FreeCAD(G4GDMLParser* parser, std::string fileNameWoExt)
{
	Parser=parser;
	G4GDMLReadStructure * Reader=Parser->GetReader();
	//G4String temp=(Reader->GetVolume(Reader->GetSetup("Default")))->GetName();

	G4LogicalVolume *wvolume=Reader->GetVolume(Reader->GetSetup("Default"));
	G4LogicalVolume *tvolume=wvolume;

//	App::Document *pcDoc = App::GetApplication().newDocument("Unnamed_toto");
	App::Document *pcDoc = App::GetApplication().newDocument(fileNameWoExt.c_str());
	int num=1;

        G4AffineTransform vat_null;
        //G4RotationMatrix() => initialized to identity?
        vat_null.SetNetRotation(G4RotationMatrix());//const G4RotationMatrix &rot
        vat_null.SetNetTranslation(G4ThreeVector(0,0,0));//const G4ThreeVector &tlate
        num=browsePhysVol(wvolume,&vat_null,pcDoc);//read GDML

        /* //////////////////////////////////////////////////////////////^M
        //Tests for writing
        G4PVPlacement *pVol=browseDoc(pcDoc);//write GDML
	Base::Console().Message("pVol=%s\n",pVol->GetName().c_str());
	Base::Console().Message("lVol=%s\n",pVol->GetLogicalVolume()->GetName().c_str());
//	Base::Console().Message("filename=%s\n",filename.c_str());
	parser->Write("write.gdml",pVol);//parser.GetWorldVolume());
	////////////////////////////////////////////////////////////// */
}

//void G4FreeCAD::FreeCAD2GDML(G4GDMLParser* parser,G4String filename,std::string fileNameWoExt)
void G4FreeCAD::FreeCAD2GDML(G4GDMLParser* parser,G4String filename)
{
	int num=1;
//	App::Document *pcDoc = App::GetApplication().getDocument("Unnamed_toto");
	App::Document *pcDoc = App::GetApplication().getActiveDocument();

	G4PVPlacement *pVol=browseDoc(pcDoc);//write GDML
	Base::Console().Message("pVol=%s\n",pVol->GetName().c_str());

	parser->Write(filename,pVol);//parser.GetWorldVolume());
}

////////////////////////////////////////////////Read
int G4FreeCAD::browsePhysVol(G4LogicalVolume *lv,const G4AffineTransform *at, App::Document * pcDoc) const
{
	//See G4LogicalVolume IsAncestor() for better for!
		int num=lv->GetNoDaughters();
		for (int ind=0;ind<num;ind++)
		{
			G4VPhysicalVolume* pv=lv->GetDaughter(ind);
			G4LogicalVolume * lvd=pv->GetLogicalVolume();

			//Compound transformation
			G4AffineTransform at_vide;
			at_vide.SetNetRotation(G4RotationMatrix());//const G4RotationMatrix &rot
			at_vide.SetNetTranslation(G4ThreeVector(0,0,0));//const G4ThreeVector &tlate
//			G4AffineTransform at_temp=G4AffineTransform(pv->GetObjectRotation(),pv->GetObjectTranslation());
			G4AffineTransform at_temp=G4AffineTransform(pv->GetObjectRotation()->inverse(),pv->GetObjectTranslation());
			if (*at!=at_vide) at_temp=at_temp*(*at);

			if (lvd->GetNoDaughters()==0) 
			{
				if (lvd->GetSolid()->GetEntityType()==G4String("G4Tubs"))
				{
///////////////////////////////////////////////////////////////////////////
//////////////////////////////G4Tubs/////////////////////////////////////////
					G4Tubs * tbs = dynamic_cast<G4Tubs*>(lvd->GetSolid());

					Part::Cylinder *pcFeature;
					pcFeature = static_cast<Part::Cylinder*>(pcDoc->addObject("Part::Cylinder",
						lvd->GetSolid()->GetName().c_str()));

					/*BEGIN******wmayer**********/
					pcFeature->Radius.setValue(tbs->GetRMax());
					//pcFeature->Height.setValue(tbs->GetDz());
                    pcFeature->Height.setValue(tbs->GetDz()*2.);
					pcFeature->Angle.setValue(360.*tbs->GetDPhi()/(D_PI*2.));
					/*END********wmayer**********/

					G4ThreeVector  g4axis;
					double delta;
					at_temp.NetRotation().getAngleAxis(delta,g4axis);

					double convTransX=at_temp.NetTranslation().getX();
					double convTransY=at_temp.NetTranslation().getY();
					double convTransZ=at_temp.NetTranslation().getZ();

					gp_Vec veclocation(convTransX,convTransY,convTransZ-tbs->GetDz());
					gp_Trsf trsf;

					trsf.SetRotation(gp_Ax1(gp_Pnt(convTransX,convTransY,convTransZ),
/** /											gp_Dir((double)g4axis.getX(),
												(double)g4axis.getY(),
												(double)g4axis.getZ())),
/**//**/											gp_Dir(-(double)g4axis.getX(),
												-(double)g4axis.getY(),
												-(double)g4axis.getZ())),
	/**/											delta);

					gp_Trsf t;
					t.SetTranslation(veclocation);
					trsf *=t;          

					/*BEGIN******wmayer**********/
                    // I will put this into utility class
                    //{
                    Base::Matrix4D mtrx;
#if OCC_VERSION_HEX >= 0x070000
                    gp_Mat m = trsf.VectorialPart();
                    gp_XYZ p = trsf.TranslationPart();

                    // set Rotation matrix
                    mtrx[0][0] = m(1,1);
                    mtrx[0][1] = m(1,2);
                    mtrx[0][2] = m(1,3);

                    mtrx[1][0] = m(2,1);
                    mtrx[1][1] = m(2,2);
                    mtrx[1][2] = m(2,3);

                    mtrx[2][0] = m(3,1);
                    mtrx[2][1] = m(3,2);
                    mtrx[2][2] = m(3,3);

                    // set pos vector
                    mtrx[0][3] = p.X();
                    mtrx[1][3] = p.Y();
                    mtrx[2][3] = p.Z();
#else
                    gp_Mat m = trsf._CSFDB_Getgp_Trsfmatrix();
                    gp_XYZ p = trsf._CSFDB_Getgp_Trsfloc();
                    Standard_Real scale = trsf._CSFDB_Getgp_Trsfscale();

                    // set Rotation matrix
                    mtrx[0][0] = scale * m._CSFDB_Getgp_Matmatrix(0,0);
                    mtrx[0][1] = scale * m._CSFDB_Getgp_Matmatrix(0,1);
                    mtrx[0][2] = scale * m._CSFDB_Getgp_Matmatrix(0,2);

                    mtrx[1][0] = scale * m._CSFDB_Getgp_Matmatrix(1,0);
                    mtrx[1][1] = scale * m._CSFDB_Getgp_Matmatrix(1,1);
                    mtrx[1][2] = scale * m._CSFDB_Getgp_Matmatrix(1,2);

                    mtrx[2][0] = scale * m._CSFDB_Getgp_Matmatrix(2,0);
                    mtrx[2][1] = scale * m._CSFDB_Getgp_Matmatrix(2,1);
                    mtrx[2][2] = scale * m._CSFDB_Getgp_Matmatrix(2,2);

                    // set pos vector
                    mtrx[0][3] = p._CSFDB_Getgp_XYZx();
                    mtrx[1][3] = p._CSFDB_Getgp_XYZy();
                    mtrx[2][3] = p._CSFDB_Getgp_XYZz();
#endif
                    //}

                    pcFeature->Placement.setValue(Base::Placement(mtrx));
					/*END********wmayer**********/					
				} //end of if (lvd->GetSolid()->GetEntityType()==G4String("G4Tubs"))
				else 
				if (lvd->GetSolid()->GetEntityType()==G4String("G4Box"))
				{
///////////////////////////////////////////////////////////////////////////
//////////////////////////////G4Box/////////////////////////////////////////
					G4Box * bx = dynamic_cast<G4Box*>(lvd->GetSolid());

					Part::Box *pcFeature;
					pcFeature = static_cast<Part::Box*>(pcDoc->addObject("Part::Box",
					lvd->GetSolid()->GetName().c_str()));

					/*BEGIN******wmayer**********/
                    pcFeature->Length.setValue(bx->GetXHalfLength()*2.);
                    pcFeature->Width.setValue(bx->GetYHalfLength()*2.);
                    pcFeature->Height.setValue(bx->GetZHalfLength()*2.);
					/*END********wmayer**********/

					gp_Trsf t;
					//G4AffineTransform at_temp;
					double convTransX=at_temp.NetTranslation().getX()-bx->GetXHalfLength();
					double convTransY=at_temp.NetTranslation().getY()-bx->GetYHalfLength();
					double convTransZ=at_temp.NetTranslation().getZ()-bx->GetZHalfLength();
					gp_Vec veclocation(convTransX,convTransY,convTransZ);
					t.SetTranslation(veclocation);
					
					gp_Trsf trsf;
					G4ThreeVector  g4axis;
					double delta;
					at_temp.NetRotation().getAngleAxis(delta,g4axis);
					trsf.SetRotation(gp_Ax1(gp_Pnt(at_temp.NetTranslation().getX(),
												at_temp.NetTranslation().getY(),
												at_temp.NetTranslation().getZ()),
/**/											gp_Dir(-(double)g4axis.getX(),
												-(double)g4axis.getY(),
												-(double)g4axis.getZ())),
/**//** /											gp_Dir((double)g4axis.getX(),
												(double)g4axis.getY(),
												(double)g4axis.getZ())),/**/
												delta);
					trsf *=t;          

					/*BEGIN******wmayer**********/
                    // I will put this into utility class
                    //{
                    Base::Matrix4D mtrx;

#if OCC_VERSION_HEX >= 0x070000^M
                    gp_Mat m = trsf.VectorialPart();
                    gp_XYZ p = trsf.TranslationPart();

                    // set Rotation matrix
                    mtrx[0][0] = m(1,1);
                    mtrx[0][1] = m(1,2);
                    mtrx[0][2] = m(1,3);

                    mtrx[1][0] = m(2,1);
                    mtrx[1][1] = m(2,2);
                    mtrx[1][2] = m(2,3);

                    mtrx[2][0] = m(3,1);
                    mtrx[2][1] = m(3,2);
                    mtrx[2][2] = m(3,3);

                    // set pos vector
                    mtrx[0][3] = p.X();
                    mtrx[1][3] = p.Y();
                    mtrx[2][3] = p.Z();
#else
                    gp_Mat m = trsf._CSFDB_Getgp_Trsfmatrix();
                    gp_XYZ p = trsf._CSFDB_Getgp_Trsfloc();
                    Standard_Real scale = trsf._CSFDB_Getgp_Trsfscale();

                    // set Rotation matrix
                    mtrx[0][0] = scale * m._CSFDB_Getgp_Matmatrix(0,0);
                    mtrx[0][1] = scale * m._CSFDB_Getgp_Matmatrix(0,1);
                    mtrx[0][2] = scale * m._CSFDB_Getgp_Matmatrix(0,2);

                    mtrx[1][0] = scale * m._CSFDB_Getgp_Matmatrix(1,0);
                    mtrx[1][1] = scale * m._CSFDB_Getgp_Matmatrix(1,1);
                    mtrx[1][2] = scale * m._CSFDB_Getgp_Matmatrix(1,2);

                    mtrx[2][0] = scale * m._CSFDB_Getgp_Matmatrix(2,0);
                    mtrx[2][1] = scale * m._CSFDB_Getgp_Matmatrix(2,1);
                    mtrx[2][2] = scale * m._CSFDB_Getgp_Matmatrix(2,2);

                    // set pos vector
                    mtrx[0][3] = p._CSFDB_Getgp_XYZx();
                    mtrx[1][3] = p._CSFDB_Getgp_XYZy();
                    mtrx[2][3] = p._CSFDB_Getgp_XYZz(); 
#endif
                    //}

                    pcFeature->Placement.setValue(Base::Placement(mtrx));
					/*END********wmayer**********/
				}//End of <<if (lvd->GetSolid()->GetEntityType()==G4String("G4Box"))>>
				else 
				if (lvd->GetSolid()->GetEntityType()==G4String("G4Cons"))
				{
///////////////////////////////////////////////////////////////////////////
//////////////////////////////G4Cons/////////////////////////////////////////
					//Base::Console().Message("G4Cons\n");
					G4Cons * cs = dynamic_cast<G4Cons*>(lvd->GetSolid());

					Part::Cone *pcFeature;
					pcFeature = static_cast<Part::Cone*>(pcDoc->addObject("Part::Cone",
					lvd->GetSolid()->GetName().c_str()));
					
					pcFeature->Height.setValue(cs->GetZHalfLength()*2.);
					pcFeature->Angle.setValue(360.*cs->GetDPhi()/(D_PI*2.));
//					pcFeature->Angle.setValue(360.*cs->GetDeltaPhiAngle()/(D_PI*2.));

					pcFeature->Radius1.setValue(cs->GetRmax1());
					pcFeature->Radius2.setValue(cs->GetRmax2());
					//missing G4double  pRmin1, G4double  pRmin2,G4double  pSPhi.

					G4ThreeVector  g4axis;
					double delta;
					at_temp.NetRotation().getAngleAxis(delta,g4axis);

					double convTransX=at_temp.NetTranslation().getX();
					double convTransY=at_temp.NetTranslation().getY();
					double convTransZ=at_temp.NetTranslation().getZ();

					gp_Vec veclocation(convTransX,convTransY,convTransZ-cs->GetZHalfLength());
					gp_Trsf trsf;

					trsf.SetRotation(gp_Ax1(gp_Pnt(convTransX,convTransY,convTransZ),
											gp_Dir(-(double)g4axis.getX(),
												-(double)g4axis.getY(),
												-(double)g4axis.getZ())),
											delta);

					gp_Trsf t;
					t.SetTranslation(veclocation);
					trsf *=t;          

					/*BEGIN******wmayer**********/
                    // I will put this into utility class
                    //{
                    Base::Matrix4D mtrx;
#if OCC_VERSION_HEX >= 0x070000^M
                    gp_Mat m = trsf.VectorialPart();
                    gp_XYZ p = trsf.TranslationPart();

                    // set Rotation matrix
                    mtrx[0][0] = m(1,1);
                    mtrx[0][1] = m(1,2);
                    mtrx[0][2] = m(1,3);

                    mtrx[1][0] = m(2,1);
                    mtrx[1][1] = m(2,2);
                    mtrx[1][2] = m(2,3);

                    mtrx[2][0] = m(3,1);
                    mtrx[2][1] = m(3,2);
                    mtrx[2][2] = m(3,3);

                    // set pos vector
                    mtrx[0][3] = p.X();
                    mtrx[1][3] = p.Y();
                    mtrx[2][3] = p.Z();
#else


                    gp_Mat m = trsf._CSFDB_Getgp_Trsfmatrix();
                    gp_XYZ p = trsf._CSFDB_Getgp_Trsfloc();
                    Standard_Real scale = trsf._CSFDB_Getgp_Trsfscale();

                    // set Rotation matrix
                    mtrx[0][0] = scale * m._CSFDB_Getgp_Matmatrix(0,0);
                    mtrx[0][1] = scale * m._CSFDB_Getgp_Matmatrix(0,1);
                    mtrx[0][2] = scale * m._CSFDB_Getgp_Matmatrix(0,2);

                    mtrx[1][0] = scale * m._CSFDB_Getgp_Matmatrix(1,0);
                    mtrx[1][1] = scale * m._CSFDB_Getgp_Matmatrix(1,1);
                    mtrx[1][2] = scale * m._CSFDB_Getgp_Matmatrix(1,2);

                    mtrx[2][0] = scale * m._CSFDB_Getgp_Matmatrix(2,0);
                    mtrx[2][1] = scale * m._CSFDB_Getgp_Matmatrix(2,1);
                    mtrx[2][2] = scale * m._CSFDB_Getgp_Matmatrix(2,2);

                    // set pos vector
                    mtrx[0][3] = p._CSFDB_Getgp_XYZx();
                    mtrx[1][3] = p._CSFDB_Getgp_XYZy();
                    mtrx[2][3] = p._CSFDB_Getgp_XYZz();
#endif
                    //}

                    pcFeature->Placement.setValue(Base::Placement(mtrx));
					/*END********wmayer**********/					
				}
				else 
				if (lvd->GetSolid()->GetEntityType()==G4String("G4UnionSolid"))
				{
///////////////////////////////////////////////////////////////////////////
//////////////////////////////G4UnionSolid/////////////////////////////////////////
					//Base::Console().Message("G4UnionSolid\n");
					G4UnionSolid * us = dynamic_cast<G4UnionSolid*>(lvd->GetSolid());
					Part::Boolean *pcFeature;
					pcFeature = static_cast<Part::Boolean*>(pcDoc->addObject("Part::Boolean",
					lvd->GetSolid()->GetName().c_str()));


					//pcFeature->
        /*            pcFeature->Length.setValue(bx->GetXHalfLength()*2.);
                    pcFeature->Width.setValue(bx->GetYHalfLength()*2.);
                    pcFeature->Height.setValue(bx->GetZHalfLength()*2.);*/
				}
				else 
				if (lvd->GetSolid()->GetEntityType()==G4String("G4Trap"))
				{
///////////////////////////////////////////////////////////////////////////
//////////////////////////////G4Trap/////////////////////////////////////////
					//Base::Console().Message("G4Trap\n");
				}
				else 
				if (lvd->GetSolid()->GetEntityType()==G4String("G4Trd"))
				{
///////////////////////////////////////////////////////////////////////////
//////////////////////////////G4Trd/////////////////////////////////////////
					Base::Console().Message("G4Trd\n");
				}
			}//End of <<if (lvd->GetNoDaughters()==0)>>
			else 
			{
				browsePhysVol(lvd,&at_temp,pcDoc);
			}
		}//end of for for (int ind=0;ind<num;ind++)

		return 0;

}

/////////////////////////////////////////////Write GDML
//int G4FreeCAD::browseDoc( App::Document * pcDoc,G4VPhysicalVolume *pVolume) const
G4PVPlacement * G4FreeCAD::browseDoc( App::Document * pcDoc) const
{
	//void DlgBooleanOperation::slotChangedObject(const App::DocumentObject& obj,
	Base::Console().Message("Write\n");

/******* world of Air ********/
  G4Element* N = new G4Element("Nitrogen", "N", 7.,  14.01*g/mole);
  G4Element* O = new G4Element("Oxygen"  , "O", 8., 16.00*g/mole);
  G4double density;   
  G4int nel;
  G4Material* Air = new G4Material("Air", density= 1.29*mg/cm3, nel=2);
  Air->AddElement(N, 70*perCent);
  Air->AddElement(O, 30*perCent);
  G4Material* Pb = new G4Material("Lead", 82., 207.19*g/mole,  1.29*mg/cm3);

  G4Box *solidWorld= new G4Box("world",1000,1000,1000);
  G4LogicalVolume *logicWorld= new G4LogicalVolume( solidWorld, Air, "World", 0, 0, 0);
  G4PVPlacement *physiWorld = new G4PVPlacement(G4Transform3D(),               // no rotation
                                 logicWorld,      // its logical volume
								 "World",         // its name
                                 0,               // its mother  volume
                                 false,           // no boolean operations
                                 0);              // copy number
/******** end world *******/


	std::vector<App::DocumentObject*> objs = pcDoc->getObjectsOfType(Part::Feature::getClassTypeId());
	for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {

		if ((*it)->getTypeId().isDerivedFrom(Part::Box::getClassTypeId())) {

		Part::Box* box = static_cast<Part::Box*>(*it);
		Base::Placement plac;
		plac=box->Placement.getValue();
		//plac.invert();

		//dimensions:
		float xGDML=box->Length.getValue()/2.;
		float yGDML=box->Width.getValue()/2.;
		float zGDML=box->Height.getValue()/2.;

		Base::Rotation brot=plac.getRotation();
/*BEGIN********wmayer**********/
		Base::Vector3d vmove(xGDML,yGDML,zGDML);
//		Base::Vector3d vmove(xGDML*2.,yGDML*2.,zGDML*2.);
        brot.multVec(vmove,vmove);
/*END********wmayer**********/		
		Base::Vector3d vaxis;
		double rfAngle;
		brot.getValue(vaxis, rfAngle);
		G4RotationMatrix *rBox=new G4RotationMatrix;
		rBox->rotate(rfAngle,G4ThreeVector(-vaxis.x,-vaxis.y,-vaxis.z));
///		rBox->rotate(rfAngle,G4ThreeVector(vaxis.x,vaxis.y,vaxis.z));

		Base::Vector3d vecPos=plac.getPosition();
	
/*BEGIN********wmayer**********/
		G4ThreeVector tvBox=G4ThreeVector(vecPos.x+vmove.x,vecPos.y+vmove.y,vecPos.z+vmove.z);
/*END********wmayer**********/		

		G4Box *solidBox= new G4Box((*it)->getNameInDocument(),xGDML,yGDML,zGDML);
		G4LogicalVolume *logicBox= new G4LogicalVolume( solidBox, Air, (*it)->getNameInDocument(), 0, 0, 0);
		G4PVPlacement *physiBox = new G4PVPlacement(rBox,tvBox, 
                                 logicBox,      // its logical volume
								 (*it)->getNameInDocument(),         // its name
                                 logicWorld,               // its mother  volume
                                 false,           // no boolean operations
                                 0);              // copy number
////////////////////////////////////////////////////////////////////////////

		}
		else if ((*it)->getTypeId().isDerivedFrom(Part::Cylinder::getClassTypeId())) {
        Part::Cylinder* cylinder = static_cast<Part::Cylinder*>(*it);

		Base::Placement plac;
		plac=cylinder->Placement.getValue();

		//dimensions:
		float rGDML=cylinder->Radius.getValue();
///		float hGDML=cylinder->Height.getValue();
		float hGDML=cylinder->Height.getValue()/2.;
		float aGDML=cylinder->Angle.getValue();//convert this into radian maybe .setValue(360.*tbs->GetDPhi()/(2.*D_PI))
		
		//rotation:
		Base::Rotation trot=plac.getRotation();
/*BEGIN********wmayer**********/		
//		Base::Vector3d vmove(0,0,hGDML);
		Base::Vector3d vmove(0,0,-hGDML);
        trot.multVec(vmove,vmove);
/*END********wmayer**********/		
		Base::Vector3d vaxis;
		double rfAngle;
		trot.getValue(vaxis, rfAngle);
		G4RotationMatrix *rTub=new G4RotationMatrix;//= G4RotationMatrix(test,rfAngle);
		rTub->rotate(rfAngle,G4ThreeVector(-vaxis.x,-vaxis.y,-vaxis.z));
//	    rTub->rotate(rfAngle,G4ThreeVector(vaxis.x,vaxis.y,vaxis.z));
	
		//translation:
		Base::Vector3d vecPos=plac.getPosition();
/*BEGIN********wmayer**********/
		G4ThreeVector tvBox=G4ThreeVector(vecPos.x-vmove.x,vecPos.y-vmove.y,vecPos.z-vmove.z);
/*END********wmayer**********/

		///Base::Console().Message("RAD%.2f|HEI%.2f|ANG%.2f\n",xGDML,yGDML,zGDML);
		G4Tubs *solidTub= new G4Tubs((*it)->getNameInDocument(),0,rGDML,hGDML,0,aGDML*(D_PI*2.)/360.);
		G4LogicalVolume *logicTub= new G4LogicalVolume( solidTub, Air, (*it)->getNameInDocument(), 0, 0, 0);
		G4PVPlacement *physiTub = new G4PVPlacement(rTub,tvBox, 
                                 logicTub,      // its logical volume
								 (*it)->getNameInDocument(),         // its name
                                 logicWorld,               // its mother  volume
                                 false,           // no boolean operations
                                 0);              // copy number

		}
	}//end of for

			
/******** debut *******/
//G4GDMLWriteStructure * Writer;
//Writer->BoxWrite(
///pVolume=physiWorld;
/******** fin *********/
			
	//		return 0;
return	physiWorld;
//\ No newline at end of file
}
