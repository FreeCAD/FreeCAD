// Persistence of Vision Ray Tracer Scene Description File
// File: ?.pov
// Vers: 3.6
// Desc: Basic Scene Example
// Date: mm/dd/yy
// Auth: ?
//

#version 3.6;

#include "colors.inc"

global_settings {
  assumed_gamma 1.0
}  


// ----------------------------------------

// includes the Part mesh writen from FreeCAD
#include "TempPart.inc"
object {Part
   texture { pigment {rgb <0.3,0.8,0.3>} finish {ambient 0.2 reflection 0.2 specular 0.7} }
 }



// ----------------------------------------

// includes the camera from FreeCAD
#include "TempCamera.inc"
camera {
  location  CamPos[0]
  look_at   LookAt[0]
  sky       Up[0]
  angle     50
}


light_source {
  CamPos[0] - CamDir[0] * 5 + Up[0] * 10           // light's position
  color rgb <1, 1, 1>  // light's color
} 

// ----------------------------------------


sky_sphere {
  pigment {
    gradient y
    color_map {
      [0.0 rgb <0.6,0.7,1.0>]
      [0.7 rgb <0.0,0.1,0.8>]
    }
  }
}


// ----------------------------------------

plane {
  y, -1
  pigment { color rgb <0.7,0.5,0.3> }
}

sphere {
  0.0, 1
  texture {
    pigment {
      radial
      frequency 8
      color_map {
        [0.00 color rgb <1.0,0.4,0.2> ]
        [0.33 color rgb <0.2,0.4,1.0> ]
        [0.66 color rgb <0.4,1.0,0.2> ]
        [1.00 color rgb <1.0,0.4,0.2> ]
      }
    }
    finish{
      specular 0.6
    }
  }
}

