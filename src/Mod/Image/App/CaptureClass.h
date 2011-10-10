/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
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


#ifndef CaptureClassH
#define CaptureClassH


#include <cv.h>
#include <highgui.h>
//---------------------------------------------------------------------------

class ImageAppExport Capturerer
{
public:
	/// Constructor with an Capture number, 0 will ask for one
	Capturerer(int num = 0);
	/// Constructor with an File name. 	Object will capture from that Video file
	Capturerer(const char* fileName);

    ~Capturerer();

    static int chooseCamNum(void);

	void setCaptureWindows(bool On);

	char getOneCapture(const char *text=0);

private:
  CvCapture* capture;
  IplImage *captureImage;
  bool _bIsWinOn;

  CvSize size;

    // font stuff
    CvFont font;
    double hScale;
    double vScale;
    int    lineWidth;
    bool   useLabel;
    char   buff[100];


};






#endif
