/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/FileInfo.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Property.h>

#include "Points.h"
#include "PointsPy.h"
#include "PointsAlgos.h"
#include "FeaturePointsImportAscii.h"

using namespace Points;

/* module functions */
static PyObject *
open(PyObject *self, PyObject *args)
{
    const char* Name;
    if (! PyArg_ParseTuple(args, "s",&Name))
        return NULL;

    PY_TRY {
        Base::Console().Log("Open in Points with %s",Name);
        Base::FileInfo file(Name);

        // extract ending
        if (file.extension() == "")
            Py_Error(PyExc_Exception,"no file ending");

        if (file.hasExtension("asc")) {
            // create new document and add Import feature
            App::Document *pcDoc = App::GetApplication().newDocument("Unnamed");
            Points::Feature *pcFeature = (Points::Feature *)pcDoc->addObject("Points::Feature", file.fileNamePure().c_str());
            Points::PointKernel pkTemp;
            pkTemp.load(Name);
            pcFeature->Points.setValue( pkTemp );

        }
        else {
            Py_Error(PyExc_Exception,"unknown file ending");
        }
    } PY_CATCH;

    Py_Return;
}

static PyObject *
insert(PyObject *self, PyObject *args)
{
    const char* Name;
    const char* DocName;
    if (!PyArg_ParseTuple(args, "ss",&Name,&DocName))
        return NULL;

    PY_TRY {
        Base::Console().Log("Import in Points with %s",Name);
        Base::FileInfo file(Name);

        // extract ending
        if (file.extension() == "")
            Py_Error(PyExc_Exception,"no file ending");

        if (file.hasExtension("asc")) {
            // add Import feature
            App::Document *pcDoc = App::GetApplication().getDocument(DocName);
            if (!pcDoc) {
                pcDoc = App::GetApplication().newDocument(DocName);
            }

            Points::Feature *pcFeature = (Points::Feature *)pcDoc->addObject("Points::Feature", file.fileNamePure().c_str());
            Points::PointKernel pkTemp;
            pkTemp.load(Name);
            pcFeature->Points.setValue( pkTemp );
        }
        else {
            Py_Error(PyExc_Exception,"unknown file ending");
        }
    } PY_CATCH;

    Py_Return;
}

static PyObject * 
show(PyObject *self, PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(PointsPy::Type), &pcObj))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY {
        App::Document *pcDoc = App::GetApplication().getActiveDocument(); 	 
        if (!pcDoc)
            pcDoc = App::GetApplication().newDocument();
        PointsPy* pPoints = static_cast<PointsPy*>(pcObj);
        Points::Feature *pcFeature = (Points::Feature *)pcDoc->addObject("Points::Feature", "Points");
        // copy the data
        //TopoShape* shape = new MeshObject(*pShape->getTopoShapeObjectPtr());
        pcFeature->Points.setValue(*(pPoints->getPointKernelPtr()));
        //pcDoc->recompute();
    } PY_CATCH;

    Py_Return;
}

// http://svn.pointclouds.org/pcl/tags/pcl-1.5.1/test/
#if defined(PCL_FOUND)
#include <pcl/point_types.h>
#include <pcl/features/normal_3d.h>
#include <pcl/surface/mls.h>
#include <pcl/surface/mls_omp.h>
#include <pcl/surface/gp3.h>
#include <pcl/surface/grid_projection.h>
#include <pcl/surface/convex_hull.h>
#include <pcl/surface/concave_hull.h>
#include <pcl/surface/organized_fast_mesh.h>
#include <pcl/surface/ear_clipping.h>
#include <pcl/common/common.h>
#include <boost/random.hpp>

using namespace pcl;
using namespace pcl::io;
using namespace std;


int 
saveOBJFile (const std::string &file_name,
             const pcl::PolygonMesh &mesh, unsigned precision=5)
{
  if (mesh.cloud.data.empty ())
  {
    std::cerr << "[pcl::io::saveOBJFile] Input point cloud has no data!\n";
    return (-1);
  }
  // Open file
  std::ofstream fs;
  fs.precision (precision);
  fs.open (file_name.c_str ());

  /* Write 3D information */
  // number of points
  int nr_points  = mesh.cloud.width * mesh.cloud.height;
  // point size
  int point_size = mesh.cloud.data.size () / nr_points;
  // number of faces for header
  int nr_faces = mesh.polygons.size ();
  // Do we have vertices normals?
  int normal_index = getFieldIndex (mesh.cloud, "normal");

  // Write the header information
  fs << "####" << std::endl;
  fs << "# OBJ dataFile simple version. File name: " << file_name << std::endl;
  fs << "# Vertices: " << nr_points << std::endl;
  if (normal_index != -1)
    fs << "# Vertices normals : " << nr_points << std::endl;
  fs << "# Faces: " <<nr_faces << std::endl;
  fs << "####" << std::endl;

  // Write vertex coordinates
  fs << "# List of Vertices, with (x,y,z) coordinates, w is optional." << std::endl;
  for (int i = 0; i < nr_points; ++i)
  {
    int xyz = 0;
    for (size_t d = 0; d < mesh.cloud.fields.size (); ++d)
    {
      int c = 0;
      // adding vertex
      if ((mesh.cloud.fields[d].datatype == sensor_msgs::PointField::FLOAT32) && (
          mesh.cloud.fields[d].name == "x" ||
          mesh.cloud.fields[d].name == "y" ||
          mesh.cloud.fields[d].name == "z"))
      {
        if (mesh.cloud.fields[d].name == "x")
           // write vertices beginning with v
          fs << "v ";

        float value;
        memcpy (&value, &mesh.cloud.data[i * point_size + mesh.cloud.fields[d].offset + c * sizeof (float)], sizeof (float));
        fs << value;
        if (++xyz == 3)
          break;
        fs << " ";
      }
    }
    if (xyz != 3)
    {
      std::cerr << "[pcl::io::saveOBJFile] Input point cloud has no XYZ data!\n";
      return (-2);
    }
    fs << std::endl;
  }

  fs << "# "<< nr_points <<" vertices" << std::endl;

  if(normal_index != -1)
  {    
    fs << "# Normals in (x,y,z) form; normals might not be unit." <<  std::endl;
    // Write vertex normals
    for (int i = 0; i < nr_points; ++i)
    {
      int nxyz = 0;
      for (size_t d = 0; d < mesh.cloud.fields.size (); ++d)
      {
        int c = 0;
        // adding vertex
        if ((mesh.cloud.fields[d].datatype == sensor_msgs::PointField::FLOAT32) && (
              mesh.cloud.fields[d].name == "normal_x" ||
              mesh.cloud.fields[d].name == "normal_y" ||
              mesh.cloud.fields[d].name == "normal_z"))
        {
          if (mesh.cloud.fields[d].name == "normal_x")
            // write vertices beginning with vn
            fs << "vn ";
          
          float value;
          memcpy (&value, &mesh.cloud.data[i * point_size + mesh.cloud.fields[d].offset + c * sizeof (float)], sizeof (float));
          fs << value;
          if (++nxyz == 3)
            break;
          fs << " ";
        }
      }
      if (nxyz != 3)
      {
        std::cerr << "[pcl::io::saveOBJFile] Input point cloud has no normals!\n";
        return (-2);
      }
      fs << std::endl;
    }

    fs << "# "<< nr_points <<" vertices normals" << std::endl;
  }

  fs << "# Face Definitions" << std::endl;
  // Write down faces
  if(normal_index == -1)
  {
    for(int i = 0; i < nr_faces; i++)
    {
      fs << "f ";
      size_t j = 0;    
      for (; j < mesh.polygons[i].vertices.size () - 1; ++j)
        fs << mesh.polygons[i].vertices[j] + 1 << " ";
      fs << mesh.polygons[i].vertices[j] + 1 << std::endl;
    }
  }
  else
  {
    for(int i = 0; i < nr_faces; i++)
    {
      fs << "f ";
      size_t j = 0;    
      for (; j < mesh.polygons[i].vertices.size () - 1; ++j)
        fs << mesh.polygons[i].vertices[j] + 1 << "//" << mesh.polygons[i].vertices[j] + 1;
      fs << mesh.polygons[i].vertices[j] + 1 << "//" << mesh.polygons[i].vertices[j] + 1 << std::endl;
    }
  }
  fs << "# End of File" << std::endl;

  // Close obj file
  fs.close ();  
  return 0;
}

static PyObject * 
testPCL(PyObject *self, PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(PointsPy::Type), &pcObj))     // convert args: Python->C
        return NULL;                             // NULL triggers exception
        
    PointsPy* pPoints = static_cast<PointsPy*>(pcObj);
    PointKernel* kernel = pPoints->getPointKernelPtr();
    
    PointCloud<PointXYZ>::Ptr cloud (new PointCloud<PointXYZ>);
    PointCloud<PointNormal>::Ptr cloud_with_normals (new PointCloud<PointNormal>);
    search::KdTree<PointXYZ>::Ptr tree;
    search::KdTree<PointNormal>::Ptr tree2;
    
    for (PointKernel::const_iterator it = kernel->begin(); it != kernel->end(); ++it) {
        cloud->push_back(PointXYZ(it->x, it->y, it->z));
    }

    // Create search tree
    tree.reset (new search::KdTree<PointXYZ> (false));
    tree->setInputCloud (cloud);

    // Normal estimation
    NormalEstimation<PointXYZ, Normal> n;
    PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
    n.setInputCloud (cloud);
    //n.setIndices (indices[B);
    n.setSearchMethod (tree);
    n.setKSearch (20);
    n.compute (*normals);

    // Concatenate XYZ and normal information
    pcl::concatenateFields (*cloud, *normals, *cloud_with_normals);
      
    // Create search tree
    tree2.reset (new search::KdTree<PointNormal>);
    tree2->setInputCloud (cloud_with_normals);

    // Init objects
    PolygonMesh triangles;
    GreedyProjectionTriangulation<PointNormal> gp3;

    // Set parameters
    gp3.setInputCloud (cloud_with_normals);
    gp3.setSearchMethod (tree2);
    gp3.setSearchRadius (2.025);
    gp3.setMu (2.5);
    gp3.setMaximumNearestNeighbors (100);
    gp3.setMaximumSurfaceAngle(M_PI/4); // 45 degrees
    gp3.setMinimumAngle(M_PI/18); // 10 degrees
    gp3.setMaximumAngle(2*M_PI/3); // 120 degrees
    gp3.setNormalConsistency(false);

    // Reconstruct
    gp3.reconstruct (triangles);

    // Additional vertex information
    std::vector<int> parts = gp3.getPartIDs();
    std::vector<int> states = gp3.getPointStates();
    int nr_points = cloud_with_normals->width * cloud_with_normals->height;
    
    std::cerr << "Number of triangles: " << triangles.polygons.size() << std::endl;
    saveOBJFile("/tmp/pcl.obj", triangles);


    Py_Return;
}
#endif // PCL_FOUND

// registration table  
struct PyMethodDef Points_Import_methods[] = {
    {"open",  open,   1},				/* method name, C func ptr, always-tuple */
    {"insert",insert, 1},
    {"show",show, 1},
#if defined(PCL_FOUND)
    {"testPCL",testPCL, 1},
#endif

    {NULL, NULL}                /* end of table marker */
};

