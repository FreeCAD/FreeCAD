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
# include <Standard_math.hxx>
# include <Inventor/SoDB.h>
# include <Inventor/SoInput.h>
# include <Inventor/SbVec3f.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoLightModel.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoRotation.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoPointSet.h>
# include <Inventor/nodes/SoPolygonOffset.h>
# include <QFile>
#endif

#include "ViewProviderFemMesh.h"

#include <Mod/Fem/App/FemMeshObject.h>
#include <Mod/Fem/App/FemMesh.h>
#include <Gui/SoFCSelection.h>
#include <App/Document.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/Console.h>
#include <Base/TimeInfo.h>
#include <Base/BoundBox.h>
#include <sstream>

#include <SMESH_Mesh.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMDSAbs_ElementType.hxx>

using namespace FemGui;






struct FemFace 
{
	const SMDS_MeshNode *Nodes[8];
	unsigned long  ElementNumber; 
    const SMDS_MeshElement* Element;
	unsigned short Size;
	unsigned short FaceNo;
    bool hide;
    Base::Vector3d getFirstNodePoint(void) {
        return Base::Vector3d(Nodes[0]->X(),Nodes[0]->Y(),Nodes[0]->Z());
    }
	
	Base::Vector3d set(short size,const SMDS_MeshElement* element,unsigned short id, short faceNo, const SMDS_MeshNode* n1,const SMDS_MeshNode* n2,const SMDS_MeshNode* n3,const SMDS_MeshNode* n4=0,const SMDS_MeshNode* n5=0,const SMDS_MeshNode* n6=0,const SMDS_MeshNode* n7=0,const SMDS_MeshNode* n8=0);
	
	bool isSameFace (FemFace &face);
};

Base::Vector3d FemFace::set(short size,const SMDS_MeshElement* element,unsigned short id,short faceNo, const SMDS_MeshNode* n1,const SMDS_MeshNode* n2,const SMDS_MeshNode* n3,const SMDS_MeshNode* n4,const SMDS_MeshNode* n5,const SMDS_MeshNode* n6,const SMDS_MeshNode* n7,const SMDS_MeshNode* n8)
{
	Nodes[0] = n1;
	Nodes[1] = n2;
	Nodes[2] = n3;
	Nodes[3] = n4;
	Nodes[4] = n5;
	Nodes[5] = n6;
	Nodes[6] = n7;
	Nodes[7] = n8;

    Element         = element;
    ElementNumber   = id; 
    Size            = size;
    FaceNo          = faceNo;
    hide            = false;

	// sorting the nodes for later easier comparison (bubble sort)
    int i, j, flag = 1;    // set flag to 1 to start first pass
	const SMDS_MeshNode* temp;   // holding variable
	
	for(i = 1; (i <= size) && flag; i++)
	{
		flag = 0;
		for (j=0; j < (size -1); j++)
		{
			if (Nodes[j+1] > Nodes[j])      // ascending order simply changes to <
			{ 
				temp = Nodes[j];             // swap elements
				Nodes[j] = Nodes[j+1];
				Nodes[j+1] = temp;
				flag = 1;               // indicates that a swap occurred.
			}
		}
	}

    return Base::Vector3d(Nodes[0]->X(),Nodes[0]->Y(),Nodes[0]->Z());
};

class FemFaceGridItem :public std::vector<FemFace*>{
public:
    //FemFaceGridItem(void){reserve(200);}
};

bool FemFace::isSameFace (FemFace &face) 
{
    // the same element can not have the same face
    if(face.ElementNumber == ElementNumber)
        return false;
	assert(face.Size == Size);
	// if the same face size just compare if the sorted nodes are the same
	if( Nodes[0] == face.Nodes[0] &&
	    Nodes[1] == face.Nodes[1] &&
		Nodes[2] == face.Nodes[2] &&
		Nodes[3] == face.Nodes[3] &&
		Nodes[4] == face.Nodes[4] &&
		Nodes[5] == face.Nodes[5] &&
		Nodes[6] == face.Nodes[6] &&
        Nodes[7] == face.Nodes[7] ){
            hide = true;
            face.hide = true;
            return true;
    }

    return false;
};

PROPERTY_SOURCE(FemGui::ViewProviderFemMesh, Gui::ViewProviderGeometryObject)

App::PropertyFloatConstraint::Constraints ViewProviderFemMesh::floatRange = {1.0f,64.0f,1.0f};

ViewProviderFemMesh::ViewProviderFemMesh()
{
    App::Material mat;
    mat.ambientColor.set(0.2f,0.2f,0.2f);
    mat.diffuseColor.set(0.1f,0.1f,0.1f);
    mat.specularColor.set(0.0f,0.0f,0.0f);
    mat.emissiveColor.set(0.0f,0.0f,0.0f);
    mat.shininess = 0.0f;
    mat.transparency = 0.0f;
    ADD_PROPERTY(PointMaterial,(mat));
    ADD_PROPERTY(PointColor,(mat.diffuseColor));
    ADD_PROPERTY(PointSize,(2.0f));
    PointSize.setConstraints(&floatRange);
    ADD_PROPERTY(LineWidth,(4.0f));
    LineWidth.setConstraints(&floatRange);

    ADD_PROPERTY(BackfaceCulling,(true));
    ADD_PROPERTY(ShowInner,      (false));

    pcDrawStyle = new SoDrawStyle();
    pcDrawStyle->ref();
    pcDrawStyle->style = SoDrawStyle::LINES;
    pcDrawStyle->lineWidth = LineWidth.getValue();

    pShapeHints = new SoShapeHints;
    pShapeHints->shapeType = SoShapeHints::SOLID;
    pShapeHints->vertexOrdering = SoShapeHints::CLOCKWISE;
    pShapeHints->ref();

    pcMatBinding = new SoMaterialBinding;
    pcMatBinding->value = SoMaterialBinding::OVERALL;
    pcMatBinding->ref();

    pcCoords = new SoCoordinate3();
    pcCoords->ref();

    pcFaces = new SoIndexedFaceSet;
    pcFaces->ref();

    pcLines = new SoIndexedLineSet;
    pcLines->ref();

    pcPointStyle = new SoDrawStyle();
    pcPointStyle->ref();
    pcPointStyle->style = SoDrawStyle::POINTS;
    pcPointStyle->pointSize = PointSize.getValue();

    pcPointMaterial = new SoMaterial;
    pcPointMaterial->ref();
    PointMaterial.touch();

}

ViewProviderFemMesh::~ViewProviderFemMesh()
{
    pcCoords->unref();
    pcDrawStyle->unref();
    pcFaces->unref();
    pcLines->unref();
    pShapeHints->unref();
    pcMatBinding->unref();
    pcPointMaterial->unref();
    pcPointStyle->unref();

}

void ViewProviderFemMesh::attach(App::DocumentObject *pcObj)
{
    ViewProviderGeometryObject::attach(pcObj);

    // flat
    SoGroup* pcFlatRoot = new SoGroup();
    // face nodes
    pcFlatRoot->addChild(pcCoords);
    pcFlatRoot->addChild(pShapeHints);
    pcFlatRoot->addChild(pcShapeMaterial);
    pcFlatRoot->addChild(pcMatBinding);
    pcFlatRoot->addChild(pcFaces);
    addDisplayMaskMode(pcFlatRoot, "Flat");

    // line
    SoLightModel* pcLightModel = new SoLightModel();
    pcLightModel->model = SoLightModel::BASE_COLOR;
    SoGroup* pcWireRoot = new SoGroup();
    pcWireRoot->addChild(pcCoords);
    pcWireRoot->addChild(pcDrawStyle);
    pcWireRoot->addChild(pcLightModel);
    SoBaseColor* color = new SoBaseColor();
    color->rgb.setValue(0.0f,0.0f,0.0f);
    pcWireRoot->addChild(color);
    pcWireRoot->addChild(pcLines);
    addDisplayMaskMode(pcWireRoot, "Wireframe");


    // Points
    SoGroup* pcPointsRoot = new SoSeparator();
    pcPointsRoot->addChild(pcPointMaterial);  
    pcPointsRoot->addChild(pcPointStyle);
    pcPointsRoot->addChild(pcCoords);
    SoPointSet * pointset = new SoPointSet;
    pcPointsRoot->addChild(pointset);
    addDisplayMaskMode(pcPointsRoot, "Points");

    // flat+line
    SoPolygonOffset* offset = new SoPolygonOffset();
    offset->styles = SoPolygonOffset::LINES;
    offset->factor = -2.0f;
    offset->units = 1.0f;
    SoGroup* pcFlatWireRoot = new SoSeparator();
    // add the complete flat group (contains the coordinates)
    pcFlatWireRoot->addChild(pcFlatRoot);
    pcFlatWireRoot->addChild(offset);
    //pcFlatWireRoot->addChild(pcWireRoot);
    // add the line nodes
    pcFlatWireRoot->addChild(pcDrawStyle);
    pcFlatWireRoot->addChild(pcLightModel);
    pcFlatWireRoot->addChild(color);
    pcFlatWireRoot->addChild(pcLines);

    addDisplayMaskMode(pcFlatWireRoot, "Flat Lines");

    //pcHighlight->addChild(pcCoords);
    //pcHighlight->addChild(pcFaces);
    //pcHighlight->addChild(pcLines);
}

void ViewProviderFemMesh::setDisplayMode(const char* ModeName)
{
    if (strcmp("Flat Lines",ModeName)==0)
        setDisplayMaskMode("Flat Lines");
    else if (strcmp("Shaded",ModeName)==0)
        setDisplayMaskMode("Flat");
    else if (strcmp("Wireframe",ModeName)==0)
        setDisplayMaskMode("Wireframe");
    else if (strcmp("Points",ModeName)==0)
        setDisplayMaskMode("Points");

    ViewProviderGeometryObject::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderFemMesh::getDisplayModes(void) const
{
    std::vector<std::string> StrList;
    StrList.push_back("Flat Lines");
    StrList.push_back("Shaded");
    StrList.push_back("Wireframe");
    StrList.push_back("Points");
    return StrList;
}

void ViewProviderFemMesh::updateData(const App::Property* prop)
{
    if (prop->isDerivedFrom(Fem::PropertyFemMesh::getClassTypeId())) {
        ViewProviderFEMMeshBuilder builder;
        builder.createMesh(prop, pcCoords, pcFaces, pcLines, ShowInner.getValue());
    }
    Gui::ViewProviderGeometryObject::updateData(prop);
}

void ViewProviderFemMesh::onChanged(const App::Property* prop)
{
    if (prop == &PointSize) {
        pcPointStyle->pointSize = PointSize.getValue();
    }
    else if (prop == &PointColor) {
        const App::Color& c = PointColor.getValue();
        pcPointMaterial->diffuseColor.setValue(c.r,c.g,c.b);
        if (c != PointMaterial.getValue().diffuseColor)
        PointMaterial.setDiffuseColor(c);
    }
    else if (prop == &BackfaceCulling) {
        if(BackfaceCulling.getValue()){
            pShapeHints->shapeType = SoShapeHints::SOLID;
            //pShapeHints->vertexOrdering = SoShapeHints::CLOCKWISE;
        }else{
            pShapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
            //pShapeHints->vertexOrdering = SoShapeHints::CLOCKWISE;
        }
    }
    else if (prop == &ShowInner) {
        // recalc mesh with new settings
        ViewProviderFEMMeshBuilder builder;
        builder.createMesh(&(dynamic_cast<Fem::FemMeshObject*>(this->pcObject)->FemMesh), pcCoords, pcFaces, pcLines, ShowInner.getValue());
    }
    else if (prop == &PointMaterial) {
        const App::Material& Mat = PointMaterial.getValue();
        if (PointColor.getValue() != Mat.diffuseColor)
        PointColor.setValue(Mat.diffuseColor);
        pcPointMaterial->ambientColor.setValue(Mat.ambientColor.r,Mat.ambientColor.g,Mat.ambientColor.b);
        pcPointMaterial->diffuseColor.setValue(Mat.diffuseColor.r,Mat.diffuseColor.g,Mat.diffuseColor.b);
        pcPointMaterial->specularColor.setValue(Mat.specularColor.r,Mat.specularColor.g,Mat.specularColor.b);
        pcPointMaterial->emissiveColor.setValue(Mat.emissiveColor.r,Mat.emissiveColor.g,Mat.emissiveColor.b);
        pcPointMaterial->shininess.setValue(Mat.shininess);
        pcPointMaterial->transparency.setValue(Mat.transparency);
    }
    else if (prop == &LineWidth) {
        pcDrawStyle->lineWidth = LineWidth.getValue();
    }
    else {
        ViewProviderGeometryObject::onChanged(prop);
    }
}

// ----------------------------------------------------------------------------

void ViewProviderFEMMeshBuilder::buildNodes(const App::Property* prop, std::vector<SoNode*>& nodes) const
{
    SoCoordinate3 *pcPointsCoord=0;
    SoIndexedFaceSet *pcFaces=0;
    SoIndexedLineSet *pcLines=0;

    if (nodes.empty()) {
        pcPointsCoord = new SoCoordinate3();
        nodes.push_back(pcPointsCoord);
        pcFaces = new SoIndexedFaceSet();
        pcLines = new SoIndexedLineSet();
        nodes.push_back(pcFaces);
    }
    else if (nodes.size() == 2) {
        if (nodes[0]->getTypeId() == SoCoordinate3::getClassTypeId())
            pcPointsCoord = static_cast<SoCoordinate3*>(nodes[0]);
        if (nodes[1]->getTypeId() == SoIndexedFaceSet::getClassTypeId())
            pcFaces = static_cast<SoIndexedFaceSet*>(nodes[1]);
    }

    if (pcPointsCoord && pcFaces)
        createMesh(prop, pcPointsCoord, pcFaces,pcLines);
}
#if 1 // new visual


inline void insEdgeVec(std::map<int,std::set<int> > &map, int n1, int n2)
{
    if(n1<n2)
        map[n2].insert(n1);
    else
        map[n2].insert(n1);
};

void ViewProviderFEMMeshBuilder::createMesh(const App::Property* prop, SoCoordinate3* coords, SoIndexedFaceSet* faces, SoIndexedLineSet* lines, bool ShowInner) const
{

    const Fem::PropertyFemMesh* mesh = static_cast<const Fem::PropertyFemMesh*>(prop);

    SMESHDS_Mesh* data = const_cast<SMESH_Mesh*>(mesh->getValue().getSMesh())->GetMeshDS();

	int numFaces = data->NbFaces();
	int numNodes = data->NbNodes();
	int numEdges = data->NbEdges();
	
    if(numFaces+numNodes+numEdges == 0) return;
    Base::TimeInfo Start;
    Base::Console().Log("Start: ViewProviderFEMMeshBuilder::createMesh() =================================\n");

	const SMDS_MeshInfo& info = data->GetMeshInfo();
    int numNode = info.NbNodes();
    int numTria = info.NbTriangles();
    int numQuad = info.NbQuadrangles();
    int numPoly = info.NbPolygons();
    int numVolu = info.NbVolumes();
    int numTetr = info.NbTetras();
    int numHexa = info.NbHexas();
    int numPyrd = info.NbPyramids();
    int numPris = info.NbPrisms();
    int numHedr = info.NbPolyhedrons();


    

    std::vector<FemFace> facesHelper(numTria+numQuad+numPoly+numTetr*4+numHexa*6+numPyrd*5+numPris*6);
    Base::Console().Log("    %f: Start build up %i face helper\n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()),facesHelper.size());
    SMDS_VolumeIteratorPtr aVolIter = data->volumesIterator();
    Base::BoundBox3d BndBox;

    for (int i=0;aVolIter->more();) {
        const SMDS_MeshVolume* aVol = aVolIter->next();
        
        int num = aVol->NbNodes();

        switch(num){
            // tet 4 element 
            case 4:
                // face 1
                BndBox.Add(facesHelper[i++].set(3,aVol,aVol->GetID(),1,aVol->GetNode(0),aVol->GetNode(1),aVol->GetNode(2)));
                // face 2
                BndBox.Add(facesHelper[i++].set(3,aVol,aVol->GetID(),2,aVol->GetNode(0),aVol->GetNode(3),aVol->GetNode(1)));
                // face 3
                BndBox.Add(facesHelper[i++].set(3,aVol,aVol->GetID(),3,aVol->GetNode(1),aVol->GetNode(3),aVol->GetNode(2)));
                // face 4
                BndBox.Add(facesHelper[i++].set(3,aVol,aVol->GetID(),4,aVol->GetNode(2),aVol->GetNode(3),aVol->GetNode(0)));
                break;
                //unknown case
            case 8:
                // face 1
                BndBox.Add(facesHelper[i++].set(4,aVol,aVol->GetID(),1,aVol->GetNode(0),aVol->GetNode(1),aVol->GetNode(2),aVol->GetNode(3)));
                // face 2
                BndBox.Add(facesHelper[i++].set(4,aVol,aVol->GetID(),2,aVol->GetNode(4),aVol->GetNode(5),aVol->GetNode(6),aVol->GetNode(7)));
                // face 3
                BndBox.Add(facesHelper[i++].set(4,aVol,aVol->GetID(),3,aVol->GetNode(0),aVol->GetNode(1),aVol->GetNode(4),aVol->GetNode(5)));
                // face 4
                BndBox.Add(facesHelper[i++].set(4,aVol,aVol->GetID(),4,aVol->GetNode(1),aVol->GetNode(2),aVol->GetNode(5),aVol->GetNode(6)));
                // face 5
                BndBox.Add(facesHelper[i++].set(4,aVol,aVol->GetID(),5,aVol->GetNode(2),aVol->GetNode(3),aVol->GetNode(6),aVol->GetNode(7)));
                // face 6
                BndBox.Add(facesHelper[i++].set(4,aVol,aVol->GetID(),6,aVol->GetNode(0),aVol->GetNode(3),aVol->GetNode(4),aVol->GetNode(7)));
                break;
                //unknown case
            case 10:
                // face 1
                BndBox.Add(facesHelper[i++].set(6,aVol,aVol->GetID(),1,aVol->GetNode(0),aVol->GetNode(1),aVol->GetNode(2),aVol->GetNode(4),aVol->GetNode(5),aVol->GetNode(6)));
                // face 2
                BndBox.Add(facesHelper[i++].set(6,aVol,aVol->GetID(),2,aVol->GetNode(0),aVol->GetNode(3),aVol->GetNode(1),aVol->GetNode(7),aVol->GetNode(8),aVol->GetNode(4)));
                // face 3
                BndBox.Add(facesHelper[i++].set(6,aVol,aVol->GetID(),3,aVol->GetNode(1),aVol->GetNode(3),aVol->GetNode(2),aVol->GetNode(8),aVol->GetNode(9),aVol->GetNode(5)));
                // face 4
                BndBox.Add(facesHelper[i++].set(6,aVol,aVol->GetID(),4,aVol->GetNode(2),aVol->GetNode(3),aVol->GetNode(0),aVol->GetNode(9),aVol->GetNode(7),aVol->GetNode(6)));
                break;
                //unknown case
            default: assert(0);
        }
    }

    int FaceSize = facesHelper.size();


    if( FaceSize < 5000){
        Base::Console().Log("    %f: Start eliminate internal faces SIMPLE\n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));
        
        // search for double (inside) faces and hide them
        if(!ShowInner){
            for(int l=0; l< FaceSize;l++){
                if(! facesHelper[l].hide){
                    for(int i=l+1; i<FaceSize; i++){
                        if(facesHelper[l].isSameFace(facesHelper[i]) ){
                            break;
                        }
                    }
                }
            }
        }
    }else{
        Base::Console().Log("    %f: Start eliminate internal faces GRID\n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));
        BndBox.Enlarge(BndBox.CalcDiagonalLength()/10000.0);
        // calculate grid properties
        double edge = pow(FaceSize,1.0/3.0);
        double edgeL = BndBox.LengthX() + BndBox.LengthY() + BndBox.LengthZ();
        double gridFactor = 50.0;
        double size = ((3*edge) / edgeL)*gridFactor;

        unsigned int NbrX = (unsigned int)(BndBox.LengthX()/size)+1;
        unsigned int NbrY = (unsigned int)(BndBox.LengthY()/size)+1;
        unsigned int NbrZ = (unsigned int)(BndBox.LengthZ()/size)+1;
        Base::Console().Log("      Size:F:%f,  X:%i  ,Y:%i  ,Z:%i\n",gridFactor,NbrX,NbrY,NbrZ);

        double Xmin = BndBox.MinX;
        double Ymin = BndBox.MinY;
        double Zmin = BndBox.MinZ;
        double Xln  = BndBox.LengthX() / NbrX;
        double Yln  = BndBox.LengthY() / NbrY;
        double Zln  = BndBox.LengthZ() / NbrZ;

        std::vector<FemFaceGridItem> Grid(NbrX*NbrY*NbrZ);


        unsigned int iX = 0;
        unsigned int iY = 0;
        unsigned int iZ = 0;

        for(int l=0; l< FaceSize;l++){
            Base::Vector3d point(facesHelper[l].getFirstNodePoint());
            double x = (point.x - Xmin) / Xln;
            double y = (point.y - Ymin) / Yln;
            double z = (point.z - Zmin) / Zln;

            iX = x;
            iY = y;
            iZ = z;

            if(iX >= NbrX || iY >= NbrY || iZ >= NbrZ)
                Base::Console().Log("      Outof range!\n");
            
            Grid[iX + iY*NbrX + iZ*NbrX*NbrY].push_back(&facesHelper[l]);
        }

        unsigned int max =0, avg = 0;
        for(std::vector<FemFaceGridItem>::iterator it=Grid.begin();it!=Grid.end();++it){
            for(unsigned int l=0; l< it->size();l++){
                if(! it->operator[](l)->hide){
                    for(unsigned int i=l+1; i<it->size(); i++){
                        if(it->operator[](l)->isSameFace(*(it->operator[](i))) ){
                            break;
                        }
                    }
                }
            }
            if(it->size() > max)max=it->size();
            avg += it->size();
        }
        avg = avg/Grid.size();
         
        Base::Console().Log("      VoxelSize: Max:%i ,Average:%i\n",max,avg);

    } //if( FaceSize < 1000)


    Base::Console().Log("    %f: Start build up node map\n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));

    // sort out double nodes and build up index map
    std::map<const SMDS_MeshNode*, int> mapNodeIndex;
    for(int l=0; l< FaceSize;l++){
        if(!facesHelper[l].hide)
            for(int i=0; i<8;i++)
                if(facesHelper[l].Nodes[i])
                    mapNodeIndex[facesHelper[l].Nodes[i]]=0;
                else
                    break;
    }
    Base::Console().Log("    %f: Start set point vector\n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));

    // set the point coordinates
    coords->point.setNum(mapNodeIndex.size());
    std::map<const SMDS_MeshNode*, int>::iterator it=  mapNodeIndex.begin();
    SbVec3f* verts = coords->point.startEditing();
    for (int i=0;it != mapNodeIndex.end() ;++it,i++) {
        verts[i].setValue((float)it->first->X(),(float)it->first->Y(),(float)it->first->Z());
        it->second = i;
    }
    coords->point.finishEditing();



    // count triangle size
    int triangleCount=0;
    for(int l=0; l< FaceSize;l++)
        if(! facesHelper[l].hide)
            switch(facesHelper[l].Size){
                case 3:triangleCount++  ;break;
                case 4:triangleCount+=2 ;break;
                case 6:triangleCount+=4 ;break;
                default: assert(0);
        }

    // edge map collect and sort edges of the faces to be shown. 
    std::map<int,std::set<int> > EdgeMap;

    Base::Console().Log("    %f: Start build up triangle vector\n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));
    // set the triangle face indices
    faces->coordIndex.setNum(4*triangleCount);
    int index=0;
    int32_t* indices = faces->coordIndex.startEditing();
	// iterate all element faces, allways assure CLOCKWISE triangle ordering to allow backface culling
    for(int l=0; l< FaceSize;l++){
        if(! facesHelper[l].hide){
            switch( facesHelper[l].Element->NbNodes()){
                case 4: // Tet 4
                    switch(facesHelper[l].FaceNo){
                        case 1: {
                            int nIdx0 = mapNodeIndex[facesHelper[l].Element->GetNode(0)];
                            int nIdx1 = mapNodeIndex[facesHelper[l].Element->GetNode(1)];
                            int nIdx2 = mapNodeIndex[facesHelper[l].Element->GetNode(2)];
                            indices[index++] = nIdx0;     
                            indices[index++] = nIdx2;     
                            indices[index++] = nIdx1;     
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx0,nIdx1);
                            insEdgeVec(EdgeMap,nIdx0,nIdx2);
                            insEdgeVec(EdgeMap,nIdx1,nIdx2);
                            break;    }
                        case 2: {
                            int nIdx0 = mapNodeIndex[facesHelper[l].Element->GetNode(0)];
                            int nIdx1 = mapNodeIndex[facesHelper[l].Element->GetNode(1)];
                            int nIdx3 = mapNodeIndex[facesHelper[l].Element->GetNode(3)];
                            indices[index++] = nIdx0;   
                            indices[index++] = nIdx1;   
                            indices[index++] = nIdx3;   
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx0,nIdx1);
                            insEdgeVec(EdgeMap,nIdx0,nIdx3);
                            insEdgeVec(EdgeMap,nIdx1,nIdx3);
                            break;    }
                        case 3: {
                            int nIdx1 = mapNodeIndex[facesHelper[l].Element->GetNode(1)];
                            int nIdx2 = mapNodeIndex[facesHelper[l].Element->GetNode(2)];
                            int nIdx3 = mapNodeIndex[facesHelper[l].Element->GetNode(3)];
                            indices[index++] = nIdx1;
                            indices[index++] = nIdx2;
                            indices[index++] = nIdx3;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx1,nIdx2);
                            insEdgeVec(EdgeMap,nIdx1,nIdx3);
                            insEdgeVec(EdgeMap,nIdx2,nIdx3);
                            break;    }
                        case 4: {
                            int nIdx0 = mapNodeIndex[facesHelper[l].Element->GetNode(0)];
                            int nIdx2 = mapNodeIndex[facesHelper[l].Element->GetNode(2)];
                            int nIdx3 = mapNodeIndex[facesHelper[l].Element->GetNode(3)];
                            indices[index++] = nIdx0;
                            indices[index++] = nIdx3;
                            indices[index++] = nIdx2;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx0,nIdx2);
                            insEdgeVec(EdgeMap,nIdx0,nIdx3);
                            insEdgeVec(EdgeMap,nIdx3,nIdx2);
                            break;    }
                        default: assert(0);

                    }
                    break;
                case 8: // Hex 8
                    switch(facesHelper[l].FaceNo){
                        case 1: {
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(1)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(0)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(3)];
                            indices[index++] = SO_END_FACE_INDEX;
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(3)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(2)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(1)];
                            indices[index++] = SO_END_FACE_INDEX;
                            break;    }
                        case 2: {
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(4)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(5)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(7)];
                            indices[index++] = SO_END_FACE_INDEX;
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(5)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(6)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(7)];
                            indices[index++] = SO_END_FACE_INDEX;
                            break;    }
                        case 3: {
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(0)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(1)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(5)];
                            indices[index++] = SO_END_FACE_INDEX;
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(0)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(5)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(4)];
                            indices[index++] = SO_END_FACE_INDEX;
                            break;    }
                        case 4: {
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(5)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(1)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(2)];
                            indices[index++] = SO_END_FACE_INDEX;
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(5)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(2)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(6)];
                            indices[index++] = SO_END_FACE_INDEX;
                            break;    }
                        case 5: {
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(2)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(3)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(7)];
                            indices[index++] = SO_END_FACE_INDEX;
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(2)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(7)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(6)];
                            indices[index++] = SO_END_FACE_INDEX;
                            break;    }
                        case 6: {
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(3)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(0)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(4)];
                            indices[index++] = SO_END_FACE_INDEX;
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(3)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(4)];
                            indices[index++] = mapNodeIndex[facesHelper[l].Element->GetNode(7)];
                            indices[index++] = SO_END_FACE_INDEX;
                            break;    }
                    }
                    break;
                case 10: // Tet 10
                    switch(facesHelper[l].FaceNo){
                        case 1: {
                            int nIdx0 = mapNodeIndex[facesHelper[l].Element->GetNode(0)];
                            int nIdx1 = mapNodeIndex[facesHelper[l].Element->GetNode(1)];
                            int nIdx2 = mapNodeIndex[facesHelper[l].Element->GetNode(2)];
                            int nIdx4 = mapNodeIndex[facesHelper[l].Element->GetNode(4)];
                            int nIdx5 = mapNodeIndex[facesHelper[l].Element->GetNode(5)];
                            int nIdx6 = mapNodeIndex[facesHelper[l].Element->GetNode(6)];
                            indices[index++] = nIdx0;
                            indices[index++] = nIdx6;
                            indices[index++] = nIdx4;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx0,nIdx6);
                            insEdgeVec(EdgeMap,nIdx0,nIdx4);
                            indices[index++] = nIdx6;
                            indices[index++] = nIdx2;
                            indices[index++] = nIdx5;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx2,nIdx6);
                            insEdgeVec(EdgeMap,nIdx2,nIdx5);
                            indices[index++] = nIdx5;
                            indices[index++] = nIdx1;
                            indices[index++] = nIdx4;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx1,nIdx5);
                            insEdgeVec(EdgeMap,nIdx1,nIdx4);
                            indices[index++] = nIdx4;
                            indices[index++] = nIdx6;
                            indices[index++] = nIdx5;
                            indices[index++] = SO_END_FACE_INDEX;
                            break;    }
                        case 2: {
                            int nIdx0 = mapNodeIndex[facesHelper[l].Element->GetNode(0)];
                            int nIdx1 = mapNodeIndex[facesHelper[l].Element->GetNode(1)];
                            int nIdx3 = mapNodeIndex[facesHelper[l].Element->GetNode(3)];
                            int nIdx4 = mapNodeIndex[facesHelper[l].Element->GetNode(4)];
                            int nIdx7 = mapNodeIndex[facesHelper[l].Element->GetNode(7)];
                            int nIdx8 = mapNodeIndex[facesHelper[l].Element->GetNode(8)];
                            indices[index++] = nIdx0;
                            indices[index++] = nIdx4;
                            indices[index++] = nIdx7;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx0,nIdx7);
                            insEdgeVec(EdgeMap,nIdx0,nIdx4);
                            indices[index++] = nIdx4;
                            indices[index++] = nIdx1;
                            indices[index++] = nIdx8;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx1,nIdx8);
                            insEdgeVec(EdgeMap,nIdx1,nIdx4);
                            indices[index++] = nIdx8;
                            indices[index++] = nIdx3;
                            indices[index++] = nIdx7;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx3,nIdx7);
                            insEdgeVec(EdgeMap,nIdx3,nIdx8);
                            indices[index++] = nIdx4;
                            indices[index++] = nIdx8;
                            indices[index++] = nIdx7;
                            indices[index++] = SO_END_FACE_INDEX;
                            break;    }
                        case 3: {
                            int nIdx1 = mapNodeIndex[facesHelper[l].Element->GetNode(1)];
                            int nIdx2 = mapNodeIndex[facesHelper[l].Element->GetNode(2)];
                            int nIdx3 = mapNodeIndex[facesHelper[l].Element->GetNode(3)];
                            int nIdx5 = mapNodeIndex[facesHelper[l].Element->GetNode(5)];
                            int nIdx8 = mapNodeIndex[facesHelper[l].Element->GetNode(8)];
                            int nIdx9 = mapNodeIndex[facesHelper[l].Element->GetNode(9)];
                            indices[index++] = nIdx1;
                            indices[index++] = nIdx5;
                            indices[index++] = nIdx8;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx1,nIdx5);
                            insEdgeVec(EdgeMap,nIdx1,nIdx8);
                            indices[index++] = nIdx5;
                            indices[index++] = nIdx2;
                            indices[index++] = nIdx9;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx2,nIdx5);
                            insEdgeVec(EdgeMap,nIdx2,nIdx9);
                            indices[index++] = nIdx9;
                            indices[index++] = nIdx3;
                            indices[index++] = nIdx8;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx3,nIdx9);
                            insEdgeVec(EdgeMap,nIdx3,nIdx8);
                            indices[index++] = nIdx5;
                            indices[index++] = nIdx9;
                            indices[index++] = nIdx8;
                            indices[index++] = SO_END_FACE_INDEX;
                            break;    }
                        case 4: {
                            int nIdx0 = mapNodeIndex[facesHelper[l].Element->GetNode(0)];
                            int nIdx2 = mapNodeIndex[facesHelper[l].Element->GetNode(2)];
                            int nIdx3 = mapNodeIndex[facesHelper[l].Element->GetNode(3)];
                            int nIdx6 = mapNodeIndex[facesHelper[l].Element->GetNode(6)];
                            int nIdx7 = mapNodeIndex[facesHelper[l].Element->GetNode(7)];
                            int nIdx9 = mapNodeIndex[facesHelper[l].Element->GetNode(9)];
                            indices[index++] = nIdx6;
                            indices[index++] = nIdx0;
                            indices[index++] = nIdx7;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx0,nIdx6);
                            insEdgeVec(EdgeMap,nIdx0,nIdx7);
                            indices[index++] = nIdx2;
                            indices[index++] = nIdx6;
                            indices[index++] = nIdx9;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx2,nIdx6);
                            insEdgeVec(EdgeMap,nIdx2,nIdx9);
                            indices[index++] = nIdx9;
                            indices[index++] = nIdx7;
                            indices[index++] = nIdx3;
                            indices[index++] = SO_END_FACE_INDEX;
                            insEdgeVec(EdgeMap,nIdx3,nIdx9);
                            insEdgeVec(EdgeMap,nIdx3,nIdx7);
                            indices[index++] = nIdx6;
                            indices[index++] = nIdx7;
                            indices[index++] = nIdx9;
                            indices[index++] = SO_END_FACE_INDEX;
                            break;    }
                        default: assert(0);

                    }
                    break;

                default:assert(0); // not implemented node
            }
        }
    }

    faces->coordIndex.finishEditing();

    Base::Console().Log("    %f: Start build up edge vector\n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));
    // std::map<int,std::set<int> > EdgeMap;
    // count edges
    int EdgeSize = 0;
    for(std::map<int,std::set<int> >::const_iterator it= EdgeMap.begin();it!= EdgeMap.end();++it)
        EdgeSize += it->second.size();

    // set the triangle face indices
    lines->coordIndex.setNum(3*EdgeSize);
    index=0;
    indices = lines->coordIndex.startEditing();

    for(std::map<int,std::set<int> >::const_iterator it= EdgeMap.begin();it!= EdgeMap.end();++it){
        for(std::set<int>::const_iterator it2=it->second.begin();it2!=it->second.end();++it2){
            indices[index++] = it->first;
            indices[index++] = *it2;
            indices[index++] = -1;
        }
    }

    lines->coordIndex.finishEditing();
    Base::Console().Log("    NumEdges:%i\n",EdgeSize);

    Base::Console().Log("    %f: Finish =========================================================\n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));


}
#else // old version of createMesh()
void ViewProviderFEMMeshBuilder::createMesh(const App::Property* prop, SoCoordinate3* coords, SoIndexedFaceSet* faces) const
{
    const Fem::PropertyFemMesh* mesh = static_cast<const Fem::PropertyFemMesh*>(prop);

    SMESHDS_Mesh* data = const_cast<SMESH_Mesh*>(mesh->getValue().getSMesh())->GetMeshDS();

	int numFaces = data->NbFaces();
	int numNodes = data->NbNodes 	();
	int numEdges = data->NbEdges 	();
	
	const SMDS_MeshInfo& info = data->GetMeshInfo();
    int numNode = info.NbNodes();
    int numTria = info.NbTriangles();
    int numQuad = info.NbQuadrangles();
    int numPoly = info.NbPolygons();
    int numVolu = info.NbVolumes();
    int numTetr = info.NbTetras();
    int numHexa = info.NbHexas();
    int numPyrd = info.NbPyramids();
    int numPris = info.NbPrisms();
    int numHedr = info.NbPolyhedrons();

    int index=0;
    std::map<const SMDS_MeshNode*, int> mapNodeIndex;

    // set the point coordinates
    coords->point.setNum(numNode);
    SMDS_NodeIteratorPtr aNodeIter = data->nodesIterator();
    unsigned int i=0;
    SbVec3f* verts = coords->point.startEditing();
    for (;aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        verts[i++].setValue((float)aNode->X(),(float)aNode->Y(),(float)aNode->Z());
        mapNodeIndex[aNode] = index++;
    }
    coords->point.finishEditing();

    // set the face indices
    index=0;
    faces->coordIndex.setNum(4*numTria + 5*numQuad + 16*numTetr);
    int32_t* indices = faces->coordIndex.startEditing();
	// iterate all faces 
    SMDS_FaceIteratorPtr aFaceIter = data->facesIterator();
    for (;aFaceIter->more();) {
        const SMDS_MeshFace* aFace = aFaceIter->next();
        int num = aFace->NbNodes();
        if (num != 3 && num != 4)
            continue;
        for (int j=0; j<num;j++) {
            const SMDS_MeshNode* node = aFace->GetNode(j);
            indices[index++] = mapNodeIndex[node];
        }
        indices[index++] = SO_END_FACE_INDEX;
    }
    SMDS_VolumeIteratorPtr aVolIter = data->volumesIterator();
    for (;aVolIter->more();) {
        const SMDS_MeshVolume* aVol = aVolIter->next();
        int num = aVol->NbNodes();
        if (num != 4)
            continue;
        int i1 = mapNodeIndex[aVol->GetNode(0)];
        int i2 = mapNodeIndex[aVol->GetNode(1)];
        int i3 = mapNodeIndex[aVol->GetNode(2)];
        int i4 = mapNodeIndex[aVol->GetNode(3)];
        indices[index++] = i1;
        indices[index++] = i3;
        indices[index++] = i2;
        indices[index++] = SO_END_FACE_INDEX;
        indices[index++] = i1;
        indices[index++] = i2;
        indices[index++] = i4;
        indices[index++] = SO_END_FACE_INDEX;
        indices[index++] = i1;
        indices[index++] = i4;
        indices[index++] = i3;
        indices[index++] = SO_END_FACE_INDEX;
        indices[index++] = i2;
        indices[index++] = i3;
        indices[index++] = i4;
        indices[index++] = SO_END_FACE_INDEX;
    }
    faces->coordIndex.finishEditing();
}
#endif 