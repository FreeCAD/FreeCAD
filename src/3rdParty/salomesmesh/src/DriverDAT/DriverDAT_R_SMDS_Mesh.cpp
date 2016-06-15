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

#include "DriverDAT_R_SMDS_Mesh.h"
#include "SMDS_Mesh.hxx"

#include "utilities.h"

#include <Basics_Utils.hxx>

using namespace std;

Driver_Mesh::Status DriverDAT_R_SMDS_Mesh::Perform()
{
//  Kernel_Utils::Localizer loc;
  Status aResult = DRS_OK;

  int i, j;
  int nbNodes, nbCells;
  int intNumPoint;
  float coordX, coordY, coordZ;
  int nbNoeuds;
  
  int intNumMaille, Degre;
  int ValElement;
  int NoeudsMaille[20];
  int NoeudMaille;
  
  MESSAGE("in DriverDAT_R_SMDS_Mesh::Read()");
  /****************************************************************************
   *                      OUVERTURE DU FICHIER EN LECTURE                      *
   ****************************************************************************/
  char *file2Read = (char *)myFile.c_str();
  FILE* aFileId = fopen(file2Read, "r");
  if ( !aFileId ) {
    fprintf(stderr, ">> ERREUR : ouverture du fichier %s \n", file2Read);
    return DRS_FAIL;
  }
  
  fscanf(aFileId, "%d %d\n", &nbNodes, &nbCells);
  
  /****************************************************************************
   *                       LECTURE DES NOEUDS                                  *
   ****************************************************************************/
  fprintf(stdout, "\n(************************)\n");
  fprintf(stdout, "(* NOEUDS DU MAILLAGE : *)\n");
  fprintf(stdout, "(************************)\n");
  
  for (i = 0; i < nbNodes; i++){
    fscanf(aFileId, "%d %e %e %e\n", &intNumPoint, &coordX, &coordY, &coordZ);
    myMesh->AddNodeWithID(coordX, coordY, coordZ, intNumPoint);
  }
  
  fprintf(stdout, "%d noeuds\n", myMesh->NbNodes());
  /****************************************************************************
   *                       LECTURE DES ELEMENTS                                *
   ****************************************************************************/
  fprintf(stdout, "\n(**************************)\n");
  fprintf(stdout, "(* ELEMENTS DU MAILLAGE : *)\n");
  fprintf(stdout, "(**************************)");
  
  fprintf(stdout, "%d elements\n", nbCells);
  
  for (i = 0; i < nbCells; i++) {
    fscanf(aFileId, "%d %d", &intNumMaille, &ValElement);
    Degre = abs(ValElement / 100);
    nbNoeuds = ValElement - (Degre * 100);
    
    // Recuperation des noeuds de la maille
    for (j = 0; j < nbNoeuds; j++) {
      fscanf(aFileId, "%d", &NoeudMaille);
      NoeudsMaille[j] = NoeudMaille;
    }
    
    // Analyse des cas de cellules
    switch (ValElement) {
    case 102:
    case 103:
      nbNoeuds = 2;
      myMesh->AddEdgeWithID(NoeudsMaille[0], NoeudsMaille[1], 
                                 intNumMaille);
      break;
    case 204:
    case 208:
      nbNoeuds = 4;
      myMesh->AddFaceWithID(NoeudsMaille[0], NoeudsMaille[1],
                                 NoeudsMaille[2], NoeudsMaille[3], 
                                 intNumMaille);
      break;
    case 203:
    case 206:
      nbNoeuds = 3;
      myMesh->AddFaceWithID(NoeudsMaille[0], NoeudsMaille[1],
                                 NoeudsMaille[2], intNumMaille);
      break;
    case 308:
    case 320:
      nbNoeuds = 8;
      if (ValElement == 320){
        //A voir, correspondance VTK
        NoeudsMaille[4] = NoeudsMaille[8];
        NoeudsMaille[5] = NoeudsMaille[9];
        NoeudsMaille[6] = NoeudsMaille[10];
        NoeudsMaille[7] = NoeudsMaille[11];
      }
      myMesh->AddVolumeWithID(NoeudsMaille[0], NoeudsMaille[1],
                                   NoeudsMaille[2], NoeudsMaille[3], 
                                   NoeudsMaille[4], NoeudsMaille[5], 
                                   NoeudsMaille[6], NoeudsMaille[7],
                                   intNumMaille);
      break;
    case 304:
    case 310:
      nbNoeuds = 4;
      if (ValElement == 310)
        NoeudsMaille[3] = NoeudsMaille[6];
      myMesh->AddVolumeWithID(NoeudsMaille[0], NoeudsMaille[1],
                                   NoeudsMaille[2], NoeudsMaille[3], 
                                   intNumMaille);
      break;
    case 306:
    case 315:
      nbNoeuds = 8;
      if (ValElement == 315) {
        NoeudsMaille[3] = NoeudsMaille[6];
        NoeudsMaille[4] = NoeudsMaille[7];
        NoeudsMaille[5] = NoeudsMaille[8];
      }
      NoeudsMaille[7] = NoeudsMaille[5];
      NoeudsMaille[6] = NoeudsMaille[5];
      NoeudsMaille[5] = NoeudsMaille[4];
      NoeudsMaille[4] = NoeudsMaille[3];
      NoeudsMaille[3] = NoeudsMaille[2];
      myMesh->AddVolumeWithID(NoeudsMaille[0], NoeudsMaille[1],
                                   NoeudsMaille[2], NoeudsMaille[3], 
                                   NoeudsMaille[4], NoeudsMaille[5], 
                                   intNumMaille);
                                break;
    }
  }
  /****************************************************************************
   *                      FERMETURE DU FICHIER                      *
   ****************************************************************************/
  fclose(aFileId);
  return aResult;
}
