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
#ifdef FC_OS_LINUX
# include <unistd.h>
#endif
# include <sstream>
#endif

#ifdef HAVE_PCL_IO
# include <pcl/io/ply_io.h>
# include <pcl/io/pcd_io.h>
# include <pcl/point_types.h>
#endif


#include "PointsAlgos.h"
#include "Points.h"

#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Console.h>
#include <Base/Sequencer.h>
#include <Base/Stream.h>

#include <boost/regex.hpp>

using namespace Points;

void PointsAlgos::Load(PointKernel &points, const char *FileName)
{
    Base::FileInfo File(FileName);

    // checking on the file
    if (!File.isReadable())
        throw Base::FileException("File to load not existing or not readable", FileName);

    if (File.extension() == "asc" ||File.extension() == "ASC")
        LoadAscii(points,FileName);
    else
        throw Base::Exception("Unknown ending");
}

void PointsAlgos::LoadAscii(PointKernel &points, const char *FileName)
{
    boost::regex rx("^\\s*([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                     "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                     "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$");
    //boost::regex rx("(\\b[0-9]+\\.([0-9]+\\b)?|\\.[0-9]+\\b)");
    //boost::regex rx("^\\s*(-?[0-9]*)\\.([0-9]+)\\s+(-?[0-9]*)\\.([0-9]+)\\s+(-?[0-9]*)\\.([0-9]+)\\s*$");
    boost::cmatch what;

    Base::Vector3d pt;
    int LineCnt=0;
    std::string line;
    Base::FileInfo fi(FileName);

    Base::ifstream tmp_str(fi, std::ios::in);

    // estimating size
    while (std::getline(tmp_str,line))
        LineCnt++;

    // resize the PointKernel
    points.resize(LineCnt);

    Base::SequencerLauncher seq( "Loading points...", LineCnt );

    // again to the beginning
    Base::ifstream file(fi, std::ios::in);
    LineCnt = 0;

    try {
        // read file
        while (std::getline(file, line)) {
            if (boost::regex_match(line.c_str(), what, rx)) {
                pt.x = std::atof(what[1].first);
                pt.y = std::atof(what[4].first);
                pt.z = std::atof(what[7].first);

                points.setPoint(LineCnt,pt);
                seq.next();
                LineCnt++;
            }
        }
    }
    catch (...) {
        points.clear();
        throw Base::Exception("Reading in points failed.");
    }

    // now remove the last points from the kernel
    // Note: first we allocate memory corresponding to the number of lines (points and comments)
    //       and read in the file twice. But then the size of the kernel is too high
    if (LineCnt < (int)points.size())
        points.erase(LineCnt, points.size());
}

// ----------------------------------------------------------------------------

Reader::Reader()
{
    width = 0;
    height = 0;
}

Reader::~Reader()
{
}

void Reader::clear()
{
    intensity.clear();
    colors.clear();
    normals.clear();
}

const PointKernel& Reader::getPoints() const
{
    return points;
}

bool Reader::hasProperties() const
{
    return (hasIntensities() || hasColors() || hasNormals());
}

const std::vector<float>& Reader::getIntensities() const
{
    return intensity;
}

bool Reader::hasIntensities() const
{
    return (!intensity.empty());
}

const std::vector<App::Color>& Reader::getColors() const
{
    return colors;
}

bool Reader::hasColors() const
{
    return (!colors.empty());
}

const std::vector<Base::Vector3f>& Reader::getNormals() const
{
    return normals;
}

bool Reader::hasNormals() const
{
    return (!normals.empty());
}

bool Reader::isStructured() const
{
    return (width > 1 && height > 1);
}

int Reader::getWidth() const
{
    return width;
}

int Reader::getHeight() const
{
    return height;
}

// ----------------------------------------------------------------------------

AscReader::AscReader()
{
}

AscReader::~AscReader()
{
}

void AscReader::read(const std::string& filename)
{
    points.load(filename.c_str());
}

// ----------------------------------------------------------------------------

#ifdef HAVE_PCL_IO
PlyReader::PlyReader()
{
}

PlyReader::~PlyReader()
{
}

void PlyReader::read(const std::string& filename)
{
    clear();

    // pcl test
    pcl::PCLPointCloud2 cloud2;
    Eigen::Vector4f origin;
    Eigen::Quaternionf orientation;
    int ply_version;
    int data_type;
    unsigned int data_idx;
    pcl::PLYReader ply;
    ply.readHeader(filename, cloud2, origin, orientation, ply_version, data_type, data_idx);

    bool hasIntensity = false;
    bool hasColors = false;
    bool hasNormals = false;
    for (size_t i = 0; i < cloud2.fields.size (); ++i) {
        if (cloud2.fields[i].name == "intensity")
            hasIntensity = true;
        if (cloud2.fields[i].name == "normal_x" || cloud2.fields[i].name == "nx")
            hasNormals = true;
        if (cloud2.fields[i].name == "normal_y" || cloud2.fields[i].name == "ny")
            hasNormals = true;
        if (cloud2.fields[i].name == "normal_z" || cloud2.fields[i].name == "nz")
            hasNormals = true;
        if (cloud2.fields[i].name == "red")
            hasColors = true;
        if (cloud2.fields[i].name == "green")
            hasColors = true;
        if (cloud2.fields[i].name == "blue")
            hasColors = true;
        if (cloud2.fields[i].name == "rgb")
            hasColors = true;
        if (cloud2.fields[i].name == "rgba")
            hasColors = true;
    }

    if (hasNormals && hasColors) {
        pcl::PointCloud<pcl::PointXYZRGBNormal> cloud_in;
        pcl::io::loadPLYFile<pcl::PointXYZRGBNormal>(filename, cloud_in);
        points.reserve(cloud_in.size());
        colors.reserve(cloud_in.size());
        normals.reserve(cloud_in.size());
        for (pcl::PointCloud<pcl::PointXYZRGBNormal>::const_iterator it = cloud_in.begin();it!=cloud_in.end();++it) {
            points.push_back(Base::Vector3d(it->x,it->y,it->z));
            colors.push_back(App::Color(it->r/255.0f,it->g/255.0f,it->b/255.0f));
            normals.push_back(Base::Vector3f(it->normal_x,it->normal_y,it->normal_z));
        }
    }
    else if (hasNormals && hasIntensity) {
        pcl::PointCloud<pcl::PointXYZINormal> cloud_in;
        pcl::io::loadPLYFile<pcl::PointXYZINormal>(filename, cloud_in);
        points.reserve(cloud_in.size());
        intensity.reserve(cloud_in.size());
        normals.reserve(cloud_in.size());
        for (pcl::PointCloud<pcl::PointXYZINormal>::const_iterator it = cloud_in.begin();it!=cloud_in.end();++it) {
            points.push_back(Base::Vector3d(it->x,it->y,it->z));
            intensity.push_back(it->intensity);
            normals.push_back(Base::Vector3f(it->normal_x,it->normal_y,it->normal_z));
        }
    }
    else if (hasColors) {
        pcl::PointCloud<pcl::PointXYZRGBA> cloud_in;
        pcl::io::loadPLYFile<pcl::PointXYZRGBA>(filename, cloud_in);
        points.reserve(cloud_in.size());
        colors.reserve(cloud_in.size());
        for (pcl::PointCloud<pcl::PointXYZRGBA>::const_iterator it = cloud_in.begin();it!=cloud_in.end();++it) {
            points.push_back(Base::Vector3d(it->x,it->y,it->z));
            colors.push_back(App::Color(it->r/255.0f,it->g/255.0f,it->b/255.0f,it->a/255.0f));
        }
    }
    else if (hasIntensity) {
        pcl::PointCloud<pcl::PointXYZI> cloud_in;
        pcl::io::loadPLYFile<pcl::PointXYZI>(filename, cloud_in);
        points.reserve(cloud_in.size());
        intensity.reserve(cloud_in.size());
        for (pcl::PointCloud<pcl::PointXYZI>::const_iterator it = cloud_in.begin();it!=cloud_in.end();++it) {
            points.push_back(Base::Vector3d(it->x,it->y,it->z));
            intensity.push_back(it->intensity);
        }
    }
    else if (hasNormals) {
        pcl::PointCloud<pcl::PointNormal> cloud_in;
        pcl::io::loadPLYFile<pcl::PointNormal>(filename, cloud_in);
        points.reserve(cloud_in.size());
        normals.reserve(cloud_in.size());
        for (pcl::PointCloud<pcl::PointNormal>::const_iterator it = cloud_in.begin();it!=cloud_in.end();++it) {
            points.push_back(Base::Vector3d(it->x,it->y,it->z));
            normals.push_back(Base::Vector3f(it->normal_x,it->normal_y,it->normal_z));
        }
    }
    else {
        pcl::PointCloud<pcl::PointXYZ> cloud_in;
        pcl::io::loadPLYFile<pcl::PointXYZ>(filename, cloud_in);
        points.reserve(cloud_in.size());
        for (pcl::PointCloud<pcl::PointXYZ>::const_iterator it = cloud_in.begin();it!=cloud_in.end();++it) {
            points.push_back(Base::Vector3d(it->x,it->y,it->z));
        }
    }
}

// ----------------------------------------------------------------------------

PcdReader::PcdReader()
{
}

PcdReader::~PcdReader()
{
}

void PcdReader::read(const std::string& filename)
{
    clear();

    // pcl test
    pcl::PCLPointCloud2 cloud2;
    Eigen::Vector4f origin;
    Eigen::Quaternionf orientation;
    int ply_version;
    int data_type;
    unsigned int data_idx;
    pcl::PCDReader pcd;
    pcd.readHeader(filename, cloud2, origin, orientation, ply_version, data_type, data_idx);

    bool hasIntensity = false;
    bool hasColors = false;
    bool hasNormals = false;
    for (size_t i = 0; i < cloud2.fields.size (); ++i) {
        if (cloud2.fields[i].name == "intensity")
            hasIntensity = true;
        if (cloud2.fields[i].name == "normal_x" || cloud2.fields[i].name == "nx")
            hasNormals = true;
        if (cloud2.fields[i].name == "normal_y" || cloud2.fields[i].name == "ny")
            hasNormals = true;
        if (cloud2.fields[i].name == "normal_z" || cloud2.fields[i].name == "nz")
            hasNormals = true;
        if (cloud2.fields[i].name == "red")
            hasColors = true;
        if (cloud2.fields[i].name == "green")
            hasColors = true;
        if (cloud2.fields[i].name == "blue")
            hasColors = true;
        if (cloud2.fields[i].name == "rgb")
            hasColors = true;
        if (cloud2.fields[i].name == "rgba")
            hasColors = true;
    }

    width = cloud2.width;
    height = cloud2.height;

    if (hasNormals && hasColors) {
        pcl::PointCloud<pcl::PointXYZRGBNormal> cloud_in;
        pcl::io::loadPCDFile<pcl::PointXYZRGBNormal>(filename, cloud_in);
        points.reserve(cloud_in.size());
        colors.reserve(cloud_in.size());
        normals.reserve(cloud_in.size());
        for (pcl::PointCloud<pcl::PointXYZRGBNormal>::const_iterator it = cloud_in.begin();it!=cloud_in.end();++it) {
            points.push_back(Base::Vector3d(it->x,it->y,it->z));
            colors.push_back(App::Color(it->r/255.0f,it->g/255.0f,it->b/255.0f));
            normals.push_back(Base::Vector3f(it->normal_x,it->normal_y,it->normal_z));
        }
    }
    else if (hasNormals && hasIntensity) {
        pcl::PointCloud<pcl::PointXYZINormal> cloud_in;
        pcl::io::loadPCDFile<pcl::PointXYZINormal>(filename, cloud_in);
        points.reserve(cloud_in.size());
        intensity.reserve(cloud_in.size());
        normals.reserve(cloud_in.size());
        for (pcl::PointCloud<pcl::PointXYZINormal>::const_iterator it = cloud_in.begin();it!=cloud_in.end();++it) {
            points.push_back(Base::Vector3d(it->x,it->y,it->z));
            intensity.push_back(it->intensity);
            normals.push_back(Base::Vector3f(it->normal_x,it->normal_y,it->normal_z));
        }
    }
    else if (hasColors) {
        pcl::PointCloud<pcl::PointXYZRGBA> cloud_in;
        pcl::io::loadPCDFile<pcl::PointXYZRGBA>(filename, cloud_in);
        points.reserve(cloud_in.size());
        colors.reserve(cloud_in.size());
        for (pcl::PointCloud<pcl::PointXYZRGBA>::const_iterator it = cloud_in.begin();it!=cloud_in.end();++it) {
            points.push_back(Base::Vector3d(it->x,it->y,it->z));
            colors.push_back(App::Color(it->r/255.0f,it->g/255.0f,it->b/255.0f,it->a/255.0f));
        }
    }
    else if (hasIntensity) {
        pcl::PointCloud<pcl::PointXYZI> cloud_in;
        pcl::io::loadPCDFile<pcl::PointXYZI>(filename, cloud_in);
        points.reserve(cloud_in.size());
        intensity.reserve(cloud_in.size());
        for (pcl::PointCloud<pcl::PointXYZI>::const_iterator it = cloud_in.begin();it!=cloud_in.end();++it) {
            points.push_back(Base::Vector3d(it->x,it->y,it->z));
            intensity.push_back(it->intensity);
        }
    }
    else if (hasNormals) {
        pcl::PointCloud<pcl::PointNormal> cloud_in;
        pcl::io::loadPCDFile<pcl::PointNormal>(filename, cloud_in);
        points.reserve(cloud_in.size());
        normals.reserve(cloud_in.size());
        for (pcl::PointCloud<pcl::PointNormal>::const_iterator it = cloud_in.begin();it!=cloud_in.end();++it) {
            points.push_back(Base::Vector3d(it->x,it->y,it->z));
            normals.push_back(Base::Vector3f(it->normal_x,it->normal_y,it->normal_z));
        }
    }
    else {
        pcl::PointCloud<pcl::PointXYZ> cloud_in;
        pcl::io::loadPCDFile<pcl::PointXYZ>(filename, cloud_in);
        points.reserve(cloud_in.size());
        for (pcl::PointCloud<pcl::PointXYZ>::const_iterator it = cloud_in.begin();it!=cloud_in.end();++it) {
            points.push_back(Base::Vector3d(it->x,it->y,it->z));
        }
    }
}

#endif
