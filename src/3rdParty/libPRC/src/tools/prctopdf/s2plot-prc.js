/* s2plot-prc.js
 *
 * Copyright 2006-2012 David G. Barnes, Paul Bourke, Christopher Fluke
 *
 * This file is part of S2PLOT.
 *
 * S2PLOT is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * S2PLOT is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with S2PLOT.  If not, see <http://www.gnu.org/licenses/>. 
 *
 * We would appreciate it if research outcomes using S2PLOT would
 * provide the following acknowledgement:
 *
 * "Three-dimensional visualisation was conducted with the S2PLOT
 * progamming library"
 *
 * and a reference to
 *
 * D.G.Barnes, C.J.Fluke, P.D.Bourke & O.T.Parry, 2006, Publications
 * of the Astronomical Society of Australia, 23(2), 82-93.
 *
 * $Id: s2plot-prc.js 5827 2012-11-01 21:51:03Z dbarnes $
 *
 */

// arrays of specially-named sections of the model tree
var BBOARDobjsArray = new Array(); // stores the billboard meshes
var BBOARDobjsCount = 0;           // how many billboard meshes are there?
var VRSET1objsArray = new Array();
var VRSET1objsCount = 0;
var VRSET2objsArray = new Array();
var VRSET2objsCount = 0;
var VRSET3objsArray = new Array();
var VRSET3objsCount = 0;
var vrVisible = true;

var FRAMEobjsArray = new Array(); // stores the frame meshes
var FRAMEobjsCount = 0;           // how many frame meshes are there?
var FRAMEobjsOwningFrame = new Array(); // stores which frame the corresponding mesh belongs to
// how many individual frames are there?
var nframes = -1;

// temporary variables
var currMesh = null;
var currParentName = "";
var currPPname = "";
var currPPPname = "";


console.println("* * * S2PRC *** HI_THERE! * * *");
console.println("Number of materials in the scene: " + scene.materials.count);

// N.B. this function expects globals currParentName, currPPname and currPPPname to be set!
getMatchingName = function(searchstr) {
    var framestr = "";
    if ( currParentName.indexOf(searchstr) > -1 ) {
	framestr = currParentName;
    } else if ( currPPname.indexOf(searchstr) > -1 ) {
	framestr = currPPname;
    } else if ( currPPPname.indexOf(searchstr) > -1 ) {
	framestr = currPPPname;
    }
    return(framestr);
}


// compile lists of BBOARD*, VRSET* and FRAME* meshes
for (i=0; i<scene.meshes.count; i++) { 
    currMesh = scene.meshes.getByIndex(i); 
    
    currParentName = currMesh.parent.name.toString(); 
    currPPname = currMesh.parent.parent.name.toString();
    if (currMesh.parent.parent.hasOwnProperty("parent") &&
	currMesh.parent.parent.parent.hasOwnProperty("name")) {
	currPPPname = currMesh.parent.parent.parent.name.toString();
    } else {
	currPPPname = "";
    }
    framestr = "";
    if (( currParentName.indexOf("BBOARDS1.") > -1 ) ||
	( currPPPname.indexOf("BBOARDS1.") > -1) ||
	( currPPname.indexOf("BBOARDS1.") > -1)) {
	BBOARDobjsArray[BBOARDobjsCount] = currMesh;
	BBOARDobjsCount++;
    } else if (( currParentName.indexOf("VRSET1.") > -1) ||
	       ( currPPPname.indexOf("VRSET1.") > -1 ) ||
	       ( currPPname.indexOf("VRSET1.") > -1)) {
	VRSET1objsArray[VRSET1objsCount] = currMesh;
	VRSET1objsCount++;
    } else if (( currParentName.indexOf("VRSET2.") > -1) ||
	       ( currPPPname.indexOf("VRSET2.") > -1) ||
	       ( currPPname.indexOf("VRSET2.") > -1)) {
	VRSET2objsArray[VRSET2objsCount] = currMesh;
	VRSET2objsCount++;
    } else if (( currParentName.indexOf("VRSET3.") > -1) ||
	       ( currPPPname.indexOf("VRSET3.") > -1) ||
	       ( currPPname.indexOf("VRSET3.") > -1)) {
	VRSET3objsArray[VRSET3objsCount] = currMesh;
	VRSET3objsCount++;
    } 
    
    framestr = "";
    if ( currParentName.indexOf("FRAME") > -1 ) {
	framestr = currParentName;
    } else if ( currPPPname.indexOf("FRAME") > -1 ) {
	framestr = currPPPname;
    } else if ( currPPname.indexOf("FRAME") > -1 ) {
	framestr = currPPname;
    }
    if (framestr != "") {
	var tmpstr = framestr.match(/FRAME\d+./g)[0];
	var frame = parseInt(tmpstr.match(/\d+/g)[0]);
	
	if (frame+1 > nframes && frame < 100) {
	    nframes = frame+1;
	}
	
	FRAMEobjsArray[FRAMEobjsCount] = currMesh;
	FRAMEobjsOwningFrame[FRAMEobjsCount] = frame;
	FRAMEobjsCount++;
    }
    
}

var doVolume = (VRSET1objsArray.length > 1) && (VRSET2objsArray.length > 1) && (VRSET3objsArray.length > 1);
var doBillboards = (BBOARDobjsArray.length > 0);


// Get Camera
var camera = scene.cameras.getByIndex(0);

function doBillboard() {	
  // Loop through all billboards, orientating their view direction with the camera position.
  for (j = 0; j < BBOARDobjsArray.length; j++) {
    var bb_posn = BBOARDobjsArray[j].transform.translation; // bb position
    //console.println("bb_posn = " + bb_posn);

    var cp = camera.position;
    var ctp = camera.targetPosition;
    var cam_posn = cp.subtract(ctp);
    // fix for facing billboards the Right Way from Michail 20111007    
    // var cam_posn = camera.targetPosition.subtract(camera.position);

    // camera.up has extra components in it, we need to remove the offset related to the 
    // targetPosition, so that strafing doesn't modify the up vector used in the setView below....
    var cam_up = camera.up;
    cam_up = cam_up.subtract(ctp);
    //console.println("cam_up = " + cam_up);

    BBOARDobjsArray[j].transform.setView(bb_posn, cam_posn, cam_up);	// Set billboard view.
  }
}

// volume rendering handling code
function setVRSETvis(which, visibility, update) {
  if (which == 0) {
    for (j=0; j<VRSET1objsArray.length; j++) { 
      VRSET1objsArray[j].visible = visibility;
    } 
  } else if (which == 1) {
    for (j=0; j<VRSET2objsArray.length; j++) { 
      VRSET2objsArray[j].visible = visibility;
    }
  } else if (which == 2) {
    for (j=0; j<VRSET3objsArray.length; j++) { 
      VRSET3objsArray[j].visible = visibility;
    }
  }
  if (update) {
    scene.update();
    runtime.refresh();
  }
}

function allSetVis(which) {
  setVRSETvis(which, true && vrVisible, 0);
  setVRSETvis((which + 1) % 3, false && vrVisible, 0);
  setVRSETvis((which + 2) % 3, false && vrVisible, 1);
}

function pickVRset() {
  var camera = scene.cameras.getByIndex(0);
  var camdir = camera.position.subtract(camera.targetPosition);
  
  // default
  var whichframeset = 0;
  
  if ((Math.abs(camdir.y) >= Math.abs(camdir.x)) && 
      (Math.abs(camdir.y) >= Math.abs(camdir.z))) {
    whichframeset = 1;
  } else if ((Math.abs(camdir.z) >= Math.abs(camdir.x)) &&
	     (Math.abs(camdir.z) >= Math.abs(camdir.y))) {
    whichframeset = 2;
  }
  allSetVis(whichframeset);
}

function toggleVRvisible() {
    vrVisible = !vrVisible;
    scene.update();
    runtime.refresh();
}
var firstmouseevent = true;
mreh = new MouseEventHandler();
mreh.onMouseDown = true;
mreh.onMouseMove = true;
mreh.reportAllTargets = false;
mreh.onEvent = function(event) {
    if (event.leftButtonDown) {
	doBillboard();
	pickVRset();

    }
}
runtime.addEventHandler(mreh);

ceh = new RenderEventHandler();
ceh.onEvent = function(event) {
  doBillboard();
  pickVRset();
}
runtime.addEventHandler(ceh);

doBillboard();
pickVRset();


camspeed = 1.0;
flyCamera = function(amount) {
  // get original position
  var MyCamera = scene.cameras.getByIndex(0);
  originalPos = new Vector3();
  originalPos.set(MyCamera.position);
  // get vector to target
  moveDir = new Vector3();
  moveDir.set(MyCamera.targetPosition);
  moveDir.subtractInPlace(MyCamera.position);
  // modify the position
  MyCamera.position.set(originalPos.addScaled(moveDir, amount * camspeed));
}

debugOut = function() {
  // get and print camera properties
  var MyCamera = scene.cameras.getByIndex(0);

  vp = new Vector3();
  vp.set(MyCamera.position);

  vu = new Vector3();
  vu.set(MyCamera.up);
  vu.subtractInPlace(vp);
  vu.normalize();

  vd = new Vector3();
  vd.set(MyCamera.targetPosition);
  vd.subtractInPlace(MyCamera.position);
  vd.normalize();

  right = new Vector3();
  right.set(vd.cross(vu));
  right.normalize();

  pr = new Vector3();
  pr.set(MyCamera.targetPosition);
}

// rotate (ix,iy) or roll (iz) the camera.
rotateCamera = function(ix, iy, iz) {

  // get camera properties
  var MyCamera = scene.cameras.getByIndex(0);

  vp = new Vector3();
  vp.set(MyCamera.position);

  vu = new Vector3();
  vu.set(MyCamera.up);
  vu.subtractInPlace(vp);
  vu.normalize();

  vd = new Vector3();
  vd.set(MyCamera.targetPosition);
  vd.subtractInPlace(MyCamera.position);
  vd.normalize();

  right = new Vector3();
  right.set(vd.cross(vu));

  pr = new Vector3();
  pr.set(MyCamera.targetPosition);

  delta = camspeed * 3.14159265 / 180.00;
  
  // roll? just change the up vector
  if (Math.abs(iz) > 0.0001) {
    vu.x += iz * right.x * delta;
    vu.y += iz * right.y * delta;
    vu.z += iz * right.z * delta;
    vu.normalize();
    MyCamera.up.set(vu.add(vp));
    return;
  }

  // rotate about focus point, provided camera is not coincident with 
  // this point
  dx = vp.x - pr.x;
  dy = vp.y - pr.y;
  dz = vp.z - pr.z;
  radius = Math.sqrt(dx*dx + dy*dy + dz*dz);
  if (Math.abs(radius) > 0.0001) {
    
    /* Determine the new view point */
    delta *= radius;
    newvp = new Vector3();
    newvp.x = vp.x + delta * ix * right.x + delta * iy * vu.x - pr.x; 
    newvp.y = vp.y + delta * ix * right.y + delta * iy * vu.y - pr.y;
    newvp.z = vp.z + delta * ix * right.z + delta * iy * vu.z - pr.z;
    newvp.normalize();
    vp.x = pr.x + radius * newvp.x;
    vp.y = pr.y + radius * newvp.y;
    vp.z = pr.z + radius * newvp.z;
    
    /* Determine the new right vector */
    newr = new Vector3();
    newr.x = vp.x + right.x - pr.x;
    newr.y = vp.y + right.y - pr.y;
    newr.z = vp.z + right.z - pr.z;
    newr.normalize();
    newr.x = pr.x + radius * newr.x - vp.x;
    newr.y = pr.y + radius * newr.y - vp.y;
    newr.z = pr.z + radius * newr.z - vp.z;
    newr.normalize();
    
    vd.x = pr.x - vp.x;
    vd.y = pr.y - vp.y;
    vd.z = pr.z - vp.z;
    vd.normalize();
    
    /* Determine the new up vector */
    vu = newr.cross(vd);
    vu.normalize();
    
  }

  MyCamera.position.set(vp);
  MyCamera.up.set(vu.add(vp));
}


autospin = 0;
spinspeed = 1;
frcount = 0;
autoadvance = 1;
teh = new TimeEventHandler();
teh.onEvent = function(event) {
    frcount++;
  if (autospin) {
    tmp = camspeed;
    camspeed = spinspeed;
    rotateCamera(1., 0., 0.);
    camspeed = tmp;
  }
  if (autoadvance) {
      if (!(frcount % 20)) {
	  //nextFrame();
      }
  }
}
runtime.addEventHandler(teh);

gotoPresetView = function(which) {
  console.println("gotoPresetView " + which);
  runtime.setView(which, true);
}

toggleVisibilityOfSerialisedNode = function(branch) {
  for (i=0; i<scene.meshes.count; i++) { 
    currMesh = scene.meshes.getByIndex(i); 
    currName = currMesh.name.toString();
      if( currName.indexOf(branch) > -1) {
      currMesh.visible = !currMesh.visible;
    }
  }
  scene.update();
  runtime.refresh();
}


toggleVisibilityOfNamedBranch = function(branch) {

  strbranch = branch + ".";

  for (i=0; i<scene.meshes.count; i++) { 
    currMesh = scene.meshes.getByIndex(i); 

    currParentName = currMesh.parent.name.toString(); 
    currPPname = currMesh.parent.parent.name.toString();
    if (currMesh.parent.parent.hasOwnProperty("parent") &&
	currMesh.parent.parent.parent.hasOwnProperty("name")) {
      currPPPname = currMesh.parent.parent.parent.name.toString();
    } else {
      currPPPname = "";
    }
    //  may need this addition at some point? if(( currName.indexOf(strbranch) > -1) ||
    if (( currParentName.indexOf(strbranch) > -1 ) ||
	( currPPPname.indexOf(strbranch) > -1) ||
	( currPPname.indexOf(strbranch) > -1)) {
      currMesh.visible = !currMesh.visible;
      
    }
  }

  scene.update();
  runtime.refresh();
}


nextFrame = function() {
  if (nframes < 0) {
    return;
  }
  whichframe = (whichframe + 1) % nframes;

/*
  // THIS NEEDS TO BE CHANGED TO USE THE LIST OF FRAME* MESHES
  // AS IT WILL THEN BE A BIT FASTER  
  for (i=0; i<scene.meshes.count; i++) { 
    currMesh = scene.meshes.getByIndex(i); 
    currParentName = currMesh.parent.name.toString(); 
    currPPname = currMesh.parent.parent.name.toString();
    if (currMesh.parent.parent.hasOwnProperty("parent") &&
	currMesh.parent.parent.parent.hasOwnProperty("name")) {
      currPPPname = currMesh.parent.parent.parent.name.toString();
    } else {
      currPPPname = "";
    }
    framestr = "";
    if ( currParentName.indexOf("FRAME") > -1 ) {
      framestr = currParentName;
    } else if ( currPPPname.indexOf("FRAME") > -1 ) {
      framestr = currPPPname;
    } else if ( currPPname.indexOf("FRAME") > -1 ) {
      framestr = currPPname;
    }
    if (framestr != "") {
      var tmpstr = framestr.match(/FRAME\d+./g)[0];
      var frame = parseInt(tmpstr.match(/\d+/g)[0]);
      
      if (frame == whichframe) {
	currMesh.visible = true;
      } else {
	currMesh.visible = false;
      }
    }
  }
*/

    for (i = 0; i < FRAMEobjsCount; i++) {
	if (FRAMEobjsOwningFrame[i] == whichframe) {
	    FRAMEobjsArray[i].visble = true;
	} else {
	    FRAMEobjsArray[i].visible = false;
	}
    }

  runtime.refresh();
}



myKeyHandler = new KeyEventHandler();
myKeyHandler.onEvent = function( event ) {
    
  switch(event.characterCode) {

  case 45: // -,_ : move backwards
  case 95:
    flyCamera(-0.05);
    break;

  case 61: // =,+ : move forwards
  case 43:
    flyCamera(0.05);
    break;

  case 65: // A,a : toggle autospin
  case 97: 
    autospin = (autospin + 1) % 2;
    break;

  case 68: // D,d : print debug info
  case 100:
    debugOut();
    break;

  case 62: // > : increase camera delta / step
    camspeed = camspeed * 2.5;
    break;
  case 60: // < : decrease camera delta / step
    camspeed = camspeed * 0.4;
    break;

  case 42: // * : increase spin speed
    spinspeed = spinspeed * 2.5;
    break;
  case 47: // / : decrease spin speed
    spinspeed = spinspeed * 0.4;
    break;

  case 28: // left-arrow : rotate *camera* left
    rotateCamera(-1., 0., 0.);
    break;
  case 29: // right-arrow : rotate *camera* right
    rotateCamera(1., 0., 0.);
    break;
  case 30: // up-arrow : rotate up and over
    rotateCamera(0., 1., 0.);
    break;
  case 31: // down-arrow : rotate down and under
    rotateCamera(0., -1., 0.);
    break;
    
  case 91: // [ : roll ccw
    rotateCamera(0., 0., -1.);
    break;
  case 93: // ] : roll cw
    rotateCamera(0., 0., 1.);
    break;
    
  case 32: // space : next frame
      nextFrame();
    break;

  case 49: //  '1' : home view
  case 72:  // 'H' : home view
  case 104: // 'h' : home view
    gotoPresetView(0);
    break;

  case 50:
  case 51:
  case 52:
  case 53:
  case 54:
  case 55:
    gotoPresetView(event.characterCode - 49);
    break;

  default:
      console.println("Unknown char code: " + event.characterCode);
      break;

  }
}
runtime.addEventHandler( myKeyHandler );

if (nframes > 0) {
  whichframe = -1;
  nextFrame();
}


gotoPresetView(0);

