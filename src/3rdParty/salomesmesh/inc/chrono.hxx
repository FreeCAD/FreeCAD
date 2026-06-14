// Copyright (C) 2006-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

#ifndef _CHRONO_HXX_
#define _CHRONO_HXX_

#include "SMESH_SMDS.hxx"

#include <vector>
#include <string>
#include <iostream>
#include <ctime>

typedef struct acnt
{
  char*  _ctrNames;
  int    _ctrLines;
  int    _ctrOccur;
  double _ctrCumul;
} cntStruct;

class SMDS_EXPORT counters
{
public:
  static cntStruct *_ctrs;
  counters(int nb);
  ~counters();
  static void stats();
protected:
  static int _nbChrono;
};

class SMDS_EXPORT chrono
{
public:
  chrono(int i);
  ~chrono();
  void stop();
protected:
  bool _run;
  int _ctr;
  clock_t _start, _end;
};

#ifdef CHRONODEF
#define CHRONO(i) counters::_ctrs[i]._ctrNames = (char *)__FILE__; \
  counters::_ctrs[i]._ctrLines = __LINE__; \
  chrono aChrono##i(i);

#define CHRONOSTOP(i) aChrono##i.stop();

#else  // CHRONODEF

#define CHRONO(i)
#define CHRONOSTOP(i)

#endif // CHRONODEF

#endif // _CHRONO_HXX_
