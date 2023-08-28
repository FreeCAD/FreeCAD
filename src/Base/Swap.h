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


#ifndef BASE_SWAP_H
#define BASE_SWAP_H

#define LOW_ENDIAN	(unsigned short) 0x4949
#define HIGH_ENDIAN	(unsigned short) 0x4D4D


namespace Base {

/**
 * \brief Definition of functions that allow swapping of data types
 * dependent on the architecture.
 */

/** Returns machine type (low endian, high endian) */
unsigned short SwapOrder ();

void SwapVar (char&);
void SwapVar (unsigned char&);
void SwapVar (short&);
void SwapVar (unsigned short&);
void SwapVar (long&);
void SwapVar (unsigned long&);
void SwapVar (float&);
void SwapVar (double&);

template <class T>
void SwapEndian(T& v)
{
  T tmp = v;
  int i = 0;

  for (i = 0; i < (int)sizeof (T); i++)
    *(((char*) &tmp) + i) = *(((char*) &v) + sizeof (T) - i - 1);
  v = tmp;
}

} // namespace Base


#endif // BASE_SWAP_H
