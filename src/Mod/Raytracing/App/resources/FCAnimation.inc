/***************************************************************************
 *   Copyright (c) 2005 Georg Wiora         <georg.wiora@quarkbox.de>      *
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

// Persistence of Vision Ray Tracer Scene Description File
// File: FreeCadAnimation.inc
// Vers: 3.6
// Desc: Provides functionality to create povray animations with freecad
//       Povray clock variable counts 0 to 1
//       Kamera positions must be defined before including this file
// Date: 03-Mar-2005 
// Auth: Dr. Georg Wiora
// FreeCAD Povray standard file


// Made for Povray version 3.6
#version 3.6;

// Check for animation active
#if (clock_on)

  // Time per scene in seconds
  #declare TimePerScene = final_clock/(nCamPos-1);
  #debug concat("Time per Scene: ",str(TimePerScene,5,3),"\n")
  
  #macro debugprint()
    #debug concat("Szene ",str(sz,3,0),"\n")
    #debug concat("T=",str(T,5,2),"  T0=",str(T0,5,2),"  T1=",str(T1,5,2),"  T2=",str(T2,5,2),"  T1Sin=",str(T1Sin,5,2),"\n")
  #end
  
  // Current time is allways zero based
  #declare T = clock;

  // Compute Scene number: One scene less than camera positions
  #declare sz = floor(T / TimePerScene) ; 
  // check for end of scene definition
  #if ((clock / TimePerScene) >= (nCamPos-1))
    #warning "Clock value is beyond last camera position. I will try to extrapolate the last motion vector."
    // limit scene counter to maximum
    #declare sz = nCamPos-1;
    #declare sz1 = nCamPos-2;
    #declare sz2 = nCamPos-1;
  #else
    // Scene index for vector computations
    #declare sz1 = sz;
    #declare sz2 = sz+1;
  #end
  
  // Time Variables
  #declare T0 = T-TimePerScene*sz ;  // Relative scene time
  #declare T1 = T0/TimePerScene ; // Normalised scene time
  #declare T2 = 1-T1 ;                 // Reversed normalised scene time
  #declare T1Sin = pow(sin(radians(T1*90)),2);  // Sin-square time for smooth movements
  debugprint()
  
  // Interpolate camera position between scenes
  #declare CamPosT  = CamPos[sz] + (CamPos[sz2]-CamPos[sz1])*T1 ;
  #declare CamDirT  = CamDir[sz] + (CamDir[sz2]-CamDir[sz1])*T1 ; 
  #declare LookAtT  = LookAt[sz] + (LookAt[sz2]-LookAt[sz1])*T1 ; 
  #declare UpT      = Up[sz]     + (Up[sz2]    -Up[sz1]   )*T1 ; 
  #declare CamZoomT = CamZoom[sz] + (CamZoom[sz2]-CamZoom[sz1])*T1Sin ;
  
  
  // Cameradefinition
  #declare MovieCamera = camera
  {
    perspective
    up UpT
    right x*image_width/image_height
    angle CamZoomT
    location  CamPosT
    look_at   LookAtT
    up        UpT
  }
#else
 // No animation. Define default camera at position 0
  #declare MovieCamera = camera
  {
    perspective
    up Up[0]
    right x*image_width/image_height
    angle CamZoom[0]
    location  CamPos[0]
    look_at   LookAt[0]
    up        Up[0]
  }
 
#end