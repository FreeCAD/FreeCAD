/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef FEM_FEMMESH_H
#define FEM_FEMMESH_H

#include <list>
#include <memory>
#include <vector>

#include <SMDSAbs_ElementType.hxx>
#include <SMESH_Version.h>

#include <App/ComplexGeoData.h>
#include <Base/Quantity.h>
#include <Mod/Fem/FemGlobal.h>


class SMESH_Gen;
class SMESH_Mesh;
class SMESH_Hypothesis;
class TopoDS_Shape;
class TopoDS_Face;
class TopoDS_Edge;
class TopoDS_Vertex;
class TopoDS_Solid;

namespace Fem
{

using SMESH_HypothesisPtr = std::shared_ptr<SMESH_Hypothesis>;

/** The representation of a FemMesh
 */
class FemExport FemMesh: public Data::ComplexGeoData
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    FemMesh();
    FemMesh(const FemMesh&);
    ~FemMesh() override;

    FemMesh& operator=(const FemMesh&);
    const SMESH_Mesh* getSMesh() const;
    SMESH_Mesh* getSMesh();
    static SMESH_Gen* getGenerator();
    void addHypothesis(const TopoDS_Shape& aSubShape, SMESH_HypothesisPtr hyp);
    void setStandardHypotheses();
    void compute();

    // from base class
    unsigned int getMemSize() const override;
    void Save(Base::Writer& /*writer*/) const override;
    void Restore(Base::XMLReader& /*reader*/) override;
    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    /** @name Subelement management */
    //@{
    /** Sub type list
     *  List of different subelement types
     *  it is NOT a list of the subelements itself
     */
    std::vector<const char*> getElementTypes() const override;
    unsigned long countSubElements(const char* Type) const override;
    /// get the subelement by type and number
    Data::Segment* getSubElement(const char* Type, unsigned long) const override;
    /** Get points from object with given accuracy */
    void getPoints(std::vector<Base::Vector3d>& Points,
                   std::vector<Base::Vector3d>& Normals,
                   double Accuracy,
                   uint16_t flags = 0) const override;
    //@}

    /** @name search and retrieval */
    //@{
    /// retrieving by region growing
    std::set<long> getSurfaceNodes(long ElemId, short FaceId, float Angle = 360) const;
    /// retrieving by solid
    std::set<int> getNodesBySolid(const TopoDS_Solid& solid) const;
    /// retrieving by face
    std::set<int> getNodesByFace(const TopoDS_Face& face) const;
    /// retrieving by edge
    std::set<int> getNodesByEdge(const TopoDS_Edge& edge) const;
    /// retrieving by vertex
    std::set<int> getNodesByVertex(const TopoDS_Vertex& vertex) const;
    /// retrieving node IDs by element ID
    std::list<int> getElementNodes(int id) const;
    /// retrieving elements IDs by node ID
    std::list<int> getNodeElements(int id, SMDSAbs_ElementType type = SMDSAbs_All) const;
    /// retrieving face IDs number by face
    std::list<int> getFacesByFace(const TopoDS_Face& face) const;
    /// retrieving edge IDs number by edge
    std::list<int> getEdgesByEdge(const TopoDS_Edge& edge) const;
    /// retrieving volume IDs and face IDs number by face
    std::list<std::pair<int, int>> getVolumesByFace(const TopoDS_Face& face) const;
    /// retrieving volume IDs and CalculiX face number by face
    std::map<int, int> getccxVolumesByFace(const TopoDS_Face& face) const;
    /// retrieving IDs of edges not belonging to any face (and thus not belonging to any volume too)
    std::set<int> getEdgesOnly() const;
    /// retrieving IDs of faces not belonging to any volume
    std::set<int> getFacesOnly() const;
    //@}

    /** @name Placement control */
    //@{
    /// set the transformation
    void setTransform(const Base::Matrix4D& rclTrf) override;
    /// get the transformation
    Base::Matrix4D getTransform() const override;
    /// Bound box from the shape
    Base::BoundBox3d getBoundBox() const override;
    /// get the volume (when there are volume elements)
    Base::Quantity getVolume() const;
    //@}

    /** @name Modification */
    //@{
    /// Applies a transformation on the real geometric data type
    void transformGeometry(const Base::Matrix4D& rclMat) override;
    //@}

    /** @name Group management */
    //@{
    /// Adds group to mesh
    int addGroup(const std::string, const std::string, const int = -1);
    /// Adds elements to group (int due to int used by raw SMESH functions)
    void addGroupElements(int, const std::set<int>&);
    /// Remove group (Name due to similarity to SMESH basis functions)
    bool removeGroup(int);
    //@}


    struct FemMeshInfo
    {
        int numFaces;
        int numNode;
        int numTria;
        int numQuad;
        int numPoly;
        int numVolu;
        int numTetr;
        int numHexa;
        int numPyrd;
        int numPris;
        int numHedr;
    };

    ///
    struct FemMeshInfo getInfo() const;

    /// import from files
    void read(const char* FileName);
    void write(const char* FileName) const;
    void writeABAQUS(const std::string& Filename, int elemParam, bool groupParam) const;
    void writeZ88(const std::string& FileName) const;

private:
    void copyMeshData(const FemMesh&);
    void readNastran(const std::string& Filename);
    void readNastran95(const std::string& Filename);
    void readZ88(const std::string& Filename);
    void readAbaqus(const std::string& Filename);

private:
    /// positioning matrix
    Base::Matrix4D _Mtrx;
    SMESH_Mesh* myMesh;

    std::list<SMESH_HypothesisPtr> hypoth;
    static SMESH_Gen* _mesh_gen;
};

}  // namespace Fem


#endif  // FEM_FEMMESH_H
