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
// $Id: G4String.hh,v 1.15 2009/08/10 10:18:09 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
//---------------------------------------------------------------
//  GEANT 4 class header file
//
//  G4String
//
//  Class description:
//
//  Definition of a Geant4 string.
//  Derived from the Rogue Wave implementation of RWCString;
//  it uses intrinsically STL string.

//---------------------------------------------------------------

#ifndef __G4String
#define __G4String

#include <stdio.h>
#include <string>
#include <cstring>

#include "G4Types.hh"
#include <iostream>

#ifdef WIN32
  #define strcasecmp _stricmp
#endif

typedef std::string::size_type str_size;

class G4String;

class G4SubString
{
public:

  inline G4SubString(const G4SubString&);

  inline G4SubString& operator=(const char*);         

  inline G4SubString& operator=(const G4String&);
  inline G4SubString& operator=(const G4SubString&);
 
  inline char& operator()(str_size);
  inline char  operator()(str_size) const;
  inline char& operator[](str_size);
  inline char  operator[](str_size) const;

  inline G4int operator!() const;

  inline G4bool operator==(const G4String&) const;
  inline G4bool operator==(const char*) const;
  inline G4bool operator!=(const G4String&) const;
  inline G4bool operator!=(const char*) const;

  inline str_size length() const;
  inline str_size start() const;

  // For detecting null substrings
  //
  inline G4bool isNull() const;

private:

  inline G4SubString(G4String&, str_size, str_size);

  G4String*    mystring;     
  str_size     mystart;
  str_size     extent;

  friend class G4String;

};
 

class G4String : public std::string
{

  typedef std::string std_string;

public: 

  enum caseCompare { exact, ignoreCase };
  enum stripType { leading, trailing, both };

  inline G4String ();
  inline G4String ( char );
  inline G4String ( const char * );
  inline G4String ( const char *, str_size );
  inline G4String ( const G4String& );
  inline G4String ( const G4SubString& );
  inline G4String ( const std::string & );
  ~G4String () {}

  inline G4String& operator=(const G4String&);
  inline G4String& operator=(const std::string &);
  inline G4String& operator=(const char*);

  inline char operator () (str_size) const; 
  inline char& operator () (str_size);

  inline G4String& operator+=(const G4SubString&);
  inline G4String& operator+=(const char*);
  inline G4String& operator+=(const std::string &);
  inline G4String& operator+=(const char&);
  inline G4bool operator==(const G4String&) const;
  inline G4bool operator==(const char*) const;
  inline G4bool operator!=(const G4String&) const;
  inline G4bool operator!=(const char*) const;

  inline operator const char*() const;
  inline G4SubString operator()(str_size, str_size);

  inline G4int compareTo(const char*, caseCompare mode=exact);
  inline G4int compareTo(const G4String&, caseCompare mode=exact);

  inline G4String& prepend (const char*);
  inline G4String& append (const G4String&);

  inline std::istream& readLine (std::istream&, G4bool skipWhite=true);
  
  inline G4String& replace (unsigned int, unsigned int, 
                             const char*, unsigned int );
  inline G4String& replace(str_size, str_size, const char*);

  inline G4String& remove(str_size);
  inline G4String& remove(str_size, str_size);

  inline G4int first(char) const;
  inline G4int last(char) const;

  inline G4bool contains(const std::string&) const;
  inline G4bool contains(char) const;

  // stripType = 0 beginning
  // stripType = 1 end
  // stripType = 2 both
  //
  inline G4String strip (G4int strip_Type=trailing, char c=' ');

  inline void toLower ();
  inline void toUpper ();

  inline G4bool isNull() const;

  inline str_size index (const char*, G4int pos=0) const; 
  inline str_size index (char, G4int pos=0) const; 
  inline str_size index (const G4String&, str_size, str_size, caseCompare) const;

  inline const char* data() const;

  inline G4int strcasecompare(const char*, const char*) const;

  inline unsigned int hash( caseCompare cmp = exact ) const;
  inline unsigned int stlhash() const;
};

#include "G4String.icc"

#endif
