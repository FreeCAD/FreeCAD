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
//
// $Id: G4ExceptionSeverity.hh,v 1.3 2006/06/29 19:01:27 gunter Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// Class Description:
//
// Specifies the severity of G4Exception
//
//  FatalException
//   Error is severe or it happens at the initialization time.
//   Program should be aborted and core dump will be generated.
//
//  FatalErrorInArgument
//   Fatal error caused by most likely the mis-use of interfaces
//   by the user's code. Program should be aborted and core dump 
//   will be generated.
//
//  RunMustBeAborted
//   Error happens at initialization of a run (ex. at the 
//   moment of closing geometry), or some unpleasant situation
//   occurs during the event loop. Current run will be aborted
//   and the application returns to "Idle" state.
//
//  EventMustBeAborted
//   Error happens during tracking a particle. The event currently
//   being processed should be aborted, run will not be aborted.
//
//  JustWarning
//   Just display messages.
//

#ifndef G4ExceptionSeverity_H
#define G4ExceptionSeverity_H 1

enum G4ExceptionSeverity 
  { FatalException,
    FatalErrorInArgument,
    RunMustBeAborted,
    EventMustBeAborted,
    JustWarning };

#endif

