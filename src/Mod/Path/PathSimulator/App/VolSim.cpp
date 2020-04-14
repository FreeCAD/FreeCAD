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
***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <algorithm>
#endif

#include "VolSim.h"

//************************************************************************************************************
// stock
//************************************************************************************************************
cStock::cStock(float px, float py, float pz, float lx, float ly, float lz, float res)
	: m_px(px), m_py(py), m_pz(pz), m_lx(lx), m_ly(ly), m_lz(lz), m_res(res)
{
	m_x = (int)(m_lx / res) + 1;
	m_y = (int)(m_ly / res) + 1;
	m_stock.Init(m_x, m_y);
	m_attr.Init(m_x, m_y);
	m_plane = pz + lz;
	for (int y = 0; y < m_y; y++)
		for (int x = 0; x < m_x; x++)
		{
			m_stock[x][y] = m_plane;
			m_attr[x][y] = 0;
		}
}

cStock::~cStock()
{
}


float cStock::FindRectTop(int & xp, int & yp, int & x_size, int & y_size, bool scanHoriz)
{
	float z = m_stock[xp][yp];
	bool xr_ok = true;
	bool xl_ok = scanHoriz;
	bool yu_ok = true;
	bool yd_ok = !scanHoriz;
	x_size = 1;
	y_size = 1;
	while (xr_ok || xl_ok || yu_ok || yd_ok) {
		// sweep right x direction
		if (xr_ok)
		{
			int tx = xp + x_size;
			if (tx >= m_x)
				xr_ok = false;
			else
			{
				for (int y = yp; y < yp + y_size; y++)
				{
					if ((m_attr[tx][y] & SIM_TESSEL_TOP) != 0 || fabs(z - m_stock[tx][y]) > m_res)
					{
						xr_ok = false;
						break;
					}
				}
				if (xr_ok)
					x_size++;
			}
		}

		// sweep left x direction
		if (xl_ok)
		{
			int tx = xp - 1;
			if (tx < 0)
				xl_ok = false;
			else
			{
				for (int y = yp; y < yp + y_size; y++)
				{
					if ((m_attr[tx][y] & SIM_TESSEL_TOP) != 0 || fabs(z - m_stock[tx][y]) > m_res)
					{
						xl_ok = false;
						break;
					}
				}
				if (xl_ok)
				{
					x_size++;
					xp--;
				}
			}
		}

		// sweep up y direction
		if (yu_ok)
		{
			int ty = yp + y_size;
			if (ty >= m_y)
				yu_ok = false;
			else
			{
				for (int x = xp; x < xp + x_size; x++)
				{
					if ((m_attr[x][ty] & SIM_TESSEL_TOP) != 0 || fabs(z - m_stock[x][ty]) > m_res)
					{
						yu_ok = false;
						break;
					}
				}
				if (yu_ok)
					y_size++;
			}
		}

		// sweep down y direction
		if (yd_ok)
		{
			int ty = yp - 1;
			if (ty < 0)
				yd_ok = false;
			else
			{
				for (int x = xp; x < xp + x_size; x++)
				{
					if ((m_attr[x][ty] & SIM_TESSEL_TOP) != 0 || fabs(z - m_stock[x][ty]) > m_res)
					{
						yd_ok = false;
						break;
					}
				}
				if (yd_ok)
				{
					y_size++;
					yp--;
				}
			}
		}
	}
	return z;
}

int cStock::TesselTop(int xp, int yp)
{
	int x_size, y_size;
	float z = FindRectTop(xp, yp, x_size, y_size, true);
	bool farRect = false;
	while (y_size / x_size > 5)
	{
		farRect = true;
		yp += x_size * 5;
		z = FindRectTop(xp, yp, x_size, y_size, true);
	}

	while (x_size / y_size > 5)
	{
		farRect = true;
		xp += y_size * 5;
		z = FindRectTop(xp, yp, x_size, y_size, false);
	}

	// mark all points inside
	for (int y = yp; y < yp + y_size; y++)
		for (int x = xp; x < xp + x_size; x++)
			m_attr[x][y] |= SIM_TESSEL_TOP;

	if (z > m_pz + m_res)
	{
		// generate 4 3d points
		Point3D pbl(xp, yp, z);
		Point3D pbr(xp + x_size, yp, z);
		Point3D ptl(xp, yp + y_size, z);
		Point3D ptr(xp + x_size, yp + y_size, z);
		if (fabs(m_pz + m_lz - z) < SIM_EPSILON)
			AddQuad(pbl, pbr, ptr, ptl, facetsOuter);
		else
			AddQuad(pbl, pbr, ptr, ptl, facetsInner);
	}

	if (farRect)
		return -1;
	return std::max(0, x_size - 1);
	//return 0;
}


void cStock::FindRectBot(int & xp, int & yp, int & x_size, int & y_size, bool scanHoriz)
{
	bool xr_ok = true;
	bool xl_ok = scanHoriz;
	bool yu_ok = true;
	bool yd_ok = !scanHoriz;
	x_size = 1;
	y_size = 1;
	while (xr_ok || xl_ok || yu_ok || yd_ok) {
		// sweep right x direction
		if (xr_ok)
		{
			int tx = xp + x_size;
			if (tx >= m_x)
				xr_ok = false;
			else
			{
				for (int y = yp; y < yp + y_size; y++)
				{
					if ((m_attr[tx][y] & SIM_TESSEL_BOT) != 0 || (m_stock[tx][y] - m_pz) < m_res)
					{
						xr_ok = false;
						break;
					}
				}
				if (xr_ok)
					x_size++;
			}
		}

		// sweep left x direction
		if (xl_ok)
		{
			int tx = xp - 1;
			if (tx < 0)
				xl_ok = false;
			else
			{
				for (int y = yp; y < yp + y_size; y++)
				{
					if ((m_attr[tx][y] & SIM_TESSEL_BOT) != 0 || (m_stock[tx][y] - m_pz) < m_res)
					{
						xl_ok = false;
						break;
					}
				}
				if (xl_ok)
				{
					x_size++;
					xp--;
				}
			}
		}

		// sweep up y direction
		if (yu_ok)
		{
			int ty = yp + y_size;
			if (ty >= m_y)
				yu_ok = false;
			else
			{
				for (int x = xp; x < xp + x_size; x++)
				{
					if ((m_attr[x][ty] & SIM_TESSEL_BOT) != 0 || (m_stock[x][ty] - m_pz) < m_res)
					{
						yu_ok = false;
						break;
					}
				}
				if (yu_ok)
					y_size++;
			}
		}

		// sweep down y direction
		if (yd_ok)
		{
			int ty = yp - 1;
			if (ty < 0)
				yd_ok = false;
			else
			{
				for (int x = xp; x < xp + x_size; x++)
				{
					if ((m_attr[x][ty] & SIM_TESSEL_BOT) != 0 || (m_stock[x][ty] - m_pz) < m_res)
					{
						yd_ok = false;
						break;
					}
				}
				if (yd_ok)
				{
					y_size++;
					yp--;
				}
			}
		}
	}
}


int cStock::TesselBot(int xp, int yp)
{
	int x_size, y_size;
	FindRectBot(xp, yp, x_size, y_size, true);
	bool farRect = false;
	while (y_size / x_size > 5)
	{
		farRect = true;
		yp += x_size * 5;
		FindRectTop(xp, yp, x_size, y_size, true);
	}

	while (x_size / y_size > 5)
	{
		farRect = true;
		xp += y_size * 5;
		FindRectTop(xp, yp, x_size, y_size, false);
	}

	// mark all points inside
	for (int y = yp; y < yp + y_size; y++)
		for (int x = xp; x < xp + x_size; x++)
			m_attr[x][y] |= SIM_TESSEL_BOT;

	// generate 4 3d points
	Point3D pbl(xp, yp, m_pz);
	Point3D pbr(xp + x_size, yp, m_pz);
	Point3D ptl(xp, yp + y_size, m_pz);
	Point3D ptr(xp + x_size, yp + y_size, m_pz);
	AddQuad(pbl, ptl, ptr, pbr, facetsOuter);

	if (farRect)
		return -1;
	return std::max(0, x_size - 1);
	//return 0;
}


int cStock::TesselSidesX(int yp)
{
	float lastz1 = m_pz;
	if (yp < m_y)
		lastz1 = std::max(m_stock[0][yp], m_pz);
	float lastz2 = m_pz;
	if (yp > 0)
		lastz2 = std::max(m_stock[0][yp - 1], m_pz);

	std::vector<MeshCore::MeshGeomFacet> *facets = &facetsInner;
	if (yp == 0 || yp == m_y)
		facets = &facetsOuter;

	//bool lastzclip = (lastz - m_pz) < m_res;
	int lastpoint = 0;
	for (int x = 1; x <= m_x; x++)
	{
		float newz1 = m_pz;
		if (yp < m_y && x < m_x)
			newz1 = std::max(m_stock[x][yp], m_pz);
		float newz2 = m_pz;
		if (yp > 0 && x < m_x)
			newz2 = std::max(m_stock[x][yp - 1], m_pz);

		if (fabs(lastz1 - lastz2) > m_res)
		{
			if (fabs(newz1 - lastz1) < m_res && fabs(newz2 - lastz2) < m_res)
				continue;
			Point3D pbl(lastpoint, yp, lastz1);
			Point3D pbr(x, yp, lastz1);
			Point3D ptl(lastpoint, yp, lastz2);
			Point3D ptr(x, yp, lastz2);
			AddQuad(pbl, ptl, ptr, pbr, *facets);
		}
		lastz1 = newz1;
		lastz2 = newz2;
		lastpoint = x;
	}
	return 0;
}

int cStock::TesselSidesY(int xp)
{
	float lastz1 = m_pz;
	if (xp < m_x)
		lastz1 = std::max(m_stock[xp][0], m_pz);
	float lastz2 = m_pz;
	if (xp > 0)
		lastz2 = std::max(m_stock[xp - 1][0], m_pz);

	std::vector<MeshCore::MeshGeomFacet> *facets = &facetsInner;
	if (xp == 0 || xp == m_x)
		facets = &facetsOuter;

	//bool lastzclip = (lastz - m_pz) < m_res;
	int lastpoint = 0;
	for (int y = 1; y <= m_y; y++)
	{
		float newz1 = m_pz;
		if (xp < m_x && y < m_y)
			newz1 = std::max(m_stock[xp][y], m_pz);
		float newz2 = m_pz;
		if (xp > 0 && y < m_y)
			newz2 = std::max(m_stock[xp - 1][y], m_pz);

		if (fabs(lastz1 - lastz2) > m_res)
		{
			if (fabs(newz1 - lastz1) < m_res && fabs(newz2 - lastz2) < m_res)
				continue;
			Point3D pbr(xp, lastpoint, lastz1);
			Point3D pbl(xp, y, lastz1);
			Point3D ptr(xp, lastpoint, lastz2);
			Point3D ptl(xp, y, lastz2);
			AddQuad(pbl, ptl, ptr, pbr, *facets);
		}
		lastz1 = newz1;
		lastz2 = newz2;
		lastpoint = y;
	}
	return 0;
}

void cStock::SetFacetPoints(MeshCore::MeshGeomFacet & facet, Point3D & p1, Point3D & p2, Point3D & p3)
{
	facet._aclPoints[0][0] = p1.x * m_res + m_px;
	facet._aclPoints[0][1] = p1.y * m_res + m_py;
	facet._aclPoints[0][2] = p1.z;
	facet._aclPoints[1][0] = p2.x * m_res + m_px;
	facet._aclPoints[1][1] = p2.y * m_res + m_py;
	facet._aclPoints[1][2] = p2.z;
	facet._aclPoints[2][0] = p3.x * m_res + m_px;
	facet._aclPoints[2][1] = p3.y * m_res + m_py;
	facet._aclPoints[2][2] = p3.z;
	facet.CalcNormal();
}

void cStock::AddQuad(Point3D & p1, Point3D & p2, Point3D & p3, Point3D & p4, std::vector<MeshCore::MeshGeomFacet> & facets)
{
	MeshCore::MeshGeomFacet facet;
	SetFacetPoints(facet, p1, p2, p3);
	facets.push_back(facet);
	SetFacetPoints(facet, p1, p3, p4);
	facets.push_back(facet);
}

void cStock::Tessellate(Mesh::MeshObject & meshOuter, Mesh::MeshObject & meshInner)
{
	// reset attribs
	for (int y = 0; y < m_y; y++)
	for (int x = 0; x < m_x; x++)
		m_attr[x][y] = 0;

	facetsOuter.clear();
	facetsInner.clear();

	for (int y = 0; y < m_y; y++)
	{
		for (int x = 0; x < m_x; x++)
		{
			int attr = m_attr[x][y];
			if ((attr & SIM_TESSEL_TOP) == 0)
				x += TesselTop(x, y);
		}
	}
	for (int y = 0; y < m_y; y++)
	{
		for (int x = 0; x < m_x; x++)
		{
			if ((m_stock[x][y] - m_pz) < m_res)
				m_attr[x][y] |= SIM_TESSEL_BOT;
			if ((m_attr[x][y] & SIM_TESSEL_BOT) == 0)
				x += TesselBot(x, y);
		}
	}
	for (int y = 0; y <= m_y; y++)
		TesselSidesX(y);
	for (int x = 0; x <= m_x; x++)
		TesselSidesY(x);
	meshOuter.addFacets(facetsOuter);
	meshInner.addFacets(facetsInner);
	facetsOuter.clear();
	facetsInner.clear();
}


void cStock::CreatePocket(float cxf, float cyf, float radf, float height)
{
	int cx = (int)((cxf - m_px) / m_res);
	int cy = (int)((cyf - m_py) / m_res);
	int rad = (int)(radf / m_res);
	int drad = rad * rad;
	int ys = std::max(0, cy - rad);
	int ye = std::min(m_x, cy + rad);
	int xs = std::max(0, cx - rad);
	int xe = std::min(m_x, cx + rad);
	for (int y = ys; y < ye; y++)
	{
		for (int x = xs; x < xe; x++)
		{
			if (((x - cx)*(x - cx) + (y - cy) * (y - cy)) < drad)
				if (m_stock[x][y] > height) m_stock[x][y] = height;
		}
	}
}

void cStock::ApplyLinearTool(Point3D & p1, Point3D & p2, cSimTool & tool)
{
	// translate coordinates
	Point3D pi1 = ToInner(p1);
	Point3D pi2 = ToInner(p2);
	float rad = tool.radius;
	rad /= m_res;
	float cupAngle = 180;

	// strait motion
	float perpDirX = 1;
	float perpDirY = 0;
	cLineSegment path(pi1, pi2);
	if (path.lenXY > SIM_EPSILON) // only if moving along xy
	{
		perpDirX = -path.pDirXY.y;
		perpDirY = path.pDirXY.x;
		Point3D start(perpDirX * rad + pi1.x, perpDirY * rad + pi1.y, pi1.z);
		Point3D mainWay = path.pDir * SIM_WALK_RES;
		Point3D sideWay(-perpDirX * SIM_WALK_RES, -perpDirY * SIM_WALK_RES, 0);
		int lenSteps = (int)(path.len / SIM_WALK_RES) + 1;
		int radSteps = (int)(rad * 2 / SIM_WALK_RES) + 1;
		float zstep = (pi2.z - pi1.z) / radSteps;
		float tstep = 2.0 / radSteps;
		float t = -1;
		for (int j = 0; j < radSteps; j++)
		{
			float z = pi1.z + tool.GetToolProfileAt(t);
			Point3D p = start;
			for (int i = 0; i < lenSteps; i++)
			{
				int x = (int)p.x;
				int y = (int)p.y;
				if (x >= 0 && y >= 0 && x < m_x && y < m_y)
				{
					if (m_stock[x][y] > z)
						m_stock[x][y] = z;
				}
				p.Add(mainWay);
				z += zstep;
			}
			t += tstep;
			start.Add(sideWay);
		}
	}
	else
		cupAngle = 360;

	// end cup
	for (float r = 0.5f; r <= rad; r += (float)SIM_WALK_RES)
	{
		Point3D cupCirc(perpDirX * r, perpDirY * r, pi2.z);
		float rotang = 180 * SIM_WALK_RES / (3.1415926535 * r);
		cupCirc.SetRotationAngle(-rotang);
		float z = pi2.z + tool.GetToolProfileAt(r / rad);
		for (float a = 0; a < cupAngle; a += rotang)
		{
			int x = (int)(pi2.x + cupCirc.x);
			int y = (int)(pi2.y + cupCirc.y);
			if (x >= 0 && y >= 0 && x < m_x && y < m_y)
			{
				if (m_stock[x][y] > z)
					m_stock[x][y] = z;
			}
			cupCirc.Rotate();
		}
	}
}

void cStock::ApplyCircularTool(Point3D & p1, Point3D & p2, Point3D & cent, cSimTool & tool, bool isCCW)
{
	// translate coordinates
	Point3D pi1 = ToInner(p1);
	Point3D pi2 = ToInner(p2);
	Point3D centi(cent.x / m_res, cent.y / m_res, cent.z);
	float rad = tool.radius;
	rad /= m_res;
	float cpx = centi.x;
	float cpy = centi.y;

	Point3D xynorm = unit(Point3D(-cpx, -cpy, 0));
	float crad = sqrt(cpx * cpx + cpy * cpy);
	//bool shortRad = (crad - rad) < 0.0001;
	//if (shortRad)
	//	rad = crad;
	float crad1 = std::max((float)0.5, crad - rad);
	float crad2 = crad + rad;

	float sang = atan2(-cpy, -cpx); // start angle

	cpx += pi1.x;
	cpy += pi1.y;
	double eang = atan2(pi2.y - cpy, pi2.x - cpx); // end angle

	double ang = eang - sang;
	if (!isCCW && ang > 0)
		ang -= 2 * 3.1415926;
	if (isCCW && ang < 0)
		ang += 2 * 3.1415926;
	ang = fabs(ang);

	// apply path
	Point3D cupCirc;
	float tstep = (float)SIM_WALK_RES / rad;
	float t = -1;
	for (float r = crad1; r <= crad2; r += (float)SIM_WALK_RES)
	{
		cupCirc.x = xynorm.x * r;
		cupCirc.y = xynorm.y * r;
		float rotang = (float)SIM_WALK_RES / r;
		int ndivs = (int)(ang / rotang) + 1;
		if (!isCCW)
			rotang = -rotang;
		cupCirc.SetRotationAngleRad(rotang);
		float z = pi1.z + tool.GetToolProfileAt(t);
		float zstep = (pi2.z - pi1.z) / ndivs;
		for (int i = 0; i< ndivs; i++)
		{
			int x = (int)(cpx + cupCirc.x);
			int y = (int)(cpy + cupCirc.y);
			if (x >= 0 && y >= 0 && x < m_x && y < m_y)
			{
				if (m_stock[x][y] > z)
					m_stock[x][y] = z;
			}
			z += zstep;
			cupCirc.Rotate();
		}
		t += tstep;
	}

	// apply end cup
	xynorm.SetRotationAngleRad(ang);
	xynorm.Rotate();
	for (float r = 0.5f; r <= rad; r += (float)SIM_WALK_RES)
	{
		Point3D cupCirc(xynorm.x * r, xynorm.y * r, 0);
		float rotang = (float)SIM_WALK_RES / r;
		int ndivs = (int)(3.1415926535 / rotang) + 1;
		if (!isCCW)
			rotang = -rotang;
		cupCirc.SetRotationAngleRad(rotang);
		float z = pi2.z + tool.GetToolProfileAt(r / rad);
		for (int i = 0; i < ndivs; i++)
		{
			int x = (int)(pi2.x + cupCirc.x);
			int y = (int)(pi2.y + cupCirc.y);
			if (x >= 0 && y >= 0 && x < m_x && y < m_y)
			{
				if (m_stock[x][y] > z)
					m_stock[x][y] = z;
			}
			cupCirc.Rotate();
		}
	}
}


//************************************************************************************************************
// Line Segment
//************************************************************************************************************

void cLineSegment::SetPoints(Point3D & p1, Point3D & p2)
{
	pStart = p1;
	pDir = unit(p2 - p1);
	Point3D dirXY(pDir.x, pDir.y, 0);
	lenXY = length(dirXY);
	len = length(p2 - p1);
	if (len > SIM_EPSILON)
		pDirXY = unit(dirXY);
}

void cLineSegment::PointAt(float dist, Point3D & retp)
{
	retp.x = pStart.x + pDir.x * dist;
	retp.y = pStart.y + pDir.y * dist;
	retp.z = pStart.z + pDir.z * dist;
}

//************************************************************************************************************
// Point (or vector)
//************************************************************************************************************

void Point3D::SetRotationAngleRad(float angle)
{
	sina = sin(angle);
	cosa = cos(angle);
}

void Point3D::SetRotationAngle(float angle)
{
	SetRotationAngleRad(angle * 2 * 3.1415926535 / 360);
}

void Point3D::UpdateCmd(Path::Command & cmd)
{
	if (cmd.has("X"))
		x = cmd.getPlacement().getPosition()[0];
	if (cmd.has("Y"))
		y = cmd.getPlacement().getPosition()[1];
	if (cmd.has("Z"))
		z = cmd.getPlacement().getPosition()[2];
}

//************************************************************************************************************
// Simulation tool
//************************************************************************************************************
float cSimTool::GetToolProfileAt(float pos)  // pos is -1..1 location along the radius of the tool (0 is center)
{
	switch (type)
	{
	case FLAT:
		return 0;

	case CHAMFER:
	{
		if (pos < 0) return -chamRatio * pos;
		return chamRatio * pos;
	}

	case ROUND:
		pos *= radius;
		return radius - sqrt(dradius - pos * pos);
		break;
	}
	return 0;
}

void cSimTool::InitTool()  // pos is 0..1 location along the radius of the tool
{
	switch (type)
	{
	case CHAMFER:
		chamRatio = radius * tan((90.0 - tipAngle / 2) * 3.1415926535 / 180);
		break;

	case ROUND:
		dradius = radius * radius;
		break;

	case FLAT:
		break;
	}
}

cVolSim::cVolSim() : stock(nullptr)
{
}


cVolSim::~cVolSim()
{
}
