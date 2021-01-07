#***************************************************************************	
#*                                                                         *	
#*   Copyright (c) 2018 - 2020                                             *	
#*   Paul Lee <paullee0@gmail.com>                                         *	
#*                                                                         *	
#*   This program is free software; you can redistribute it and/or modify  *	
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *	
#*   as published by the Free Software Foundation; either version 2 of     *	
#*   the License, or (at your option) any later version.                   *	
#*   for detail see the LICENCE text file.                                 *	
#*                                                                         *	
#*   This program is distributed in the hope that it will be useful,       *	
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *	
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *	
#*   GNU Library General Public License for more details.                  *	
#*                                                                         *	
#*   You should have received a copy of the GNU Library General Public     *	
#*   License along with this program; if not, write to the Free Software   *	
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *	
#*   USA                                                                   *	
#*                                                                         *	
#***************************************************************************	
										
import FreeCAD, FreeCADGui, Sketcher, Part, Draft, DraftVecUtils		
import ArchComponent, ArchBuilding, ArchFloor, ArchAxis, ArchWall, ArchWindow	
import ArchSpace, ArchStairs, ArchSectionPlane					
										
import SketchArchIcon								
import SketchArchCommands							
										
import math, time								
from PySide import QtGui, QtCore						
from FreeCAD import Vector							
App = FreeCAD									
Gui = FreeCADGui								
pi = math.pi									
zeroMM = FreeCAD.Units.Quantity('0mm')						
MM = FreeCAD.Units.Quantity('1mm')						
tolerance = 0.000000001							
										
										
										
#--------------------------------------------------------------------------#	
#                           Class Definition                               #	
#--------------------------------------------------------------------------#	
										
										
class ArchSketchObject:							
  def __init__(self, obj):							
      #self.Type = "ArchSketchObject"						
      pass									
										
										
class ArchSketch(ArchSketchObject):						
										
  ''' ArchSketch - Sketcher::SketchObjectPython for Architectual Layout '''	
										
  MasterSketchSubelementTags = [ "MasterSketchSubelementTag", "MasterSketchIntersectingSubelementTag" ]										
										
  def __init__(self, obj):							
      ArchSketchObject.__init__(self, obj)					
										
      ''' call self.setProperties '''						
										
      self.setProperties(obj)							
      self.setPropertiesLinkCommon(obj)					
      self.initEditorMode(obj)							
      obj.ViewObject.Proxy=0							
      return None								
										
										
  def initEditorMode(self, obj):						
										
      ''' Set DispayMode for Data Properties in Combo View Editor '''		
										
      obj.setEditorMode("MapMode",1)						
      obj.setEditorMode("MapReversed",1)					
      obj.setEditorMode("Constraints",2)					
      obj.setEditorMode("Placement",1)						
										
										
  def setProperties(self, fp):							
										
      ''' Add self.properties '''						
										
      fp.Proxy = self								
      if not hasattr(self,"Type"):						
          self.Type = "ArchSketch"						
										
      if not hasattr(self,"clEdgeSameIndexFlat"):				
          self.clEdgeSameIndexFlat = []					
																	
																	
      ''' Added ArchSketch Properties '''												
																	
																	
																	
										
  def setPropertiesLinkCommon(self, orgFp, linkFp=None, mode=None):		
  # mode='init', 'ODR' for different settings					
										
      if linkFp:								
          fp = linkFp								
      else:									
          fp = orgFp								
										
      prop = fp.PropertiesList							
										
      for i in ArchSketch.MasterSketchSubelementTags:				
          if linkFp:  # no Proxy						
              if i not in prop:						
                  linkFp.addProperty("App::PropertyPythonObject", i)		
                  setattr(linkFp, i, str())					
          else:  # either ArchSketch or ArchObjects, should have Proxy		
              if orgFp.Proxy.Type == "ArchSketch":				
                  if not hasattr(fp.Proxy, i): 				
                      setattr(orgFp.Proxy, i, str())				
              else:  # i.e. other ArchObjects					
                  if i not in prop:						
                      orgFp.addProperty("App::PropertyPythonObject", i)	
                      setattr(orgFp, i, str())					
										
      ''' Referenced Object '''						
										
      if Draft.getType(fp.getLinkedObject()) not in ['Window', 'ArchSketch']:  # Not to be added for ArchSketch, nor for Arch Windows also which already has "Hosts"					
          if "Host" not in prop:																					
              fp.addProperty("App::PropertyLink","Host","Referenced Object","The object that host this object / this object attach to")								
																									
      if "MasterSketch" not in prop:																					
          fp.addProperty("App::PropertyLink","MasterSketch","Referenced Object","Master Sketch to Attach on")												
      if "MasterSketchSubelement" not in prop:																				
          fp.addProperty("App::PropertyString","MasterSketchSubelement","Referenced Object","Master Sketch Sub-Element to Attach on")									
      if "MasterSketchSubelementOffset" not in prop:																			
          fp.addProperty("App::PropertyDistance","MasterSketchSubelementOffset","Referenced Object","Master Sketch Sub-Element Attached Offset from Startpoint")					
																									
      if "MasterSketchIntersectingSubelement" not in prop:																		
          fp.addProperty("App::PropertyString","MasterSketchIntersectingSubelement",															
                         "Referenced Object","Master Sketch Subelement Intersecting the Sub-Element Attached on")											
      if "AttachToSubelementOrOffset" not in prop:																			
          fp.addProperty("App::PropertyEnumeration","AttachToSubelementOrOffset","Referenced Object","Select MasterSketch Subelement or Specify Offset to Attach")					
          fp.AttachToSubelementOrOffset = [ "Attach To Edge & Alignment", "Attach to Edge", "Follow Only Offset XYZ & Rotation" ]									
      if "AttachmentOffsetXyzAndRotation" not in prop:																			
          fp.addProperty("App::PropertyPlacement","AttachmentOffsetXyzAndRotation","Referenced Object","Specify XYZ and Rotation Offset")								
      if "AttachmentOffsetExtraRotation" not in prop:																			
          fp.addProperty("App::PropertyEnumeration","AttachmentOffsetExtraRotation","Referenced Object","Extra Rotation about X, Y or Z Axis")								
          fp.AttachmentOffsetExtraRotation = [ "None", "X-Axis CW90", "X-Axis CCW90", "X-Axis CW180", "Y-Axis CW90", "Y-Axis CCW90", "Y-Axis CW180","Z-Axis CW90", "Z-Axis CCW90", "Z-Axis CW180"]	
      if "OriginOffsetXyzAndRotation" not in prop:																			
          fp.addProperty("App::PropertyPlacement","OriginOffsetXyzAndRotation","Referenced Object","Specify Origin's XYZ and Rotation Offset")								
      if "FlipOffsetOriginToOtherEnd" not in prop:																			
          fp.addProperty("App::PropertyBool","FlipOffsetOriginToOtherEnd","Referenced Object","Flip Offset Origin to Other End of Edge / Wall ")							
      if "Flip180Degree" not in prop:																					
          fp.addProperty("App::PropertyBool","Flip180Degree","Referenced Object","Flip Orientation 180 Degree / Inside-Outside / Front-Back")								
      if "AttachmentAlignment" not in prop:																				
          fp.addProperty("App::PropertyEnumeration","AttachmentAlignment","Referenced Object","If AttachToEdge&Alignment, Set EdgeGroupWidthLeft/Right to alignt to EdgeGroupWidth ")			
          fp.AttachmentAlignment = [ "Edge", "EdgeGroupWidthLeft", "EdgeGroupWidthRight" ]														
          if Draft.getType(fp.getLinkedObject()) == 'Window':  																	
              fp.AttachmentAlignment = "EdgeGroupWidthRight"  # default for Windows which have normal 0,1,0 so somehow set to ArchWindows								
          else:  																							
              fp.AttachmentAlignment = "EdgeGroupWidthLeft"  # default for cases other than Windows 													
      if "AttachmentAlignmentOffset" not in prop:																			
          fp.addProperty("App::PropertyDistance","AttachmentAlignmentOffset","Referenced Object","Set Offset from Edge / EdgeGroupWidth +ve Right / -ve Left")						
																									
      attachToAxisOrSketchExisting = None																				
      fpLinkedObject = fp.getLinkedObject()																				
      if "AttachToAxisOrSketch" in prop:																				
          attachToAxisOrSketchExisting = fp.AttachToAxisOrSketch																	
      else:  # elif "AttachToAxisOrSketch" not in prop:																		
          fp.addProperty("App::PropertyEnumeration","AttachToAxisOrSketch","Referenced Object","Select Object Type to Attach on ")									
      if fpLinkedObject.Proxy.Type == "ArchSketch":																			
          fp.AttachToAxisOrSketch = [ "Host", "Master Sketch", "Placement Axis" ]															
      else:  # i.e. other ArchObjects																					
          fp.AttachToAxisOrSketch = [ "None", "Host", "Master Sketch"]																	
																									
      # has existing selection																						
      if attachToAxisOrSketchExisting is not None:																			
          if attachToAxisOrSketchExisting == "Hosts":																			
              attachToAxisOrSketchExisting = "Host"  # Can attach to only 1 host															
          fp.AttachToAxisOrSketch = attachToAxisOrSketchExisting																	
																									
      # no existing selection, ie. newly added "AttachToAxisOrSketch" attribute															
      elif fpLinkedObject.Proxy.Type == "ArchSketch":																			
          fp.AttachToAxisOrSketch = "Master Sketch"  # default option for ArchSketch + Link to ArchSketch												
																									
      else:  # other Arch Objects  # elif fpLinkedObject.Proxy.Type != "ArchSketch":															
          # currently only if fp is Window and mode is 'ODR', not to attach to Host or otherwise it would relocate to 1st edge										
          if mode == 'ODR':																						
              if Draft.getType(fp) == 'Window':																			
                  fp.AttachToAxisOrSketch = "None"																			
              else: 																							
                  pass  # currenlty no other ArchObjects use 'ODR'																	
          else:  # default 'ODR' (or None), i.e. if 																			
              fp.AttachToAxisOrSketch = "Host"  # default option for Arch Objects in general														
																									
																									
  def appLinkExecute(self, fp, linkFp, index, linkElement):			
      self.setPropertiesLinkCommon(fp, linkFp)					
      updateAttachmentOffset(fp, linkFp)					
										
										
  def execute(self, fp):							
										
      ''' Features to Run in Addition to Sketcher.execute() '''		
										
      ''' (VII) - Update attachment angle and attachment point coordinate '''	
										
      updateAttachmentOffset(fp)						
										
										
      ''' (IX or XI) - Update the order of edges by getSortedClusters '''	
										
      self.updateSortedClustersEdgesOrder(fp)					
										
										
      ''' (X) - Instances fp.resolve / fp.recompute - '''			
										
      fp.solve()								
      fp.recompute()								
										
										
  def updateSortedClustersEdgesOrder(self, fp):				
										
      clEdgePartnerIndex, clEdgeSameIndex, clEdgeEqualIndex, clEdgePartnerIndexFlat, clEdgeSameIndexFlat, clEdgeEqualIndexFlat = getSortedClustersEdgesOrder(fp)				
										
      self.clEdgePartnerIndex = clEdgePartnerIndex				
      self.clEdgeSameIndex = clEdgeSameIndex					
      self.clEdgeEqualIndex = clEdgeEqualIndex					
										
      self.clEdgePartnerIndexFlat = clEdgePartnerIndexFlat			
      self.clEdgeSameIndexFlat = clEdgeSameIndexFlat				
      self.clEdgeEqualIndexFlat = clEdgeEqualIndexFlat				
										
										
  def onChanged(self, fp, prop):						
      if prop in ["MasterSketch", "PlacementAxis", "AttachToAxisOrSketch"]:	
          changeAttachMode(fp, prop)						
										
										
  def getWidths(self, fp):							
  										
      ''' wrapper function for uniform format '''				
  										
      return self.getSortedClustersEdgesWidth(fp)				
										
										
  def getSortedClustersEdgesWidth(self, fp):					
										
      '''  This method check the SortedClusters-isSame-(flat)List (omitted	
           construction geometry), find the corresponding edgesWidth and make	
           a list of (WidthX, WidthX+1 ...) '''				
										
      '''  Options of data to store width (& other) information conceived	
										
           1st Option - Use self.widths: a Dict of { EdgeX : WidthX, ...}	
                        But when Sketch is edited with some edges deleted, the	
                        edges indexes change, the widths stored become wrong	
										
           2nd Option - Use abdullah's edge geometry				
                        .... bugs found, not working yet			
										
           3rd Option - Use self.EdgeTagDictSync				
                        .... convoluted object sync, restoreOnLoad '''		
      return None								
										
										
  def getAligns(self, fp):							
  										
      ''' wrapper function for uniform format '''				
  										
      return self.getSortedClustersEdgesAlign(fp)				
										
										
  def getSortedClustersEdgesAlign(self, fp):					
      '''  									
           This method check the SortedClusters-isSame-(flat)List		
           find the corresponding edgesAlign ... 				
           and make a list of (AlignX, AlignX+1 ...)				
      '''									
      return None								
										
										
  def onDocumentRestored(self, fp):						
										
      self.setProperties(fp)							
      self.setPropertiesLinkCommon(fp)						
      self.initEditorMode(fp)							
#---------------------------------------------------------------------------#	
#             FreeCAD Commands Classes & Associated Functions               #	
#---------------------------------------------------------------------------#	
										
class _CommandEditWallAlign():							
										
    '''Edit Wall Segment (Underlying [Arch]Sketch) Align Command Definition'''	
										
    def GetResources(self):							
        return {'Pixmap'  : SketchArchIcon.getIconPath()+'/icons/Edit_Align',	
                'Accel'   : "E, A",						
                'MenuText': "Edit Wall Segment Align",				
                'ToolTip' : "Select Wall/ArchSketch to Flip Segment Align ",	
                'CmdType' : "ForEdit"}						
										
    def IsActive(self):							
        return not FreeCAD.ActiveDocument is None				
										
    def Activated(self):							
        try:									
            sel0 = Gui.Selection.getSelection()[0]				
        except:								
            reply = QtGui.QMessageBox.information(None,"","Select an Arch Wall ( with underlying Base ArchSketch or Sketch ) or ArchSketch ")	
            return								
        targetObjectBase = None						
										
        if Draft.getType(sel0) not in ["Wall","ArchSketch"]:			
            reply = QtGui.QMessageBox.information(None,"","Select an Arch Wall ( with underlying Base ArchSketch or Sketch ) or ArchSketch ")	
            return								
        if hasattr(sel0, "Base"): # Wall has Base, ArchSketch does not		
            if sel0.Base:							
                targetObjectBase = sel0.Base					
            else:								
                reply = QtGui.QMessageBox.information(None,"","Arch Wall without Base is not supported - Select an Arch Wall ( with underlying Base ArchSketch or Sketch )")	
                return								
        else:									
            targetObjectBase = sel0						
            if Draft.getType(sel0.InList[0]) in ["Wall"]:			
                sel0 = sel0.InList[0]						
            else:								
                sel0 = None							
        if Draft.getType(targetObjectBase) in ['ArchSketch', 'Sketch']:	
            if Draft.getType(targetObjectBase) == 'Sketch':			
                reply = QtGui.QMessageBox.information(None,"","Multi-Align support Sketch with Part Geometry Extension (abdullah's development) / ArchSketch primarily.  Support on Sketch could only be done 'partially' (indexes of edges is disturbed if sketch is edited) until bug in Part Geometry Extension is fixed - currently for demonstration purpose.  Procced now. ")	
            elif Draft.getType(targetObjectBase) == 'ArchSketch':		
                reply = QtGui.QMessageBox.information(None,"","ArchSketch features being added, fallback to treat as Sketch if particular feature not implemented yet - currently for demonstration purpose.  Procced now. ")	
            targetObjectBase.ViewObject.HideDependent = False			
            Gui.ActiveDocument.ActiveView.setCameraType("Orthographic")	
            Gui.ActiveDocument.setEdit(targetObjectBase)			
            App.Console.PrintMessage("Select target Edge of the ArchSketch / Sketch to Flip the corresponding Wall Segment Align "+ "\n")	
            FreeCADGui.Selection.clearSelection()				
            s=GuiEditWallAlignObserver(sel0, targetObjectBase)			
            self.observer = s							
            FreeCADGui.Selection.addObserver(s)				
        elif Draft.getType(targetObjectBase) == 'Wire':			
            reply = QtGui.QMessageBox.information(None,"","Gui to edit Arch Wall with a DWire Base is not implemented yet - Please directly edit ArchWall OverrideAlign attribute for the purpose.")	
										
										
FreeCADGui.addCommand('EditWallAlign', _CommandEditWallAlign())		
										
										
class GuiEditWallAlignObserver(SketchArchCommands.selectObjectObserver):	
										
    def __init__(self, targetWall, targetBaseSketch):				
        SketchArchCommands.selectObjectObserver.__init__(self,None,None,None,'Edge')					
        self.targetWall = targetWall  # maybe None				
        self.targetArchSketch = targetBaseSketch  # maybe None			
        if self.targetWall:							
            self.targetWallTransparentcy = targetWall.ViewObject.Transparency	
            targetWall.ViewObject.Transparency = 60				
        if targetBaseSketch:							
            if Draft.getType(self.targetArchSketch) in ['Sketch','ArchSketch']: 
                if self.targetWall:						
                    tempOverrideAlign = self.targetWall.OverrideAlign		
                    wallAlign = targetWall.Align # use Wall's Align		
                    # filling OverrideAlign if entry is missing for a particular index		
                    while len(tempOverrideAlign) < len(self.targetArchSketch.Geometry):	
                        tempOverrideAlign.append(wallAlign) #('Left')		
                    self.targetWall.OverrideAlign = tempOverrideAlign		
										
    def proceed(self, doc, obj, sub, pnt):					
        self.edge = sub							
        self.pickedEdgePlacement = App.Vector(pnt)				
        subIndex = int( sub.lstrip('Edge'))-1					
										
        if self.targetArchSketch is not None:					
            if Draft.getType(self.targetArchSketch) == 'Sketch':		
                print (" It is a Sketch")					
                curAlign = self.targetWall.OverrideAlign[subIndex]		
                if curAlign == 'Left':						
                    curAlign = 'Right'						
                elif curAlign == 'Right':					
                    curAlign = 'Center'					
                elif curAlign == 'Center':					
                    curAlign = 'Left'						
                else:	# 'Center' or else?					
                    curAlign = 'Right'						
                # Save information in ArchWall					
                if self.targetWall:						
                    tempOverrideAlign = self.targetWall.OverrideAlign		
                    tempOverrideAlign[subIndex] = curAlign			
                    self.targetWall.OverrideAlign = tempOverrideAlign		
            elif Draft.getType(self.targetArchSketch) == 'ArchSketch':		
                print (" It is an ArchSketch")					
                print (" Full Support not added currently yet !")		
                print (" Fallback to treat as Sketch as 'partial preview' if particular feature Not implemented in ArchSketch yet !")	
                # Test if particular ArchSketch feature has been implemented or not -  Fallback to use 'Sketch workflow' if Not	
                if not hasattr(self.targetArchSketch.Proxy, "getEdgeTagDictSyncAlign"):						
                    curAlign = self.targetWall.OverrideAlign[subIndex]		
                if curAlign == 'Left':						
                    curAlign = 'Right'						
                elif curAlign == 'Right':					
                    curAlign = 'Center'					
                elif curAlign == 'Center':					
                    curAlign = 'Left'						
                # Test if particular ArchSketch feature has been implemented or not -  Fallback to use 'Sketch workflow' if Not	
                # Save information in ArchWall												
                if not hasattr(self.targetArchSketch.Proxy, "getEdgeTagDictSyncAlign"):						
                    if self.targetWall:					
                        tempOverrideAlign = self.targetWall.OverrideAlign	
                        tempOverrideAlign[subIndex] = curAlign			
                if self.targetWall:						
                    self.targetWall.OverrideAlign = tempOverrideAlign		
            self.targetArchSketch.recompute()					
        else:  								
            # nothing implemented if self.targetArchSketch is None		
            pass								
        if self.targetWall:							
            self.targetWall.recompute()					
										
    def escape(self,info):							
        k=info['Key']								
        if k=="ESCAPE":							
            self.targetWall.ViewObject.Transparency = self.targetWallTransparentcy	
        SketchArchCommands.selectObjectObserver.escape(self,info)		
										
										
class _CommandEditWallWidth():							
										
    '''Edit Wall Segment (Underlying [Arch]Sketch) Width Command Definition'''	
										
    def GetResources(self):							
        return {'Pixmap'  : SketchArchIcon.getIconPath()+'/icons/Edit_Width',	
                'Accel'   : "E, W",						
                'MenuText': "Edit Wall Segment Width",				
                'ToolTip' : "select Wall to Edit Wall Segment Width ",		
                'CmdType' : "ForEdit"}						
										
    def IsActive(self):							
        return not FreeCAD.ActiveDocument is None				
										
    def Activated(self):							
        try:									
            sel0 = Gui.Selection.getSelection()[0]				
        except:								
            reply = QtGui.QMessageBox.information(None,"","Select an Arch Wall ( with underlying Base ArchSketch or Sketch ) or ArchSketch ")	
            return								
        targetObjectBase = None						
        if Draft.getType(sel0) not in ["Wall","ArchSketch"]:			
            reply = QtGui.QMessageBox.information(None,"","Select an Arch Wall ( with underlying Base ArchSketch or Sketch ) or ArchSketch ")	
            return								
        if hasattr(sel0, "Base"): # Wall has Base, ArchSketch does not		
            if sel0.Base:							
                targetObjectBase = sel0.Base					
            else:								
                reply = QtGui.QMessageBox.information(None,"","Arch Wall without Base is not supported - Select an Arch Wall ( with underlying Base ArchSketch or Sketch ) or ArchSketch")	
                return								
        else:									
            targetObjectBase = sel0						
            if Draft.getType(sel0.InList[0]) in ["Wall"]:			
                sel0 = sel0.InList[0]						
            else:								
                sel0 = None							
        if Draft.getType(targetObjectBase) in ['ArchSketch', 'Sketch']:	
            if Draft.getType(targetObjectBase) == 'Sketch':			
                reply = QtGui.QMessageBox.information(None,"","Multi-Width support Sketch with Part Geometry Extension (abdullah's development) / ArchSketch primarily.  Support on Sketch could only be done 'partially' (indexes of edges is disturbed if sketch is edited) until bug in Part Geometry Extension is fixed - currently for demonstration purpose.  Procced now. ")	
            elif Draft.getType(targetObjectBase) == 'ArchSketch':		
                reply = QtGui.QMessageBox.information(None,"","ArchSketch features being added, fallback to treat as Sketch if particular feature not implemented yet - currently for demonstration purpose.  Procced now. ")	
            targetObjectBase.ViewObject.HideDependent = False			
            Gui.ActiveDocument.ActiveView.setCameraType("Orthographic")	
            Gui.ActiveDocument.setEdit(targetObjectBase)			
            App.Console.PrintMessage("Select target Edge of the ArchSketch / Sketch to Edit the corresponding Wall Segment Width "+ "\n")	
            FreeCADGui.Selection.clearSelection()				
            s=GuiEditWallWidthObserver(sel0, targetObjectBase)			
            self.observer = s							
            FreeCADGui.Selection.addObserver(s)				
										
        elif Draft.getType(targetObjectBase) == 'Wire':			
            reply = QtGui.QMessageBox.information(None,"","Gui to edit Arch Wall with a DWire Base is not implemented yet - Please directly edit ArchWall OverrideAlign attribute for the purpose.")	
										
FreeCADGui.addCommand('EditWallWidth', _CommandEditWallWidth())		
										
										
class GuiEditWallWidthObserver(SketchArchCommands.selectObjectObserver):	
										
    def __init__(self, targetWall, targetBaseSketch):				
        SketchArchCommands.selectObjectObserver.__init__(self,None,None,None,'Edge')					
        self.targetWall = targetWall						
        self.targetArchSketch = targetBaseSketch				
        self.targetWallTransparentcy = targetWall.ViewObject.Transparency	
        targetWall.ViewObject.Transparency = 60				
        if targetBaseSketch:  # would be none ?				
            tempOverrideWidth = None						
            wallWidth = None							
            if not wallWidth:							
                wallWidth = targetWall.Width.Value  # use Wall's Width		
            if Draft.getType(self.targetArchSketch) == 'ArchSketch':					
                if hasattr(self.targetArchSketch.Proxy, "getUnsortedEdgesWidth"):			
                    tempOverrideWidth = targetBaseSketch.Proxy.getUnsortedEdgesWidth(targetBaseSketch)	
                    tempOverrideWidth = [i if i is not None else wallWidth for i in tempOverrideWidth]	
            if not tempOverrideWidth:						
                tempOverrideWidth = self.targetWall.OverrideWidth		
                # filling OverrideWidth for geometry edges		 	
                while len(tempOverrideWidth) < len(targetBaseSketch.Geometry):	
                    tempOverrideWidth.append(wallWidth)  #(0)			
                tempOverrideWidth = [i if i is not None else wallWidth for i in tempOverrideWidth]	
            self.targetWall.OverrideWidth = tempOverrideWidth						
													
    def proceed(self, doc, obj, sub, pnt):					
        self.edge = sub							
        self.pickedEdgePlacement = App.Vector(pnt)				
        subIndex = int( sub.lstrip('Edge'))-1					
        App.Console.PrintMessage("Input Width"+ "\n")				
        if hasattr(self.targetArchSketch.Proxy, 'getEdgeTagDictSyncWidth'):	
            curWidth = self.targetArchSketch.Proxy.getEdgeTagDictSyncWidth(self.targetArchSketch, None, subIndex)	
            if not curWidth:												
                curWidth = self.targetArchSketch.ArchSketchWidth.Value							
        else:														
            curWidth = self.targetWall.OverrideWidth[subIndex]								
        reply = QtGui.QInputDialog.getText(None, "Input Width","Width of Wall Segment", text=str(curWidth))		
        if reply[1]:  # user clicked OK					
            if reply[0]:							
                replyWidth = float(reply[0])					
            else:  # no input							
                return None							
        else:  # user clicked not OK, i.e. Cancel ?				
            return None							
        if self.targetArchSketch is not None:					
            if Draft.getType(self.targetArchSketch) == 'Sketch':		
                # Save information in ArchWall					
                tempOverrideWidth = self.targetWall.OverrideWidth		
                tempOverrideWidth[subIndex] = replyWidth			
                self.targetWall.OverrideWidth = tempOverrideWidth		
            elif Draft.getType(self.targetArchSketch) == 'ArchSketch':		
                print (" It is an ArchSketch")					
                print (" Full Support not added currently yet !")		
                print (" Fallback to treat as Sketch as 'partial preview' if particular feature Not implemented in ArchSketch yet !")	
                # Save information in ArchWall												
                tempOverrideWidth = self.targetWall.OverrideWidth		
                tempOverrideWidth[subIndex] = replyWidth			
                self.targetWall.OverrideWidth = tempOverrideWidth		
            self.targetArchSketch.recompute()					
        else:  								
            # nothing implemented if self.targetArchSketch is None		
            pass								
        self.targetWall.recompute()						
										
    def escape(self,info):							
        k=info['Key']								
        if k=="ESCAPE":							
            self.targetWall.ViewObject.Transparency = self.targetWallTransparentcy	
        SketchArchCommands.selectObjectObserver.escape(self,info)		
										
										
class _CommandEditWallAttach():						
										
    '''Edit ArchSketch and ArchObjects Attachment Command Definition		
       edit attachment to Wall Segment (Underlying Arch]Sketch)'''		
										
    def GetResources(self):							
        return {'Pixmap'  : SketchArchIcon.getIconPath()+'/icons/Edit_Attach',	
                'Accel'   : "E, T",						
                'MenuText': "Edit Attachment Edge",				
                'ToolTip' : "Select ArchSketch or Arch Window/Equipment to change attachment edge ",	
                'CmdType' : "ForEdit"}							
										
    def IsActive(self):							
        return not FreeCAD.ActiveDocument is None				
										
    def Activated(self):							
        try:									
            sel0 = Gui.Selection.getSelection()[0]				
        except:								
            reply = QtGui.QMessageBox.information(None,"","Select an ArchSketch or Arch Window/Equipment, Click this Button, and select the edge to attach ")	
            return								
        targetHostWall = None							
        targetBaseSketch = None						
										
        if Draft.getType(sel0.getLinkedObject()) not in ['ArchSketch','Window','Equipment']:									
            reply = QtGui.QMessageBox.information(None,"","Select an ArchSketch or Arch Window/Equipment, Click this Button, and select the edge to attach ")	
            return																		
        # Winddow has Hosts, Equipment Host					
        if hasattr(sel0, "Host"):						
            if sel0.Host:							
                targetHostWall = sel0.Host					
        elif hasattr(sel0, "Hosts"):						
            if sel0.Hosts:							
                targetHostWall = sel0.Hosts[0]  # TODO to scan through ?	
        if not targetHostWall:							
            if Draft.getType(sel0.getLinkedObject()) != 'ArchSketch':  # ArchSketch can has no hostWall / attach to (Arch)Sketch directly										
                reply = QtGui.QMessageBox.information(None,"","Select a Window/Equipment with Host which is Arch Wall ")										
                return																							
        elif Draft.getType(targetHostWall) != 'Wall':																			
            reply = QtGui.QMessageBox.information(None,"","Window/Equipment's Host needs to be a Wall to function")											
            return																							
        if targetHostWall:																						
            if targetHostWall.Base:																					
                targetBaseSketch = targetHostWall.Base																			
        elif hasattr(sel0, "MasterSketch"):																				
            if sel0.MasterSketch:																					
                targetBaseSketch = sel0.MasterSketch																			
        if not targetBaseSketch:																					
            reply = QtGui.QMessageBox.information(None,"","Wall needs to have Base which is to be Sketch or ArchSketch to function")								
            return																							
        if Draft.getType(targetBaseSketch) in ['ArchSketch', 'Sketch']:	
            targetBaseSketch.ViewObject.HideDependent = False			
            Gui.ActiveDocument.ActiveView.setCameraType("Orthographic")	
            Gui.ActiveDocument.setEdit(targetBaseSketch)			
            App.Console.PrintMessage("Select target Edge of the ArchSketch / Sketch to attach to "+ "\n")	
            FreeCADGui.Selection.clearSelection()								
            s=GuiEditWallAttachObserver(sel0, targetHostWall, targetBaseSketch)				
            self.observer = s							
            FreeCADGui.Selection.addObserver(s)				
        elif Draft.getType(targetBaseSketch) == 'Wire':			
            reply = QtGui.QMessageBox.information(None,"","Gui to edit Arch Wall with a DWire Base is not implemented yet - Please directly edit ArchWall OverrideAlign attribute for the purpose.")	
        else:																								
            reply = QtGui.QMessageBox.information(None,"","Wall needs to have Base which is to be Sketch or ArchSketch to function")									
            return																							
										
										
FreeCADGui.addCommand('EditWallAttach', _CommandEditWallAttach())		
										
										
class GuiEditWallAttachObserver(SketchArchCommands.selectObjectObserver):	
										
    def __init__(self, targetObject, targetHostWall, targetBaseSketch):	
        SketchArchCommands.selectObjectObserver.__init__(self,None,None,None,'Edge')				
        self.targetObject = targetObject									
        self.targetWall = targetHostWall									
        self.targetArchSketch = targetBaseSketch								
        if self.targetWall:											
            self.targetWallTransparentcy=targetHostWall.ViewObject.Transparency				
            targetHostWall.ViewObject.Transparency = 60							
														
    def proceed(self, doc, obj, sub, pnt):									
        self.edge = sub							
        self.pickedEdgePlacement = App.Vector(pnt)				
        subElement = sub.lstrip('Edge')					
										
        if self.targetArchSketch is not None:					
            if Draft.getType(self.targetArchSketch) == 'Sketch':		
                print (" It is a Sketch")					
            elif Draft.getType(self.targetArchSketch) == 'ArchSketch':		
                print (" It is an ArchSketch")					
            self.targetObject.MasterSketchSubelement = subElement		
        else:  								
            # nothing implemented if self.targetArchSketch is None		
            pass								
        self.targetObject.recompute()						
        if self.targetWall:							
            self.targetWall.recompute()					
										
    def escape(self,info):							
        k=info['Key']								
        if k=="ESCAPE":							
            if self.targetWall:										
                self.targetWall.ViewObject.Transparency = self.targetWallTransparentcy				
        SketchArchCommands.selectObjectObserver.escape(self,info)						
										
										
class _Command_ArchSketch():							
										
    ''' ArchSketch Command Definition - Gui to make an ArchSketch '''		
										
    def GetResources(self):							
        return {'Pixmap' : SketchArchIcon.getIconPath() + '/icons/SketchArchWorkbench.svg',			
                'Accel' : "Alt+S",						
                'MenuText': "New ArchSketch",					
                'ToolTip' : "create an ArchSketch"}				
										
    def IsActive(self):							
        return not FreeCAD.ActiveDocument is None				
										
    def Activated(self):							
        reply = QtGui.QMessageBox.information(None,"","ArchSketch functionalities being developed :) ")	
        makeArchSketch()							
										
FreeCADGui.addCommand('ArchSketch', _Command_ArchSketch())			
										
										
#----------------------------------------------------------------------------#	
#                             Functions                                      #	
#----------------------------------------------------------------------------#	
										
										
def attachToMasterSketch(subject, target=None, subelement=None,		
                         attachmentOffset=None, zOffset='0',			
                         intersectingSubelement=None, mapMode='ObjectXY'):	
										
  if Draft.getType(subject) == "ArchSketch":					
      subject.MapReversed = False						
      subject.MapMode = mapMode						
      subject.Support = subject.MasterSketch					
										
										
def detachFromMasterSketch(fp):						
  fp.MapMode = 'Deactivated'							
  fp.Support = None								
										
										
#def updateAttachment(fp, linkFp=None):					
def updateAttachmentOffset(fp, linkFp=None):					
										
    fpOrgSelf = fp.Proxy							
    if linkFp:									
        fp = linkFp								
        print (" fp is re-directed to linkFp...  ")				
										
    if fp.AttachToAxisOrSketch == "None":					
        return									
										
    hostSketch = None								
    hostWall = None								
    hostObject = None								
    if fp.AttachToAxisOrSketch == "Host":			 		
            if hasattr(fp, "Hosts"):  # Arch Window				
                if fp.Hosts:							
                    hostWall = fp.Hosts[0]  # Can just take 1st Host wall	
                    if hostWall.Base.isDerivedFrom("Sketcher::SketchObject"):	
                        hostSketch = hostWall.Base  # Host wall's base Sketch	
            elif hasattr(fp, "Host"):  # Other ArchObjects (except ArchSketch)	
                if fp.Host:							
                    hostObject = fp.Host					
                    if hasattr(fp.Host, "Base"):  # Arch Wall ?		
                        if hostObject.Base.isDerivedFrom("Sketcher::SketchObject"):	
                            hostSketch = fp.Host.Base  # Host wall's base 		
            if (not hostWall) and (not hostObject):				
                return								
										
    elif fp.AttachToAxisOrSketch == "Master Sketch":			 	
            if fp.MasterSketch:						
                hostSketch = fp.MasterSketch					
            else:								
                return								
										
    attachToSubelementOrOffset = fp.AttachToSubelementOrOffset			
										
    masterSketchSubelement = fp.MasterSketchSubelement				
    msSubelementOffset = fp.MasterSketchSubelementOffset			
    msIntersectingSubelement = fp.MasterSketchIntersectingSubelement		
										
    attachmentAlignment = fp.AttachmentAlignment				
    attachmentAlignmentOffset = fp.AttachmentAlignmentOffset			
    attachmentOffsetXyzAndRotation = fp.AttachmentOffsetXyzAndRotation		
    flip180Degree = fp.Flip180Degree						
    flipOffsetOriginToOtherEnd = fp.FlipOffsetOriginToOtherEnd			
										
    masterSketchSubelementTag = None						
    msIntersectingSubelementTag = None						
    msSubelementEdge = None							
    msSubelementIndex = None							
    msIntersectingSubelementEdge = None					
										
    if hasattr(fp, "Proxy"):  # ArchSketch/ ArchObjects (Window/Equipment)	
            if fp.Proxy.Type == "ArchSketch":					
                msSubelementEdge = masterSketchSubelement			
            else:								
                # Other Arch Objects (Windows / Doors)				
                msSubelementEdge = masterSketchSubelement			
            if fp.Proxy.Type == "ArchSketch":					
                msIntersectingSubelementEdge = msIntersectingSubelement				
            else:											
                # Other Arch Objects (Windows / Doors)							
                msIntersectingSubelementEdge = msIntersectingSubelement				
													
    else:  # Link objects (of ArchSketch or Arch Windows / Doors)					
            msSubelementEdge = masterSketchSubelement							
            msIntersectingSubelementEdge = msIntersectingSubelement					
													
    if not msSubelementEdge:							
            msSubelementEdge = "Edge1"  # default be 1				
    msSubelementIndex = int(msSubelementEdge.lstrip('Edge'))-1			
										
    #if attachToAxisOrSketch in ["Hosts", "Master Sketch"]: 			
    tempAttachmentOffset = FreeCAD.Placement()					
    winSketchPl = FreeCAD.Placement()						
																										
    # Calculate the Position & Rotation (of the point of the edge to attach to)																
																										
    if (attachToSubelementOrOffset in [ "Attach to Edge", "Attach To Edge & Alignment"] ):															
																										
            if hostSketch:  # only calculate 'offset' if hostSketch, otherwise, still proceed but 'offset' is kept to 'origin' of host										
																										
                # Calculate the position of the point of the edge to attach to																	
                edgeOffsetPointVector = getSketchEdgeOffsetPointVector(fp, hostSketch, msSubelementEdge, msSubelementOffset,											
                                                                       attachmentOffsetXyzAndRotation, flipOffsetOriginToOtherEnd, flip180Degree,								
                                                                       attachToSubelementOrOffset, msIntersectingSubelementEdge)  # offsetFromIntersectingSubelement, 						
                tempAttachmentOffset.Base= edgeOffsetPointVector																		
																										
                # Calculate the rotation of the edge																				
                if attachToSubelementOrOffset == "Attach To Edge & Alignment":																	
                    edgeAngle = getSketchEdgeAngle(hostSketch, msSubelementEdge)																
                    if (flip180Degree and not flipOffsetOriginToOtherEnd) or (not flip180Degree and flipOffsetOriginToOtherEnd):										
                        edgeAngle = edgeAngle + math.pi																			
                    tempAttachmentOffset.Rotation.Angle = edgeAngle																		
                else:																								
                    tempAttachmentOffset.Rotation.Angle = attachmentOffsetXyzAndRotation.Rotation.Angle													
																										
                # Offset Parallel from Line Alignment																				
																										
                masterSketchSubelementEdgeVec = getSketchEdgeVec(hostSketch, msSubelementEdge)															
                msSubelementWidth = zeroMM																					
                align = None																							
																										
                if attachmentAlignment in ["EdgeGroupWidthLeft", "EdgeGroupWidthRight"]:															
                    if hasattr(hostSketch.Proxy, "getEdgeTagDictSyncWidth") and hasattr(hostSketch.Proxy,"EdgeTagDictSync"):											
                        pass																							
                    elif hostWall:																						
                        try:																							
                            msSubelementWidth = hostWall.OverrideWidth[msSubelementIndex]*MM															
                        except:																						
                            msSubelementWidth = hostWall.Width																			
                    elif hostObject:																						
                        try:																							
                            msSubelementWidth = hostObject.OverrideWidth[msSubelementIndex]*MM															
                        except:																						
                            msSubelementWidth = hostObject.Width																		
                    else:																							
                        print (" something wrong ?")																				
																										
                    if hasattr(hostSketch.Proxy, "getEdgeTagDictSyncAlign") and hasattr(hostSketch.Proxy,"EdgeTagDictSync"):											
                        pass																							
                    elif hostWall:																						
                        try:																							
                            align = hostWall.OverrideAlign[msSubelementIndex]																	
                        except:																						
                            align = hostWall.Align																				
                    elif hostObject:																						
                        try:																							
                            align = hostObject.OverrideAlign[msSubelementIndex]																
                        except:																						
                            align = hostObject.Align																				
                    else:																							
                        print (" something wrong ?")																				
																										
                offsetValue = 0																						
                if (msSubelementWidth is not None) and (msSubelementWidth.Value != 0):  # TODO If None, latter condition will result in exception	#zeroMM is 0 -> False					
                        offsetValue = msSubelementWidth.Value # + attachmentAlignmentOffset.Value														
                else:																								
                    print (" something wrong...")																				
                if attachmentAlignment == "EdgeGroupWidthLeft":																		
                    if align == "Left":																					
                        offsetValue = attachmentAlignmentOffset.Value  # no need offsetValue (msSubelementWidth.Value)												
                    elif align == "Right":																					
                        offsetValue = offsetValue + attachmentAlignmentOffset.Value																
                    elif align == "Center":																					
                        offsetValue = offsetValue/2 + attachmentAlignmentOffset.Value																
                elif attachmentAlignment == "EdgeGroupWidthRight":																		
                    if align == "Left":																					
                        offsetValue = -offsetValue+attachmentAlignmentOffset.Value																
                    elif align == "Right":																					
                        offsetValue = attachmentAlignmentOffset.Value  # no need offsetValue (msSubelementWidth.Value)												
                    elif align == "Center":																					
                        offsetValue = -offsetValue/2 + attachmentAlignmentOffset.Value																
                else:																								
                        offsetValue = attachmentAlignmentOffset.Value  # no need offsetValue (msSubelementWidth.Value)												
                if offsetValue != 0:																						
                        vOffsetH = DraftVecUtils.scaleTo(masterSketchSubelementEdgeVec.cross(Vector(0,0,1)), -offsetValue)  # -ve										
                        tempAttachmentOffset.Base = tempAttachmentOffset.Base.add(vOffsetH)															
																										
    elif attachToSubelementOrOffset == "Follow Only Offset XYZ & Rotation":																	
                tempAttachmentOffset = attachmentOffsetXyzAndRotation																		
																										
    # Extra Rotation as user input											
															
    extraRotation = FreeCAD.Placement(App.Vector(0,0,0),App.Rotation(0,0,0)) #, 0)					
    if fp.AttachmentOffsetExtraRotation == "X-Axis CCW90":  # [ "X-Axis CW90", "X-Axis CCW90", "X-Axis CW180", ]	
                extraRotation = FreeCAD.Placement(App.Vector(0,0,0),App.Rotation(0,0,90))  #,winSketchPl.Base)		
    elif fp.AttachmentOffsetExtraRotation == "X-Axis CW90":								
                extraRotation = FreeCAD.Placement(App.Vector(0,0,0),App.Rotation(0,0,-90))  #,winSketchPl.Base)	
    elif fp.AttachmentOffsetExtraRotation == "X-Axis CW180":								
                extraRotation = FreeCAD.Placement(App.Vector(0,0,0),App.Rotation(0,0,180))  #,winSketchPl.Base)	
    elif fp.AttachmentOffsetExtraRotation == "Y-Axis CW90":								
                extraRotation = FreeCAD.Placement(App.Vector(0,0,0),App.Rotation(0,90,0))  #,winSketchPl.Base)		
    elif fp.AttachmentOffsetExtraRotation == "Y-Axis CCW90":								
                extraRotation = FreeCAD.Placement(App.Vector(0,0,0),App.Rotation(0,-90,0))  #,winSketchPl.Base)	
    elif fp.AttachmentOffsetExtraRotation == "Y-Axis CW180":								
                extraRotation = FreeCAD.Placement(App.Vector(0,0,0),App.Rotation(0,180,0))  #,winSketchPl.Base)	
    elif fp.AttachmentOffsetExtraRotation == "Z-Axis CCW90":								
                extraRotation = FreeCAD.Placement(App.Vector(0,0,0),App.Rotation(90,0,0))  #,winSketchPl.Base)		
    elif fp.AttachmentOffsetExtraRotation == "Z-Axis CW90":								
                extraRotation = FreeCAD.Placement(App.Vector(0,0,0),App.Rotation(-90,0,0))  #,winSketchPl.Base)	
    elif fp.AttachmentOffsetExtraRotation == "Z-Axis CW180":								
                extraRotation = FreeCAD.Placement(App.Vector(0,0,0),App.Rotation(180,0,0))  #,winSketchPl.Base)	
															
    tempAttachmentOffset = tempAttachmentOffset.multiply(extraRotation)						
															
    # Alternative OriginOffset manually input by user									
    originOffset = fp.OriginOffsetXyzAndRotation									
    invOriginOffset = originOffset.inverse()										
    tempAttachmentOffset = tempAttachmentOffset.multiply(invOriginOffset)						
															
    # ArchObjects, link of ArchSketch, link of ArchObjects								
    # i.e. Not ArchSketch												
    if linkFp or not hasattr(fp, "AttachmentOffset"):  ## TODO or if hostWall ...					
                hostSketchPl = FreeCAD.Placement()									
                if hostSketch:												
                    hostSketchPl = hostSketch.Placement								
                if Draft.getType(fp.getLinkedObject()) == 'Window':							
                    winSketchPl = fp.Base.Placement									
                    # Reset Window's placement to factor out base sketch's placement					
                    invWinSketchPl = winSketchPl.inverse()								
                    # make the placement 'upright' again								
                    winSkRotation = FreeCAD.Placement(App.Vector(0,0,0),App.Rotation(0,0,90))				
                    tempAttachmentOffset = tempAttachmentOffset.multiply(winSkRotation).multiply(invWinSketchPl)	
                originBase = App.Vector(0,0,0)						  				
                if hostWall:												
                    hostWallPl = hostWall.Placement									
                    hostWallRotation = FreeCAD.Placement(App.Vector(0,0,0),hostWallPl.Rotation,originBase)		
                    #tempBaseOffset = hostSketchPl.multiply(hostWallRotation)						
                    tempBaseOffset = hostWallRotation.multiply(hostSketchPl)						
                    tempBaseOffset.Base = tempBaseOffset.Base.add(hostWallPl.Base)					
                    tempAttachmentOffset = tempBaseOffset.multiply(tempAttachmentOffset)				
                elif hostObject:											
                    hostObjectPl = hostObject.Placement								
                    #tempAttachmentOffset = (hostSketchPl.multiply(hostObjectPl)).multiply(tempAttachmentOffset)	
                    hostObjectRotation = FreeCAD.Placement(App.Vector(0,0,0),hostObjectPl.Rotation,originBase)		
                    #tempBaseOffset = hostSketchPl.multiply(hostObjectRotation)					
                    tempBaseOffset = hostObjectRotation.multiply(hostSketchPl)						
                    tempBaseOffset.Base = tempBaseOffset.Base.add(hostObjectPl.Base)					
                    tempAttachmentOffset = tempBaseOffset.multiply(tempAttachmentOffset)				
                else:  # WOULD HAPPEN ?										
                    tempAttachmentOffset = hostSketchPl.multiply(tempAttachmentOffset)					
                    print (" fp.Placement (superimposed) is thus ... ", tempAttachmentOffset)				
    if linkFp or not hasattr(fp, "AttachmentOffset"):  ## TODO or if hostWall ...					
                fp.Placement = tempAttachmentOffset									
    else:														
                fp.AttachmentOffset = tempAttachmentOffset								
										
										
										
'''------------------- Creation/Insertion Functions ---------------------'''	
										
										
def makeArchSketch(grp=None, label="ArchSketch__NAME", attachToAxisOrSketch=None, placementAxis_Or_masterSketch=None, copyFlag=None, visibility=None):		
  name = "ArchSketch"								
  if grp:									
      archSketch = grp.newObject("Sketcher::SketchObjectPython",name)		
  else:									
      archSketch=App.ActiveDocument.addObject("Sketcher::SketchObjectPython",name)	
  archSketch.Label = label							
  archSketchInsta=ArchSketch(archSketch)					
  archSketch.AttachToAxisOrSketch = "Master Sketch"				
  return archSketch								
										
										
'''------------------------- Low Level Operation --------------------------'''	
										
def changeAttachMode(fp, fpProp):						
    if fpProp == "AttachToAxisOrSketch":					
        if fp.AttachToAxisOrSketch == "Master Sketch":				
                fp.setEditorMode("AttachToSubelementOrOffset",0)		
                fp.setEditorMode("AttachmentOffsetXyzAndRotation",0)		
                if fp.MasterSketch:						
                      attachToMasterSketch(fp)					
        else:									
                  fp.setEditorMode("AttachToSubelementOrOffset",1)		
                  fp.setEditorMode("AttachmentOffsetXyzAndRotation",1)		
                  if True:  # TODO						
                      if fp.MasterSketch:					
                          detachFromMasterSketch(fp)				
    # change in "target" in attachToMasterSketch()				
    elif fpProp == "MasterSketch":						
              if fp.MasterSketch:						
                  if fp.AttachToAxisOrSketch == "Master Sketch":		
                      attachToMasterSketch(fp, fp.MasterSketch)		
              else:								
                  if fp.AttachToAxisOrSketch == "Master Sketch":		
                      detachFromMasterSketch(fp)				
										
										
def getSketchEdgeAngle(masterSketch, subelement):				
    vec = getSketchEdgeVec(masterSketch, subelement)				
    draftAngle = -DraftVecUtils.angle(vec)					
    return draftAngle								
										
										
def getSketchEdgeVec(sketch, subelement):  					
    geoindex = int(subelement.lstrip('Edge'))-1				
    lp1=sketch.Geometry[geoindex].EndPoint					
    lp2=sketch.Geometry[geoindex].StartPoint					
    vec = lp1 - lp2								
    return vec									
										
										
def getSketchEdgeIntersection(sketch, line1Index, line2Index):			
    import DraftGeomUtils							
    e1=sketch.Geometry[int(line1Index)].toShape()				
    e2=sketch.Geometry[int(line2Index)].toShape()				
    i=DraftGeomUtils.findIntersection(e1,e2,True,True)				
    if i:									
        return i[0]								
    else:									
        return None								
										
										
def getSketchEdgeOffsetPointVector(subject, masterSketch, subelement, attachmentOffset, zOffset, flipOffsetOriginToOtherEnd=False,								
                                   flip180Degree=False, attachToSubelementOrOffset=None, masterSketchIntersectingSubelement=None): # offsetFromIntersectingSubelement=False,			
    geoindex = None																						
    geoindex2 = None																						
    geoindex = int(subelement.lstrip('Edge'))-1																		
    if masterSketchIntersectingSubelement:  #  and offsetFromIntersectingSubelement:  ('' empty string is not 'None ) is not None:								
        geoindex2 = int(masterSketchIntersectingSubelement.lstrip('Edge'))-1															
																								
    #if ((not flipOffsetOriginToOtherEnd) and (not offsetFromIntersectingSubelement)) or (flipOffsetOriginToOtherEnd and offsetFromIntersectingSubelement):					
    if not flipOffsetOriginToOtherEnd:																				
        #if not flip180Degree:																					
        edgeOffsetPoint = masterSketch.Geometry[geoindex].value(float(attachmentOffset))													
        if geoindex2 is not None:																				
            intersectVec = getSketchEdgeIntersection(masterSketch, geoindex, geoindex2)													
            offsetVec = intersectVec.sub(masterSketch.Geometry[geoindex].StartPoint)														
            edgeOffsetPoint = edgeOffsetPoint.add(offsetVec)																	
    else:  # elif  flipOffsetOriginToOtherEnd:																			
        edgeLength = masterSketch.Geometry[geoindex].length()																	
        edgeOffsetPoint = masterSketch.Geometry[geoindex].value(edgeLength - float(attachmentOffset))												
        if geoindex2 is not None:																				
            intersectVec = getSketchEdgeIntersection(masterSketch, geoindex, geoindex2)													
            offsetVec = intersectVec.sub(masterSketch.Geometry[geoindex].EndPoint)														
            edgeOffsetPoint = edgeOffsetPoint.add(offsetVec)																	
																								
    edgeOffsetPoint.z = zOffset.Base.z																				
    return edgeOffsetPoint																					
																								
																								
def getSortedClustersEdgesOrder(sketch):					
										
      ''' Do Part.getSortedClusters() on geometry of a Sketch (omit		
          construction geometry), check the order of edges to return lists of	
          indexes in the order of sorted edges	 			 	
										
          return:								
          - clEdgePartnerIndex, clEdgeSameIndex, clEdgeEqualIndex, and		
          - clEdgePartnerIndexFlat, clEdgeSameIndexFlat, clEdgeEqualIndexFlat	
      '''									
										
      skGeom = sketch.Geometry							
      skGeomEdges = []								
      skGeomEdgesShort = []							
      for c, i in enumerate(skGeom):						
          skGeomEdge = i.toShape()						
          skGeomEdges.append(skGeomEdge)					
          if hasattr(i, 'Construction'):					
              construction = i.Construction					
          elif hasattr(sketch, 'getConstruction'):				
              construction = sketch.getConstruction(c)				
          if not construction:							
              skGeomEdgesShort.append(skGeomEdge)				
      skGeomEdgesSorted = Part.getSortedClusters(skGeomEdgesShort)		
										
      ## a list of lists (not exactly array / matrix) to contain index of found matching geometry			
      clEdgePartnerIndex = []							
      clEdgeSameIndex = []							
      clEdgeEqualIndex = []							
										
      ## a flat list containing above information - but just flat, not a list of lists ..				
      clEdgePartnerIndexFlat = []						
      clEdgeSameIndexFlat = []							
      clEdgeEqualIndexFlat = []						
										
      for h, c in enumerate(skGeomEdgesSorted):				
          clEdgePartnerIndex.append([])					
          clEdgeSameIndex.append([])						
          clEdgeEqualIndex.append([])						
										
          ''' Build the full sub-list '''					
          for a in c:								
              clEdgePartnerIndex[h].append(None)				
              clEdgeSameIndex[h].append(None)					
              clEdgeEqualIndex[h].append(None)					
										
          for i, skGeomEdgesSortedI in enumerate(c):				
              for j, skGeomEdgesI in enumerate(skGeomEdges):			
                  if skGeomEdgesI: # is not None / i.e. Construction Geometry	
                      if j not in clEdgePartnerIndexFlat:			
                        if skGeomEdgesSortedI.isPartner(skGeomEdgesI):		
                          clEdgePartnerIndex[h][i] = j				
                          clEdgePartnerIndexFlat.append(j)			
										
                      if j not in clEdgeSameIndexFlat:				
                        if skGeomEdgesSortedI.isSame(skGeomEdgesI):		
                          clEdgeSameIndex[h][i] = j				
                          clEdgeSameIndexFlat.append(j)			
										
                      if j not in clEdgeEqualIndexFlat:			
                        if skGeomEdgesSortedI.isEqual(skGeomEdgesI):		
                          clEdgeEqualIndex[h][i] = j				
                          clEdgeEqualIndexFlat.append(j)			
										
              if clEdgePartnerIndex[h][i] == None:				
                  clEdgePartnerIndexFlat.append(None)				
              if clEdgeSameIndex[h][i] == None:				
                  clEdgeSameIndexFlat.append(None)				
              if clEdgeEqualIndex[h][i] == None:				
                  clEdgeEqualIndexFlat.append(None)				
      return clEdgePartnerIndex, clEdgeSameIndex, clEdgeEqualIndex, clEdgePartnerIndexFlat, clEdgeSameIndexFlat, clEdgeEqualIndexFlat		
										
										
def sortSketchAlign(sketch,edgeAlignList):					
										
    '''									
        This function is primarily to support Ordinary Sketch + Arch Wall	
        to gain feature that individual edge / wall segment to have		
        individual Align setting with OverrideAlign attribute in Arch Wall	
										
        This function arrange the edgeAlignList 				
        - a list of Align in the order of Edge Indexes -			
        into a list of Align following the order of edges			
        sorted by Part.getSortedClusters()					
    '''									
										
    sortedIndexes = getSortedClustersEdgesOrder(sketch)			
    clEdgeSameIndexFlat = sortedIndexes[4]					
    alignsList = []								
    for i in clEdgeSameIndexFlat:						
        try:									
            curAlign = edgeAlignList[i]					
        # if edgeAlignList does not cover the edge				
        except:								
            curAlign = 'Left'  # default					
        alignsList.append(curAlign)						
    return alignsList								
										
										
def sortSketchWidth(sketch,edgeWidthList):					
										
    '''									
        This function is primarily to support Ordinary Sketch + Arch Wall	
        to gain feature that individual edge / wall segment to have		
        individual Width setting with OverrideWidth attribute in Arch Wall	
										
        This function arrange the edgeWidthList 				
        - a list of Width in the order of Edge Indexes -			
        into a list of Width following the order of edges			
        sorted by Part.getSortedClusters()					
    '''									
										
    sortedIndexes = getSortedClustersEdgesOrder(sketch)			
    clEdgeSameIndexFlat = sortedIndexes[4]					
    widthList = []								
    for i in clEdgeSameIndexFlat:						
        try:									
            curWidth = edgeWidthList[i]					
        # if edgeWidthList does not cover the edge				
        except:								
            curWidth = '200'  # default					
        widthList.append(curWidth)						
    return widthList								
										
										
