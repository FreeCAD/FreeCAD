// Wild Magic Source Code
// David Eberly
// http://www.geometrictools.com
// Copyright (c) 1998-2007
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version.  The license is available for reading at
// either of the locations:
//     http://www.gnu.org/copyleft/lgpl.html
//     http://www.geometrictools.com/License/WildMagicLicense.pdf
// The license applies to versions 0 through 4 of Wild Magic.
//
// Version: 4.0.0 (2006/06/28)

namespace Wm4
{
//----------------------------------------------------------------------------
/// @cond DOXERR
template <class T>
void Allocate (int iCols, int iRows, T**& raatArray)
{
    raatArray = WM4_NEW T*[iRows];
    raatArray[0] = WM4_NEW T[iRows*iCols];
    for (int iRow = 1; iRow < iRows; iRow++)
    {
        raatArray[iRow] = &raatArray[0][iCols*iRow];
    }
}
//----------------------------------------------------------------------------
template <class T>
void Deallocate (T**& raatArray)
{
    if (raatArray)
    {
        WM4_DELETE[] raatArray[0];
        WM4_DELETE[] raatArray;
        raatArray = nullptr;
    }
}
//----------------------------------------------------------------------------
template <class T>
void Allocate (int iCols, int iRows, int iSlices, T***& raaatArray)
{
    raaatArray = WM4_NEW T**[iSlices];
    raaatArray[0] = WM4_NEW T*[iSlices*iRows];
    raaatArray[0][0] = WM4_NEW T[iSlices*iRows*iCols];
    for (int iSlice = 0; iSlice < iSlices; iSlice++)
    {
        raaatArray[iSlice] = &raaatArray[0][iRows*iSlice];
        for (int iRow = 0; iRow < iRows; iRow++)
        {
            raaatArray[iSlice][iRow] =
                &raaatArray[0][0][iCols*(iRow+iRows*iSlice)];
        }
    }
}
//----------------------------------------------------------------------------
template <class T>
void Deallocate (T***& raaatArray)
{
    if (raaatArray)
    {
        WM4_DELETE[] raaatArray[0][0];
        WM4_DELETE[] raaatArray[0];
        WM4_DELETE[] raaatArray;
        raaatArray = 0;
    }
}
/// @endcond
//----------------------------------------------------------------------------
} //namespace Wm4
