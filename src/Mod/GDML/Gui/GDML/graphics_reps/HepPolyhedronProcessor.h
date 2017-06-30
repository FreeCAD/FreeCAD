//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//

#ifndef HEP_POLYHEDRONPROCESSOR_HH
#define HEP_POLYHEDRONPROCESSOR_HH

///#include <HepPolyhedron.h>
#include "HepPolyhedron.h"

#include <vector>

class HepPolyhedronProcessor {
public:
  enum Operation { //Must be the same than BooleanProcessor OP_XXX.
     UNION  = 0
    ,INTERSECTION = 1
    ,SUBTRACTION = 2
  };
private:
  typedef std::pair<Operation,HepPolyhedron> op_t;
public:
  HepPolyhedronProcessor();
  virtual ~HepPolyhedronProcessor();
private:
  HepPolyhedronProcessor(const HepPolyhedronProcessor&);
  HepPolyhedronProcessor& operator=(const HepPolyhedronProcessor&);
public:
  void push_back(Operation,const HepPolyhedron&);
  bool execute(HepPolyhedron&);
  void clear();
  bool is_same_op() const;
//private:
  bool execute1(HepPolyhedron&,const std::vector<unsigned int>&);
private:
  std::vector<op_t> m_ops; 
};

#endif /* HEP_POLYHEDRONPROCESSOR_HH */
