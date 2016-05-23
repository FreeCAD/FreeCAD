def tool_defn(self, id, params):
    self.write('(TOOL/')
    type = params['type']
    if type == 0:#eDrill = 0,
#;TOOL/DRILL, Diameter, Point Angle, Height
        self.write('DRILL, ' + str(params['diameter']))
        self.write(', ' + str(params['cutting edge angle'] * 2.0))
        self.write(', ' + str(params['cutting edge height']))
    elif type == 1:#eCentreDrill,
#;TOOL/CDRILL, D1, A1, L, D2, A2, H (see Fig. below)
        self.write('CDRILL, ' + str(params['flat radius'] * 2))
        self.write(', ' + str(params['cutting edge angle']))
        self.write(', ' + str(params['flat radius'] * 2))
        self.write(', ' + str(params['diameter']))
        self.write(', ' + str(params['cutting edge angle'] * 2.0))
        self.write(', ' + str(params['cutting edge height']))
    elif type == 2 or type == 3 or type == 4:#eSlotCutter,#eEndmill,#eBallEndMill,
#TOOL/MILL, Diameter, Corner radius, Height, Taper Angle
        self.write('MILL, ' + str(params['diameter']))
        self.write(', ' + str(params['corner radius']))
        self.write(', ' + str(params['cutting edge height']))
        self.write(', ' + str(params['cutting edge angle']))
    elif type == 5 or type == 6:#eChamfer,#eEngravingTool,
#;TOOL/CHAMFER, Diameter, Point Angle, Height  
#;TOOL/CHAMFER, Diameter, Point Angle, Height, Chamfer Length
        self.write('CHAMFER, ' + str(params['diameter']))
        self.write(', ' + str(params['cutting edge angle']))
        self.write(', ' + str(params['cutting edge height']))
    else:#eUndefinedToolType
        pass
    self.write(')\n')

# to do        
#;TOOL/CRMILL, Diameter1, Diameter2, Radius, Height, Length
