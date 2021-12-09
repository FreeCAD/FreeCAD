/***************************************************************************
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

#ifndef BEST_FIT_H
#define BEST_FIT_H


#include <Mod/Mesh/App/Core/Approximation.h>
#include <Mod/Mesh/App/Core/Evaluation.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Base/Exception.h>
#include <gp_Vec.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <SMESH_Mesh.hxx>
#include <SMDS_VolumeTool.hxx>


#define SMALL_NUM  1e-6
#define ERR_TOL    0.001       // Abbruchkriterium für Least-Square-Matching (Fehleränderung zweier aufeinanderfolgenden Iterationsschritten)


/*! \brief The main class for the best_fit routine

 It takes a mesh and a Topo_Shape as it's input parameter.

 As output, it gives a transformed mesh (rotation + translation)
 based on a weighted ICP-Algorithm (ICP: Iterative Closed Point) which fits the
 Topo_Shape
*/

class CamExport best_fit
{
public:
    best_fit();
    ~best_fit();

    /*! \brief Load the input-shapes. Must be called before running the main program

        \param InputMesh Input-mesh
        \param CAD_Shape Input-shape
    */
    void Load(const MeshCore::MeshKernel &InputMesh, const TopoDS_Shape &CAD_Shape);

    bool Initialize_Mesh_Geometrie_1();
    bool Initialize_Mesh_Geometrie_2();

    /*! \brief Determines the local-coordinate-systems of the input-shapes and
               apply a coordinate transformation to the mesh
    */
    bool MeshFit_Coarse();

    /*! \brief Determines the center of mass of the input-shape and
               translates the shape to the global origin
    */
    bool ShapeFit_Coarse();

    /*! \brief Determines the center of mass of the point-cloud and
               translates the shape to the global origin
    */
    bool PointCloud_Coarse();

    /*! \brief Main function of the best-fit-algorithm */
    bool Perform();

    /*! \brief Main function of the best-fit-algorithm only on point clouds */
    bool Perform_PointCloud();

    bool output_best_fit_mesh();

    //double CompError(std::vector<Base::Vector3f> &pnts, std::vector<Base::Vector3f> &normals);
    //double CompError(std::vector<Base::Vector3f> &pnts, std::vector<Base::Vector3f> &normals, bool plot);

    //std::vector<double> CompError_GetPnts(std::vector<Base::Vector3f> pnts,
    //                                     std::vector<Base::Vector3f> &normals);

    /*! \brief Computes error between the input-shapes through intersection
               along the normal-directions and returns an average-error-value
    */
    double CompTotalError();

    /*! \brief Computes error between mesh and the input-shape through
               intersection along the normal-directions and returns an
               average-error-value
    */
    double CompTotalError(MeshCore::MeshKernel &mesh);
	
    /*! \brief Computes a triangulation on shape.

        \param shape      specifies the shape to be tessellated
        \param mesh       output-mesh to store the computed triangulation
        \param deflection parameter which determines the accuracy of the
                          triangulation
    */
    static bool Tesselate_Shape(const TopoDS_Shape &shape, MeshCore::MeshKernel &mesh, float deflection);
	
    /*! \brief Computes a triangulation on aface.

        \param aface      specifies the face to be tessellated
        \param mesh       output-mesh to store the computed triangulation
        \param deflection parameter which determines the accuracy of the
                          triangulation
    */
    static bool Tesselate_Face (const TopoDS_Face  &aface, MeshCore::MeshKernel &mesh, float deflection);

    /*! \brief Returns a normal-vector of Mesh at the knots with uniform
               weighting

        \param Mesh Input-Mesh
    */
    static std::vector<Base::Vector3f> Comp_Normals(MeshCore::MeshKernel &Mesh);
	
    /*! \brief Check and corrects mesh-position by rotating around all
               coordinate-axes with 180 degree
    */
    bool Coarse_correction();

    /*! \brief Determines two corresponding point-sets for the ICP-Method
               using the Nearest-Neighbour-Algorithm
    */
    double ANN();

    /*! \brief Input-shape from the function Load */
    TopoDS_Shape m_Cad;              // CAD-Geometrie

    /*! \brief Input-mesh from the function Load */
    MeshCore::MeshKernel m_Mesh;     // das zu fittende Netz

    /*! \brief A copy of m_Mesh */
    MeshCore::MeshKernel m_MeshWork;

    /*! \brief Triangulated input-shape m_Cad */
    MeshCore::MeshKernel m_CadMesh;  // Netz aus CAD-Triangulierung


	std::vector<Base::Vector3f> m_pntCloud_1;
	std::vector<Base::Vector3f> m_pntCloud_2;


    /*! \brief Stores the knots of m_CadMesh in relative order */
    std::vector<Base::Vector3f> m_pnts;

    /*! \brief Stores the normals of m_CadMesh in relative order */
    std::vector<Base::Vector3f> m_normals;

    /*! \brief Stores the error-values of m_CadMesh in relative order */
    std::vector<double>         m_error;

    /*! \brief Stores the point-sets computed with the function ANN() */
    std::vector<std::vector<Base::Vector3f> > m_LSPnts;  // zu fittende Punktesätze für den Least-Square

    /*! \brief Stores the weights computed with the function Comp_Weights() */
    std::vector<double> m_weights;                       // gewichtungen für den Least-Square bzgl. allen Netzpunkte

    /*! \brief A working-copy of m_weights */
    std::vector<double> m_weights_loc;                   // gewichtungen für den Least-Square bzgl. den projezierten Netzpunkten

    /*! \brief Translation-vector from the function ShapeFit_Coarse() */
    gp_Vec m_cad2orig;                     // Translationsvektor welche die CAD-Geometrie um den Ursprung zentriert
    
    /*! \brief Vector of the preselected-faces for the weighting */
    std::vector<TopoDS_Face> m_LowFaces;   // Vektor der in der GUI selektierten Faces mit geringer Gewichtung

private:
    /*! \brief Computes the rotation-matrix with reference to the given
               parameters

        \param matrix       4x4-output-matrix
        \param degree       rotation-angle in degree
        \param rotationAxis rotation-axis (1: x-axis, 2: y-axis, 3: z-axis)
    */
    inline bool RotMat(Base::Matrix4D &matrix, double degree, int rotationAxis);
	
    /*! \brief Computes the translation-matrix with reference to the given
               parameters

        \param matrix          4x4-output-matrix
        \param translation     translation-value
        \param translationAxis translation-axis (1: x-axis, 2: y-axis, 3: z-axis)
    */
    inline bool TransMat(Base::Matrix4D &matrix, double translation, int translationAxis);
	
    /*! \brief Transforms the point-set \p pnts and the corresponding
               surface-normals normals with reference to the input-matrix

        \param pnts    point-vector to transform
        \param normals normal-vector to transform
        \param M       4x4-input-matrix
    */
    inline bool PointNormalTransform(std::vector<Base::Vector3f> &pnts,
                                     std::vector<Base::Vector3f> &normals,
                                     Base::Matrix4D              &M);
	
    /*! \brief Transforms the point-set pnts with reference to the input-matrix

        \param pnts point-vector to transform
        \param M    is the 4x4-input-matrix
    */
    bool PointTransform(std::vector<Base::Vector3f> &pnts, const Base::Matrix4D &M);
	
    /*! \brief Sets the weights for the ICP-Algorithm */
    bool Comp_Weights();

    /*! \brief Performing the ICP-Algorithm */
    bool LSM();

    /*! \brief Returns the first derivative of the rotation-matrix at the
               position x

        \param params is a three-dimensional-vector specifying the rotation
                      angle along the x-,y- and z-axis
     */
    std::vector<double> Comp_Jacobi(const std::vector<double> &params);

    /*! \brief Returns the second derivative (Hessian matrix) of the rotation
               matrix at the position x in form of a two-dimensional vector
               of type double

        \param params is a three-dimensional-vector specifying the rotation
                      angle along the x-,y- and z-axis
    */
    std::vector<std::vector<double> > Comp_Hess (const std::vector<double> &params);

    SMESH_Mesh *m_referencemesh;
    SMESH_Mesh *m_meshtobefit;
    SMESH_Gen *m_aMeshGen1;
    SMESH_Gen *m_aMeshGen2;

	
    
	//int intersect_RayTriangle(const Base::Vector3f &normal,const MeshCore::MeshGeomFacet &T, Base::Vector3f &P, Base::Vector3f &I);
	// bool Intersect(const Base::Vector3f &normal,const MeshCore::MeshKernel &mesh, Base::Vector3f &P, Base::Vector3f &I);
};

#endif
