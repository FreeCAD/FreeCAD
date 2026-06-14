// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

#include <stdio.h>

#include "DriverDAT_W_SMDS_Mesh.h"

#include "SMDS_Mesh.hxx"

#include "utilities.h"

#include <Basics_Utils.hxx>

using namespace std;

Driver_Mesh::Status DriverDAT_W_SMDS_Mesh::Perform()
{
//  Kernel_Utils::Localizer loc;
  Status aResult = DRS_OK;

  int nbNodes, nbCells;

  char *file2Read = (char *)myFile.c_str();
  FILE* aFileId = fopen(file2Read, "w+");
  if ( !aFileId ) {
    fprintf(stderr, ">> ERREUR : ouverture du fichier %s \n", file2Read);
    return DRS_FAIL;
  }
  SCRUTE(myMesh);
  /****************************************************************************
   *                       NOMBRES D'OBJETS                                    *
   ****************************************************************************/

  /* Combien de noeuds ? */
  nbNodes = myMesh->NbNodes();

  /* Combien de mailles, faces ou aretes ? */
  int nb_of_edges, nb_of_faces, nb_of_volumes;
  nb_of_edges = myMesh->NbEdges();
  nb_of_faces = myMesh->NbFaces();
  nb_of_volumes = myMesh->NbVolumes();
  nbCells = nb_of_edges + nb_of_faces + nb_of_volumes;
  SCRUTE(nb_of_edges);
  SCRUTE(nb_of_faces);
  SCRUTE(nb_of_volumes);

  //fprintf(stdout, "%d %d\n", nbNodes, nbCells);
  fprintf(aFileId, "%d %d\n", nbNodes, nbCells);

  /****************************************************************************
   *                       ECRITURE DES NOEUDS                                 *
   ****************************************************************************/

  SMDS_NodeIteratorPtr itNodes=myMesh->nodesIterator();
  while(itNodes->more()){               
    const SMDS_MeshNode * node = itNodes->next();
    fprintf(aFileId, "%d %.14e %.14e %.14e\n", node->GetID(), node->X(), node->Y(), node->Z());
  }
        
  /****************************************************************************
   *                       ECRITURE DES ELEMENTS                                *
   ****************************************************************************/
  /* Ecriture des connectivites, noms, numeros des mailles */
  
  SMDS_EdgeIteratorPtr itEdges=myMesh->edgesIterator();
  while(itEdges->more()){
    const SMDS_MeshElement * elem = itEdges->next();
    switch (elem->NbNodes()) {
    case 2:
      fprintf(aFileId, "%d %d ", elem->GetID(), 102);
      break;
    case 3:
      fprintf(aFileId, "%d %d ", elem->GetID(), 103);
      break;
    }
    SMDS_ElemIteratorPtr it=elem->nodesIterator();
    while(it->more()) 
      fprintf(aFileId, "%d ", it->next()->GetID());
    fprintf(aFileId, "\n");
  }
  
  SMDS_FaceIteratorPtr itFaces=myMesh->facesIterator();
  while(itFaces->more()){
    const SMDS_MeshElement * elem = itFaces->next();
    if ( elem->IsPoly() )
      fprintf(aFileId, "%d %d ", elem->GetID(), 400+elem->NbNodes());
    else
      fprintf(aFileId, "%d %d ", elem->GetID(), 200+elem->NbNodes());
    SMDS_ElemIteratorPtr it=elem->nodesIterator();
    while(it->more()) 
      fprintf(aFileId, "%d ", it->next()->GetID());
    fprintf(aFileId, "\n");
  }

  SMDS_VolumeIteratorPtr itVolumes=myMesh->volumesIterator();
  while(itVolumes->more()){
    const SMDS_MeshElement * elem = itVolumes->next();
    if ( elem->IsPoly() )
      fprintf(aFileId, "%d %d ", elem->GetID(), 500+elem->NbNodes());
    else
      fprintf(aFileId, "%d %d ", elem->GetID(), 300+elem->NbNodes());
    SMDS_ElemIteratorPtr it=elem->nodesIterator();
    while(it->more()) 
      fprintf(aFileId, "%d ", it->next()->GetID());

    fprintf(aFileId, "\n");
  }
  
  fclose(aFileId);

  return aResult;
}
