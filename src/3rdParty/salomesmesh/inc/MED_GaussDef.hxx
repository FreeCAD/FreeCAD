// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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
// File   : MED_GaussDef.hxx
// Author : Edward AGAPOV (eap)
//
#ifndef MED_GaussDef_HeaderFile
#define MED_GaussDef_HeaderFile

#include "MED_WrapperBase.hxx"

//#include "MED_GaussUtils.hxx" <<<---- avoid dependence on boost
#include <vector>

namespace MED
{
  struct TShapeFun;
  typedef std::vector<double> TDoubleVector;
  /*!
   * \brief Description of family of integration points
   */
  struct TGaussDef
  {
    int           myType;      //!< element geometry (EGeometrieElement or med_geometrie_element)
    TDoubleVector myRefCoords; //!< description of reference points
    TDoubleVector myCoords;    //!< coordinates of Gauss points
    TDoubleVector myWeights;   //!< weights, len(weights)==<nb of gauss points>

    /*!
     * \brief Creates definition of gauss points family
     *  \param geomType - element geometry (EGeometrieElement or med_geometrie_element)
     *  \param nbPoints - nb gauss point
     *  \param variant - [1-3] to choose the variant of definition
     * 
     * Throws in case of invalid parameters
     * variant == 1 refers to "Fonctions de forme et points d'integration 
     *              des elements finis" v7.4 by J. PELLET, X. DESROCHES, 15/09/05
     * variant == 2 refers to the same doc v6.4 by J.P. LEFEBVRE, X. DESROCHES, 03/07/03
     * variant == 3 refers to the same doc v6.4, second variant for 2D elements
     */
    MEDWRAPPER_EXPORT TGaussDef(const int geomType, const int nbPoints, const int variant=1);

    MEDWRAPPER_EXPORT int dim() const { return myType/100; }
    MEDWRAPPER_EXPORT int nbPoints() const { return myWeights.capacity(); }

  private:
    void add(const double x, const double weight);
    void add(const double x, const double y, const double weight);
    void add(const double x, const double y, const double z, const double weight);
    void setRefCoords(const TShapeFun& aShapeFun);
  };
}

#endif
