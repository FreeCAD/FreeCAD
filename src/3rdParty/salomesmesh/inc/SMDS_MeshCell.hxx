// Copyright (C) 2010-2015  CEA/DEN, EDF R&D, OPEN CASCADE
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

#ifndef _SMDS_MESHCELL_HXX_
#define _SMDS_MESHCELL_HXX_

#include "SMDS_MeshElement.hxx"

/*!
 * \brief Base class for all cells
 */

class SMDS_EXPORT SMDS_MeshCell: public SMDS_MeshElement
{
public:
  SMDS_MeshCell();
  virtual ~SMDS_MeshCell();

  virtual bool ChangeNodes(const SMDS_MeshNode* nodes[], const int nbNodes)= 0;
  virtual bool vtkOrder(const SMDS_MeshNode* /*nodes*/[], const int /*nbNodes*/) {return true; }

  static VTKCellType         toVtkType (SMDSAbs_EntityType vtkType);
  static SMDSAbs_EntityType  toSmdsType(VTKCellType vtkType);
  static SMDSAbs_ElementType toSmdsType(SMDSAbs_GeometryType geomType);
  static SMDSAbs_ElementType toSmdsType(SMDSAbs_EntityType entityType);

  static const std::vector<int>& toVtkOrder(VTKCellType vtkType);
  static const std::vector<int>& toVtkOrder(SMDSAbs_EntityType smdsType);
  static const std::vector<int>& fromVtkOrder(VTKCellType vtkType);
  static const std::vector<int>& fromVtkOrder(SMDSAbs_EntityType smdsType);

  static const std::vector<int>& reverseSmdsOrder(SMDSAbs_EntityType smdsType,
                                                  const size_t       nbNodes=0);
  static const std::vector<int>& interlacedSmdsOrder(SMDSAbs_EntityType smdsType,
                                                     const size_t       nbNodes=0);

  template< class VECT > // interlacedIDs[i] = smdsIDs[ indices[ i ]]
    static void applyInterlace( const std::vector<int>& interlace, VECT & data)
  {
    if ( interlace.empty() ) return;
    VECT tmpData( data.size() );
    for ( size_t i = 0; i < data.size(); ++i )
      tmpData[i] = data[ interlace[i] ];
    data.swap( tmpData );
  }
  template< class VECT > // interlacedIDs[ indices[ i ]] = smdsIDs[i]
    static void applyInterlaceRev( const std::vector<int>& interlace, VECT & data)
  {
    if ( interlace.empty() ) return;
    VECT tmpData( data.size() );
    for ( size_t i = 0; i < data.size(); ++i )
      tmpData[ interlace[i] ] = data[i];
    data.swap( tmpData );
  }

  static int nbCells;

protected:
  inline void exchange(const SMDS_MeshNode* nodes[],int a, int b)
  {
    const SMDS_MeshNode* noda = nodes[a];
    nodes[a] = nodes[b];
    nodes[b] = noda;
  }
};

#endif
