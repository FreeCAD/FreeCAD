/***************************************************************************
*   Copyright (c) Shsi Seger (shaise at gmail) 2017                       *
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
***************************************************************************
*   Volumetric Path simulation engine                                     *
***************************************************************************/


#ifndef PATHSIMULATOR_VolSim_H
#define PATHSIMULATOR_VolSim_H

#include <vector>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Path/App/Command.h>

#define SIM_EPSILON 0.00001
#define SIM_TESSEL_TOP		1
#define SIM_TESSEL_BOT		2
#define SIM_WALK_RES		0.6   // step size in pixel units (to make sure all pixels in the path are visited)
struct Point3D
{
	Point3D() {}
	Point3D(float x, float y, float z) : x(x), y(y), z(z) {}
	Point3D(Base::Vector3d & vec) : x(vec[0]), y(vec[1]), z(vec[2]) {}
	Point3D(Base::Placement & pl) : x(pl.getPosition()[0]), y(pl.getPosition()[1]), z(pl.getPosition()[2]) {}
	inline void set(float px, float py, float pz) { x = px; y = py; z = pz; }
	inline void Add(Point3D & p) { x += p.x; y += p.y; z += p.z; }
	inline void Rotate() { float tx = x;  x = x * cosa - y * sina; y = tx * sina + y * cosa; }
	void UpdateCmd(Path::Command & cmd);
	void SetRotationAngle(float angle);
	void SetRotationAngleRad(float angle);
	float x, y, z;
	float sina, cosa;
};

// some vector manipulations
inline static Point3D operator + (const Point3D & a, const Point3D & b) { return Point3D(a.x + b.x, a.y + b.y, a.z + b.z); }
inline static Point3D operator - (const Point3D & a, const Point3D & b) { return Point3D(a.x - b.x, a.y - b.y, a.z - b.z); }
inline static Point3D operator * (const Point3D & a, double b) { return Point3D(a.x * b, a.y * b, a.z * b); }
inline static Point3D operator / (const Point3D & a, double b) { return a * (1.0f / b); }
inline static double dot(const Point3D & a, const Point3D & b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline static double length(const Point3D & a) { return sqrtf(dot(a, a)); }
inline static Point3D unit(const Point3D & a) { return a / length(a); }


struct Triangle3D
{
	Triangle3D() {}
	Triangle3D(Point3D & p1, Point3D & p2, Point3D & p3) 
	{
		points[0] = p1;
		points[1] = p2;
		points[2] = p3;
	}
	Point3D points[3];
};

struct cLineSegment
{
	cLineSegment() {}
	cLineSegment(Point3D & p1, Point3D & p2) { SetPoints(p1, p2); }
	void SetPoints(Point3D & p1, Point3D & p2);
	void PointAt(float dist, Point3D & retp); 
	Point3D pStart;
	Point3D pDir;
	Point3D pDirXY;
	float len;
	float lenXY;
};

struct Model3D
{
	Model3D(float px, float py, float res) : pos_x(px), pos_y(py), resolution(res) {}
	void SetFacetPoints(MeshCore::MeshGeomFacet & facet, Point3D & p1, Point3D & p2, Point3D & p3)
	{
		facet._aclPoints[0][0] = p1.x * resolution + pos_x;
		facet._aclPoints[0][1] = p1.y * resolution + pos_y;
		facet._aclPoints[0][2] = p1.z;
		facet._aclPoints[1][0] = p2.x * resolution + pos_x;
		facet._aclPoints[1][1] = p2.y * resolution + pos_y;
		facet._aclPoints[1][2] = p2.z;
		facet._aclPoints[2][0] = p3.x * resolution + pos_x;
		facet._aclPoints[2][1] = p3.y * resolution + pos_y;
		facet._aclPoints[2][2] = p3.z;
		facet.CalcNormal();
	}

	inline void AddQuad(Point3D & p1, Point3D & p2, Point3D & p3, Point3D & p4)
	{
		MeshCore::MeshGeomFacet facet;
		SetFacetPoints(facet, p1, p2, p3);
		mesh.addFacet(facet);
		SetFacetPoints(facet, p1, p3, p4);
		mesh.addFacet(facet);
	}

	float pos_x, pos_y, resolution;
	Mesh::MeshObject mesh;
};

class cSimTool
{
public:
	enum Type {
		FLAT = 0,
		CHAMFER,
		ROUND
	};
	cSimTool() {}
	cSimTool(Type t, float rad, float tipang = 180) : type(t), radius(rad), tipAngle(tipang) { InitTool(); }
	~cSimTool() {}
	void InitTool();

	Type type;
	float tipAngle;
	float radius;
	float dradius;
	float chamRatio;
	float GetToolProfileAt(float pos);
};

template <class T>
class Array2D
{
public:
	Array2D() : data(nullptr) {}

	~Array2D()
	{
		if (data != nullptr)
			delete[] data;
	}

	void Init(int x, int y)
	{
		data = new T[x * y];
		height = y;
	}

	T *operator [] (int i) { return data + i * height; }

private:
	T *data;
	int height;
};


class cStock
{
public:
	cStock(float px, float py, float pz, float lx, float ly, float lz, float res);
	~cStock();
	void Tesselate(Mesh::MeshObject & mesh);
    void CreatePocket(float x, float y, float rad, float height);
    void ApplyLinearTool(Point3D & p1, Point3D & p2, cSimTool &tool);
    void ApplyCircularTool(Point3D & p1, Point3D & p2, Point3D & cent, cSimTool &tool, bool isCCW);
    inline Point3D ToInner(Point3D & p) {
		return Point3D((p.x - m_px) / m_res, (p.y - m_py) / m_res, p.z);
	}

private:
	float FindRectTop(int & xp, int & yp, int & x_size, int & y_size, bool scanHoriz);
	void FindRectBot(int & xp, int & yp, int & x_size, int & y_size, bool scanHoriz);
	void SetFacetPoints(MeshCore::MeshGeomFacet & facet, Point3D & p1, Point3D & p2, Point3D & p3);
	void AddQuad(Point3D & p1, Point3D & p2, Point3D & p3, Point3D & p4);
	int TesselTop(int x, int y);
	int TesselBot(int x, int y);
	int TesselSidesX(int yp);
	int TesselSidesY(int xp);
	Array2D<float>  m_stock;
	Array2D<char> m_attr;
	float m_px, m_py, m_pz;  // stock zero position
	float m_lx, m_ly, m_lz;  // stock dimensions
	float m_res;        // resoulution
	float m_plane;		// stock plane height
	int m_x, m_y;            // stock array size
	std::vector<MeshCore::MeshGeomFacet> facets;
};

class cVolSim
{
public:
	cVolSim();
	~cVolSim();
	void CreateStock();

private:
	cStock *stock;
};

#endif  // PATHSIMULATOR_VolSim_H
