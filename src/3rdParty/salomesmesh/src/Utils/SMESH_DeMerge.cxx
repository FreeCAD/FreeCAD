// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
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
// File      : SMESH_DeMerge.hxx
// Created   : Fri Mar 10 16:06:54 2017
// Author    : Edward AGAPOV (eap)

// Implementation of SMESH_MeshAlgos::DeMerge()

#include "SMESH_MeshAlgos.hxx"

#include "SMDS_VolumeTool.hxx"
#include "SMDS_MeshVolume.hxx"

namespace
{
  bool isDegenFace(const std::vector< const SMDS_MeshNode* >& nodes)
  {
    // in a degenerated face each link sticks to another

    typedef std::map< SMESH_TLink , int > TLink2Nb;
    TLink2Nb link2nb;
    for ( size_t iPrev = nodes.size() - 1, i = 0; i < nodes.size(); iPrev = i++ )
    {
      SMESH_TLink link( nodes[iPrev], nodes[i] );
      TLink2Nb::iterator l2n = link2nb.insert( std::make_pair( link, 0 )).first;
      l2n->second++;
    }

    if ( link2nb.size() == 1 )
      return true;

    for ( TLink2Nb::iterator l2n = link2nb.begin(); l2n != link2nb.end(); ++l2n )
      if ( l2n->second == 1 )
        return false;

    return true;
  }

  void deMergeFace(const SMDS_MeshElement*              face,
                   std::vector< const SMDS_MeshNode* >& newNodes,
                   std::vector< const SMDS_MeshNode* >& noMergeNodes)
  {
    if ( face->IsQuadratic() )
    {
      const int nbCorners = face->NbCornerNodes();
      const int nbNodes = (int) newNodes.size();

      // de-merge sticking medium nodes
      for ( int i = 1; i < nbNodes; i += 2 ) // loop om medium nodes
      {
        int iPrev = ( i - 1 );
        int iNext = ( i + 1 ) % nbNodes;
        if ( newNodes[ iPrev ] == newNodes[ iNext ] )
        {
          if ( newNodes[ iPrev ] != newNodes[ i ] || nbCorners == 3 )
          {
            // corners stick but the medium does not, or a link of triangle collapses
            noMergeNodes.push_back( face->GetNode( iPrev / 2 ));
            noMergeNodes.push_back( face->GetNode( iNext / 2 ));
            noMergeNodes.push_back( face->GetNode( nbCorners + i / 2 ));
          }
        }
        else if ( newNodes[ i ] == newNodes[ iPrev ] )
        {
          // the medium node sticks to a neighbor corner one
          noMergeNodes.push_back( face->GetNode( nbCorners + i / 2 ));
          noMergeNodes.push_back( face->GetNode( iPrev / 2 ));
        }
        else if ( newNodes[ i ] == newNodes[ iNext ] )
        {
          // the medium node sticks to a neighbor corner one
          noMergeNodes.push_back( face->GetNode( nbCorners + i / 2 ));
          noMergeNodes.push_back( face->GetNode( iNext / 2 ));
        }
        else
        {
          // find if the medium sticks to any other node
          std::vector<const SMDS_MeshNode*>::iterator pos;
          pos = std::find( newNodes.begin(), newNodes.begin() + iPrev, newNodes[i] );
          if ( pos == newNodes.begin() + iPrev )
            pos = std::find( newNodes.begin() + i + 1, newNodes.end(), newNodes[i] );
          if ( pos == newNodes.end() )
            continue;

          int iStick = std::distance( newNodes.begin(), pos );
          if ( iStick % 2 == 0 )
          {
            // the medium sticks to a distant corner
            noMergeNodes.push_back( face->GetNode( nbCorners + i / 2 ));
            noMergeNodes.push_back( face->GetNode( iStick / 2 ));
          }
          else
          {
            // the medium sticks to a distant medium;
            // it's OK if two links stick
            int iPrev2 = ( iStick - 1 );
            int iNext2 = ( iStick + 1 ) % nbNodes;
            if (( newNodes[ iPrev ] == newNodes[ iPrev2 ] &&
                  newNodes[ iNext ] == newNodes[ iNext2 ] )
                ||
                ( newNodes[ iPrev ] == newNodes[ iNext2 ] &&
                  newNodes[ iNext ] == newNodes[ iPrev2 ] ))
              ; // OK
            else
            {
              noMergeNodes.push_back( face->GetNode( nbCorners + i / 2 ));
              noMergeNodes.push_back( face->GetNode( nbCorners + iStick / 2 ));
            }
          }
        }
      }
    }
  } // deMergeFace()

  bool isDegenVolume(const SMDS_VolumeTool& vt)
  {
    // TMP: it's necessary to use a topological check instead of a geometrical one
    return vt.GetSize() < 1e-100;
  }

  void deMergeVolume(const SMDS_VolumeTool&               vt,
                     std::vector< const SMDS_MeshNode* >& noMergeNodes)
  {
    // temporary de-merge all nodes
    for ( int i = 0; i < vt.NbNodes(); ++i )
    {
      const SMDS_MeshNode* n = vt.GetNodes()[i];
      if ( n != vt.Element()->GetNode( i ))
        noMergeNodes.push_back( n );
    }
  }

} // namespace

//================================================================================
/*!
 * \brief Find nodes whose merge makes the element invalid. (Degenerated elem is OK)
 *  \param [in] elem - the element
 *  \param [in] newNodes - nodes of the element after the merge
 *  \param [out] noMergeNodes - nodes to undo merge 
 */
//================================================================================

void SMESH_MeshAlgos::DeMerge(const SMDS_MeshElement*              elem,
                              std::vector< const SMDS_MeshNode* >& newNodes,
                              std::vector< const SMDS_MeshNode* >& noMergeNodes)
{
  switch ( elem->GetType() )
  {
  case SMDSAbs_Face:
  {
    if ( newNodes.size() <= 4 )
      return; // degenerated

    if ( elem->IsQuadratic() )
      SMDS_MeshCell::applyInterlace // interlace medium and corner nodes
        ( SMDS_MeshCell::interlacedSmdsOrder( elem->GetEntityType(), newNodes.size() ), newNodes );

    if ( isDegenFace( newNodes ))
      return;

    deMergeFace( elem, newNodes, noMergeNodes );
  }
  break;

  case SMDSAbs_Volume:
  {
    if ( newNodes.size() <= 4 )
      return; // degenerated

    SMDS_VolumeTool vt;
    if ( !vt.Set( elem, /*skipCentral=*/true, &newNodes ))
      return; // strange

    if ( isDegenVolume( vt ))
      return;

    deMergeVolume( elem, noMergeNodes );
  }
  break;

  case SMDSAbs_Edge:
  {
    if ( newNodes.size() == 3 )
      if (( newNodes[2] == newNodes[0] && newNodes[2] != newNodes[1] ) ||
          ( newNodes[2] == newNodes[1] && newNodes[2] != newNodes[0]))
      {
        // the medium node sticks to a corner
        noMergeNodes.push_back( newNodes[2] );
        noMergeNodes.push_back( newNodes[ newNodes[2] == newNodes[1] ]);
      }
  }
  break;
  default:;
  }
}
