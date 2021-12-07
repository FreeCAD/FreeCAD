/**************************************************************************
*   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
*   Copyright (c) 2007 Human Rezai <human@mytum.de>                       *
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
#include "deviation.h"

#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Builder.h>
#include <Mod/Mesh/App/Core/Grid.h>

#include <BRepAdaptor_Surface.hxx>
#include <Base/Builder3D.h>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Surface.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>


Deviation::Deviation() 
{}

Deviation::~Deviation()
{}

void Deviation::ImportGeometry(const TopoDS_Shape& aShape, const MeshCore::MeshKernel& aMesh)
{
	m_Mesh = aMesh;
	m_Cad  = aShape;
}

bool Deviation::GenNormals()
{
	Base::Builder3D log;
	TopExp_Explorer aExpFace;

	MeshPnt aMeshStruct;
	std::pair<Base::Vector3f, MeshPnt> inp;
	std::map<Base::Vector3f,MeshPnt,MeshPntLess >::iterator meshIt;
	
	MeshCore::MeshKernel FaceMesh;
	MeshCore::MeshPointArray MeshPnts;
	
	int n;

	MeshPnts = m_MeshCad.GetPoints();
	m_pnts.resize(m_MeshCad.CountPoints());
	m_nlvec.resize((m_MeshCad.CountPoints()));
	
	for (unsigned int i=0; i<MeshPnts.size(); ++i)
	{
		aMeshStruct.pnt = MeshPnts[i];        // stores point
		aMeshStruct.index = i;                // stores index
		inp.first  = aMeshStruct.pnt;
		inp.second = aMeshStruct;
		MeshMap.insert(inp);
	}

	// explores all faces  ------------  Hauptschleife
	for (aExpFace.Init(m_Cad,TopAbs_FACE);aExpFace.More();aExpFace.Next())
	{
		TopoDS_Face aFace = TopoDS::Face(aExpFace.Current());
		TransferFaceTriangulationtoFreeCAD(aFace, FaceMesh);
		
		MeshPnts.clear();
		MeshPnts = FaceMesh.GetPoints();
		n = MeshPnts.size();

		TopLoc_Location aLocation;
		
		Handle_Poly_Triangulation aTr = BRep_Tool::Triangulation(aFace,aLocation);
		const TColgp_Array1OfPnt& aNodes = aTr->Nodes();
		
		// create array of node points in absolute coordinate system
		TColgp_Array1OfPnt aPoints(1, aNodes.Length());
		for (Standard_Integer i = 1; i <= aNodes.Length(); i++)
			aPoints(i) = aNodes(i).Transformed(aLocation);

		const TColgp_Array1OfPnt2d& aUVNodes = aTr->UVNodes();

		BRepAdaptor_Surface aSurface(aFace);
		Base::Vector3f pnt, normal;
		gp_Pnt2d par;
		gp_Pnt P;
		gp_Vec D1U, D1V;

		for (int i=1; i<n+1; ++i)
		{
			par = aUVNodes.Value(i);
			aSurface.D1(par.X(),par.Y(),P,D1U,D1V);
			P = aPoints(i);
			pnt.x = (float) P.X();
			pnt.y = (float) P.Y();
			pnt.z = (float) P.Z();

			meshIt = MeshMap.find(pnt);
			if (meshIt == MeshMap.end())
			{
				cout << "error";
				return false;
			}
			
			D1U.Cross(D1V);
			D1U.Normalize();
			
			if (aFace.Orientation() == TopAbs_FORWARD) D1U.Scale(-1.0);

			
			normal.x = (float) D1U.X();
			normal.y = (float) D1U.Y();
			normal.z = (float) D1U.Z();
			
			m_pnts[((*meshIt).second).index] = pnt;
			m_nlvec[((*meshIt).second).index] = normal;

			log.addSingleArrow(pnt,pnt+normal);
		}
	}

	log.saveToFile("c:/deviation.iv");
	return true;
}

bool Deviation::Compute()
{
	Base::Builder3D log;

	best_fit::Tesselate_Shape(m_Cad, m_MeshCad, float(0.1));
	GenNormals();

	//return true;

	std::vector<int> FailProj;

	MeshCore::MeshFacetGrid aFacetGrid(m_Mesh,10);
	MeshCore::MeshAlgorithm malg(m_Mesh);
	MeshCore::MeshAlgorithm malg2(m_Mesh);
	MeshCore::MeshPointIterator p_it(m_MeshCad);

	Base::Vector3f projPoint, distVec, nvec(0,0,0), projPoint2;
	unsigned long  facetIndex;
	std::stringstream text;

	unsigned int c=0;
	int i=0;

	for (p_it.Begin(); p_it.More(); p_it.Next())
	{
		if (malg.NearestFacetOnRay(*p_it, m_nlvec[i], aFacetGrid, projPoint, facetIndex))   // gridoptimiert
		{
			distVec  = projPoint - *p_it;
			m_nlvec[i] = distVec;   // überschreibt normalenvektor
		}
		else
		{
			if (!malg2.NearestFacetOnRay(*p_it, m_nlvec[i], projPoint, facetIndex))   // nicht gridoptimiert
			{
				c++;
				FailProj.push_back(i);
				m_nlvec[i] = nvec;
			}
			else
			{
				distVec  = projPoint - *p_it;
				m_nlvec[i] = distVec;   // überschreibt normalenvektor
			}
		}

		++i;
	}

	for(int i=0; i<m_nlvec.size(); i++)
	{
		log.addSingleArrow(m_pnts[i], m_pnts[i] + m_nlvec[i]);
	}


	log.saveToFile("c:/deviation2.iv");
	return true;
}

#include <QFile>
#include <QTextStream>
void Deviation::WriteOutput(const QString &dateiname)
{
	QFile anOutputFile(dateiname);
	if (!anOutputFile.open(QIODevice::WriteOnly | QIODevice::Text))
		return;
	QTextStream out(&anOutputFile);

	out << m_nlvec.size() << endl;

	for(int i=0; i<m_nlvec.size(); i++)
	{
				out  <<  m_pnts[i].x    << "," 
			         <<  m_pnts[i].y    << "," 
					 <<  m_pnts[i].z    << "," 
					 <<  m_nlvec[i].x   << "," 
					 <<  m_nlvec[i].y   << "," 
					 <<  m_nlvec[i].z   << endl;
	}

	anOutputFile.close();
}