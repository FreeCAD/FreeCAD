c  MEFISTO : library to compute 2D triangulation from segmented boundaries
c
c Copyright (C) 2006-2016  CEA/DEN, EDF R&D, OPEN CASCADE
c
c This library is free software; you can redistribute it and/or
c modify it under the terms of the GNU Lesser General Public
c License as published by the Free Software Foundation; either
c version 2.1 of the License, or (at your option) any later version.
c
c This library is distributed in the hope that it will be useful,
c but WITHOUT ANY WARRANTY; without even the implied warranty of
c MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
c Lesser General Public License for more details.
c
c You should have received a copy of the GNU Lesser General Public
c License along with this library; if not, write to the Free Software
c Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
c
c See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
c
c  File   : areteideale.f
c  Module : SMESH
c  Author : Alain PERRONNET
c  Date   : 13 novembre 2006

      double precision function areteideale( xyz, direction )
      double precision xyz(3), direction(3)
      areteideale = 10
      return
      end
