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


#ifndef DEVIATION_H
#define DEVIATION_H

#include "best_fit.h"
#include "SpringbackCorrection.h"
#include <string.h>
#include <QString>

class CamExport Deviation : public SpringbackCorrection
{
public:
	Deviation();
	~Deviation();

	bool GenNormals();
	void ImportGeometry(const TopoDS_Shape& aShape, const MeshCore::MeshKernel& aMesh);
	void WriteOutput(const QString &dateiname);
	bool Compute();
	
	TopoDS_Shape         m_Cad;              // CAD-Geometrie
	MeshCore::MeshKernel m_MeshCad;
	MeshCore::MeshKernel m_Mesh;

	std::vector<Base::Vector3f> m_pnts, m_nlvec;
};


#endif //DEVIATION_H