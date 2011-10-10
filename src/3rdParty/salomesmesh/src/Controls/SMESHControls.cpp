//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
#include "SMESH_ControlsDef.hxx"

int main(int argc, char** argv)
{
  using namespace SMESH::Controls;
  new MinimumAngle();
  new AspectRatio();
  new Warping();
  new Taper();
  new Skew();
  new Area();
  new Length();
  //  new Length2D();
  new MultiConnection();
  //  new MultiConnection2D();
  new FreeBorders();
  new LessThan();
  new MoreThan();
  new EqualTo();
  new LogicalNOT();
  new LogicalAND();
  new LogicalOR();
  new ManifoldPart();

  return 1;
}
