/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2009     *
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
# include <cstdlib>
# include <memory>
# include <strstream>
# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <BRepAlgo_NormalProjection.hxx>
#endif

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/TimeInfo.h>
#include <Base/Console.h>

#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Evaluation.h>
#include <Mod/Mesh/App/Core/Iterator.h>

#include "FemMesh.h"

#include <SMESH_Gen.hxx>
#include <SMESH_Mesh.hxx>
#include <SMDS_PolyhedralVolumeOfNodes.hxx>
#include <SMDS_VolumeTool.hxx>
#include <StdMeshers_MaxLength.hxx>
#include <StdMeshers_LocalLength.hxx>
#include <StdMeshers_MaxElementArea.hxx>
#include <StdMeshers_NumberOfSegments.hxx>
#include <StdMeshers_Deflection1D.hxx>
#include <StdMeshers_Regular_1D.hxx>
#include <StdMeshers_StartEndLength.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
#include <StdMeshers_Quadrangle_2D.hxx>
#include <StdMeshers_QuadraticMesh.hxx>

# include <TopoDS_Face.hxx>

//to simplify parsing input files we use the boost lib
#include <boost/tokenizer.hpp>


using namespace Fem;
using namespace Base;
using namespace boost;

static int StatCount = 0;

TYPESYSTEM_SOURCE(Fem::FemMesh , Base::Persistence);

FemMesh::FemMesh()
{
    //Base::Console().Log("FemMesh::FemMesh():%p (id=%i)\n",this,StatCount);
    myGen = new SMESH_Gen();
    // create a mesh allways with new StudyId to avoid overlapping destruction
    myMesh = myGen->CreateMesh(StatCount++,false);

}

FemMesh::FemMesh(const FemMesh& mesh)
{
    //Base::Console().Log("FemMesh::FemMesh(mesh):%p (id=%i)\n",this,StatCount);
    myGen = new SMESH_Gen();
    myMesh = myGen->CreateMesh(StatCount++,false);
    copyMeshData(mesh);
}

FemMesh::~FemMesh()
{
    //Base::Console().Log("FemMesh::~FemMesh():%p\n",this);

    TopoDS_Shape aNull;
    myMesh->ShapeToMesh(aNull);
    myMesh->Clear();
    //myMesh->ClearLog();
    delete myMesh;
#if defined(__GNUC__)
    delete myGen; // crashes with MSVC
#endif
}

FemMesh &FemMesh::operator=(const FemMesh& mesh)
{
    if (this != &mesh) {
        copyMeshData(mesh);
    }
    return *this;
}

void FemMesh::copyMeshData(const FemMesh& mesh)
{
    //const SMDS_MeshInfo& info = mesh.myMesh->GetMeshDS()->GetMeshInfo();
    //int numPoly = info.NbPolygons();
    //int numVolu = info.NbVolumes();
    //int numTetr = info.NbTetras();
    //int numHexa = info.NbHexas();
    //int numPyrd = info.NbPyramids();
    //int numPris = info.NbPrisms();
    //int numHedr = info.NbPolyhedrons();

    _Mtrx = mesh._Mtrx;

    SMESHDS_Mesh* meshds = this->myMesh->GetMeshDS();
    meshds->ClearMesh();

    SMDS_NodeIteratorPtr aNodeIter = mesh.myMesh->GetMeshDS()->nodesIterator();
    for (;aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        meshds->AddNodeWithID(aNode->X(),aNode->Y(),aNode->Z(), aNode->GetID());
    }

    SMDS_EdgeIteratorPtr aEdgeIter = mesh.myMesh->GetMeshDS()->edgesIterator();
    for (;aEdgeIter->more();) {
        const SMDS_MeshEdge* aEdge = aEdgeIter->next();
        meshds->AddEdgeWithID(aEdge->GetNode(0), aEdge->GetNode(1), aEdge->GetID());
    }

    SMDS_FaceIteratorPtr aFaceIter = mesh.myMesh->GetMeshDS()->facesIterator();
    for (;aFaceIter->more();) {
        const SMDS_MeshFace* aFace = aFaceIter->next();
        switch (aFace->NbNodes()) {
            case 3:
                meshds->AddFaceWithID(aFace->GetNode(0),
                                      aFace->GetNode(1),
                                      aFace->GetNode(2),
                                      aFace->GetID());
                break;
            case 4:
                meshds->AddFaceWithID(aFace->GetNode(0),
                                      aFace->GetNode(1),
                                      aFace->GetNode(2),
                                      aFace->GetNode(3),
                                      aFace->GetID());
                break;
            case 6:
                meshds->AddFaceWithID(aFace->GetNode(0),
                                      aFace->GetNode(1),
                                      aFace->GetNode(2),
                                      aFace->GetNode(3),
                                      aFace->GetNode(4),
                                      aFace->GetNode(5),
                                      aFace->GetID());
                break;
            case 8:
                meshds->AddFaceWithID(aFace->GetNode(0),
                                      aFace->GetNode(1),
                                      aFace->GetNode(2),
                                      aFace->GetNode(3),
                                      aFace->GetNode(4),
                                      aFace->GetNode(5),
                                      aFace->GetNode(6),
                                      aFace->GetNode(7),
                                      aFace->GetID());
                break;
            default:
                {
                    std::vector<const SMDS_MeshNode*> aNodes;
                    for (int i=0; aFace->NbNodes(); i++)
                        aNodes.push_back(aFace->GetNode(0));
                    meshds->AddPolygonalFaceWithID(aNodes, aFace->GetID());
                }
                break;
        }
    }

    SMDS_VolumeIteratorPtr aVolIter = mesh.myMesh->GetMeshDS()->volumesIterator();
    for (;aVolIter->more();) {
        const SMDS_MeshVolume* aVol = aVolIter->next();
        switch (aVol->NbNodes()) {
            case 4:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetID());
                break;
            case 5:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetID());
                break;
            case 6:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetNode(5),
                                        aVol->GetID());
                break;
            case 8:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetNode(5),
                                        aVol->GetNode(6),
                                        aVol->GetNode(7),
                                        aVol->GetID());
                break;
            case 10:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetNode(5),
                                        aVol->GetNode(6),
                                        aVol->GetNode(7),
                                        aVol->GetNode(8),
                                        aVol->GetNode(9),
                                        aVol->GetID());
                break;
            case 13:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetNode(5),
                                        aVol->GetNode(6),
                                        aVol->GetNode(7),
                                        aVol->GetNode(8),
                                        aVol->GetNode(9),
                                        aVol->GetNode(10),
                                        aVol->GetNode(11),
                                        aVol->GetNode(12),
                                        aVol->GetID());
                break;
            case 15:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetNode(5),
                                        aVol->GetNode(6),
                                        aVol->GetNode(7),
                                        aVol->GetNode(8),
                                        aVol->GetNode(9),
                                        aVol->GetNode(10),
                                        aVol->GetNode(11),
                                        aVol->GetNode(12),
                                        aVol->GetNode(13),
                                        aVol->GetNode(14),
                                        aVol->GetID());
                break;
            case 20:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetNode(5),
                                        aVol->GetNode(6),
                                        aVol->GetNode(7),
                                        aVol->GetNode(8),
                                        aVol->GetNode(9),
                                        aVol->GetNode(10),
                                        aVol->GetNode(11),
                                        aVol->GetNode(12),
                                        aVol->GetNode(13),
                                        aVol->GetNode(14),
                                        aVol->GetNode(15),
                                        aVol->GetNode(16),
                                        aVol->GetNode(17),
                                        aVol->GetNode(18),
                                        aVol->GetNode(19),
                                        aVol->GetID());
                break;
            default:
                {
                    if (aVol->IsPoly()) {
                        const SMDS_PolyhedralVolumeOfNodes* aPolyVol = dynamic_cast<const SMDS_PolyhedralVolumeOfNodes*>(aVol);
                        if (!aPolyVol) break;
                        std::vector<const SMDS_MeshNode*> aNodes;
                        for (int i=0; i<aPolyVol->NbNodes(); i++)
                            aNodes.push_back(aPolyVol->GetNode(i));
                        meshds->AddPolyhedralVolumeWithID(aNodes,
                            aPolyVol->GetQuanities(), aPolyVol->GetID());
                    }
                }
                break;
        }
    }
}

const SMESH_Mesh* FemMesh::getSMesh() const
{
    return myMesh;
}

SMESH_Mesh* FemMesh::getSMesh()
{
    return myMesh;
}

SMESH_Gen * FemMesh::getGenerator()
{
    return myGen;
}

void FemMesh::addHypothesis(const TopoDS_Shape & aSubShape, SMESH_HypothesisPtr hyp)
{
    myMesh->AddHypothesis(aSubShape, hyp->GetID());
    SMESH_HypothesisPtr ptr(hyp);
    hypoth.push_back(ptr);
}

void FemMesh::setStanardHypotheses()
{
    if (!hypoth.empty())
        return;
    int hyp=0;
    SMESH_HypothesisPtr len(new StdMeshers_MaxLength(hyp++, 1, myGen));
    static_cast<StdMeshers_MaxLength*>(len.get())->SetLength(1.0);
    hypoth.push_back(len);

    SMESH_HypothesisPtr loc(new StdMeshers_LocalLength(hyp++, 1, myGen));
    static_cast<StdMeshers_LocalLength*>(loc.get())->SetLength(1.0);
    hypoth.push_back(loc);

    SMESH_HypothesisPtr area(new StdMeshers_MaxElementArea(hyp++, 1, myGen));
    static_cast<StdMeshers_MaxElementArea*>(area.get())->SetMaxArea(1.0);
    hypoth.push_back(area);

    SMESH_HypothesisPtr segm(new StdMeshers_NumberOfSegments(hyp++, 1, myGen));
    static_cast<StdMeshers_NumberOfSegments*>(segm.get())->SetNumberOfSegments(1);
    hypoth.push_back(segm);

    SMESH_HypothesisPtr defl(new StdMeshers_Deflection1D(hyp++, 1, myGen));
    static_cast<StdMeshers_Deflection1D*>(defl.get())->SetDeflection(0.01);
    hypoth.push_back(defl);

    SMESH_HypothesisPtr reg(new StdMeshers_Regular_1D(hyp++, 1, myGen));
    hypoth.push_back(reg);

    //SMESH_HypothesisPtr sel(new StdMeshers_StartEndLength(hyp++, 1, myGen));
    //static_cast<StdMeshers_StartEndLength*>(sel.get())->SetLength(1.0, true);
    //hypoth.push_back(sel);

    SMESH_HypothesisPtr qdp(new StdMeshers_QuadranglePreference(hyp++,1,myGen));
    hypoth.push_back(qdp);

    SMESH_HypothesisPtr q2d(new StdMeshers_Quadrangle_2D(hyp++,1,myGen));
    hypoth.push_back(q2d);

    // Apply hypothesis
    for (int i=0; i<hyp;i++)
        myMesh->AddHypothesis(myMesh->GetShapeToMesh(), i);
}

void FemMesh::compute()
{
    myGen->Compute(*myMesh, myMesh->GetShapeToMesh());
}

std::set<long> FemMesh::getSurfaceNodes(long ElemId,short FaceId, float Angle) const
{
    std::set<long> result;
    const SMESHDS_Mesh* data = myMesh->GetMeshDS();

    const SMDS_MeshElement * element = data->FindElement(ElemId);
    int fNbr = element->NbFaces();
    //element->

    return result;
}

std::set<long> FemMesh::getSurfaceNodes(const TopoDS_Face &face)const
{

    std::set<long> result;
    const SMESHDS_Mesh* data = myMesh->GetMeshDS();

    BRepAlgo_NormalProjection algo;


    return result;
}



void FemMesh::readNastran(const std::string &Filename)
{
    Base::TimeInfo Start;
    Base::Console().Log("Start: FemMesh::readNastran() =================================\n");

    _Mtrx = Base::Matrix4D();

	std::ifstream inputfile;
	inputfile.open(Filename.c_str());
	inputfile.seekg(std::ifstream::beg);
	std::string line1,line2,temp;
	std::vector<string> token_results;
	token_results.clear();
	Base::Vector3d current_node;
	std::vector<Base::Vector3d> vertices;
	vertices.clear();
	std::vector<unsigned int> nodal_id;
	nodal_id.clear();
	std::vector<unsigned int> tetra_element;
	std::vector<std::vector<unsigned int> > all_elements;
	std::vector<unsigned int> element_id;
	element_id.clear();
	bool nastran_free_format = false;
	do
	{
		std::getline(inputfile,line1);
		if (line1.size() == 0) continue;
		if (!nastran_free_format && line1.find(",")!= std::string::npos)
			nastran_free_format = true;
		if (!nastran_free_format && line1.find("GRID*")!= std::string::npos ) //We found a Grid line
		{
			//Now lets extract the GRID Points = Nodes
			//As each GRID Line consists of two subsequent lines we have to
			//take care of that as well
			std::getline(inputfile,line2);
			//Get the Nodal ID
			nodal_id.push_back(atoi(line1.substr(8,24).c_str()));
			//Extract X Value
			current_node.x = atof(line1.substr(40,56).c_str());
			//Extract Y Value
			current_node.y = atof(line1.substr(56,72).c_str());
			//Extract Z Value
			current_node.z = atof(line2.substr(8,24).c_str());

			vertices.push_back(current_node);
		}
		else if (!nastran_free_format && line1.find("CTETRA")!= std::string::npos)
		{
			tetra_element.clear();
			//Lets extract the elements
			//As each Element Line consists of two subsequent lines as well 
			//we have to take care of that
			//At a first step we only extract Quadratic Tetrahedral Elements
			std::getline(inputfile,line2);
			element_id.push_back(atoi(line1.substr(8,16).c_str()));
			tetra_element.push_back(atoi(line1.substr(24,32).c_str()));
			tetra_element.push_back(atoi(line1.substr(32,40).c_str()));
			tetra_element.push_back(atoi(line1.substr(40,48).c_str()));
			tetra_element.push_back(atoi(line1.substr(48,56).c_str()));
			tetra_element.push_back(atoi(line1.substr(56,64).c_str()));
			tetra_element.push_back(atoi(line1.substr(64,72).c_str()));
			tetra_element.push_back(atoi(line2.substr(8,16).c_str()));
			tetra_element.push_back(atoi(line2.substr(16,24).c_str()));
			tetra_element.push_back(atoi(line2.substr(24,32).c_str()));
			tetra_element.push_back(atoi(line2.substr(32,40).c_str()));

			all_elements.push_back(tetra_element);
		}
		else if (nastran_free_format && line1.find("GRID")!= std::string::npos ) //We found a Grid line
		{
			char_separator<char> sep(",");
			tokenizer<char_separator<char> > tokens(line1, sep);
			token_results.assign(tokens.begin(),tokens.end());
			if (token_results.size() < 3)
				continue;//Line does not include Nodal coordinates
			nodal_id.push_back(atoi(token_results[1].c_str()));
			current_node.x = atof(token_results[3].c_str());
			current_node.y = atof(token_results[4].c_str());		
			current_node.z = atof(token_results[5].c_str());
			vertices.push_back(current_node);
		}
		else if (nastran_free_format && line1.find("CTETRA")!= std::string::npos)
		{
			tetra_element.clear();
			//Lets extract the elements
			//As each Element Line consists of two subsequent lines as well 
			//we have to take care of that
			//At a first step we only extract Quadratic Tetrahedral Elements
			std::getline(inputfile,line2);
			char_separator<char> sep(",");
			tokenizer<char_separator<char> > tokens(line1.append(line2), sep);
			token_results.assign(tokens.begin(),tokens.end());
			if (token_results.size() < 11)
				continue;//Line does not include enough nodal IDs
			element_id.push_back(atoi(token_results[1].c_str()));
			tetra_element.push_back(atoi(token_results[3].c_str()));
			tetra_element.push_back(atoi(token_results[4].c_str()));
			tetra_element.push_back(atoi(token_results[5].c_str()));
			tetra_element.push_back(atoi(token_results[6].c_str()));
			tetra_element.push_back(atoi(token_results[7].c_str()));
			tetra_element.push_back(atoi(token_results[8].c_str()));
			tetra_element.push_back(atoi(token_results[10].c_str()));
			tetra_element.push_back(atoi(token_results[11].c_str()));
			tetra_element.push_back(atoi(token_results[12].c_str()));
			tetra_element.push_back(atoi(token_results[13].c_str()));

			all_elements.push_back(tetra_element);
		}

	}
	while (inputfile.good());
	inputfile.close();

    Base::Console().Log("    %f: File read, start building mesh\n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));

	//Now fill the SMESH datastructure
	std::vector<Base::Vector3d>::const_iterator anodeiterator;
	SMESHDS_Mesh* meshds = this->myMesh->GetMeshDS();
	meshds->ClearMesh();
	unsigned int j=0;
	for(anodeiterator=vertices.begin(); anodeiterator!=vertices.end(); anodeiterator++)
	{
		meshds->AddNodeWithID((*anodeiterator).x,(*anodeiterator).y,(*anodeiterator).z,nodal_id[j]);
		j++;
	}

	for(unsigned int i=0;i<all_elements.size();i++)
	{
		//Die Reihenfolge wie hier die Elemente hinzugefügt werden ist sehr wichtig. 
		//Ansonsten ist eine konsistente Datenstruktur nicht möglich
		//meshds->AddVolumeWithID
		//(
		//	meshds->FindNode(all_elements[i][0]),
		//	meshds->FindNode(all_elements[i][2]),
		//	meshds->FindNode(all_elements[i][1]),
		//	meshds->FindNode(all_elements[i][3]),
		//	meshds->FindNode(all_elements[i][6]),
		//	meshds->FindNode(all_elements[i][5]),
		//	meshds->FindNode(all_elements[i][4]),
		//	meshds->FindNode(all_elements[i][9]),
		//	meshds->FindNode(all_elements[i][7]),
		//	meshds->FindNode(all_elements[i][8]),
		//	element_id[i]
		//);
		meshds->AddVolumeWithID
		(
			meshds->FindNode(all_elements[i][1]),
			meshds->FindNode(all_elements[i][0]),
			meshds->FindNode(all_elements[i][2]),
			meshds->FindNode(all_elements[i][3]),
			meshds->FindNode(all_elements[i][4]),
			meshds->FindNode(all_elements[i][6]),
			meshds->FindNode(all_elements[i][5]),
			meshds->FindNode(all_elements[i][8]),
			meshds->FindNode(all_elements[i][7]),
			meshds->FindNode(all_elements[i][9]),
			element_id[i]
		);
	}
    Base::Console().Log("    %f: Done \n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));

}


void FemMesh::read(const char *FileName)
{
    Base::FileInfo File(FileName);
    _Mtrx = Base::Matrix4D();
  
    // checking on the file
    if (!File.isReadable())
        throw Base::Exception("File to load not existing or not readable");
    
    if (File.hasExtension("unv") ) {
        // read UNV file
        myMesh->UNVToMesh(File.filePath().c_str());
    }
    else if (File.hasExtension("med") ) {
        myMesh->MEDToMesh(File.filePath().c_str(),File.fileNamePure().c_str());
    }
    else if (File.hasExtension("stl") ) {
        // read brep-file
        myMesh->STLToMesh(File.filePath().c_str());
    }
    else if (File.hasExtension("dat") ) {
        // read brep-file
        myMesh->DATToMesh(File.filePath().c_str());
    }
	else if (File.hasExtension("bdf") ) {
		// read Nastran-file
		readNastran(File.filePath());
	}
    else{
        throw Base::Exception("Unknown extension");
    }
}

void FemMesh::writeABAQUS(const std::string &Filename) const
{
    std::ofstream anABAQUS_Output;
    anABAQUS_Output.open(Filename.c_str());
    anABAQUS_Output << "*Node , NSET=Nall" << std::endl;

    //Extract Nodes and Elements of the current SMESH datastructure
    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();

    Base::Vector3d current_node;
    for (;aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        current_node.Set(aNode->X(),aNode->Y(),aNode->Z());
        current_node = _Mtrx * current_node;
        anABAQUS_Output << aNode->GetID() << ","
            << current_node.x << "," 
            << current_node.y << ","
            << current_node.z << std::endl;
    }

	anABAQUS_Output << "*Element, TYPE=C3D10, ELSET=Eall" << std::endl;
	SMDS_VolumeIteratorPtr aVolIter = myMesh->GetMeshDS()->volumesIterator();

	std::map<int,std::vector<int> > temp_map;
	std::pair<int,std::vector<int> > apair;
	temp_map.clear();
	for (;aVolIter->more();) 
	{
		const SMDS_MeshVolume* aVol = aVolIter->next();
		//Dont ask about the order in which we have to output the SMESH structure
		//I absolute dont understand the scheme behind it but somehow its working like this
		apair.first = aVol->GetID();
		apair.second.clear();

		//Neuer Versuch
		apair.second.push_back(aVol->GetNode(1)->GetID());
		apair.second.push_back(aVol->GetNode(0)->GetID());
		apair.second.push_back(aVol->GetNode(2)->GetID());
		apair.second.push_back(aVol->GetNode(3)->GetID());
		apair.second.push_back(aVol->GetNode(4)->GetID());
		apair.second.push_back(aVol->GetNode(6)->GetID());
		apair.second.push_back(aVol->GetNode(5)->GetID());
		apair.second.push_back(aVol->GetNode(8)->GetID());
		apair.second.push_back(aVol->GetNode(7)->GetID());
		apair.second.push_back(aVol->GetNode(9)->GetID());
	
        temp_map.insert(apair);
	}

	std::map<int,std::vector<int> >::iterator it_map;
	std::vector<int>::iterator it_vector;
	for(it_map = temp_map.begin();it_map!=temp_map.end();it_map++)
	{
		anABAQUS_Output << it_map->first << ",";
		for(it_vector = it_map->second.begin();it_vector!=it_map->second.end();it_vector++)
		{
			anABAQUS_Output << *it_vector << ",";
		}
		anABAQUS_Output << std::endl;
	}
	anABAQUS_Output.close();
}

void FemMesh::write(const char *FileName) const
{
    Base::FileInfo File(FileName);

    if (File.hasExtension("unv") ) {
        // read UNV file
         myMesh->ExportUNV(File.filePath().c_str());
    }
    else if (File.hasExtension("med") ) {
         myMesh->ExportMED(File.filePath().c_str());
    }
    else if (File.hasExtension("stl") ) {
        // read brep-file
        myMesh->ExportSTL(File.filePath().c_str(),false);
    }
    else if (File.hasExtension("dat") ) {
        // read brep-file
        myMesh->ExportDAT(File.filePath().c_str());
    }
	else if (File.hasExtension("inp") ) {
		// write ABAQUS Output
		writeABAQUS(File.filePath());
	}
    else{
        throw Base::Exception("Unknown extension");
    }
}

// ==== Base class implementer ==============================================================

unsigned int FemMesh::getMemSize (void) const
{
    return 0;
}

void FemMesh::Save (Base::Writer &writer) const
{
    //See SaveDocFile(), RestoreDocFile()
    writer.Stream() << writer.ind() << "<FemMesh file=\"" ;
    writer.Stream() << writer.addFile("FemMesh.unv", this) << "\"";
    writer.Stream() << " a11=\"" <<  _Mtrx[0][0] << "\" a12=\"" <<  _Mtrx[0][1] << "\" a13=\"" <<  _Mtrx[0][2] << "\" a14=\"" <<  _Mtrx[0][3] << "\"";
    writer.Stream() << " a21=\"" <<  _Mtrx[1][0] << "\" a22=\"" <<  _Mtrx[1][1] << "\" a23=\"" <<  _Mtrx[1][2] << "\" a24=\"" <<  _Mtrx[1][3] << "\"";
    writer.Stream() << " a31=\"" <<  _Mtrx[2][0] << "\" a32=\"" <<  _Mtrx[2][1] << "\" a33=\"" <<  _Mtrx[2][2] << "\" a34=\"" <<  _Mtrx[2][3] << "\"";
    writer.Stream() << " a41=\"" <<  _Mtrx[3][0] << "\" a42=\"" <<  _Mtrx[3][1] << "\" a43=\"" <<  _Mtrx[3][2] << "\" a44=\"" <<  _Mtrx[3][3] << "\"";
    writer.Stream() << "/>" << std::endl;


}

void FemMesh::Restore(Base::XMLReader &reader)
{

    reader.readElement("FemMesh");
    std::string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // initate a file read
        reader.addFile(file.c_str(),this);
    }
    if( reader.hasAttribute("a11")){
        _Mtrx[0][0] = (float)reader.getAttributeAsFloat("a11");
        _Mtrx[0][1] = (float)reader.getAttributeAsFloat("a12");
        _Mtrx[0][2] = (float)reader.getAttributeAsFloat("a13");
        _Mtrx[0][3] = (float)reader.getAttributeAsFloat("a14");

        _Mtrx[1][0] = (float)reader.getAttributeAsFloat("a21");
        _Mtrx[1][1] = (float)reader.getAttributeAsFloat("a22");
        _Mtrx[1][2] = (float)reader.getAttributeAsFloat("a23");
        _Mtrx[1][3] = (float)reader.getAttributeAsFloat("a24");

        _Mtrx[2][0] = (float)reader.getAttributeAsFloat("a31");
        _Mtrx[2][1] = (float)reader.getAttributeAsFloat("a32");
        _Mtrx[2][2] = (float)reader.getAttributeAsFloat("a33");
        _Mtrx[2][3] = (float)reader.getAttributeAsFloat("a34");

        _Mtrx[3][0] = (float)reader.getAttributeAsFloat("a41");
        _Mtrx[3][1] = (float)reader.getAttributeAsFloat("a42");
        _Mtrx[3][2] = (float)reader.getAttributeAsFloat("a43");
        _Mtrx[3][3] = (float)reader.getAttributeAsFloat("a44");
    }
}

void FemMesh::SaveDocFile (Base::Writer &writer) const
{
    // create a temporary file and copy the content to the zip stream
    Base::FileInfo fi(Base::FileInfo::getTempFileName().c_str());

    myMesh->ExportUNV(fi.filePath().c_str());
 
    Base::ifstream file(fi, std::ios::in | std::ios::binary);
    if (file){
        unsigned long ulSize = 0; 
        std::streambuf* buf = file.rdbuf();
        if (buf) {
            unsigned long ulCurr;
            ulCurr = buf->pubseekoff(0, std::ios::cur, std::ios::in);
            ulSize = buf->pubseekoff(0, std::ios::end, std::ios::in);
            buf->pubseekoff(ulCurr, std::ios::beg, std::ios::in);
        }

        // read in the ASCII file and write back to the stream
        std::strstreambuf sbuf(ulSize);
        file >> &sbuf;
        writer.Stream() << &sbuf;
    }

    file.close();
    // remove temp file
    fi.deleteFile();
}

void FemMesh::RestoreDocFile(Base::Reader &reader)
{
    // create a temporary file and copy the content from the zip stream
    Base::FileInfo fi(Base::FileInfo::getTempFileName().c_str());

    // read in the ASCII file and write back to the file stream
    Base::ofstream file(fi, std::ios::out | std::ios::binary);
    if (reader)
        reader >> file.rdbuf();
    file.close();

    // read the shape from the temp file
    myMesh->UNVToMesh(fi.filePath().c_str());

    // delete the temp file
    fi.deleteFile();
}

void FemMesh::transformGeometry(const Base::Matrix4D& rclTrf)
{
	//We perform a translation and rotation of the current active Mesh object
	Base::Matrix4D clMatrix(rclTrf);
	SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();
	Base::Vector3d current_node;
	for (;aNodeIter->more();) {
		const SMDS_MeshNode* aNode = aNodeIter->next();
		current_node.Set(aNode->X(),aNode->Y(),aNode->Z());
		current_node = clMatrix * current_node;
		myMesh->GetMeshDS()->MoveNode(aNode,current_node.x,current_node.y,current_node.z);
	}
}

void FemMesh::setTransform(const Base::Matrix4D& rclTrf)
{
    // Placement handling, no geometric transformation
    _Mtrx = rclTrf;
}

Base::Matrix4D FemMesh::getTransform(void) const
{
    return _Mtrx;
}

Base::BoundBox3d FemMesh::getBoundBox(void) const
{
    Base::BoundBox3d box;

    SMESHDS_Mesh* data = const_cast<SMESH_Mesh*>(getSMesh())->GetMeshDS();

	SMDS_NodeIteratorPtr aNodeIter = data->nodesIterator();
	for (;aNodeIter->more();) {
		const SMDS_MeshNode* aNode = aNodeIter->next();
        Base::Vector3d vec(aNode->X(),aNode->Y(),aNode->Z());
        // Apply the matrix to hold the BoundBox in absolute space. 
        vec = _Mtrx * vec;
        box.Add(vec);
	}

    return box;
}

std::vector<const char*> FemMesh::getElementTypes(void) const
{
    std::vector<const char*> temp;
    temp.push_back("Vertex");
    temp.push_back("Edge");
    temp.push_back("Face");
    temp.push_back("Volume");

    return temp;
}

unsigned long FemMesh::countSubElements(const char* Type) const
{
    return 0;
}

Data::Segment* FemMesh::getSubElement(const char* Type, unsigned long n) const
{
    // FIXME implement subelement interface 
    //std::stringstream str;
    //str << Type << n;
    //std::string temp = str.str();
    //return new ShapeSegment(getSubShape(temp.c_str()));
    return 0;
}

struct Fem::FemMesh::FemMeshInfo FemMesh::getInfo(void) const{

    struct FemMeshInfo rtrn;

    SMESHDS_Mesh* data =  const_cast<SMESH_Mesh*>(getSMesh())->GetMeshDS();
	const SMDS_MeshInfo& info = data->GetMeshInfo();
	rtrn.numFaces = data->NbFaces();
    rtrn.numNode = info.NbNodes();
    rtrn.numTria = info.NbTriangles();
    rtrn.numQuad = info.NbQuadrangles();
    rtrn.numPoly = info.NbPolygons();
    rtrn.numVolu = info.NbVolumes();
    rtrn.numTetr = info.NbTetras();
    rtrn.numHexa = info.NbHexas();
    rtrn.numPyrd = info.NbPyramids();
    rtrn.numPris = info.NbPrisms();
    rtrn.numHedr = info.NbPolyhedrons();

    return rtrn;

}
//		for(unsigned int i=0;i<all_elements.size();i++)
//		{
//			//Die Reihenfolge wie hier die Elemente hinzugefügt werden ist sehr wichtig. 
//			//Ansonsten ist eine konsistente Datenstruktur nicht möglich
//			meshds->AddVolumeWithID(
//				meshds->FindNode(all_elements[i][0]),
//				meshds->FindNode(all_elements[i][2]),
//				meshds->FindNode(all_elements[i][1]),
//				meshds->FindNode(all_elements[i][3]),
//				meshds->FindNode(all_elements[i][6]),
//				meshds->FindNode(all_elements[i][5]),
//				meshds->FindNode(all_elements[i][4]),
//				meshds->FindNode(all_elements[i][9]),
//				meshds->FindNode(all_elements[i][7]),
//				meshds->FindNode(all_elements[i][8]),
//				element_id[i]
//			);
//		}

Base::Quantity FemMesh::getVolume(void)const
{
	SMDS_VolumeIteratorPtr aVolIter = myMesh->GetMeshDS()->volumesIterator();

	//Calculate Mesh Volume
	//For an accurate Volume Calculation of a quadratic Tetrahedron
	//we have to calculate the Volume of 8 Sub-Tetrahedrons
	Base::Vector3d a,b,c,a_b_product;
	double volume = 0.0;

	for (;aVolIter->more();) 
	{
        const SMDS_MeshVolume* aVol = aVolIter->next();
        
        if ( aVol->NbNodes() != 10 ) continue;

        Base::Vector3d v1(aVol->GetNode(1)->X(),aVol->GetNode(1)->Y(),aVol->GetNode(1)->Z());
        Base::Vector3d v0(aVol->GetNode(0)->X(),aVol->GetNode(0)->Y(),aVol->GetNode(0)->Z());
        Base::Vector3d v2(aVol->GetNode(2)->X(),aVol->GetNode(2)->Y(),aVol->GetNode(2)->Z());
        Base::Vector3d v3(aVol->GetNode(3)->X(),aVol->GetNode(3)->Y(),aVol->GetNode(3)->Z());
        Base::Vector3d v4(aVol->GetNode(4)->X(),aVol->GetNode(4)->Y(),aVol->GetNode(4)->Z());
        Base::Vector3d v6(aVol->GetNode(6)->X(),aVol->GetNode(6)->Y(),aVol->GetNode(6)->Z());
        Base::Vector3d v5(aVol->GetNode(5)->X(),aVol->GetNode(5)->Y(),aVol->GetNode(5)->Z());
        Base::Vector3d v8(aVol->GetNode(8)->X(),aVol->GetNode(8)->Y(),aVol->GetNode(8)->Z());
        Base::Vector3d v7(aVol->GetNode(7)->X(),aVol->GetNode(7)->Y(),aVol->GetNode(7)->Z());
        Base::Vector3d v9(aVol->GetNode(9)->X(),aVol->GetNode(9)->Y(),aVol->GetNode(9)->Z());


		//1,5,8,7
		a = v4 -v0 ;
		b = v7 -v0 ;
		c = v6 -v0 ;
		a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
		volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
		//5,9,8,7
		a = v8 -v4 ;
		b = v7 -v4 ;
		c = v6 -v4 ;
		a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
		volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
		//5,2,9,7
		a = v1 -v4 ;
		b = v8 -v4 ;
		c = v6 -v4 ;
		a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
		volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
		//2,6,9,7
		a = v5 -v1 ;
		b = v8 -v1 ;
		c = v6 -v1 ;
		a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
		volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
		//9,6,10,7
		a = v5 -v8 ;
		b = v9 -v8 ;
		c = v6 -v8 ;
		a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
		volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
		//6,3,10,7
		a = v2 -v5 ;
		b = v9 -v5 ;
		c = v6 -v5 ;
		a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
		volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
		//8,9,10,7
		a = v8 -v7 ;
		b = v9 -v7 ;
		c = v6 -v7 ;
		a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
		volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
		//8,9,10,4
		a = v8 -v7 ;
		b = v9 -v7 ;
		c = v3 -v7 ;
		a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
		volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
	
	}

    return Base::Quantity(volume,Unit::Volume);


}