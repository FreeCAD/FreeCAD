// FreeCAD Povray standard file
/***************************************************************************
 *   Copyright (c) 2005 Juergen Riegel         <juergen.riegel@web.de>     *
 *   Copyright (c) 2005 Georg Wiora            <georg.wiora@quarkbox.de>   *
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

// -w320 -h240
// -w8000 -h6000 +a0.3
// Use povray -iLehreW221animation.pov LehreW221animation.ini to trace.
// Use povray -w1280 -h720 +a0.3 -iLehreW221animation.pov LehreW221animation.ini to trace.

// Include Standard-Colors provided by povray
#include "colors.inc"
// Include Standard-Textures and Finishes provided by povray
#include "textures.inc"
#include "woods.inc"

// Include Camera definitions from FreeCad
#include "TempAnimationDef.inc"
// Include Animation functions
#include "FreeCadAnimation.inc"


// Width of texture lines in percent of period length
#declare lWidth = 0.007;
// Colors for grid
#declare GridCol1 = rgb <0.9,0,0>;
#declare GridCol2 = rgb <0.9,0.9,0>;
// Transparent color for Grid
#declare GridTran = rgbf <1,1,1,1.0>;

// Colormap for Coordinate grid
#declare scalemap = color_map {
        [0.000          color GridCol1] // Begin of red bar for main unit
        [lWidth        color GridCol1] // End of red bar
        [(0.001+lWidth) color GridTran] // Beginn of first gap
        [0.249          color GridTran] // End of first gap
        [0.250          color GridCol2] // Begin of yellow bar for sub unit
        [(0.250+lWidth) color GridCol2] // End of yellow bar
        [(0.251+lWidth) color GridTran] // Beginn of second gap
        [0.499          color GridTran] // End of first gap
        [0.500          color GridCol2] // Begin of yellow bar for sub unit
        [(0.500+lWidth)  color GridCol2] // End of yellow bar
        [(0.501+lWidth) color GridTran] // Beginn of second gap
        [0.749          color GridTran] // End of first gap
        [0.750          color GridCol2] // Begin of yellow bar for sub unit
        [(0.750+lWidth) color GridCol2] // End of yellow bar
        [(0.751+lWidth) color GridTran] // Beginn of second gap
        [1.0          color GridTran] // End of second gap
      };

// finish for objects
#declare MyFinish = finish 
{
  ambient 0.3
  diffuse 0.0 
  reflection 0.25 
  specular .6
  roughness 0.01
  brilliance 0.1
  metallic
} ;

// finish for grid lines
#declare LineFinish = finish
{ 
  ambient 0.0
  diffuse 1.0
};


// Textur für Koordinatenlinien
#declare scaletexture =   // Hintergrundfarbe
  // horizontale Linien
  texture {
    pigment {
      gradient y
      color_map {scalemap}
    }
    finish {LineFinish}
  };
  /* 
  // Höhenlinien
  texture {
    pigment {
      gradient z
      color_map {scalemap}
    }
    finish {LineFinish}
  }; 
  // Vertikale Linien
  texture {
    pigment {
      gradient x
      color_map {scalemap}
    }
    finish {LineFinish}
  };*/

// The final texture for the objects
#declare finaltexture =
  texture {
    pigment {rgb <0.7,0.9,0.7>}
    finish {MyFinish}
  }
  texture { scaletexture 
    finish 
    {
      ambient 0.4 
      reflection 0.25 
      specular .6
      roughness 0.01
      brilliance 0.1
      metallic
    }
    scale 400  
  };


// Sky sphere is a real sphere in this case with a diameter of 8 meters
sphere { 0*x 8000 inverse
  texture { pigment { rgb 0.3 }
    finish {ambient 0.5 diffuse 0.5}
  }
  texture
  {
    pigment {
    radial
      color_map {
        [0.00 color rgbt <1,1,1,0>]
        [0.08 color rgbt <1,1,1,0>]
        [0.09 color rgbt <1,1,1,1>]
        [1.00 color rgbt <1,1,1,1>]
      }  
      frequency 36
    }
    finish {LineFinish}
  }
  texture
  {
    pigment {
    gradient y
      color_map {
        [0.00 color rgbt <1,1,1,0>]
        [0.08 color rgbt <1,1,1,0>]
        [0.09 color rgbt <1,1,1,1>]
        [1.00 color rgbt <1,1,1,1>]
      }
      scale 500
    }
    finish {LineFinish}
  }
}


// Fussboden
plane {               // checkered floor
  y, -1
  texture
  {
    pigment {
      checker
      color rgb <255,246,193>/255 //<110,192,170>/255
      color rgb <220,220,220>/255
      scale 0.5
    }
    finish{
      diffuse 0.3
      ambient 0.7
    }
  }
  scale 1000
}


// Türe

#include "SeitenvandFC01.inc"
mesh { Seitenwand 
  texture { finaltexture }
} 

#include "Tuer19FC01.inc"
mesh { Tuer19
  texture { finaltexture }
}

#include "Tuer16morphFC01.inc"
mesh { Tuer16 
  texture { finaltexture }
}


//testteil

/*
cylinder {
  0*y,  405*y,  700
  open
  translate <2000,0,1200>
  texture { finaltexture }
}
  
  */

// Insert Camera
camera { MovieCamera }


// Lightsource
light_source {
<-1573.9813500000005,1310.07165000000003,-2000.1032>, color White
}
