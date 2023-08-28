/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#include "Swap.h"

unsigned short Base::SwapOrder ()
{
  unsigned short usDummy = 1;
  return *((char*) &usDummy) == 1 ? LOW_ENDIAN : HIGH_ENDIAN;
}

void Base::SwapVar (char&)
{
}

void Base::SwapVar (unsigned char&)
{
}

void Base::SwapVar (short& s)
{
  short sTmp = s;
  int i = 0;

  for (i = 0; i < (int)sizeof (short); i++)
    *(((char*) &sTmp) + i) = *(((char*) &s) + sizeof (short) - i - 1);
  s = sTmp;
}

void Base::SwapVar (unsigned short& s)
{
  short sTmp = s;
  int i = 0;

  for (i = 0; i < (int)sizeof (short); i++)
    *(((char*) &sTmp) + i) = *(((char*) &s) + sizeof (short) - i - 1);
  s = sTmp;
}

void Base::SwapVar (long& l)
{
  long lTmp = l;
  int i = 0;

  for (i = 0; i < (int)sizeof (long); i++)
    *(((char*) &lTmp) + i) = *(((char*) &l) + sizeof (long) - i - 1);
  l = lTmp;
}

void Base::SwapVar (unsigned long& l)
{
  long lTmp = l;
  int i = 0;

  for (i = 0; i < (int)sizeof (long); i++)
    *(((char*) &lTmp) + i) = *(((char*) &l) + sizeof (long) - i - 1);
  l = lTmp;
}

void Base::SwapVar (float& f)
{
  float fTmp = f;
  int i = 0;

  for (i = 0; i < (int)sizeof (float); i++)
    *(((char*) &fTmp) + i) = *(((char*) &f) + sizeof (float) - i - 1);
  f = fTmp;
}

void Base::SwapVar (double& d)
{
  double dTmp = d;
  int i = 0;

  for (i = 0; i < (int)sizeof (double); i++)
    *(((char*) &dTmp) + i) = *(((char*) &d) + sizeof (double) - i - 1);
  d = dTmp;
}


