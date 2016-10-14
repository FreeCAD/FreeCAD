################################################################################
# centroid1.py
#
# Post Processor for the centroid M40 machine
# 
#
# Dan Falck, 7th March 2010

import nc
import iso_modal
import math

import datetime

now = datetime.datetime.now()



################################################################################
class Creator(iso_modal.Creator):
    
    def __init__(self):
        iso_modal.Creator.__init__(self)

        self.useCrc = True
        self.useCrcCenterline = True
        self.absolute_flag = True
        self.prev_g91 = ''
        self.safe_z =None
    def SPINDLE(self, format, speed): return(self.SPACE() + 'S' + (format % speed))
################################################################################
#cutter comp

    #def crc_on(self):
    #    self.useCrc = True
    #    self.useCrcCenterline = True

    #def crc_off(self):
    #    self.useCrc = False

################################################################################
# general 

    def comment(self, text):
        self.write(';' + text +'\n')  
    def write_blocknum(self):
        pass 

################################################################################
# settings for absolute or incremental mode
    def absolute(self):
        self.write(self.ABSOLUTE()+'\n')        
        self.absolute_flag = True

    def incremental(self):
        self.write(self.INCREMENTAL()+'\n')
        self.absolute_flag = False

################################################################################
# APT style INSERT- insert anything into program

    def insert(self, text):
        self.write((text + '\n'))

################################################################################
# program begin and end

    def program_begin(self, id, name=''):
        self.write(';time:'+str(now)+'\n')
        self.write('G17 G20 G80 G40 G90\n')
        

    def program_end(self):
        self.write('M05\n')
        self.write('M25\n')
        self.write('G00 X-1.0 Y1.0\n')
        self.write('G17 G80 G40 G90\n')
        self.write('M99\n')

    def program_stop(self, optional=False):
        self.write_blocknum()
        if (optional) : 
            self.write(self.STOP_OPTIONAL() + '\n')
        else : 
            self.write('M05\n')
            self.write('M25\n')
            self.write(self.STOP() + '\n')
            self.prev_g0123 = ''
################################################################################
# coordinate system ie G54-G59

    def workplane(self, id):
        self.write('M25\n')
        if ((id >= 1) and (id <= 6)):
            self.g_list.append(self.WORKPLANE() % (id + self.WORKPLANE_BASE()))
        if ((id >= 7) and (id <= 9)):
            self.g_list.append(((self.WORKPLANE() % (6 + self.WORKPLANE_BASE())) + ('.%i' % (id - 6))))
        self.prev_g0123 = ''            

################################################################################
# clearance plane

    def clearanceplane(self,z=None):
        self.safe_z = z
################################################################################
# return to home
    def rapid_home(self, x=None, y=None, z=None, a=None, b=None, c=None):
        """Rapid relative to home position"""
        self.write('M05\n')              
        self.write('M25\n')
        self.write(self.RAPID())
        self.write(self.X() + (self.fmt % x))
        self.write(self.Y() + (self.fmt % y))
        self.write('\n')                     
                     
################################################################################
# tool info
    def tool_change(self, id):
        self.write_blocknum()
        self.write((self.TOOL() % id) + '\n')
        self.t = id
        self.write('M25\n')
        if self.safe_z == None:
            self.write('G43 H'+ str(id) + ' Z')
            self.write('1.0')
            self.write ('\n')
        else:
            self.write('G43 H'+ str(id) + ' Z')
            self.write(str(self.safe_z))
            self.write ('\n')


    def tool_defn(self, id, name='', params=None):
        #self.write('G43 \n')
        pass

    def write_spindle(self):
        pass


    def spindle(self, s, clockwise):
        if s < 0: 
            clockwise = not clockwise
            s = abs(s)
        
        self.s = self.SPINDLE(self.FORMAT_ANG(), s)
        if clockwise:
           #self.s =  self.SPINDLE_CW() + self.s
            self.s =  self.SPINDLE_CW()                
            self.write(self.s +  '\n')
            self.write('G04 P2.0 \n')
                
        else:
            self.s =  self.SPINDLE_CCW() + self.s

    def end_canned_cycle(self):
        self.write_blocknum()
        self.write(self.SPACE() + self.END_CANNED_CYCLE() + '\n')
        self.prev_drill = ''
        self.prev_g0123 = ''
        self.prev_z = ''   
        self.prev_f = '' 
        self.prev_retract = '' 
        self.write('M05\n')
        self.write('M25\n') 
        self.write('G00 X-1.0 Y1.0\n')

nc.creator = Creator()
