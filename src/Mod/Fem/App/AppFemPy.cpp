/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <Python.h>
# include <memory>
#endif

#include <Base/Console.h>
#include <Base/VectorPy.h>
#include <Base/PlacementPy.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Evaluation.h>
#include <Mod/Mesh/App/Core/Iterator.h>

#include <SMESH_Gen.hxx>
#include <SMESH_Group.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMDS_MeshNode.hxx>
#include <StdMeshers_MaxLength.hxx>
#include <StdMeshers_LocalLength.hxx>
#include <StdMeshers_NumberOfSegments.hxx>
#include <StdMeshers_AutomaticLength.hxx>
#include <StdMeshers_TrianglePreference.hxx>
#include <StdMeshers_MEFISTO_2D.hxx>
#include <StdMeshers_Deflection1D.hxx>
#include <StdMeshers_MaxElementArea.hxx>
#include <StdMeshers_Regular_1D.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
#include <StdMeshers_Quadrangle_2D.hxx>

#include <StdMeshers_LengthFromEdges.hxx>
#include <StdMeshers_NotConformAllowed.hxx>
#include <StdMeshers_Arithmetic1D.hxx>

#include "FemMesh.h"
#include "FemMeshObject.h"
#include "FemMeshPy.h"

#include <cstdlib>

#include <Standard_Real.hxx>
#include "Base/Vector3D.h"

using namespace Fem;


/* module functions */
static PyObject * read(PyObject *self, PyObject *args)
{
    const char* Name;
    if (!PyArg_ParseTuple(args, "s",&Name))
        return NULL;

    PY_TRY {
        std::auto_ptr<FemMesh> mesh(new FemMesh);
        try {
            mesh->read(Name);
            return new FemMeshPy(mesh.release());
        }
        catch(...) {
            PyErr_SetString(PyExc_Exception, "Loading of mesh was aborted");
            return NULL;
        }
    } PY_CATCH;

    Py_Return;
}

static PyObject * open(PyObject *self, PyObject *args)
{
    const char* Name;
    if (!PyArg_ParseTuple(args, "s",&Name))
        return NULL;

    PY_TRY {
        std::auto_ptr<FemMesh> mesh(new FemMesh);
        mesh->read(Name);
        Base::FileInfo file(Name);
        // create new document and add Import feature
        App::Document *pcDoc = App::GetApplication().newDocument("Unnamed");
        FemMeshObject *pcFeature = static_cast<FemMeshObject *>
            (pcDoc->addObject("Fem::FemMeshObject", file.fileNamePure().c_str()));
        pcFeature->Label.setValue(file.fileNamePure().c_str());
        pcFeature->FemMesh.setValuePtr(mesh.get());
        (void)mesh.release();
        pcFeature->purgeTouched();
    } PY_CATCH;

    Py_Return;
}


static PyObject * SMESH_PCA(PyObject *self, PyObject *args)
{
	PyObject *input;

	if (!PyArg_ParseTuple(args, "O",&input))
		return NULL;

	PY_TRY {

		FemMeshPy *inputMesh = static_cast<FemMeshPy*>(input); 
		MeshCore::MeshKernel aMesh;
		MeshCore::MeshPointArray vertices;
		vertices.clear();
		MeshCore::MeshFacetArray faces;
		faces.clear();
		MeshCore::MeshPoint current_node;
		SMDS_NodeIteratorPtr aNodeIter = inputMesh->getFemMeshPtr()->getSMesh()->GetMeshDS()->nodesIterator();
		for (;aNodeIter->more();) {
			const SMDS_MeshNode* aNode = aNodeIter->next();
			current_node.Set(float(aNode->X()),float(aNode->Y()),float(aNode->Z()));
			vertices.push_back(current_node);
		}

		MeshCore::MeshFacet aFacet;
		aFacet._aulPoints[0] = 0;aFacet._aulPoints[1] = 1;aFacet._aulPoints[2] = 2;
		faces.push_back(aFacet);
		//Fill the Kernel with the temp smesh structure and delete the current containers
		aMesh.Adopt(vertices,faces);
		MeshCore::MeshEigensystem pca(aMesh);
		pca.Evaluate();
		Base::Matrix4D Trafo = pca.Transform();
		/*Let´s transform the input mesh with the PCA Matrix*/
		inputMesh->getFemMeshPtr()->transformGeometry(Trafo);
		//inputMesh->getFemMeshPtr()->getSMesh()->ExportUNV("C:/PCA_alignment.unv");
	} PY_CATCH;

	Py_Return;
}

static PyObject * calcMeshVolume(PyObject *self, PyObject *args)
{
	PyObject *input;

	if (!PyArg_ParseTuple(args, "O",&input))
		return NULL;

	PY_TRY {

		FemMeshPy *inputMesh = static_cast<FemMeshPy*>(input); 

		SMDS_VolumeIteratorPtr aVolIter = inputMesh->getFemMeshPtr()->getSMesh()->GetMeshDS()->volumesIterator();
		Base::Vector3d a,b,c,a_b_product,temp,temp1;
		double volume =0.0;
		for (;aVolIter->more();) 
		{
			const SMDS_MeshVolume* aVol = aVolIter->next();
			//To make sure that the volume calculation is based on the ABAQUS element convention
			//The following Node mapping from SMESH to ABAQUS is necessary
			//ABAQUS_Node_Number|SMESH_Node_Number
			//0|0
			//1|2
			//2|1
			//3|3
			//4|6
			//5|5
			//6|4
			//7|8
			//8|9
			//9|7
			//The following coordinates of the little pyramids are based on ABAQUS convention and are numbered from
			//1 to 10
			//1,5,8,7
			temp.Set(aVol->GetNode(6)->X(),aVol->GetNode(6)->Y(),aVol->GetNode(6)->Z());
			temp1.Set(aVol->GetNode(0)->X(),aVol->GetNode(0)->Y(),aVol->GetNode(0)->Z());
			a = temp - temp1;
			temp.Set(aVol->GetNode(8)->X(),aVol->GetNode(8)->Y(),aVol->GetNode(8)->Z());
			temp1.Set(aVol->GetNode(0)->X(),aVol->GetNode(0)->Y(),aVol->GetNode(0)->Z());
			b = temp - temp1;
			temp.Set(aVol->GetNode(4)->X(),aVol->GetNode(4)->Y(),aVol->GetNode(4)->Z()); 
			temp1.Set(aVol->GetNode(0)->X(),aVol->GetNode(0)->Y(),aVol->GetNode(0)->Z());
			c = temp - temp1;
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//5,9,8,7
			temp.Set(aVol->GetNode(9)->X(),aVol->GetNode(9)->Y(),aVol->GetNode(9)->Z()); 
			temp1.Set(aVol->GetNode(6)->X(),aVol->GetNode(6)->Y(),aVol->GetNode(6)->Z());
			a = temp - temp1;
			temp.Set(aVol->GetNode(8)->X(),aVol->GetNode(8)->Y(),aVol->GetNode(8)->Z());
			temp1.Set(aVol->GetNode(6)->X(),aVol->GetNode(6)->Y(),aVol->GetNode(6)->Z());
			b = temp - temp1;
			temp.Set(aVol->GetNode(4)->X(),aVol->GetNode(4)->Y(),aVol->GetNode(4)->Z());
			temp1.Set(aVol->GetNode(6)->X(),aVol->GetNode(6)->Y(),aVol->GetNode(6)->Z());
			c = temp - temp1;
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//5,2,9,7
			temp.Set(aVol->GetNode(2)->X(),aVol->GetNode(2)->Y(),aVol->GetNode(2)->Z());
			temp1.Set(aVol->GetNode(6)->X(),aVol->GetNode(6)->Y(),aVol->GetNode(6)->Z());
			a = temp - temp1;
			temp.Set(aVol->GetNode(9)->X(),aVol->GetNode(9)->Y(),aVol->GetNode(9)->Z()); 
			temp1.Set(aVol->GetNode(6)->X(),aVol->GetNode(6)->Y(),aVol->GetNode(6)->Z());
			b = temp - temp1;
			temp.Set(aVol->GetNode(4)->X(),aVol->GetNode(4)->Y(),aVol->GetNode(4)->Z()); 
			temp1.Set(aVol->GetNode(6)->X(),aVol->GetNode(6)->Y(),aVol->GetNode(6)->Z());
			c = temp - temp1;
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//2,6,9,7
			temp.Set(aVol->GetNode(5)->X(),aVol->GetNode(5)->Y(),aVol->GetNode(5)->Z());
			temp1.Set(aVol->GetNode(2)->X(),aVol->GetNode(2)->Y(),aVol->GetNode(2)->Z());
			a = temp - temp1;
			temp.Set(aVol->GetNode(9)->X(),aVol->GetNode(9)->Y(),aVol->GetNode(9)->Z()); 
			temp1.Set(aVol->GetNode(2)->X(),aVol->GetNode(2)->Y(),aVol->GetNode(2)->Z());
			b = temp - temp1;
			temp.Set(aVol->GetNode(4)->X(),aVol->GetNode(4)->Y(),aVol->GetNode(4)->Z());
			temp1.Set(aVol->GetNode(2)->X(),aVol->GetNode(2)->Y(),aVol->GetNode(2)->Z());
			c = temp - temp1;
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//9,6,10,7
			temp.Set(aVol->GetNode(5)->X(),aVol->GetNode(5)->Y(),aVol->GetNode(5)->Z()); 
			temp1.Set(aVol->GetNode(9)->X(),aVol->GetNode(9)->Y(),aVol->GetNode(9)->Z());
			a = temp - temp1;
			temp.Set(aVol->GetNode(7)->X(),aVol->GetNode(7)->Y(),aVol->GetNode(7)->Z()); 
			temp1.Set(aVol->GetNode(9)->X(),aVol->GetNode(9)->Y(),aVol->GetNode(9)->Z());
			b = temp - temp1;
			temp.Set(aVol->GetNode(4)->X(),aVol->GetNode(4)->Y(),aVol->GetNode(4)->Z()); 
			temp1.Set(aVol->GetNode(9)->X(),aVol->GetNode(9)->Y(),aVol->GetNode(9)->Z());
			c = temp - temp1;
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//6,3,10,7
			temp.Set(aVol->GetNode(1)->X(),aVol->GetNode(1)->Y(),aVol->GetNode(1)->Z());
			temp1.Set(aVol->GetNode(5)->X(),aVol->GetNode(5)->Y(),aVol->GetNode(5)->Z());
			a = temp - temp1;
			temp.Set(aVol->GetNode(7)->X(),aVol->GetNode(7)->Y(),aVol->GetNode(7)->Z()); 
			temp1.Set(aVol->GetNode(5)->X(),aVol->GetNode(5)->Y(),aVol->GetNode(5)->Z());
			b = temp - temp1;
			temp.Set(aVol->GetNode(4)->X(),aVol->GetNode(4)->Y(),aVol->GetNode(4)->Z()); 
			temp1.Set(aVol->GetNode(5)->X(),aVol->GetNode(5)->Y(),aVol->GetNode(5)->Z());
			c = temp - temp1;
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//8,9,10,7
			temp.Set(aVol->GetNode(9)->X(),aVol->GetNode(9)->Y(),aVol->GetNode(9)->Z()); 
			temp1.Set(aVol->GetNode(8)->X(),aVol->GetNode(8)->Y(),aVol->GetNode(8)->Z());
			a = temp - temp1;
			temp.Set(aVol->GetNode(7)->X(),aVol->GetNode(7)->Y(),aVol->GetNode(7)->Z()); 
			temp1.Set(aVol->GetNode(8)->X(),aVol->GetNode(8)->Y(),aVol->GetNode(8)->Z());
			b = temp - temp1;
			temp.Set(aVol->GetNode(4)->X(),aVol->GetNode(4)->Y(),aVol->GetNode(4)->Z()); 
			temp1.Set(aVol->GetNode(8)->X(),aVol->GetNode(8)->Y(),aVol->GetNode(8)->Z());
			c = temp - temp1;
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//8,9,10,4
			temp.Set(aVol->GetNode(9)->X(),aVol->GetNode(9)->Y(),aVol->GetNode(9)->Z());
			temp1.Set(aVol->GetNode(8)->X(),aVol->GetNode(8)->Y(),aVol->GetNode(8)->Z());
			a = temp - temp1;
			temp.Set(aVol->GetNode(7)->X(),aVol->GetNode(7)->Y(),aVol->GetNode(7)->Z()); 
			temp1.Set(aVol->GetNode(8)->X(),aVol->GetNode(8)->Y(),aVol->GetNode(8)->Z());
			b = temp - temp1;
			temp.Set(aVol->GetNode(3)->X(),aVol->GetNode(3)->Y(),aVol->GetNode(3)->Z()); 
			temp1.Set(aVol->GetNode(8)->X(),aVol->GetNode(8)->Y(),aVol->GetNode(8)->Z());
			c = temp - temp1;
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
		}
		Py::Float py_volume(volume); 
		return Py::new_reference_to(py_volume);

	} PY_CATCH;

	Py_Return;
}

static PyObject * checkBB(PyObject *self, PyObject *args)
{
	PyObject *input;
	PyObject* plm=0;
	float billet_thickness;
	bool oversize = false;

    if (!PyArg_ParseTuple(args, "O|O!f", &input,&(Base::PlacementPy::Type),&plm,&billet_thickness))
        return NULL;

    try {
        Base::Placement* placement = 0;
        if (plm) {
            placement = static_cast<Base::PlacementPy*>(plm)->getPlacementPtr();

        }
		Base::Vector3d current_node;
        Base::Matrix4D matrix = placement->toMatrix();
        FemMeshPy *inputMesh = static_cast<FemMeshPy*>(input); 
		SMDS_NodeIteratorPtr aNodeIter = inputMesh->getFemMeshPtr()->getSMesh()->GetMeshDS()->nodesIterator();
		for (;aNodeIter->more();) {
			const SMDS_MeshNode* aNode = aNodeIter->next();
			current_node.Set(float(aNode->X()),float(aNode->Y()),float(aNode->Z()));
            current_node = matrix * current_node;
			if(current_node.z > billet_thickness || current_node.z < 0.0)
			{
				//lets jump out of the function as soon as we find a 
				//Node that is higher or lower than billet thickness
				oversize = true;
				Py::Boolean py_oversize(oversize); 
				return Py::new_reference_to(py_oversize);
			}
		}
		Py::Boolean py_oversize(oversize); 
		return Py::new_reference_to(py_oversize);
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return 0;
    }
    Py_Return;
}




static PyObject * getBoundary_Conditions(PyObject *self, PyObject *args)
{
	PyObject *input;
	Py::List boundary_nodes;

	if (!PyArg_ParseTuple(args, "O",&input))
		return NULL;

	PY_TRY {

		FemMeshPy *inputMesh = static_cast<FemMeshPy*>(input); 
		MeshCore::MeshKernel aMesh;
		MeshCore::MeshPointArray vertices;
		vertices.clear();
		MeshCore::MeshFacetArray faces;
		faces.clear();
		MeshCore::MeshPoint current_node;
		SMDS_NodeIteratorPtr aNodeIter = inputMesh->getFemMeshPtr()->getSMesh()->GetMeshDS()->nodesIterator();
		SMDS_VolumeIteratorPtr aVolIter = inputMesh->getFemMeshPtr()->getSMesh()->GetMeshDS()->volumesIterator();
		for (;aNodeIter->more();) {
			const SMDS_MeshNode* aNode = aNodeIter->next();
			current_node.Set(float(aNode->X()),float(aNode->Y()),float(aNode->Z()));
			vertices.push_back(current_node);
		}
		MeshCore::MeshFacet aFacet;
		aFacet._aulPoints[0] = 0;aFacet._aulPoints[1] = 1;aFacet._aulPoints[2] = 2;
		faces.push_back(aFacet);
		//Fill the Kernel with the temp smesh structure and delete the current containers
		aMesh.Adopt(vertices,faces);
		Base::BoundBox3f aBBox;
		aBBox = aMesh.GetBoundBox();

		float dist_length;
        Base::Vector3f dist;
		int minNodeID,maxNodeID,midNodeID;
		dist_length = FLOAT_MAX;
		
		aVolIter = inputMesh->getFemMeshPtr()->getSMesh()->GetMeshDS()->volumesIterator();
		//We only search in non midside nodes (equals the first four nodes of each element)
		for (;aVolIter->more();) 
		{
			const SMDS_MeshVolume* aVol = aVolIter->next();
			for (unsigned j=0;j<4;j++)
			{
				const SMDS_MeshNode* aNode = aVol->GetNode(j);
				//Calc distance between the lower left corner and the most next point of the mesh
				dist.x = float(aNode->X())-aBBox.MinX;dist.y = float(aNode->Y())-aBBox.MinY;dist.z = float(aNode->Z())-aBBox.MinZ;
				if(dist.Length()<dist_length)
				{
					minNodeID = aNode->GetID();
					dist_length = dist.Length();
				}
			}
		}

		boundary_nodes.append(Py::Int(minNodeID));
		
		dist_length = FLOAT_MAX;
		aVolIter = inputMesh->getFemMeshPtr()->getSMesh()->GetMeshDS()->volumesIterator();
		for (;aVolIter->more();) 
		{
			const SMDS_MeshVolume* aVol = aVolIter->next();
			for (unsigned j=0;j<4;j++)
			{
				const SMDS_MeshNode* aNode = aVol->GetNode(j);
			//Calc distance between the lower right corner and the most next point of the mesh
			dist.x = float(aNode->X())-aBBox.MaxX;dist.y = float(aNode->Y())-aBBox.MinY;dist.z = float(aNode->Z())-aBBox.MinZ;
			if(dist.Length()<dist_length)
			{
				midNodeID = aNode->GetID();
				dist_length = dist.Length();
			}
			}
		}
		boundary_nodes.append(Py::Int(midNodeID));
		
		dist_length = FLOAT_MAX;
		aVolIter = inputMesh->getFemMeshPtr()->getSMesh()->GetMeshDS()->volumesIterator();
		for (;aVolIter->more();) 
		{
			const SMDS_MeshVolume* aVol = aVolIter->next();
			for (unsigned j=0;j<4;j++)
			{
				const SMDS_MeshNode* aNode = aVol->GetNode(j);
			//Calc distance between the lowest lower right corner and the most next point of the mesh
			dist.x = float(aNode->X())-aBBox.MinX;dist.y = float(aNode->Y())-aBBox.MaxY;dist.z = float(aNode->Z())-aBBox.MinZ;
			if(dist.Length()<dist_length)
			{
				maxNodeID = aNode->GetID();
				dist_length = dist.Length();
			}
			}
		}
		boundary_nodes.append(Py::Int(maxNodeID));


		

		return Py::new_reference_to(boundary_nodes);
		
		
	} PY_CATCH;

	Py_Return;
}




static PyObject * minBoundingBox(PyObject *self, PyObject *args)
{
	
	PyObject *input;

	if (!PyArg_ParseTuple(args, "O",&input))
		return NULL;

	PY_TRY {
		FemMeshPy *inputMesh = static_cast<FemMeshPy*>(input); 
		MeshCore::MeshKernel aMesh;
		MeshCore::MeshPointArray vertices;
		vertices.clear();
		MeshCore::MeshFacetArray faces;
		faces.clear();
		MeshCore::MeshPoint current_node;
		SMDS_NodeIteratorPtr aNodeIter = inputMesh->getFemMeshPtr()->getSMesh()->GetMeshDS()->nodesIterator();
		for (;aNodeIter->more();) {
			const SMDS_MeshNode* aNode = aNodeIter->next();
			current_node.Set(float(aNode->X()),float(aNode->Y()),float(aNode->Z()));
			vertices.push_back(current_node);
		}
		MeshCore::MeshFacet aFacet;
		aFacet._aulPoints[0] = 0;aFacet._aulPoints[1] = 1;aFacet._aulPoints[2] = 2;
		faces.push_back(aFacet);
		//Fill the Kernel with the temp smesh structure and delete the current containers
		aMesh.Adopt(vertices,faces);

		///////////////////////////////////////////////////////////////////////////
		//Now do Monte Carlo to minimize the BBox of the part
		//Use Quaternions for the rotation stuff
		Base::Rotation rotatex,rotatey,rotatez;
		const Base::Vector3d rotate_axis_x(1.0,0.0,0.0),rotate_axis_y(0.0,1.0,0.0),rotate_axis_z(0.0,0.0,1.0);

		//Rotate around each axes and choose the settings for the min bbox
		Base::Matrix4D final_trafo;
		Base::BoundBox3f aBBox,min_bbox;
		double volumeBBOX,min_volumeBBOX;
		//Get the current min_volumeBBOX and look if we find a lower one
		aBBox = aMesh.GetBoundBox();
		min_volumeBBOX = aBBox.LengthX()*aBBox.LengthY()*aBBox.LengthZ();

		MeshCore::MeshKernel atempkernel;

		float it_steps=10.0;
		double step_size;
		double alpha_x=0.0,alpha_y=0.0,alpha_z=0.0;
		double perfect_ax=0.0,perfect_ay=0.0,perfect_az=0.0;

		//Do a Monte Carlo approach and start from the Principal Axis System
		//and rotate +/- 60° around each axis in a first iteration
		double	angle_range_min_x=-PI/3.0,angle_range_max_x=PI/3.0,
			angle_range_min_y=-PI/3.0,angle_range_max_y=PI/3.0,
			angle_range_min_z=-PI/3.0,angle_range_max_z=PI/3.0;

		//We rotate until we are 0.1° sure to be in the right position
		for (step_size = (2.0*PI/it_steps);step_size>(2.0*PI/3600.0);step_size=(2.0*PI/it_steps))
		{
			for(alpha_x=angle_range_min_x;alpha_x<angle_range_max_x;alpha_x=alpha_x+step_size)
			{
				rotatex.setValue(rotate_axis_x,alpha_x);
				for(alpha_y=angle_range_min_y;alpha_y<angle_range_max_y;alpha_y=alpha_y+step_size)
				{
					rotatey.setValue(rotate_axis_y,alpha_y);
					for(alpha_z=angle_range_min_z;alpha_z<angle_range_max_z;alpha_z=alpha_z+step_size)
					{
						rotatez.setValue(rotate_axis_z,alpha_z);
						(rotatex*rotatey*rotatez).getValue(final_trafo);
						atempkernel = aMesh;
						atempkernel.Transform(final_trafo);
						aBBox = atempkernel.GetBoundBox();
						volumeBBOX = aBBox.LengthX()*aBBox.LengthY()*aBBox.LengthZ();
						if (volumeBBOX < min_volumeBBOX)
						{ 
							min_volumeBBOX=volumeBBOX;
							perfect_ax=alpha_x;
							perfect_ay=alpha_y;
							perfect_az=alpha_z;
						}
					}
				}
			}
			//We found a better position than the PAS, now lets fine tune this position
			//and search only in the corridor +/- step_size for an even better one
			angle_range_min_x = perfect_ax - step_size;
			angle_range_max_x = perfect_ax + step_size;
			angle_range_min_y = perfect_ay - step_size;
			angle_range_max_y = perfect_ay + step_size;
			angle_range_min_z = perfect_az - step_size;
			angle_range_max_z = perfect_az + step_size;
			it_steps = it_steps*float(5.0);
		}

		//////////////////////////////////////////////////////////////////////////////////////
		//Free Memory
		atempkernel.Clear();
		//Transform the mesh to the evaluated perfect position right now
		rotatex.setValue(rotate_axis_x,perfect_ax);
		rotatey.setValue(rotate_axis_y,perfect_ay);
		rotatez.setValue(rotate_axis_z,perfect_az);
		(rotatex*rotatey*rotatez).getValue(final_trafo);
		aMesh.Transform(final_trafo);
		inputMesh->getFemMeshPtr()->transformGeometry(final_trafo);
		//////////////////////////////////////////////////////////////////////////////////////
		//Now lets also do the movement to the 1st Quadrant in this function
		aBBox = aMesh.GetBoundBox();
		//Get Distance vector from current lower left corner of BBox to origin
		Base::Vector3f dist_vector;
		dist_vector.x = -aBBox.MinX;dist_vector.y=-aBBox.MinY;dist_vector.z=-aBBox.MinZ;
		Base::Matrix4D trans_matrix(
			float(1.0),float(0.0),float(0.0),dist_vector.x,
			float(0.0),float(1.0),float(0.0),dist_vector.y,
			float(0.0),float(0.0),float(1.0),dist_vector.z,
			float(0.0),float(0.0),float(0.0),float(1.0));
		inputMesh->getFemMeshPtr()->transformGeometry(trans_matrix);
		
		//inputMesh->getFemMeshPtr()->getSMesh()->ExportUNV("C:/fine_tuning.unv");

	} PY_CATCH;

	Py_Return;
}


static PyObject * importer(PyObject *self, PyObject *args)
{
    const char* Name;
    const char* DocName=0;

    if (!PyArg_ParseTuple(args, "s|s",&Name,&DocName))
        return NULL;

    PY_TRY {
        App::Document *pcDoc = 0;
        if (DocName)
            pcDoc = App::GetApplication().getDocument(DocName);
        else
            pcDoc = App::GetApplication().getActiveDocument();

        if (!pcDoc) {
            pcDoc = App::GetApplication().newDocument(DocName);
        }

        std::auto_ptr<FemMesh> mesh(new FemMesh);
        mesh->read(Name);
        Base::FileInfo file(Name);
        FemMeshObject *pcFeature = static_cast<FemMeshObject *>
            (pcDoc->addObject("Fem::FemMeshObject", file.fileNamePure().c_str()));
        pcFeature->Label.setValue(file.fileNamePure().c_str());
        pcFeature->FemMesh.setValuePtr(mesh.get());
        (void)mesh.release();
        pcFeature->purgeTouched();
    } PY_CATCH;

    Py_Return;
}


static PyObject * import_NASTRAN(PyObject *self, PyObject *args)
{
	const char* filename_input, *filename_output;
	if (!PyArg_ParseTuple(args, "ss",&filename_input,&filename_output))
		return NULL;

	PY_TRY {

		std::ifstream inputfile;

		//Für Debugoutput
		ofstream anOutput;
		anOutput.open("c:/time_measurement.txt");
		time_t seconds1,seconds2;
		
		inputfile.open(filename_input);
		if (!inputfile.is_open())  //Exists...?
		{
			std::cerr << "File not found. Exiting..." << std::endl;
			return NULL;
		}

		//Return the line pointer to the beginning of the file
		inputfile.seekg(std::ifstream::beg);
		std::string line1,line2,temp;
		std::stringstream astream;
		std::vector<unsigned int> tetra_element;
		std::vector<unsigned int> element_id;
		element_id.clear();
		std::vector<std::vector<unsigned int> > all_elements;
		std::vector<std::vector<unsigned int> >::iterator all_element_iterator;
		std::vector<unsigned int>::iterator one_element_iterator;
		all_elements.clear();
		MeshCore::MeshKernel aMesh;
		MeshCore::MeshPointArray vertices;
		vertices.clear();
		MeshCore::MeshFacetArray faces;
		faces.clear();
		MeshCore::MeshPoint current_node;

		seconds1 = time(NULL);
		do
		{
			std::getline(inputfile,line1);
			if (line1.size() == 0) continue;
			if (line1.find("GRID*")!= std::string::npos) //We found a Grid line
			{
				//Now lets extract the GRID Points = Nodes
				//As each GRID Line consists of two subsequent lines we have to
				//take care of that as well
				std::getline(inputfile,line2);
				//Extract X Value
				current_node.x = float(atof(line1.substr(40,56).c_str()));
				//Extract Y Value
				current_node.y = float(atof(line1.substr(56,72).c_str()));
				//Extract Z Value
				current_node.z = float(atof(line2.substr(8,24).c_str()));
				
				vertices.push_back(current_node);
			}
			else if (line1.find("CTETRA")!= std::string::npos)
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
			
		}
		while (inputfile.good());
		inputfile.close();

		seconds2 = time(NULL);

		anOutput << seconds2-seconds1 <<" for Parsing the input file"<<std::endl;
		//Now lets perform some minimization routines to min the bbox volume
		//To evaluate the COG and principal axes we have to generate a "Mesh" out of the points
		//therefore at least one face is required
		MeshCore::MeshFacet aFacet;
		aFacet._aulPoints[0] = 0;aFacet._aulPoints[1] = 1;aFacet._aulPoints[2] = 2;
		faces.push_back(aFacet);
		//Fill the Kernel with the structure
	    aMesh.Assign(vertices,faces);
		
		seconds1 = time(NULL);
		//First lets get the COG and the principal axes and transfer the mesh there
		MeshCore::MeshEigensystem pca(aMesh);
		pca.Evaluate();
		Base::Matrix4D Trafo =  pca.Transform();
		aMesh.Transform(Trafo);
		///////////////////////////////////////////////////////////////////////////
		//To get the center of gravity we have to build the inverse of the pca Matrix
		pca.Evaluate();
		Trafo =  pca.Transform();
		Trafo.inverse();
		Base::Vector3f cog;
		cog.x = float(Trafo[0][3]);cog.y = float(Trafo[1][3]);cog.z = float(Trafo[2][3]);
		///////////////////////////////////////////////////////////////////////////
		//Now do Monte Carlo to minimize the BBox of the part
		//Use Quaternions for the rotation stuff
		Base::Rotation rotatex,rotatey,rotatez;
		const Base::Vector3d rotate_axis_x(1.0,0.0,0.0),rotate_axis_y(0.0,1.0,0.0),rotate_axis_z(0.0,0.0,1.0);
		
		//Rotate around each axes and choose the settings for the min bbox
		Base::Matrix4D final_trafo;
		Base::BoundBox3f aBBox,min_bbox;


		double volumeBBOX,min_volumeBBOX;

		//Get the current min_volumeBBOX and look if we find a lower one
		aBBox = aMesh.GetBoundBox();
		min_volumeBBOX = aBBox.LengthX()*aBBox.LengthY()*aBBox.LengthZ();

	
		MeshCore::MeshKernel atempkernel;

		float it_steps=10.0;
		double step_size;
		double alpha_x=0.0,alpha_y=0.0,alpha_z=0.0;
		double perfect_ax=0.0,perfect_ay=0.0,perfect_az=0.0;

		//Do a Monte Carlo approach and start from the Principal Axis System
		//and rotate +/- 60° around each axis in a first iteration
		double	angle_range_min_x=-PI/3.0,angle_range_max_x=PI/3.0,
				angle_range_min_y=-PI/3.0,angle_range_max_y=PI/3.0,
				angle_range_min_z=-PI/3.0,angle_range_max_z=PI/3.0;
		
		for (step_size = (2.0*PI/it_steps);step_size>(2.0*PI/360.0);step_size=(2.0*PI/it_steps))
		{
			for(alpha_x=angle_range_min_x;alpha_x<angle_range_max_x;alpha_x=alpha_x+step_size)
			{
				rotatex.setValue(rotate_axis_x,alpha_x);
				for(alpha_y=angle_range_min_y;alpha_y<angle_range_max_y;alpha_y=alpha_y+step_size)
				{
					rotatey.setValue(rotate_axis_y,alpha_y);
					for(alpha_z=angle_range_min_z;alpha_z<angle_range_max_z;alpha_z=alpha_z+step_size)
					{
						rotatez.setValue(rotate_axis_z,alpha_z);
						(rotatex*rotatey*rotatez).getValue(final_trafo);
						atempkernel = aMesh;
						atempkernel.Transform(final_trafo);
						aBBox = atempkernel.GetBoundBox();
						volumeBBOX = aBBox.LengthX()*aBBox.LengthY()*aBBox.LengthZ();
						if (volumeBBOX < min_volumeBBOX)
						{ 
							min_volumeBBOX=volumeBBOX;
							perfect_ax=alpha_x;
							perfect_ay=alpha_y;
							perfect_az=alpha_z;
						}
					}
				}
			}
			//We found a better position than the PAS, now lets fine tune this position
			//and search only in the corridor +/- step_size for an even better one
			angle_range_min_x = perfect_ax - step_size;
			angle_range_max_x = perfect_ax + step_size;
			angle_range_min_y = perfect_ay - step_size;
			angle_range_max_y = perfect_ay + step_size;
			angle_range_min_z = perfect_az - step_size;
			angle_range_max_z = perfect_az + step_size;
			it_steps = it_steps*float(5.0);
		}

		//////////////////////////////////////////////////////////////////////////////////////
		
		//Free Memory
		atempkernel.Clear();

		//Transform the mesh to the evaluated perfect position right now
		rotatex.setValue(rotate_axis_x,perfect_ax);
		rotatey.setValue(rotate_axis_y,perfect_ay);
		rotatez.setValue(rotate_axis_z,perfect_az);
		(rotatex*rotatey*rotatez).getValue(final_trafo);
		aMesh.Transform(final_trafo);

		anOutput << "perfect angles " << perfect_ax << "," << perfect_ay << "," << perfect_az << std::endl;


		//Move Mesh to stay fully in the positive octant
		aBBox = aMesh.GetBoundBox();
		//Get Distance vector from current lower left corner of BBox to origin
		Base::Vector3f dist_vector;
		dist_vector.x = -aBBox.MinX;dist_vector.y=-aBBox.MinY;dist_vector.z=-aBBox.MinZ;
		Base::Matrix4D trans_matrix(
			float(1.0),float(0.0),float(0.0),dist_vector.x,
			float(0.0),float(1.0),float(0.0),dist_vector.y,
			float(0.0),float(0.0),float(1.0),dist_vector.z,
			float(0.0),float(0.0),float(0.0),float(1.0));
		aMesh.Transform(trans_matrix);
		/////////////////////////////////////////////////////////////////////////////////
		seconds2=time(NULL);
		anOutput << seconds2-seconds1 << " seconds for the min bounding box stuff" << std::endl;
		
		seconds1 = time(NULL);
		//Try to build up an own mesh kernel within SMESH to export the 
		//perfect positioned mesh right now
		SMESH_Gen* meshgen = new SMESH_Gen();
		SMESH_Mesh* mesh = meshgen->CreateMesh(1, true);
		SMESHDS_Mesh* meshds = mesh->GetMeshDS();
		
		MeshCore::MeshPointIterator anodeiterator(aMesh);
		int j=1;
		for(anodeiterator.Begin(); anodeiterator.More(); anodeiterator.Next())
		{
			meshds->AddNodeWithID((*anodeiterator).x,(*anodeiterator).y,(*anodeiterator).z,j);
			j++;
		}

		for(unsigned int i=0;i<all_elements.size();i++)
		{
			//Die Reihenfolge wie hier die Elemente hinzugefügt werden ist sehr wichtig. 
			//Ansonsten ist eine konsistente Datenstruktur nicht möglich
			meshds->AddVolumeWithID(
				meshds->FindNode(all_elements[i][0]),
				meshds->FindNode(all_elements[i][2]),
				meshds->FindNode(all_elements[i][1]),
				meshds->FindNode(all_elements[i][3]),
				meshds->FindNode(all_elements[i][6]),
				meshds->FindNode(all_elements[i][5]),
				meshds->FindNode(all_elements[i][4]),
				meshds->FindNode(all_elements[i][9]),
				meshds->FindNode(all_elements[i][7]),
				meshds->FindNode(all_elements[i][8]),
				element_id[i]
			);
		}

		mesh->ExportUNV(filename_output);
		//////////////////////////////////////////////////////////////////////////////////////////
		seconds2 = time(NULL);
		anOutput << seconds2-seconds1 << " seconds for the Mesh Export" << std::endl;

		//Output also to ABAQUS Input Format
		ofstream anABAQUS_Output;
		anABAQUS_Output.open("d:/abaqus_output.inp");
		anABAQUS_Output << "*Node , NSET=Nall" << std::endl;
		j=1;
		for(anodeiterator.Begin(); anodeiterator.More(); anodeiterator.Next())
		{
			anABAQUS_Output << j <<","
		   <<(*anodeiterator).x << "," 
		   <<(*anodeiterator).y << ","
		   <<(*anodeiterator).z << std::endl;
			j++;
		}
		anABAQUS_Output << "*Element, TYPE=C3D10, ELSET=Eall" << std::endl;
		j=1;
		for(unsigned int i=0;i<all_elements.size();i++)
		{
			//In ABAQUS input format a maximum of 15 Nodes per line is allowed
			anABAQUS_Output 
			<<j <<","
			<<all_elements[i][0]<<","
			<<all_elements[i][1]<<","
			<<all_elements[i][2]<<","
			<<all_elements[i][3]<<","
			<<all_elements[i][4]<<","
			<<all_elements[i][5]<<","
			<<all_elements[i][6]<<","
			<<all_elements[i][7]<<","
			<<all_elements[i][8]<<","
			<<all_elements[i][9]<<std::endl;
			j++;
		}
		anABAQUS_Output.close();
		/////////////////////////////////////////////////////////////////////////////


		//Calculate Mesh Volume
		//For an accurate Volume Calculation of a quadratic Tetrahedron
		//we have to calculate the Volume of 8 Sub-Tetrahedrons
		Base::Vector3f a,b,c,a_b_product;
		double volume = 0.0;
		seconds1 = time(NULL);
		//Calc Volume with 1/6 * |(a x b) * c|
		for(all_element_iterator = all_elements.begin();all_element_iterator != all_elements.end();all_element_iterator++)
		{
			//1,5,8,7
			a = vertices[all_element_iterator->at(4)-1]-vertices[all_element_iterator->at(0)-1];
			b = vertices[all_element_iterator->at(7)-1]-vertices[all_element_iterator->at(0)-1];
			c = vertices[all_element_iterator->at(6)-1]-vertices[all_element_iterator->at(0)-1];
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//5,9,8,7
			a = vertices[all_element_iterator->at(8)-1]-vertices[all_element_iterator->at(4)-1];
			b = vertices[all_element_iterator->at(7)-1]-vertices[all_element_iterator->at(4)-1];
			c = vertices[all_element_iterator->at(6)-1]-vertices[all_element_iterator->at(4)-1];
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//5,2,9,7
			a = vertices[all_element_iterator->at(1)-1]-vertices[all_element_iterator->at(4)-1];
			b = vertices[all_element_iterator->at(8)-1]-vertices[all_element_iterator->at(4)-1];
			c = vertices[all_element_iterator->at(6)-1]-vertices[all_element_iterator->at(4)-1];
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//2,6,9,7
			a = vertices[all_element_iterator->at(5)-1]-vertices[all_element_iterator->at(1)-1];
			b = vertices[all_element_iterator->at(8)-1]-vertices[all_element_iterator->at(1)-1];
			c = vertices[all_element_iterator->at(6)-1]-vertices[all_element_iterator->at(1)-1];
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//9,6,10,7
			a = vertices[all_element_iterator->at(5)-1]-vertices[all_element_iterator->at(8)-1];
			b = vertices[all_element_iterator->at(9)-1]-vertices[all_element_iterator->at(8)-1];
			c = vertices[all_element_iterator->at(6)-1]-vertices[all_element_iterator->at(8)-1];
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//6,3,10,7
			a = vertices[all_element_iterator->at(2)-1]-vertices[all_element_iterator->at(5)-1];
			b = vertices[all_element_iterator->at(9)-1]-vertices[all_element_iterator->at(5)-1];
			c = vertices[all_element_iterator->at(6)-1]-vertices[all_element_iterator->at(5)-1];
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//8,9,10,7
			a = vertices[all_element_iterator->at(8)-1]-vertices[all_element_iterator->at(7)-1];
			b = vertices[all_element_iterator->at(9)-1]-vertices[all_element_iterator->at(7)-1];
			c = vertices[all_element_iterator->at(6)-1]-vertices[all_element_iterator->at(7)-1];
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
			//8,9,10,4
			a = vertices[all_element_iterator->at(8)-1]-vertices[all_element_iterator->at(7)-1];
			b = vertices[all_element_iterator->at(9)-1]-vertices[all_element_iterator->at(7)-1];
			c = vertices[all_element_iterator->at(3)-1]-vertices[all_element_iterator->at(7)-1];
			a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
			volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
		
		}
		seconds2=time(NULL);
		anOutput << seconds2-seconds1 << " seconds for Volume Calculation  " << "Volumen " << volume/1000.0/1000.0/1000.0 << " in m^3" << std::endl;
		anOutput  << "Volumen der BBox" << min_volumeBBOX/1000.0/1000.0/1000.0 << std::endl;
		anOutput << "Fly to Buy Ratio: " << min_volumeBBOX / volume << std::endl;
		anOutput.close();
		
	} PY_CATCH;

	Py_Return;
}


static PyObject * exporter(PyObject *self, PyObject *args)
{
    PyObject* object;
    const char* filename;
    if (!PyArg_ParseTuple(args, "Os",&object,&filename))
        return NULL;

    PY_TRY {
        Py::List list(object);
        Base::Type meshId = Base::Type::fromName("Fem::FemMeshObject");
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                if (obj->getTypeId().isDerivedFrom(meshId)) {
                    static_cast<FemMeshObject*>(obj)->FemMesh.getValue().write(filename);
                    break;
                }
            }
        }
    } PY_CATCH;

    Py_Return;
}

// ----------------------------------------------------------------------------

PyDoc_STRVAR(open_doc,
"open(string) -- Create a new document and a Mesh::Import feature to load the file into the document.");

PyDoc_STRVAR(inst_doc,
"insert(string|mesh,[string]) -- Load or insert a mesh into the given or active document.");

PyDoc_STRVAR(export_doc,
"export(list,string) -- Export a list of objects into a single file.");

/* registration table  */
struct PyMethodDef Fem_methods[] = {
    {"open"       ,open ,       METH_VARARGS, open_doc},
    {"insert"     ,importer,    METH_VARARGS, inst_doc},
    {"export"     ,exporter,    METH_VARARGS, export_doc},
    {"read"       ,read,        Py_NEWARGS,   "Read a mesh from a file and returns a Mesh object."},
{"calcMeshVolume", calcMeshVolume, Py_NEWARGS, "Calculate Mesh Volume for C3D10"},	
{"getBoundary_Conditions" , getBoundary_Conditions, Py_NEWARGS, "Get Boundary Conditions for Residual Stress Calculation"},
	{"SMESH_PCA" , SMESH_PCA, Py_NEWARGS, "Get a Matrix4D related to the PCA of a Mesh Object"},
	{"import_NASTRAN",import_NASTRAN, Py_NEWARGS, "Test"},
	{"minBoundingBox",minBoundingBox,Py_NEWARGS,"Minimize the Bounding Box and reorient the mesh to the 1st Quadrant"},
	{"checkBB",checkBB,Py_NEWARGS,"Check if the nodal z-values are still in the prescribed range"},
    {NULL, NULL}  /* sentinel */
};
